#pragma once

#include <chrono>
#include <ctime>
#include <atomic>
#include <string>
#include <vector>
#include <cstdio>
#include <cctype>
#include <optional>
#include <filesystem>
#include <sstream>
#include <utility>
#include <stdexcept>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <locale>
#include <limits>
#include <system_error>
#include <random>
#include "SculptureCaptureIntegrity.h"
#include "SculptureProgramSchema.h"
#include "CapturePixelLayout.h"
#include "SculptureSerializer.h"
#include "SculptureFilePublication.h"
#include "SculptureCaptureManifest.h"
#include "SculptureCaptureIdentity.h"
#include "SculptureWorldProfileSidecar.h"
#include "epi/backend/seed_image_input.hpp"

// Note: STB_IMAGE_WRITE_IMPLEMENTATION should only be defined once in a cpp file (usually main.cpp)
#include "epi/third_party/stb_image_write.h"

namespace epi_sculpt {

struct SnapshotPaths {
    std::string pngPath;
    std::string worldPath;
    std::string epimodePath;
    std::string epiPath;
    std::string profilePath;
    std::string manifestPath;
    std::string volmemPath;
    std::string causalPath;
    std::string timestamp;
};

// External producers attest completed writes for this capture attempt. Paths
// alone (including files left by older attempts) do not establish completion.
struct CaptureAttachmentWrites {
    bool volmem = false;
    bool causal = false;
};

// A host may explicitly associate a producer source with a different authored
// preset. This checked value attests the selected input, not arbitrary filenames.
// Legacy standalone captures retain their source == preset admission contract.
struct CanonicalFeedbackCaptureAssociation {
    std::string sourceStateId, presetId, presetRevision;
    std::uint64_t productionReceiptSequence = 0U;
    SculptureCaptureFrameIdentity frame;
};

// Optional canonical producer input, serialized by the owning host with the
// shared VisualMode codec. Keeping text here avoids coupling ordinary sculpture
// capture to the feedback runtime. This is semantic input, not retained GPU state.
struct CanonicalFeedbackCaptureState {
    std::string serializedMode;
    std::string sourceStateId;
    std::uint64_t generation = 0U;
    std::uint64_t resetEpoch = 0U;
    std::uint64_t submissionSerial = 0U;
    std::uint64_t readbackSubmissionSerial = 0U;
    std::uint32_t fieldWidth = 0U, fieldHeight = 0U;
    float kernelTime = 0.0f;
    float acceptedControlTime = 0.0f;
    float productionDelta = 0.0f;
    std::optional<CanonicalFeedbackCaptureAssociation> association;
    // Optional encoded asset already owned by the selected publication. The
    // host codec rewrites serializedMode to this sibling; never reopen its path.
    epi::SeedImageInput::Owner seedImageInput;
    std::string seedImageFile;

    [[nodiscard]] bool valid() const noexcept {
        return !serializedMode.empty() && serializedMode.size() <= kMaxCanonicalCaptureModeBytes
            && !sourceStateId.empty() && sourceStateId.size() <= 1024U
            && sourceStateId.find_first_of("\r\n") == std::string::npos
            && sourceStateId.find('\0') == std::string::npos
            && generation != 0U && submissionSerial != 0U
            && readbackSubmissionSerial == submissionSerial
            && fieldWidth != 0U && fieldWidth <= kMaxCanonicalCaptureFieldExtent
            && fieldHeight != 0U && fieldHeight <= kMaxCanonicalCaptureFieldExtent
            && std::isfinite(kernelTime) && std::isfinite(acceptedControlTime)
            && std::isfinite(productionDelta) && productionDelta >= 0.0f
            && (seedImageInput ? captureSeedImageSiblingName(seedImageFile) : seedImageFile.empty());
    }

    [[nodiscard]] bool matchesWorldMode(const SculptureWorldMode& world) const noexcept {
        if (!valid() || generation != world.selectedCanonicalFeedbackGeneration
            || resetEpoch != world.selectedCanonicalFeedbackResetEpoch) return false;
        if (!association) return sourceStateId == world.presetName;
        const auto& joined = *association;
        return joined.sourceStateId == sourceStateId && joined.presetId == world.presetName
            && joined.presetRevision == world.presetRevision
            && joined.productionReceiptSequence != 0U
            && joined.productionReceiptSequence == world.selectedCanonicalFeedbackProductionReceiptSequence
            && joined.frame.valid() && joined.frame.frameSequence == world.presentationFrameSequence
            && joined.frame.deviceGeneration == world.presentationDeviceGeneration
            && joined.frame.surfaceGeneration == world.presentationSurfaceGeneration
            && world.presentationOutcomeKnown && submissionSerial == world.captureSubmissionSerial;
    }
};

// -----------------------------------------------------------------------------
// Educational Annotation: State Recorder
// This module encapsulates the logic for taking a visual snapshot (PNG) and
// deterministic state snapshots (.world and .epimode JSON format) of the engine.
// By decoupling file I/O from the rendering loop, the engine remains highly performant
// and modular.
// -----------------------------------------------------------------------------
class SculptureStateRecorder {
public:
    [[nodiscard]] static std::filesystem::path canonicalSeedImagePath(const std::string& pngPath) {
        return std::filesystem::path(pngPath).replace_extension(".seed-image");
    }

    // A capture owns the resolved world state submitted for this frame.  Build
    // that immutable payload before queuing asynchronous readback so future
    // world-state fields cannot be accidentally reconstructed or omitted.
    [[nodiscard]] static SculptureWorldMode makeCaptureWorldMode(
        const SculptureWorldState& resolvedState,
        std::string presetName,
        std::string presetRevision,
        SculptureProgramParams params,
        std::vector<std::string> routingMap) {
        return SculptureWorldMode{
            .version = SculptureWorldMode::kCurrentVersion,
            .state = resolvedState,
            .presetName = std::move(presetName),
            .presetRevision = std::move(presetRevision),
            .params = std::move(params),
            .routingMap = std::move(routingMap),
        };
    }

    [[nodiscard]] static SculptureWorldMode makeCaptureWorldMode(
        const SculptureWorldState& resolvedState,
        std::string presetName,
        SculptureProgramParams params,
        std::vector<std::string> routingMap) {
        return makeCaptureWorldMode(resolvedState, std::move(presetName), "", std::move(params), std::move(routingMap));
    }

    static SnapshotPaths generatePaths(const std::string& directory = "snapshots",
                                       std::string_view presetName = "",
                                       std::string_view customTag = "") {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now{};
        localtime_r(&time_t_now, &tm_now);
        char timeStr[64];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d_%H-%M-%S", &tm_now);

        // Spacebar capture is a live-performance action: several requests can
        // arrive in one second.  Milliseconds make paths readable, while the
        // monotonic suffix prevents a same-tick overwrite of the PNG/.epimode
        // pair on every supported platform.
        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count() % 1000;
        const auto sequence = captureSequence_.fetch_add(1U, std::memory_order_relaxed);
        char suffix[96];
        std::snprintf(suffix, sizeof(suffix), "%s_%03lld_%llu", timeStr,
                      static_cast<long long>(milliseconds),
                      static_cast<unsigned long long>(sequence));

        std::string tagPart;
        if (!customTag.empty()) {
            std::string cleanTag;
            cleanTag.reserve(customTag.size());
            for (char c : customTag) {
                if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
                    cleanTag.push_back(c);
                } else if (c == '/' || c == '\\' || c == ' ') {
                    cleanTag.push_back('_');
                }
            }
            if (!cleanTag.empty()) {
                tagPart = "_" + cleanTag;
            }
        }

        std::string baseName;
        if (!presetName.empty()) {
            std::string cleanName;
            cleanName.reserve(presetName.size());
            for (char c : presetName) {
                if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
                    cleanName.push_back(c);
                } else if (c == '/' || c == '\\' || c == ' ') {
                    cleanName.push_back('_');
                }
            }
            baseName = directory.empty() ? ("capture_" + cleanName + tagPart + "_" + suffix)
                                         : (directory + "/capture_" + cleanName + tagPart + "_" + suffix);
        } else {
            baseName = directory.empty() ? ("snapshot" + tagPart + "_" + suffix)
                                         : (directory + "/snapshot" + tagPart + "_" + suffix);
        }

        return SnapshotPaths{
            .pngPath = baseName + ".png",
            .worldPath = baseName + ".world",
            .epimodePath = baseName + ".epimode",
            .epiPath = baseName + ".epi",
            .profilePath = baseName + ".profile",
            .manifestPath = baseName + ".manifest",
            .volmemPath = baseName + ".volmem",
            .causalPath = baseName + ".causal.json",
            .timestamp = std::string(suffix)
        };
    }

    // Generation-specific siblings prevent a failed new capture from replacing
    // an earlier certificate or any of its attachments. Randomness separates
    // concurrent processes; it is an identity, not an authentication token.
    static SnapshotPaths generateV2Paths(const std::string& directory, std::string_view presetName) {
        auto paths = generatePaths(directory, presetName);
        std::random_device random;
        char nonce[33];
        std::snprintf(nonce, sizeof(nonce), "%08x%08x%08x%08x", random(), random(), random(), random());
        const auto extend = [&](std::string& path, std::string_view extension) {
            const auto base = std::filesystem::path(paths.manifestPath).replace_extension("").string();
            path = base + "_" + nonce + std::string(extension);
        };
        // Compute from the original anchor before replacing the manifest.
        extend(paths.pngPath, ".png"); extend(paths.worldPath, ".world");
        extend(paths.epimodePath, ".epimode"); extend(paths.epiPath, ".epi");
        extend(paths.profilePath, ".profile"); extend(paths.volmemPath, ".volmem");
        extend(paths.causalPath, ".causal.json");
        extend(paths.manifestPath, ".manifest");
        return paths;
    }

    // External attachment producers must call this before replacing a sidecar.
    // Otherwise an older certificate could still describe the replaced file if
    // the subsequent snapshot fails or never reaches writeSnapshot.
    [[nodiscard]] static bool prepareSnapshotAttachments(const SnapshotPaths& paths,
                                                         bool includeProfile = false,
                                                         bool includeCanonical = false) {
        if (!captureReferencesAdmitted(paths.pngPath, paths.worldPath, paths.epimodePath,
                                       paths.epiPath, paths.volmemPath, paths.causalPath,
                                       includeProfile, includeCanonical)) return false;
        return retireCaptureManifest(captureManifestPath(
            paths.pngPath, paths.worldPath, paths.epimodePath, paths.epiPath));
    }

    // Unlike the v1 replacement path, a v2 attempt never retires an earlier
    // certificate. Check all names before the external causal writer runs.
    [[nodiscard]] static bool prepareV2SnapshotAttachments(const SnapshotPaths& paths,
                                                           bool includeProfile = false,
                                                           bool includeCanonical = false) {
        if (!captureReferencesAdmitted(paths.pngPath, paths.worldPath, paths.epimodePath,
                                       paths.epiPath, paths.volmemPath, paths.causalPath,
                                       includeProfile, includeCanonical)) return false;
        for (const auto& path : {paths.pngPath, paths.worldPath, paths.epimodePath, paths.epiPath,
                paths.profilePath, paths.causalPath, paths.manifestPath, paths.volmemPath,
                epimodeJsonPath(paths.epimodePath),
                std::filesystem::path(paths.pngPath).replace_extension(".canonical.epimode").string(),
                canonicalSeedImagePath(paths.pngPath).string()}) {
            if (path.empty()) continue;
            std::error_code error;
            const auto status = std::filesystem::symlink_status(path, error);
            if (error && error != std::errc::no_such_file_or_directory) return false;
            if (!error && status.type() != std::filesystem::file_type::not_found) return false;
        }
        return true;
    }

    static bool writeSnapshot(const std::string& pngPath,
                             const std::string& worldPath,
                             const std::string& epimodePath,
                             const std::string& epiPath,
                             std::uint32_t width,
                             std::uint32_t height,
                             const std::uint8_t* pixelData,
                             const SculptureWorldMode& worldMode,
                             const WorldProfileSummary* profileSummary = nullptr,
                             const std::string& volmemPath = "",
                             const std::string& causalPath = "",
                             const CanonicalFeedbackCaptureState* canonicalFeedback = nullptr,
                             const CaptureAttachmentWrites& attachments = {},
                             CapturePixelOrder pixelOrder = CapturePixelOrder::Bgra,
                             const CaptureStereoIntent* stereoIntent = nullptr,
                             bool configurationV2 = false) {
        // The certificate stores basenames resolved against its own parent.
        // Admit all potential references before retiring or writing anything.
        if (!captureReferencesAdmitted(pngPath, worldPath, epimodePath, epiPath,
                                       volmemPath, causalPath, profileSummary != nullptr,
                                       canonicalFeedback != nullptr)) return false;
        const bool writePixels = width > 0 && height > 0 && pixelData != nullptr;
        if (writePixels && !capturePixelOrderValid(pixelOrder)) return false;
        std::size_t pixelBytes = 0;
        if (writePixels) {
            // The raw-pointer API cannot prove buffer length. Check dimensions
            // and encoder arithmetic before allocation, pixel reads or file IO.
            const auto pixels = std::uint64_t{width} * height;
            if (pixels > std::numeric_limits<std::size_t>::max() / 4U) return false;
            pixelBytes = static_cast<std::size_t>(pixels) * 4U;
            const auto error = capturePixelLayoutError(width, height, pixelBytes);
            if (!error.empty()) {
                std::fprintf(stderr, "[SculptureStateRecorder] %.*s\n",
                             static_cast<int>(error.size()), error.data());
                return false;
            }
        }
        if (configurationV2) {
            // A v2 generation is never a volume checkpoint and must carry the
            // submitted frame receipt and complete required state.
            if (!writePixels || std::uint64_t(width)*height > kCaptureV2MaxPixels || worldPath.empty() || attachments.volmem ||
                worldMode.captureSubmissionSerial == 0 || !worldMode.presentationOutcomeKnown ||
                worldMode.presentationFrameSequence == 0 || worldMode.presentationDeviceGeneration == 0 ||
                worldMode.presentationSurfaceGeneration == 0) return false;
            for (const auto& path : {pngPath, worldPath, epimodePath, epiPath, causalPath,
                     captureManifestPath(pngPath, worldPath, epimodePath, epiPath).string(),
                     std::filesystem::path(worldPath).replace_extension(".profile").string(),
                     std::filesystem::path(pngPath).replace_extension(".canonical.epimode").string(),
                     canonicalSeedImagePath(pngPath).string()}) {
                if (path.empty() || (path == causalPath && attachments.causal)) continue;
                std::error_code ec;
                if (std::filesystem::exists(path, ec) || ec) return false;
            }
        }
        if (canonicalFeedback != nullptr) {
            if (!canonicalFeedback->matchesWorldMode(worldMode) || pngPath.empty() || !writePixels) {
                std::fprintf(stderr, "[SculptureStateRecorder] Invalid canonical capture identity or payload\n");
                return false;
            }
            if (canonicalFeedback->seedImageInput
                && canonicalFeedback->seedImageFile != canonicalSeedImagePath(pngPath).filename().string())
                return false;
            // A failed replacement must not leave an older manifest certifying
            // partially overwritten files. Live captures already use unique paths.
            const auto manifest = captureManifestPath(pngPath, worldPath, epimodePath, epiPath);
            std::error_code error;
            if (std::filesystem::exists(manifest, error) || error) return false;
        } else if (!retireCaptureManifest(
                       captureManifestPath(pngPath, worldPath, epimodePath, epiPath))) {
            std::fprintf(stderr, "[SculptureStateRecorder] Cannot retire previous capture manifest\n");
            return false;
        }
        // Skipping optional pixels admits state-only capture, but does not
        // establish that an image was written by this invocation.
        bool pngWritten = false;
        if (writePixels) {
            // The encoder is synchronous and consumes RGBA. Borrow native
            // RGBA directly; only BGRA requires an owned channel swap.
            std::vector<std::uint8_t> rgba;
            const auto* rgbaPixels = captureRgbaPixels(pixelData, pixelBytes, pixelOrder, rgba);

            const bool published = publishPng(pngPath, width, height, rgbaPixels);
            if (published) {
                pngWritten = true;
                std::fprintf(stderr, "[SculptureStateRecorder] Captured frame to %s\n", pngPath.c_str());
            } else {
                std::fprintf(stderr, "[SculptureStateRecorder] Failed to write %s\n", pngPath.c_str());
                // A failed frame must not replace recorder-owned state files.
                return false;
            }
        }

        // Each serializer's checked publication transaction owns parent creation
        // and errors. Repeating setup here adds IO and splits that ownership.
        bool worldSuccess = false;
        if (!worldPath.empty()) {
            worldSuccess = SculptureSerializer::saveWorldModeToFile(worldPath, worldMode);
            if (worldSuccess) {
                std::fprintf(stderr, "[SculptureStateRecorder] Captured world state to %s\n", worldPath.c_str());
            } else {
                std::fprintf(stderr, "[SculptureStateRecorder] Failed to write %s\n", worldPath.c_str());
            }
        }

        bool epimodeSuccess = false;
        if (!epimodePath.empty()) {
            epimodeSuccess = SculptureSerializer::saveEpimodeToFile(epimodePath, worldMode);
            if (epimodeSuccess) {
                std::fprintf(stderr, "[SculptureStateRecorder] Captured epimode settings to %s\n", epimodePath.c_str());
            } else {
                std::fprintf(stderr, "[SculptureStateRecorder] Failed to write %s\n", epimodePath.c_str());
            }
            
            // Also dump a .json copy for exact JSON requirement
            const std::string jsonPath = epimodeJsonPath(epimodePath);
            if (SculptureSerializer::saveEpimodeToFile(jsonPath, worldMode)) {
                std::fprintf(stderr, "[SculptureStateRecorder] Captured json settings to %s\n", jsonPath.c_str());
            }
        }

        bool epiSuccess = false;
        if (!epiPath.empty()) {
            // The .epi artifact preserves the complete world mode state
            epiSuccess = SculptureSerializer::saveWorldModeToFile(epiPath, worldMode);
            if (epiSuccess) {
                std::fprintf(stderr, "[SculptureStateRecorder] Captured epi state bundle to %s\n", epiPath.c_str());
            } else {
                std::fprintf(stderr, "[SculptureStateRecorder] Failed to write %s\n", epiPath.c_str());
            }
        }

        bool profileSuccess = true;
        if (profileSummary != nullptr) {
            const std::filesystem::path profilePath =
                std::filesystem::path(worldPath).replace_extension(".profile");
            profileSuccess = SculptureWorldProfileSidecar::writeToFile(
                profilePath.string(), *profileSummary);
            if (profileSuccess) {
                std::fprintf(stderr,
                    "[SculptureStateRecorder] Captured world profile to %s\n",
                    profilePath.string().c_str());
            } else {
                std::fprintf(stderr,
                    "[SculptureStateRecorder] Failed to write world profile to %s\n",
                    profilePath.string().c_str());
            }
        }

        const bool captureSuccess = (worldSuccess || epimodeSuccess || epiSuccess)
            && profileSuccess && (canonicalFeedback == nullptr
                || ((worldPath.empty() || worldSuccess)
                    && (epimodePath.empty() || epimodeSuccess)
                    && (epiPath.empty() || epiSuccess)));
        if (!captureSuccess) return false;

        if (canonicalFeedback != nullptr) {
            if (canonicalFeedback->seedImageInput) {
                const auto asset = canonicalSeedImagePath(pngPath);
                const auto bytes = canonicalFeedback->seedImageInput->encoded();
                if (!file_publication_detail::publishFile(asset.string(), [&](int fd) {
                        return file_publication_detail::writeAll(fd, bytes.data(), bytes.size());
                    })) return false;
            }
            const auto path = std::filesystem::path(pngPath).replace_extension(".canonical.epimode");
            if (!file_publication_detail::publishTextFile(path.string(), [&] {
                    return canonicalFeedback->serializedMode;
                })) return false;
        }

        // Publish last, naming only recorder-owned artifacts completed here.
        // External attachment evidence comes from the owning writers.
        return writeCaptureManifest(
            pngPath, worldPath, epimodePath, epiPath,
            SnapshotWriteResults{
                .png = pngWritten,
                .world = worldSuccess,
                .epimode = epimodeSuccess,
                .epi = epiSuccess,
                .profile = profileSummary != nullptr && profileSuccess,
            },
            volmemPath, causalPath, &worldMode, canonicalFeedback, attachments, stereoIntent, configurationV2, width, height);
    }

    // Backwards-compatible 8-argument overload
    static bool writeSnapshot(const std::string& pngPath,
                             const std::string& worldPath,
                             const std::string& epimodePath,
                             std::uint32_t width,
                             std::uint32_t height,
                             const std::uint8_t* bgraPixels,
                             const SculptureWorldMode& worldMode,
                             const WorldProfileSummary* profileSummary = nullptr) {
        std::string epiPath;
        if (!worldPath.empty()) {
            epiPath = std::filesystem::path(worldPath).replace_extension(".epi").string();
        } else if (!epimodePath.empty()) {
            epiPath = std::filesystem::path(epimodePath).replace_extension(".epi").string();
        }
        return writeSnapshot(pngPath, worldPath, epimodePath, epiPath, width, height,
                             bgraPixels, worldMode, profileSummary);
    }

    // Backwards-compatible 6-argument overload
    static bool writeSnapshot(const std::string& pngPath,
                             const std::string& sidecarPath,
                             std::uint32_t width,
                             std::uint32_t height,
                             const std::uint8_t* bgraPixels,
                             const SculptureWorldMode& worldMode) {
        if (sidecarPath.find(".world") != std::string::npos) {
            return writeSnapshot(pngPath, sidecarPath, "", width, height, bgraPixels, worldMode);
        }
        return writeSnapshot(pngPath, "", sidecarPath, width, height, bgraPixels, worldMode);
    }

    template <typename TCaptureResult>
    static bool captureSequenceFrame(const TCaptureResult& res, uint32_t frameIdx,
                                     const std::string& directory = "snapshots",
                                     CapturePixelOrder pixelOrder = CapturePixelOrder::Bgra) {
        try {
            char leaf[32];
            std::snprintf(leaf, sizeof(leaf), "frame_%05u.png", frameIdx);
            const auto path = std::filesystem::path(directory) / leaf;
            const uint8_t* pixelData = nullptr;
            if constexpr (requires { res.pixels.data(); }) { pixelData = res.pixels.data(); } else { pixelData = res.pixels; }
            const auto bytes = std::uint64_t{res.width} * res.height * 4;
            if (!capturePixelOrderValid(pixelOrder) || !pixelData || !res.width || !res.height || bytes > std::numeric_limits<std::size_t>::max()
                || !capturePixelLayoutError(res.width, res.height, static_cast<std::size_t>(bytes)).empty())
                throw std::runtime_error("Invalid sequence pixel layout");
            if constexpr (requires { res.pixels.size(); }) {
                if (res.pixels.size() < bytes)
                    throw std::runtime_error("Sequence pixel buffer is truncated");
            }
            std::vector<std::uint8_t> rgba;
            const auto* rgbaPixels = captureRgbaPixels(
                pixelData, static_cast<std::size_t>(bytes), pixelOrder, rgba);
            const auto filename = path.string();
            if (!publishPng(filename, res.width, res.height, rgbaPixels))
                throw std::runtime_error("PNG publication failed");
            std::fprintf(stderr, "[Capture] Sequence frame %u -> %s\n", frameIdx, filename.c_str());
            return true;
        } catch (const std::exception& error) {
            std::fprintf(stderr, "[Capture] Sequence frame %u failed: %s\n", frameIdx, error.what());
            return false;
        } catch (...) {
            std::fprintf(stderr, "[Capture] Sequence frame %u failed: unknown error\n", frameIdx);
            return false;
        }
    }

    template <typename TCaptureResult>
    static bool captureFrameAndStateWithPaths(const TCaptureResult& res,
                                             const SculptureWorldMode& worldMode,
                                             const SnapshotPaths& paths,
                                             const WorldProfileSummary* profileSummary = nullptr,
                                             bool causalAttached = false,
                                             const CanonicalFeedbackCaptureState* canonicalFeedback = nullptr,
                                             CaptureAttachmentWrites attachments = {},
                                             CapturePixelOrder pixelOrder = CapturePixelOrder::Bgra,
                                             const CaptureStereoIntent* stereoIntent = nullptr,
                                             bool configurationV2 = false) {
        const uint8_t* pixelData = nullptr;
        if constexpr (requires { res.pixels.data(); }) {
            pixelData = res.pixels.data();
        } else {
            pixelData = res.pixels;
        }
        // This entry point promises a frame. State-only saves use writeSnapshot
        // directly; a missing image must not produce a successful bundle.
        if (!capturePixelOrderValid(pixelOrder) || paths.pngPath.empty() || pixelData == nullptr || res.width == 0 || res.height == 0)
            return false;
        std::size_t availableBytes = std::numeric_limits<std::size_t>::max();
        // Borrowed pointers carry no length; owned containers do. Validate
        // all available evidence before retiring a manifest or writing files.
        if constexpr (requires { res.pixels.data(); res.pixels.size(); }) {
            availableBytes = res.pixels.size();
        }
        if (!capturePixelLayoutError(res.width, res.height, availableBytes).empty()) return false;
        // Preserve the existing causal-bool adapter while sharing the explicit
        // write-result contract with other external producers.
        attachments.causal = attachments.causal || causalAttached;
        return writeSnapshot(paths.pngPath, paths.worldPath, paths.epimodePath, paths.epiPath, res.width, res.height,
                             pixelData, worldMode, profileSummary, paths.volmemPath,
                             paths.causalPath, canonicalFeedback, attachments, pixelOrder, stereoIntent, configurationV2);
    }

    template <typename TCaptureResult>
    static std::optional<SnapshotPaths> captureFrameAndState(const TCaptureResult& res,
                                             const SculptureWorldMode& worldMode,
                                             const std::string& directory = "snapshots",
                                             const WorldProfileSummary* profileSummary = nullptr,
                                             const std::string& volmemPath = "",
                                             std::string_view customTag = "",
                                             const CaptureAttachmentWrites& attachments = {}) {
        // Paths become a completion result only after the manifest is written.
        try {
            SnapshotPaths paths = generatePaths(directory, worldMode.presetName, customTag);
            if (!volmemPath.empty()) paths.volmemPath = volmemPath;
            if (captureFrameAndStateWithPaths(res, worldMode, paths, profileSummary,
                                              false, nullptr, attachments)) {
                return paths;
            }
            std::fprintf(stderr, "[SculptureStateRecorder] Capture bundle was not published: %s\n",
                         paths.manifestPath.c_str());
        } catch (const std::exception& error) {
            std::fprintf(stderr, "[SculptureStateRecorder] Capture bundle failed (%s): %s\n",
                         directory.c_str(), error.what());
        } catch (...) {
            std::fprintf(stderr, "[SculptureStateRecorder] Capture bundle failed (%s): unknown error\n",
                         directory.c_str());
        }
        return std::nullopt;
    }

private:
    friend struct SculptureCaptureManifestTestAccess;

    // The caller validates layout and owns RGBA pixels. Publish only a complete
    // encoded file; stb's callback reports write failures through a sticky flag.
    [[nodiscard]] static bool publishPng(const std::string& path, std::uint32_t width,
                                         std::uint32_t height, const std::uint8_t* rgba) {
        return file_publication_detail::publishFile(path, [&](int fd) {
            struct Sink { int fd; bool failed = false; } sink{fd};
            const auto write = [](void* context, void* data, int size) {
                auto& output = *static_cast<Sink*>(context);
                if (output.failed) return;
                if (size < 0 || !file_publication_detail::writeAll(
                        output.fd, static_cast<const char*>(data), static_cast<std::size_t>(size))) {
                    output.failed = true;
                }
            };
            const int encoded = stbi_write_png_to_func(write, &sink, static_cast<int>(width),
                static_cast<int>(height), 4, rgba, static_cast<int>(width * 4U));
            return encoded != 0 && !sink.failed;
        });
    }

    // Writer results are evidence for this attempt. A preserved older file at
    // a failed destination must never substitute for a successful write.
    struct SnapshotWriteResults {
        bool png = false;
        bool world = false;
        bool epimode = false;
        bool epi = false;
        bool profile = false;
    };

    [[nodiscard]] static std::filesystem::path captureManifestPath(
        const std::string& pngPath, const std::string& worldPath,
        const std::string& epimodePath, const std::string& epiPath) {
        const std::filesystem::path anchor = !worldPath.empty()
            ? std::filesystem::path(worldPath)
            : !epimodePath.empty() ? std::filesystem::path(epimodePath)
            : !epiPath.empty() ? std::filesystem::path(epiPath)
            : std::filesystem::path(pngPath);
        return anchor.empty() ? std::filesystem::path{}
            : anchor.parent_path() / (anchor.stem().string() + ".manifest");
    }

    [[nodiscard]] static std::string epimodeJsonPath(const std::string& path) {
        return path.ends_with(".epimode") ? path.substr(0, path.size() - 8) + ".json"
                                          : path + ".json";
    }

    // Compare physical parents, resolving existing symlinks and missing suffixes.
    // As with file publication, this assumes no concurrent parent replacement.
    [[nodiscard]] static bool captureReferencesAdmitted(
        const std::string& png, const std::string& world, const std::string& epimode,
        const std::string& epi, const std::string& volmem, const std::string& causal,
        bool includeProfile, bool includeCanonical) {
        const auto manifest = captureManifestPath(png, world, epimode, epi);
        if (manifest.empty() || (includeProfile && world.empty()) || (includeCanonical && png.empty())) return false;
        std::error_code error;
        auto parent = std::filesystem::absolute(manifest.parent_path().empty()
            ? std::filesystem::path(".") : manifest.parent_path(), error);
        if (error) return false;
        const auto manifestParent = std::filesystem::weakly_canonical(parent, error);
        if (error) return false;
        std::vector<std::string> leaves{manifest.filename().string()};
        const auto addLeaf = [&leaves](const std::string& leaf) {
            for (const auto& prior : leaves) if (prior == leaf) return false;
            leaves.push_back(leaf);
            return true;
        };
        for (const auto* value : {&png, &world, &epimode, &epi, &volmem, &causal}) {
            if (value->empty()) continue;
            if (value->find('\0') != std::string::npos) return false;
            const std::filesystem::path path(*value);
            const auto leaf = path.filename().string();
            if (!captureManifestSiblingName(leaf)) return false;
            if (!addLeaf(leaf)) return false;
            parent = std::filesystem::absolute(path.parent_path().empty()
                ? std::filesystem::path(".") : path.parent_path(), error);
            if (error || std::filesystem::weakly_canonical(parent, error) != manifestParent || error)
                return false;
        }
        if (!epimode.empty() && !addLeaf(std::filesystem::path(epimodeJsonPath(epimode)).filename().string())) return false;
        if (includeProfile && !addLeaf(std::filesystem::path(world).replace_extension(".profile").filename().string())) return false;
        if (includeCanonical && !addLeaf(std::filesystem::path(png).replace_extension(".canonical.epimode").filename().string())) return false;
        // Reserve this derived sibling even before optional input preparation;
        // it must never overwrite world/volume/causal or another capture output.
        if (includeCanonical && !addLeaf(canonicalSeedImagePath(png).filename().string())) return false;
        return true;
    }

    // Ordinary replacement can partially change artifacts before a later sink
    // fails. Retire the prior certificate first; only last publication may
    // certify this attempt. Do not unlink symlinks or nonregular entries.
    [[nodiscard]] static bool retireCaptureManifest(const std::filesystem::path& path) {
        if (path.empty()) return false;
        std::error_code error;
        const auto status = std::filesystem::symlink_status(path, error);
        if (error) return error == std::errc::no_such_file_or_directory;
        if (status.type() == std::filesystem::file_type::not_found) return true;
        if (!std::filesystem::is_regular_file(status)) return false;
        return std::filesystem::remove(path, error) && !error;
    }

    [[nodiscard]] static bool writeCaptureManifest(
        const std::string& pngPath,
        const std::string& worldPath,
        const std::string& epimodePath,
        const std::string& epiPath,
        const SnapshotWriteResults& written,
        const std::string& volmemPath = "",
        const std::string& causalPath = "",
        const SculptureWorldMode* worldMode = nullptr,
        const CanonicalFeedbackCaptureState* canonicalFeedback = nullptr,
        const CaptureAttachmentWrites& attachments = {},
        const CaptureStereoIntent* stereoIntent = nullptr,
        bool configurationV2 = false, std::uint32_t captureWidth = 0, std::uint32_t captureHeight = 0) {
        const auto manifestPath = captureManifestPath(pngPath, worldPath, epimodePath, epiPath);
        if (manifestPath.empty()) return false;
        const std::filesystem::path profilePath =
            std::filesystem::path(worldPath).replace_extension(".profile");
        const auto relativeIfWritten = [](const std::string& value, bool completed) {
            return !completed || value.empty() ? std::string("none")
                : std::filesystem::path(value).filename().string();
        };
        std::ostringstream content;
        content << (configurationV2 ? "format=epic-capture-bundle-v2\n" : "format=epic-capture-bundle-v1\n")
                << "png=" << relativeIfWritten(pngPath, written.png) << '\n'
                << "world=" << relativeIfWritten(worldPath, written.world) << '\n'
                << "epimode=" << relativeIfWritten(epimodePath, written.epimode) << '\n'
                << "epi=" << relativeIfWritten(epiPath, written.epi) << '\n'
                << "profile=" << (written.profile
                    ? profilePath.filename().string() : "none") << '\n'
                << "volmem=" << relativeIfWritten(volmemPath, attachments.volmem) << '\n'
                << "causal=" << relativeIfWritten(causalPath, attachments.causal) << '\n';
        if (worldMode != nullptr) {
            content << "compact_generation=" << worldMode->compactGeneration << '\n'
                    << "world_generation=" << worldMode->worldGeneration << '\n'
                    << "implicit_generation=" << worldMode->implicitGeneration << '\n'
                    << "canonical_feedback_generation=" << worldMode->canonicalFeedbackGeneration << '\n'
                    << "presentation_frame_sequence=" << worldMode->presentationFrameSequence << '\n'
                    << "presentation_device_generation=" << worldMode->presentationDeviceGeneration << '\n'
                    << "presentation_surface_generation=" << worldMode->presentationSurfaceGeneration << '\n'
                    << "capture_submission_serial=" << worldMode->captureSubmissionSerial << '\n'
                    << "selected_compact_generation=" << worldMode->selectedCompactGeneration << '\n'
                    << "selected_compact_production_receipt_sequence="
                    << worldMode->selectedCompactProductionReceiptSequence << '\n'
                    << "selected_world_generation=" << worldMode->selectedWorldGeneration << '\n'
                    << "selected_world_production_receipt_sequence="
                    << worldMode->selectedWorldProductionReceiptSequence << '\n'
                    << "selected_canonical_feedback_generation=" << worldMode->selectedCanonicalFeedbackGeneration << '\n'
                    << "selected_canonical_feedback_reset_epoch=" << worldMode->selectedCanonicalFeedbackResetEpoch << '\n'
                    << "selected_canonical_feedback_production_receipt_sequence="
                    << worldMode->selectedCanonicalFeedbackProductionReceiptSequence << '\n'
                    << "selected_implicit_generation=" << worldMode->selectedImplicitGeneration << '\n'
                    << "selected_implicit_production_receipt_sequence="
                    << worldMode->selectedImplicitProductionReceiptSequence << '\n'
                    << "presentation_outcome=" << worldMode->presentationOutcome << '\n'
                    << "presentation_outcome_known=" << (worldMode->presentationOutcomeKnown ? 1 : 0) << '\n'
                    << "presentation_confirmed=" << (worldMode->presentationConfirmed ? 1 : 0) << '\n'
                    << "reduction_id=" << static_cast<std::uint32_t>(worldMode->params.reduction) << '\n'
                    << "color_effect_id=" << static_cast<std::uint32_t>(worldMode->params.colorEffect) << '\n'
                    << "inner_id=" << static_cast<std::uint32_t>(worldMode->params.inner) << '\n'
                    << "seed_id=" << static_cast<std::uint32_t>(worldMode->params.seedType) << '\n'
                    << "sculpture_stage_mask=" << worldMode->params.stageMask << '\n';
        }
        if (canonicalFeedback != nullptr) {
            const auto path = std::filesystem::path(pngPath).replace_extension(".canonical.epimode");
            content.imbue(std::locale::classic());
            if (canonicalFeedback->seedImageInput)
                content << "canonical_feedback_seed_image=" << canonicalFeedback->seedImageFile << '\n';
            content << std::setprecision(std::numeric_limits<float>::max_digits10)
                    << "canonical_feedback_capture_version=1\n"
                    << "canonical_feedback_mode=" << path.filename().string() << '\n'
                    << "canonical_feedback_mode_role=canonical-producer-input\n"
                    << "canonical_feedback_sculpture_params_role=presentation-and-current-control-projection\n"
                    << "canonical_feedback_source=" << std::quoted(canonicalFeedback->sourceStateId) << '\n'
                    << "canonical_feedback_field_generation=" << canonicalFeedback->generation << '\n'
                    << "canonical_feedback_field_reset_epoch=" << canonicalFeedback->resetEpoch << '\n'
                    << "canonical_feedback_field_width=" << canonicalFeedback->fieldWidth << '\n'
                    << "canonical_feedback_field_height=" << canonicalFeedback->fieldHeight << '\n'
                    << "canonical_feedback_kernel_time=" << canonicalFeedback->kernelTime << '\n'
                    << "canonical_feedback_accepted_control_time=" << canonicalFeedback->acceptedControlTime << '\n'
                    << "canonical_feedback_production_delta=" << canonicalFeedback->productionDelta << '\n'
                    << "canonical_feedback_submission_serial=" << canonicalFeedback->submissionSerial << '\n'
                    << "canonical_feedback_readback_submission_serial=" << canonicalFeedback->readbackSubmissionSerial << '\n'
                    << "canonical_feedback_native_identity_scope=single-host-device-lifetime\n"
                    << "canonical_feedback_mode_dimensions=authored-input\n"
                    << "canonical_feedback_recursive_field_checkpoint=false\n"
                    << "canonical_feedback_engine_motion_checkpoint=false\n"
                    << "canonical_feedback_exact_visual_replay=false\n";
        }

        if (configurationV2) {
            if (!written.png || !written.world || attachments.volmem || !captureWidth || !captureHeight ||
                std::uint64_t(captureWidth)*captureHeight > kCaptureV2MaxPixels) return false;
            content << "capture_width=" << captureWidth << '\n'
                    << "capture_height=" << captureHeight << '\n';
            const auto generation = manifestPath.stem().string();
            if (!captureV2Leaf(generation) || generation.size() > 200) return false;
            content << "capture_generation=" << generation << '\n';
            const std::array<std::pair<std::string, bool>, kCaptureV2Roles> sources{{
                {pngPath, written.png}, {worldPath, written.world},
                {epimodePath, written.epimode}, {epiPath, written.epi},
                {profilePath.string(), written.profile}, {causalPath, attachments.causal},
                {std::filesystem::path(pngPath).replace_extension(".canonical.epimode").string(), canonicalFeedback != nullptr},
                {canonicalSeedImagePath(pngPath).string(), canonicalFeedback && canonicalFeedback->seedImageInput}
            }};
            std::size_t total = 0;
            try {
                for (std::size_t i = 0; i < sources.size(); ++i) {
                    if (!sources[i].second) continue;
                    const auto name = std::filesystem::path(sources[i].first).filename().string();
                    if (!captureV2Leaf(name) || !name.starts_with(generation + ".")) return false;
                    const auto bytes = epi::io::readBoundedText(sources[i].first, kCaptureV2ByteLimits[i], "capture output");
                    if (bytes.empty() || bytes.size() > kCaptureV2TotalBytes - total) return false;
                    total += bytes.size();
                    content << "attachment=" << kCaptureV2RoleNames[i] << '|' << name << '|'
                            << bytes.size() << '|' << captureV2Hex(captureV2Hash(bytes)) << '\n';
                }
            } catch (const std::exception&) { return false; }
        }
        if (!content) return false;
        std::string payload = content.str();
        if (!captureManifestTextAdmitted(payload)) return false;
        // Optional inspection fields never displace required capture evidence.
        // Keep the base manifest publishable even when it is near the byte cap.
        if (worldMode != nullptr) {
            const auto appendRecipeField = [&payload](std::string_view key, std::string_view value) {
                const auto encoded = encodeCaptureRecipeMetadata(value);
                if (!encoded) return;
                const auto extra = key.size() + 1U + encoded->size() + 1U;
                if (extra > kMaxCaptureManifestBytes - payload.size()) return;
                payload.append(key);
                payload.push_back('=');
                payload.append(*encoded);
                payload.push_back('\n');
            };
            appendRecipeField("preset_id_hex", worldMode->presetName);
            appendRecipeField("preset_revision_hex", worldMode->presetRevision);
        }
        // Optional descriptive intent cannot displace required evidence or
        // partially publish its versioned block. The pointer is borrowed only
        // during this synchronous write, never retained by the recorder.
        if (stereoIntent != nullptr && written.png) {
            try {
                const auto stereo = serializeCaptureStereoIntent(*stereoIntent);
                if (stereo && stereo->size() <= kMaxCaptureManifestBytes - payload.size())
                    payload.append(*stereo);
            } catch (const std::exception&) {
                // Preserve the otherwise complete capture without this metadata.
            }
        }
        // Reserve an exclusive sibling instead of opening a predictable .tmp.
        // Publication certifies this recorder attempt only after complete IO.
        return file_publication_detail::publishTextFile(
            manifestPath.string(), [&] { return std::move(payload); });
    }

    inline static std::atomic<std::uint64_t> captureSequence_{0U};
};

} // namespace epi_sculpt
