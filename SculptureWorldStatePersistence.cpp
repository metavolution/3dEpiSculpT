#include "SculptureWorldStatePersistence.h"
#include "SculptureSerializer.h"
#include "SculptureStateRecorder.h"
#include "SculptureWorldTourChoreographer.h"
#include "SculptureCaptureManifest.h"
#include "SculptureCapturePixels.h"

#include <cstdio>
#include <filesystem>
#include <memory>
#include <system_error>

namespace epi_sculpt {

SculptureWorldMode SculptureWorldStatePersistence::makeCaptureWorldMode(
    const SculptureWorldNavigator& navigator,
    const mgfx::Camera& cam,
    std::uint32_t generation
) {
    SculptureWorldState worldState{};
    worldState.version = 1;
    worldState.time = navigator.coordinator_.simulatedTime();
    worldState.morph = navigator.transitionState_.active ? navigator.transitionState_.progress : 0.0f;
    worldState.cameraEye[0] = cam.eye.x;
    worldState.cameraEye[1] = cam.eye.y;
    worldState.cameraEye[2] = cam.eye.z;
    worldState.cameraTarget[0] = cam.target.x;
    worldState.cameraTarget[1] = cam.target.y;
    worldState.cameraTarget[2] = cam.target.z;
    worldState.cameraUp[0] = cam.up.x;
    worldState.cameraUp[1] = cam.up.y;
    worldState.cameraUp[2] = cam.up.z;
    worldState.hasCameraOverride = true;

    // Rich provenance routing map
    std::vector<std::string> routingMap;
    routingMap.push_back("world_index: " + std::to_string(navigator.activeIndex_ + 1U) + "/" + std::to_string(navigator.availableWorlds_.size()));
    routingMap.push_back("canonical_preset: " + navigator.currentPresetName_);
    routingMap.push_back(std::string("generator: ") + worldGeneratorName(navigator.currentRecipe_.generator).data());
    routingMap.push_back("presentation_mode: " + std::string(name(navigator.presentationMode_)));
    if (navigator.presentationMode_ != WorldPresentationMode::PointCloud) {
        routingMap.push_back(std::string("relief_projection: ") + name(navigator.reliefConfig_.projection) + " (" + metricDescription(navigator.reliefConfig_.projection) + ")");
    }
    routingMap.push_back("speed_multiplier: " + std::to_string(navigator.speedMultiplier()));
    routingMap.push_back(std::string("is_paused: ") + (navigator.isPaused() ? "true" : "false"));
    routingMap.push_back(std::string("is_mutated: ") + (navigator.areParametersMutated() ? "true" : "false"));

    if (navigator.transitionState_.active) {
        routingMap.push_back("transition_active: true");
        routingMap.push_back("transition_from: " + navigator.transitionState_.fromPresetName);
        routingMap.push_back("transition_to: " + navigator.transitionState_.toPresetName);
        routingMap.push_back("transition_progress_pct: " + std::to_string(static_cast<unsigned>(navigator.transitionState_.progress * 100.0f)));
    } else {
        routingMap.push_back("transition_active: false");
    }

    if (navigator.tourChoreographer_ != nullptr && navigator.tourChoreographer_->isEnabled()) {
        const auto tStat = navigator.tourChoreographer_->status();
        routingMap.push_back("tour_name: " + tStat.tourName);
        routingMap.push_back("tour_mode: " + std::string(name(navigator.tourChoreographer_->tourMode())));
        routingMap.push_back("tour_waypoint: " + std::to_string(tStat.waypointIndex + 1U) + "/" + std::to_string(tStat.totalWaypoints));
    }
    if (navigator.coordinator_.isStagedComputeGovernorEnabled()) {
        const auto& govTel = navigator.coordinator_.stagedGovernor().telemetry();
        routingMap.push_back(std::string("staged_governor: ") + name(govTel.activeResolutionScale).data());
    }

    // Capture the authoritative active parameters from coordinator (losslessly preserving live mutations, 4D rotors, and map adjustments)
    const auto activeParams = navigator.coordinator_.currentParams();
    if (navigator.areParametersMutated()) routingMap.push_back("parameters: mutated from preset");
    // Source provenance stays stable; the captured parameters describe mutations.
    const std::string& revision = navigator.currentPresetRevision_;

    auto mode = SculptureStateRecorder::makeCaptureWorldMode(
        worldState, navigator.currentPresetName_, revision, activeParams, std::move(routingMap));
    mode.canonicalFeedbackGeneration = generation;
    if (navigator.morphSpace_) {
        mode.morphSpace = navigator.morphSpace_->exportState();
    }
    return mode;
}

SnapshotPaths SculptureWorldStatePersistence::saveSnapshotBundle(
    SculptureWorldNavigator& navigator,
    const std::string& directory,
    const std::string& baseName,
    const mgfx::Camera* cameraOverride
) {
    std::filesystem::path dirPath(directory);
    std::error_code ec;
    if (!directory.empty()) {
        std::filesystem::create_directories(dirPath, ec);
    }

    SnapshotPaths paths;
    if (baseName.empty()) {
        paths = SculptureStateRecorder::generatePaths(directory, navigator.currentPresetName_);
    } else {
        const std::string fullBase = directory.empty() ? baseName : (directory + "/" + baseName);
        paths = SnapshotPaths{
            .pngPath = fullBase + ".png",
            .worldPath = fullBase + ".world",
            .epimodePath = fullBase + ".epimode",
            .epiPath = fullBase + ".epi",
            .profilePath = fullBase + ".profile",
            .manifestPath = fullBase + ".manifest",
            .volmemPath = fullBase + ".volmem",
            .timestamp = ""
        };
    }

    // Invalidate the previous certificate before replacing any attachment.
    if (!SculptureStateRecorder::prepareSnapshotAttachments(paths)) {
        std::fprintf(stderr, "[SculptureWorldNavigator] Cannot retire previous capture manifest\n");
        return paths;
    }

    // Keep this attempt's completion separate from any older sidecar on disk.
    CaptureAttachmentWrites attachments;
    if (auto* volMem = navigator.coordinator_.volumetricMemory()) {
        attachments.volmem = volMem->saveToFile(paths.volmemPath);
    }

    const mgfx::Camera cam = (cameraOverride != nullptr)
        ? *cameraOverride
        : navigator.savedCameraOverride(0.959931f);

    const auto captureWorldMode = makeCaptureWorldMode(navigator, cam, 0U);

    const bool ok = SculptureStateRecorder::writeSnapshot(
        paths.pngPath,
        paths.worldPath,
        paths.epimodePath,
        paths.epiPath,
        0, 0, nullptr,
        captureWorldMode,
        nullptr,
        paths.volmemPath, "", nullptr, attachments
    );

    if (ok) {
        navigator.latestSnapshotPath_ = paths.manifestPath;
        std::fprintf(stderr,
            "[SculptureWorldNavigator] Saved snapshot bundle to '%s' (world: %s, epimode: %s, manifest: %s)\n",
            directory.c_str(), paths.worldPath.c_str(), paths.epimodePath.c_str(), paths.manifestPath.c_str());
    } else {
        std::fprintf(stderr,
            "[SculptureWorldNavigator] Failed to save snapshot bundle to '%s'\n",
            directory.c_str());
    }

    return paths;
}

bool SculptureWorldStatePersistence::loadSnapshotBundle(
    SculptureWorldNavigator& navigator,
    const std::string& filepathOrManifest
) {
    if (filepathOrManifest.empty() || filepathOrManifest.find('\0') != std::string::npos) {
        return false;
    }

    std::filesystem::path p(filepathOrManifest);
    std::error_code pathError;
    if (!std::filesystem::exists(p, pathError)) {
        std::fprintf(stderr,
            "[SculptureWorldNavigator] Error: snapshot file or manifest '%s' is missing or inaccessible\n",
            filepathOrManifest.c_str());
        return false;
    }

    // If manifest file, parse target world or epimode file
    if (filepathOrManifest.ends_with(".manifest")) {
        const auto references = readCaptureManifestReferences(p);
        if (!references) {
            std::fprintf(stderr, "[SculptureWorldNavigator] Capture manifest admission failed (limit %zu bytes; no NUL): %s\n",
                         kMaxCaptureManifestBytes, filepathOrManifest.c_str());
            return false;
        }
        if (!captureManifestRestoreReferencesAdmitted(*references)) {
            std::fprintf(stderr, "[SculptureWorldNavigator] Capture manifest format or sibling reference rejected: %s\n",
                         filepathOrManifest.c_str());
            return false;
        }
        const auto& targetWorld = references->world;
        const auto& targetEpimode = references->epimode;
        const auto& targetVolmem = references->volmem;
        const auto parent = p.parent_path();
        // A failed filesystem status must not bypass the alternate state. The
        // decoder still owns admission and any semantic state takeover.
        const auto loadReferencedState = [&](const std::string& reference) {
            if (reference.empty() || reference == "none") return false;
            const auto targetPath = (parent / reference).string();
            std::error_code error;
            if (!std::filesystem::exists(targetPath, error)) {
                if (error) {
                    std::fprintf(stderr,
                        "[SculptureWorldNavigator] Cannot inspect snapshot state reference: %s\n",
                        targetPath.c_str());
                }
                return false;
            }
            return loadSnapshotState(navigator, targetPath, false, filepathOrManifest);
        };
        const bool loaded = loadReferencedState(targetWorld)
            || loadReferencedState(targetEpimode);
        if (loaded && !targetVolmem.empty() && targetVolmem != "none") {
            const auto volPath = (parent / targetVolmem).string();
            std::error_code error;
            if (!std::filesystem::exists(volPath, error)) {
                std::fprintf(stderr,
                    "[SculptureWorldNavigator] Snapshot state loaded; volume attachment missing or inaccessible: %s\n",
                    volPath.c_str());
            } else if (auto* volMem = navigator.coordinator_.volumetricMemory()) {
                if (!volMem->loadFromFile(volPath)) {
                    std::fprintf(stderr,
                        "[SculptureWorldNavigator] Snapshot state loaded; volume attachment failed to restore: %s\n",
                        volPath.c_str());
                }
            } else {
                std::fprintf(stderr,
                    "[SculptureWorldNavigator] Snapshot state loaded; volume attachment cannot be restored because volume memory is unavailable: %s\n",
                    volPath.c_str());
            }
        }
        return loaded;
    }

    return loadSnapshot(navigator, filepathOrManifest);
}

void SculptureWorldStatePersistence::requestSnapshot(SculptureWorldNavigator& navigator) noexcept {
    navigator.snapshotRequested_ = true;
    saveSnapshotBundle(navigator, "snapshots", navigator.currentPresetName_, nullptr);
    navigator.notifyObservers([&navigator](INavigatorObserver& obs) {
        obs.onSnapshotRequested(navigator.latestSnapshotPath_.empty() ? "snapshots" : navigator.latestSnapshotPath_);
    });
}

void SculptureWorldStatePersistence::requestSnapshot(
    SculptureWorldNavigator& navigator,
    mgfx::vk::FrameLoop& frameLoop,
    const mgfx::Camera& cam,
    std::uint32_t generation,
    const std::string& directory
) {
    const auto captureWorldMode = makeCaptureWorldMode(navigator, cam, generation);
    const auto paths = SculptureStateRecorder::generatePaths(directory, navigator.currentPresetName_);

    // Admission only queues the callback; it cannot run until a later frame.
    // Keep its attachment receipt shared so a rejected request writes nothing.
    struct PendingAttachments {
        CaptureAttachmentWrites writes;
        bool ready = false;
    };
    const auto pending = std::make_shared<PendingAttachments>();
    const auto admission = frameLoop.requestCaptureOutcome([&navigator, captureWorldMode, paths, directory, pending](const mgfx::vk::CaptureOutcome& outcome) {
        if (outcome.status != mgfx::vk::CaptureStatus::Completed || !pending->ready) return;
        const auto& res = outcome.image;
        bool saved = false;
        try {
            withBgraCapturePixels(res, [&](const auto& image) {
                saved = SculptureStateRecorder::captureFrameAndStateWithPaths(
                    image, captureWorldMode, paths, nullptr, false, nullptr, pending->writes);
            });
        } catch (const std::exception& error) {
            std::fprintf(stderr, "[SculptureWorldNavigator] Capture conversion failed: %s\n", error.what());
            return;
        }
        if (!saved) {
            std::fprintf(stderr,
                "[SculptureWorldNavigator] Failed to persist snapshot bundle to '%s'\n",
                directory.c_str());
            return;
        }
        navigator.latestSnapshotPath_ = paths.manifestPath;
        std::fprintf(stderr,
            "[SculptureWorldNavigator] Snapshot bundle persisted to '%s':\n"
            "  PNG:     %s\n  WORLD:   %s\n  EPIMODE: %s\n  VOLMEM:  %s\n",
            directory.c_str(), paths.pngPath.c_str(), paths.worldPath.c_str(), paths.epimodePath.c_str(),
            pending->writes.volmem ? paths.volmemPath.c_str() : "none");
        navigator.notifyObservers([&navigator](INavigatorObserver& obs) {
            obs.onSnapshotRequested(navigator.latestSnapshotPath_.empty() ? "snapshots" : navigator.latestSnapshotPath_);
        });
    });
    if (admission != mgfx::vk::CaptureAdmission::Accepted) {
        std::fprintf(stderr, "[SculptureWorldNavigator] Capture request rejected (%d)\n", int(admission));
        return;
    }
    if (!SculptureStateRecorder::prepareSnapshotAttachments(paths)) {
        std::fprintf(stderr, "[SculptureWorldNavigator] Cannot retire previous capture manifest\n");
        return;
    }
    // Freeze the field at request time, after admission and certificate retirement.
    if (auto* volMem = navigator.coordinator_.volumetricMemory()) {
        pending->writes.volmem = volMem->saveToFile(paths.volmemPath);
    }
    pending->ready = true;
}

bool SculptureWorldStatePersistence::loadSnapshot(
    SculptureWorldNavigator& navigator,
    const std::string& filepath
) {
    return loadSnapshotState(navigator, filepath, true, filepath);
}

bool SculptureWorldStatePersistence::loadSnapshotState(
    SculptureWorldNavigator& navigator,
    const std::string& filepath,
    bool restoreSiblingVolume,
    const std::string& reloadSource
) {
    // Prepare the reload identity before takeover. It can alias the navigator's
    // current path when the user invokes ReloadLatestSnapshot.
    std::string preparedReloadSource = reloadSource;
    bool loaded = false;
    // Every format probe must describe the same acquired state, even if the
    // source path is replaced while a decoder or application callback runs.
    const auto content = SculptureSerializer::readStateFile(filepath);
    // 1. Try loading as .world text format first if it ends with .world
    if (content && filepath.ends_with(".world")) {
        if (auto worldOpt = SculptureSerializer::deserializeWorldMode(*content)) {
            loaded = applyWorldMode(navigator, *worldOpt);
        }
    }

    // 2. Try loading as .epimode JSON format
    if (content && !loaded) {
        SculptureWorldMode mode{};
        SculpturePresentationOwnership ownership = SculpturePresentationOwnership::CompactInstances;
        std::string macroJson;
        if (SculptureSerializer::deserializeEpimode(*content, std::filesystem::path(filepath).stem().string(),
                mode.state, mode.presetName, mode.params, mode.routingMap, &ownership, &macroJson)) {
            mode.presentationOwnership = ownership;
            mode.macroAutomationJson = std::move(macroJson);
            loaded = applyWorldMode(navigator, mode);
        }
    }

    // 3. Fallback: try loading as .world format if not attempted yet
    if (content && !loaded && !filepath.ends_with(".world")) {
        if (auto worldOpt = SculptureSerializer::deserializeWorldMode(*content)) {
            loaded = applyWorldMode(navigator, *worldOpt);
        }
    }

    if (loaded) navigator.latestSnapshotPath_.swap(preparedReloadSource);
    if (!loaded) {
        std::fprintf(stderr,
            "[SculptureWorldNavigator] Error: failed to parse snapshot file '%s'\n",
            filepath.c_str());
    } else if (restoreSiblingVolume) {
        // Standalone state files retain legacy sibling discovery. A manifest
        // selects its own volume explicitly, including the deliberate none case.
        std::filesystem::path sp(filepath);
        auto volmemP = sp.replace_extension(".volmem");
        std::error_code ec;
        if (std::filesystem::exists(volmemP, ec)) {
            if (auto* volMem = navigator.coordinator_.volumetricMemory()) {
                volMem->loadFromFile(volmemP.string());
            }
        }
    }

    navigator.notifyObservers([&](INavigatorObserver& obs) {
        obs.onSnapshotLoaded(filepath, loaded);
    });
    return loaded;
}

bool SculptureWorldStatePersistence::applyWorldMode(
    SculptureWorldNavigator& navigator,
    const SculptureWorldMode& worldMode
) {
    // 1. Resolve and activate the target world preset / generator
    bool selected = false;
    if (!worldMode.presetName.empty()) {
        selected = navigator.selectWorld(worldMode.presetName, 0.0f);
    }

    // If presetName didn't resolve directly, scan routingMap for canonical_preset or generator
    if (!selected) {
        for (const auto& tag : worldMode.routingMap) {
            constexpr std::string_view kCanonPrefix = "canonical_preset: ";
            constexpr std::string_view kGenPrefix = "generator: ";
            if (tag.rfind(kCanonPrefix, 0) == 0) {
                const auto target = tag.substr(kCanonPrefix.size());
                if (navigator.selectWorld(target, 0.0f)) {
                    selected = true;
                    break;
                }
            } else if (tag.rfind(kGenPrefix, 0) == 0) {
                const auto genName = tag.substr(kGenPrefix.size());
                for (std::size_t i = 0; i < navigator.availableWorlds_.size(); ++i) {
                    if (worldGeneratorName(navigator.availableWorlds_[i]->recipe().generator) == genName) {
                        if (navigator.selectWorld(i, 0.0f)) {
                            selected = true;
                            break;
                        }
                    }
                }
                if (selected) break;
            }
        }
    }

    // A failed identity lookup must not apply another world's saved parameters
    // to whichever route happened to be active before the load request.
    if (!selected) return false;
    if (!worldMode.presetRevision.empty()) navigator.currentPresetRevision_ = worldMode.presetRevision;

    // 2. Restore active mathematical parameters
    navigator.currentParams_ = worldMode.params;
    navigator.baselineParams_ = navigator.currentParams_;
    navigator.coordinator_.setTargetParams(navigator.currentParams_, 0.0f);
    navigator.coordinator_.snapToTarget();

    // 3. Restore camera override if present in snapshot
    if (worldMode.state.hasCameraOverride) {
        navigator.hasSavedCameraOverride_ = true;
        navigator.savedCameraEye_ = {worldMode.state.cameraEye[0], worldMode.state.cameraEye[1], worldMode.state.cameraEye[2]};
        navigator.savedCameraTarget_ = {worldMode.state.cameraTarget[0], worldMode.state.cameraTarget[1], worldMode.state.cameraTarget[2]};
        navigator.savedCameraUp_ = {worldMode.state.cameraUp[0], worldMode.state.cameraUp[1], worldMode.state.cameraUp[2]};
    } else {
        navigator.hasSavedCameraOverride_ = false;
    }

    // 3b. Restore 2D Morph Space state if present
    if (navigator.morphSpace_ && (worldMode.morphSpace.active || !worldMode.morphSpace.activeBankName.empty() || !worldMode.morphSpace.waypoints.empty())) {
        navigator.morphSpace_->importState(worldMode.morphSpace);
    }

    // 4. Restore presentation mode and runtime routing settings
    for (const auto& tag : worldMode.routingMap) {
        constexpr std::string_view kPresPrefix = "presentation_mode: ";
        constexpr std::string_view kPausePrefix = "is_paused: ";
        constexpr std::string_view kSpeedPrefix = "speed_multiplier: ";

        if (tag.rfind(kPresPrefix, 0) == 0) {
            const auto pres = tag.substr(kPresPrefix.size());
            if (pres == "PointCloud") navigator.presentationMode_ = WorldPresentationMode::PointCloud;
            else if (pres == "ReliefManifold") navigator.presentationMode_ = WorldPresentationMode::ReliefManifold;
            else if (pres == "HybridVolumetric") navigator.presentationMode_ = WorldPresentationMode::HybridVolumetric;
        } else if (tag.rfind(kPausePrefix, 0) == 0) {
            navigator.setPaused(tag.substr(kPausePrefix.size()) == "true");
        } else if (tag.rfind(kSpeedPrefix, 0) == 0) {
            try {
                const float spd = std::stof(tag.substr(kSpeedPrefix.size()));
                if (spd > 0.01f && spd < 100.0f) {
                    navigator.coordinator_.setSpeedMultiplier(spd);
                }
            } catch (...) {}
        }
    }

    std::fprintf(stderr,
        "[SculptureWorldNavigator] Successfully applied snapshot: preset='%s' (%s), cameraOverride=%s\n",
        navigator.currentPresetName_.c_str(),
        worldGeneratorName(navigator.currentRecipe_.generator).data(),
        navigator.hasSavedCameraOverride_ ? "YES" : "NO");
    return true;
}

SculptureStateDiff SculptureWorldStatePersistence::diffFromBaseline(
    const SculptureWorldNavigator& navigator,
    float epsilon
) {
    return SculptureSerializer::diff(navigator.baselineParams_, navigator.coordinator_.currentParams(), epsilon);
}

bool SculptureWorldStatePersistence::applyPatch(
    SculptureWorldNavigator& navigator,
    const SculptureStateDiff& diff,
    std::string_view tag
) {
    if (diff.empty()) return false;
    SculptureProgramParams liveParams = navigator.coordinator_.currentParams();
    if (!SculptureSerializer::applyDiff(liveParams, diff)) {
        return false;
    }
    pushHistory(navigator, std::string("ApplyPatch: ") + std::string(tag));
    navigator.currentParams_ = liveParams;
    navigator.coordinator_.setTargetParams(liveParams, 0.0f);
    navigator.coordinator_.snapToTarget();

    navigator.notifyObservers([&](INavigatorObserver& obs) {
        obs.onParameterPatchApplied(diff, tag);
        obs.onLatentSpaceMutated(liveParams.latentW, liveParams.rotorAlpha, liveParams.rotorBeta);
    });
    return true;
}

bool SculptureWorldStatePersistence::exportBaselineDiff(
    const SculptureWorldNavigator& navigator,
    const std::string& filepath
) {
    const auto diff = diffFromBaseline(navigator, 1e-5f);
    if (diff.empty()) return false;
    return SculptureSerializer::saveDiffToFile(filepath, diff);
}

bool SculptureWorldStatePersistence::importDiffPatch(
    SculptureWorldNavigator& navigator,
    const std::string& filepath
) {
    auto maybeDiff = SculptureSerializer::loadDiffFromFile(filepath);
    if (!maybeDiff) return false;
    return applyPatch(navigator, *maybeDiff, filepath);
}

void SculptureWorldStatePersistence::pushHistory(
    SculptureWorldNavigator& navigator,
    std::string_view tag
) {
    const auto current = navigator.coordinator_.currentParams();
    if (!navigator.undoStack_.empty()) {
        const auto diff = SculptureSerializer::diff(navigator.undoStack_.back().params, current, 1e-5f);
        if (diff.empty()) {
            return;
        }
    }
    navigator.undoStack_.push_back(SculptureWorldNavigator::ParameterHistoryEntry{current, std::string(tag)});
    if (navigator.undoStack_.size() > SculptureWorldNavigator::kMaxHistoryDepth) {
        navigator.undoStack_.pop_front();
    }
    navigator.redoStack_.clear();
    navigator.notifyObservers([&navigator, tag](INavigatorObserver& obs) {
        obs.onHistoryStateChanged(navigator.undoStack_.size(), navigator.redoStack_.size(), tag);
    });
}

bool SculptureWorldStatePersistence::undo(SculptureWorldNavigator& navigator) {
    if (navigator.undoStack_.empty()) {
        return false;
    }
    const auto current = navigator.coordinator_.currentParams();
    SculptureWorldNavigator::ParameterHistoryEntry entry = std::move(navigator.undoStack_.back());
    navigator.undoStack_.pop_back();

    navigator.redoStack_.push_back(SculptureWorldNavigator::ParameterHistoryEntry{current, entry.tag});
    if (navigator.redoStack_.size() > SculptureWorldNavigator::kMaxHistoryDepth) {
        navigator.redoStack_.pop_front();
    }

    navigator.currentParams_ = entry.params;
    navigator.coordinator_.setTargetParams(entry.params, 0.0f);
    navigator.coordinator_.snapToTarget();

    navigator.notifyObservers([&navigator, &entry](INavigatorObserver& obs) {
        obs.onHistoryStateChanged(navigator.undoStack_.size(), navigator.redoStack_.size(), "Undo: " + entry.tag);
        obs.onLatentSpaceMutated(entry.params.latentW, entry.params.rotorAlpha, entry.params.rotorBeta);
    });
    return true;
}

bool SculptureWorldStatePersistence::redo(SculptureWorldNavigator& navigator) {
    if (navigator.redoStack_.empty()) {
        return false;
    }
    const auto current = navigator.coordinator_.currentParams();
    SculptureWorldNavigator::ParameterHistoryEntry entry = std::move(navigator.redoStack_.back());
    navigator.redoStack_.pop_back();

    navigator.undoStack_.push_back(SculptureWorldNavigator::ParameterHistoryEntry{current, entry.tag});
    if (navigator.undoStack_.size() > SculptureWorldNavigator::kMaxHistoryDepth) {
        navigator.undoStack_.pop_front();
    }

    navigator.currentParams_ = entry.params;
    navigator.coordinator_.setTargetParams(entry.params, 0.0f);
    navigator.coordinator_.snapToTarget();

    navigator.notifyObservers([&navigator, &entry](INavigatorObserver& obs) {
        obs.onHistoryStateChanged(navigator.undoStack_.size(), navigator.redoStack_.size(), "Redo: " + entry.tag);
        obs.onLatentSpaceMutated(entry.params.latentW, entry.params.rotorAlpha, entry.params.rotorBeta);
    });
    return true;
}

void SculptureWorldStatePersistence::clearHistory(SculptureWorldNavigator& navigator) noexcept {
    if (navigator.undoStack_.empty() && navigator.redoStack_.empty()) {
        return;
    }
    navigator.undoStack_.clear();
    navigator.redoStack_.clear();
    navigator.notifyObservers([](INavigatorObserver& obs) {
        obs.onHistoryStateChanged(0, 0, "Clear");
    });
}

bool SculptureWorldStatePersistence::startSequenceRecording(
    SculptureWorldNavigator& navigator,
    std::string_view sessionName
) {
    const bool started = navigator.coordinator_.startSequenceRecording(sessionName);
    if (started) {
        std::string sessionDir{};
        if (const auto* rec = navigator.coordinator_.sequenceRecorder()) {
            sessionDir = rec->currentSequenceDirectory();
        }
        navigator.notifyObservers([&](INavigatorObserver& obs) {
            obs.onSequenceRecordingStarted(sessionDir, 60);
        });
    }
    return started;
}

bool SculptureWorldStatePersistence::stopSequenceRecording(
    SculptureWorldNavigator& navigator
) {
    SequenceStopOutcome outcome{};
    const bool stopped = navigator.coordinator_.stopSequenceRecording(&outcome);
    if (stopped) {
        const auto* recorder = navigator.coordinator_.sequenceRecorder();
        const auto dir = std::filesystem::path(recorder->currentSequenceDirectory());
        const std::string manifestPath = outcome.manifestPublished
            ? (dir / "sequence_manifest.json").string() : std::string{};
        const std::string scriptPath = outcome.scriptPublished
            ? (dir / "export_video.sh").string() : std::string{};
        navigator.notifyObservers([&](INavigatorObserver& obs) {
            obs.onSequenceRecordingStopped(outcome.recordedFrameCount, manifestPath, scriptPath);
        });
    }
    return stopped;
}

bool SculptureWorldStatePersistence::recordFrame(
    SculptureWorldNavigator& navigator,
    const mgfx::vk::CaptureResult& res,
    const mgfx::Camera& cam,
    std::string_view customTag
) {
    if (!navigator.coordinator_.isSequenceRecording()) return false;
    return withSequenceCapturePixels(res, [&](const auto& image, bool isBgra) {
        return recordFrame(navigator, image.width, image.height, image.pixels,
                           cam, isBgra, customTag);
    });
}

SequenceFrameMetadata SculptureWorldStatePersistence::makeSequenceFrameMetadata(
    const SculptureWorldNavigator& navigator,
    const mgfx::Camera& cam,
    std::string_view customTag
) {
    SequenceFrameMetadata meta{};
    meta.origin = SequenceMetadataOrigin::DirectCall;
    meta.timestampSeconds = navigator.coordinator_.simulatedTime();
    meta.timestampExplicit = true;
    meta.presetName = navigator.currentPresetName_.empty()
        ? std::string(worldGeneratorName(navigator.currentRecipe_.generator))
        : navigator.currentPresetName_;
    meta.morph = navigator.transitionProgress();
    meta.cameraEye[0] = cam.eye.x;
    meta.cameraEye[1] = cam.eye.y;
    meta.cameraEye[2] = cam.eye.z;
    meta.cameraTarget[0] = cam.target.x;
    meta.cameraTarget[1] = cam.target.y;
    meta.cameraTarget[2] = cam.target.z;
    meta.customTag = std::string(customTag);
    return meta;
}

bool SculptureWorldStatePersistence::recordFrame(
    SculptureWorldNavigator& navigator,
    std::uint32_t width,
    std::uint32_t height,
    const std::uint8_t* pixels,
    const mgfx::Camera& cam,
    bool isBgra,
    std::string_view customTag
) {
    if (!navigator.coordinator_.isSequenceRecording() || pixels == nullptr || width == 0 || height == 0) {
        return false;
    }
    auto meta = makeSequenceFrameMetadata(navigator, cam, customTag);
    return navigator.coordinator_.recordFrame(width, height, pixels, meta, isBgra);
}

} // namespace epi_sculpt
