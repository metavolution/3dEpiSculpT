#include "SculptureCanonicalCaptureRestore.h"
#include "SculptureCaptureIntegrity.h"
#include "epi/backend/seed_image_path_policy.hpp"

// A private codec instance keeps renderer-free authoring independent of native
// backend headers and avoids exporting another global STB implementation.
#define STB_IMAGE_STATIC
#define STBIDEF static inline
#define STB_IMAGE_IMPLEMENTATION
#include "epi/third_party/stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION
#undef STB_IMAGE_STATIC
#undef STBIDEF
#include "epi/backend/seed_image_decode.hpp"

#include <new>
#include <algorithm>
#include <iterator>

namespace epi_sculpt::canonical_capture_detail {

epi::backend::seed_image_runtime::SeedImageDecodeStatus prepareSeedImageResource(
    epi::VisualMode& mode, const std::filesystem::path& manifest,
    epi::SeedImageInput::Owner* retained, epi::SeedImageRetentionBudget* retentionBudget,
    std::optional<std::string_view> documentImageSibling) {
    using namespace epi::backend::seed_image_runtime;
    using Status = SeedImageDecodeStatus;
    if (documentImageSibling
        && (!captureSeedImageSiblingName(*documentImageSibling)
            || mode.seed_image_path != *documentImageSibling
            || epi::pattern_mirror::seed_type_from_mode_int(mode.seed_type) != epi::SeedType::Image))
        return Status::InvalidRequest;
    if (epi::pattern_mirror::seed_type_from_mode_int(mode.seed_type) != epi::SeedType::Image) {
        if (retained) retained->reset();
        return Status::Decoded;
    }
    if (mode.seed_image_path.empty() || mode.seed_image_path.find('\0') != std::string::npos)
        return Status::InvalidRequest;
    try {
        std::filesystem::path resolvedPath;
        if (documentImageSibling) {
            // Resolve once, privately: literal Linux parent names remain exact.
            // No search fallback and no candidate mutation before admission.
            resolvedPath = std::filesystem::absolute(
                manifest.parent_path() / std::filesystem::path(*documentImageSibling));
        } else {
            const auto resolution = epi::backend::seed_image_path_policy::resolve_seed_image_path(
                mode.seed_image_path, manifest.parent_path());
            if (!resolution.found) return Status::AcquisitionFailed;
            resolvedPath = std::filesystem::absolute(resolution.resolved_path);
        }
        constexpr SeedImageDecodeLimits limits{};
        // The application supplies one budget across live/frozen inputs. A
        // standalone checker gets a local budget whose state follows the owner.
        std::optional<epi::SeedImageRetentionBudget> localBudget;
        if (!retentionBudget) {
            localBudget.emplace();
            retentionBudget = &*localBudget;
        }
        auto reservation = retentionBudget->reserve(limits.maxEncodedBytes);
        if (!reservation) return Status::AllocationFailed;
        // Shared acquisition owns descriptor lifetime and byte limits; this
        // domain boundary owns document resolution, codec policy and path pinning.
        auto encoded = epi::io::readBoundedText(resolvedPath, reservation->capacity(), "seed image");
        const auto status = validate_seed_image_snapshot(encoded, limits);
        if (status != Status::Decoded) return status;
        auto pinnedPath = resolvedPath.string();
        epi::SeedImageInput::Owner input;
        if (retained) {
            // Finish fallible path copying before transferring the reservation;
            // argument evaluation order must not release accounting early.
            auto inputPath = pinnedPath;
            input = epi::SeedImageInput::retain(std::move(inputPath), std::move(encoded), std::move(reservation));
            if (!input) return Status::AllocationFailed;
        }
        mode.seed_image_path.swap(pinnedPath);
        if (retained) *retained = std::move(input);
        return Status::Decoded;
    } catch (const std::bad_alloc&) {
        return Status::AllocationFailed;
    } catch (const std::exception&) {
        return Status::AcquisitionFailed;
    }
}

} // namespace epi_sculpt::canonical_capture_detail

namespace epi_sculpt {
bool validateCaptureV2Png(std::string_view bytes, std::uint32_t expectedWidth,
                          std::uint32_t expectedHeight) noexcept {
    constexpr unsigned char signature[] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (bytes.size() < sizeof(signature) || bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        !std::equal(std::begin(signature), std::end(signature), reinterpret_cast<const unsigned char*>(bytes.data()))) return false;
    int width = 0, height = 0, components = 0;
    const auto* source = reinterpret_cast<const stbi_uc*>(bytes.data());
    if (!stbi_info_from_memory(source, static_cast<int>(bytes.size()), &width, &height, &components) ||
        width <= 0 || height <= 0 || std::uint64_t(width) * std::uint64_t(height) > kCaptureV2MaxPixels ||
        static_cast<std::uint32_t>(width) != expectedWidth ||
        static_cast<std::uint32_t>(height) != expectedHeight) return false;
    auto* pixels = stbi_load_from_memory(source, static_cast<int>(bytes.size()), &width, &height, &components, 4);
    if (!pixels) return false;
    stbi_image_free(pixels);
    return static_cast<std::uint32_t>(width) == expectedWidth &&
           static_cast<std::uint32_t>(height) == expectedHeight;
}
CanonicalCaptureRestorePreparation prepareCanonicalCaptureRestoreV2(
    const CaptureManifestReferences& refs, std::string_view modeBytes,
    std::string_view seedBytes, const std::filesystem::path& manifest,
    epi::SeedImageRetentionBudget* retentionBudget) {
    using Failure = CanonicalCaptureRestoreFailure;
    auto v1refs = refs; v1refs.format = "epic-capture-bundle-v1";
    const auto declaration = assessCanonicalCaptureDeclaration(v1refs);
    if (declaration == CanonicalCaptureDeclarationAssessment::NotRecorded)
        return {modeBytes.empty() && seedBytes.empty(), std::nullopt,
                modeBytes.empty() && seedBytes.empty() ? Failure::None : Failure::InvalidInput, declaration};
    if (declaration != CanonicalCaptureDeclarationAssessment::DeclaredV1)
        return {false, std::nullopt, Failure::DeclarationRejected, declaration};
    auto mode = decodeCanonicalCaptureInput(std::string(modeBytes));
    if (!mode) return {false, std::nullopt, Failure::InvalidInput, declaration};
    const bool image = epi::pattern_mirror::seed_type_from_mode_int(mode->seed_type) == epi::SeedType::Image;
    if (image != (refs.canonicalSeedImageCount == 1) || image != !seedBytes.empty() ||
        (image && mode->seed_image_path != refs.canonicalSeedImage))
        return {false, std::nullopt, Failure::InvalidInput, declaration};
    if (!image) return {true, std::move(mode), Failure::None, declaration};
    using namespace epi::backend::seed_image_runtime;
    const auto status = validate_seed_image_snapshot(seedBytes);
    if (status != SeedImageDecodeStatus::Decoded)
        return {false, std::nullopt, Failure::ImageSeedUnavailable, declaration, status};
    try {
        std::optional<epi::SeedImageRetentionBudget> localBudget;
        if (!retentionBudget) { localBudget.emplace(); retentionBudget = &*localBudget; }
        auto reservation = retentionBudget->reserve(seedBytes.size());
        if (!reservation) return {false, std::nullopt, Failure::ImageSeedUnavailable, declaration, SeedImageDecodeStatus::AllocationFailed};
        auto path = std::filesystem::absolute(manifest.parent_path() / refs.canonicalSeedImage).string();
        auto input = epi::SeedImageInput::retain(path, std::string(seedBytes), std::move(reservation));
        if (!input) return {false, std::nullopt, Failure::ImageSeedUnavailable, declaration, SeedImageDecodeStatus::AllocationFailed};
        mode->seed_image_path = std::move(path);
        return {true, std::move(mode), Failure::None, declaration, std::nullopt, std::move(input)};
    } catch (const std::exception&) {
        return {false, std::nullopt, Failure::ImageSeedUnavailable, declaration, SeedImageDecodeStatus::AllocationFailed};
    }
}
} // namespace epi_sculpt
