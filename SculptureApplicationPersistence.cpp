// State import/export and lazy, originating-frame capture provenance.
// These translation units share one SculptureApplication owner; none is a second scheduler.

#include "SculptureApplication.h"
#include "SculptureReliefContours.h"
#include "SculptureCaptureCamera.h"
#include "SculptureCaptureEpimodeAdmission.h"
#include "SculptureCaptureManifest.h"
#include "SculptureCaptureIntegrity.h"
#include "SculptureCapturePresetProvenance.h"
#include "SculptureCanonicalCaptureRestore.h"
#include "SculptureCanonicalSnapshotInput.h"
#include "SculptureCaptureWorldAdmission.h"
#include "SculptureCapturePixels.h"
#include "SculptureCausalExecutionJson.h"
#include "SculptureGeometryExporter.h"
#include "SculptureIsosurface.h"
#include "SculptureMacroAutomation.h"
#include "SculptureModulationInspection.h"
#include "SculptureSerializer.h"
#include "SculptureSequencePlayback.h"
#include "SculptureSequenceLoadLimits.h"
#include "SculptureStateLoadLimits.h"
#include "SculptureShaderSynthesizer.h"
#include "ImplicitExternalImageTransport.h"
#include "EpimorphicReliefGenerator.h"

#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <memory>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace epi_sculpt {

void SculptureApplication::appendMorphSpaceCapture(SculptureWorldMode& mode) const {
    mode.morphSpace = control_.sources().morphSpace().exportState();
    // Composition gives the deck priority. Keep a shadowed controller recipe
    // dormant on reopen so it cannot replace the captured deck parameter moment.
    if (control_.sources().deckActive()) mode.morphSpace.active = false;
}

SculptureWorldMode SculptureApplication::makeFrameCaptureWorldMode(const SculptureFramePlan& framePlan) const {
    const auto& preset = presetAt(framePlan.presetIndex());
    const auto source = resolveCapturePresetProvenance(framePlan, preset.id, preset.revision);
    SculptureWorldMode world = SculptureStateRecorder::makeCaptureWorldMode(
        framePlan.worldState(), std::string(source.id), std::string(source.revision),
        framePlan.params(), control_.installedRoutingMap());
    world.presentationOwnership = framePlan.presentationOwnership();
    world.volumePresentation = framePlan.presentation().volumePresentation;
    // One preset identity cannot certify both a canonical source and a dormant
    // captured-world recipe. Keep this rule identical for saves and snapshots.
    world.hasResolvedWorldRecipe = !framePlan.capturedWorldRecipe()
        || framePlan.production().ownsDedicatedWorldProduction();
    world.resolvedWorldRecipe = framePlan.effectiveWorldRecipe();
    world.requestedWorldBudget = framePlan.requestedWorldRecipe().instanceBudget;
    world.effectiveWorldBudget = framePlan.effectiveWorldRecipe().instanceBudget;
    world.effectiveWorldUpdateHz = framePlan.effectiveWorldUpdateHz();
    appendLibraryPlaylistCapture(world);
    appendMorphSpaceCapture(world);
    // This is owned configuration intent. Camera selection and any observed
    // native identity belong to the caller and its later settled receipt.
    return world;
}

void SculptureApplication::flushPendingStateSaves(const SculptureFramePlan& framePlan) noexcept {
    if (pendingStateSavePaths_.empty()) return;

    // Consume this bounded batch once. No plan/catalog references are retained,
    // and a failed write cannot replay a request on a later mathematical frame.
    std::vector<std::string> paths;
    paths.swap(pendingStateSavePaths_);
    const auto failPreparation = [&paths](const char* reason) noexcept {
        for (const auto& path : paths) {
            std::fprintf(stderr, "[SculptureApplication] Failed to prepare state save %s: %s\n",
                         path.c_str(), reason);
        }
    };
    try {
        SculptureWorldMode world = makeFrameCaptureWorldMode(framePlan);
        if (!world.hasResolvedWorldRecipe)
            std::fprintf(stderr, "[SculptureApplication] Semantic save omits captured world recipe during canonical fallback.\n");

        const auto& presentation = framePlan.presentation();
        const auto camera = applyStartupCamera(presentationBindings_->director().composeCamera(
            presentation.cinematic, presentation.cinematicTime, framePlan.worldState()));
        world.state = captureCameraPose(framePlan.worldState(), camera);
        // This is frame intent. Presentation receipt/generation fields retain
        // their unknown defaults until an actual observation is recorded.

        for (const auto& path : paths) {
            try {
                const bool saved = SculptureSerializer::saveWorldModeToFile(path, world);
                std::fprintf(stderr, "[SculptureApplication] %s %s\n",
                             saved ? "Saved" : "Failed to save", path.c_str());
            } catch (const std::exception& error) {
                std::fprintf(stderr, "[SculptureApplication] Failed to save %s: %s\n",
                             path.c_str(), error.what());
            } catch (...) {
                std::fprintf(stderr, "[SculptureApplication] Failed to save %s: unknown error\n",
                             path.c_str());
            }
        }
    } catch (const std::exception& error) {
        failPreparation(error.what());
    } catch (...) {
        failPreparation("unknown error");
    }
}

void SculptureApplication::handleFileAction(const EventFileAction& action) {
    // File commands are isolated from event routing. Explicit saves wait for
    // composed frame intent; loads still admit their values before takeover.
    if (action.action == EventFileAction::Action::SaveState) {
        if (action.path.empty() || pendingStateSavePaths_.size() >= kMaxPendingStateSaves) {
            std::fprintf(stderr,
                "[SculptureApplication] State save rejected (empty path or pending limit reached): %s\n",
                action.path.c_str());
            return;
        }
        try {
            pendingStateSavePaths_.push_back(action.path);
            std::fprintf(stderr, "[SculptureApplication] Queued state save %s for the next frozen frame\n",
                         action.path.c_str());
        } catch (const std::exception& error) {
            std::fprintf(stderr, "[SculptureApplication] Failed to queue state save %s: %s\n",
                         action.path.c_str(), error.what());
        } catch (...) {
            std::fprintf(stderr, "[SculptureApplication] Failed to queue state save %s\n",
                         action.path.c_str());
        }
        return;
    }

    if (action.action == EventFileAction::Action::LoadState) {
        // Each referenced state is acquired once, and all decoding precedes
        // source takeover. A v1 certificate names siblings, not guessed paths.
        constexpr StateLoadLimits limits{};
        static_assert(std::is_nothrow_move_assignable_v<decltype(appState_.loadedModulationGraph)>);
        const bool fromManifest = std::filesystem::path(action.path).extension() == ".manifest";
        std::string statePath = action.path;
        std::optional<std::string> content;
        std::optional<SculptureWorldMode> world;
        std::optional<epi::VisualMode> canonicalMode;
        epi::SeedImageInput::Owner canonicalSeedImageInput;
        std::optional<std::pair<std::string, std::string>> canonicalPresetMetadata;
        bool decodedWorld = false;
        const auto decodeWorld = [&](const std::string& bytes) {
            auto result = SculptureSerializer::deserializeWorldMode(bytes);
            if (result && fromManifest && !captureWorldModeAdmitted(bytes, result->version)) {
                std::fprintf(stderr, "[SculptureApplication] Capture WorldMode incomplete or ambiguous: %s\n",
                             statePath.c_str());
                result.reset();
            }
            return result;
        };
        if (fromManifest) {
            std::string manifestBytes;
            try {
                manifestBytes = epi::io::readBoundedText(action.path, kMaxCaptureManifestBytes, "capture manifest");
            } catch (const std::exception&) {
                std::fprintf(stderr, "[SculptureApplication] Capture manifest acquisition failed: %s\n", action.path.c_str());
                return;
            }
            const auto references = parseCaptureManifestReferences(manifestBytes);
            if (!references) {
                std::fprintf(stderr, "[SculptureApplication] Failed to read capture manifest %s\n",
                             action.path.c_str());
                return;
            }
            const auto admission = captureManifestRestoreAdmission(*references);
            const bool v2 = references->formatCount == 1 && references->format == "epic-capture-bundle-v2";
            if (!v2 && (admission.format != CaptureManifestFormatAdmission::V1 || !admission.admitted())) {
                std::fprintf(stderr, "[SculptureApplication] Capture manifest format or references rejected: %s\n",
                             action.path.c_str());
                return;
            }
            // Acquire every v2 attachment exactly once, before semantic takeover.
            std::optional<CaptureV2Bytes> acquired;
            if (v2) {
                const auto document = parseCaptureV2(manifestBytes);
                if (!document || !(acquired = acquireCaptureV2(*document, std::filesystem::path(action.path).parent_path())) ||
                    (!captureV2ReceiptMatches(manifestBytes, acquired->get(CaptureV2Role::World)) ||
                     !validateCaptureV2Png(acquired->get(CaptureV2Role::Image), document->width, document->height))) {
                    std::fprintf(stderr, "[SculptureApplication] Capture v2 integrity or receipt rejected: %s\n", action.path.c_str());
                    return;
                }
            }
            auto canonical = v2 ? prepareCanonicalCaptureRestoreV2(
                *references, acquired->get(CaptureV2Role::CanonicalMode), acquired->get(CaptureV2Role::SeedImage),
                action.path, &canonicalSeedImageRetentionBudget_) : prepareCanonicalCaptureRestore(
                *references, action.path, &canonicalSeedImageRetentionBudget_);
            if (!canonical.admitted) {
                std::fprintf(stderr, "[SculptureApplication] Canonical capture input rejected (%s): %s\n",
                             canonical.diagnosticCode(), action.path.c_str());
                if (canonical.imageDecodeStatus) {
                    std::fprintf(stderr, "[SculptureApplication] Canonical image seed rejected (%s)\n",
                        epi::backend::seed_image_runtime::seed_image_decode_code(*canonical.imageDecodeStatus));
                }
                return;
            }
            canonicalMode = std::move(canonical.mode);
            canonicalSeedImageInput = std::move(canonical.seedImageInput);
            // Epimode's legacy decoder has no revision output. Optional manifest
            // metadata may retain it only when unique and tied to the decoded ID.
            if (references->presetIdMetadataCount == 1U && references->presetRevisionMetadataCount == 1U
                && references->presetIdMetadata && references->presetRevisionMetadata) {
                canonicalPresetMetadata.emplace(*references->presetIdMetadata,
                                                *references->presetRevisionMetadata);
            }
            const auto parent = std::filesystem::path(action.path).parent_path();
            const auto acquire = [&](const std::string& reference) -> std::optional<std::string> {
                if (reference.empty() || reference == "none") return std::nullopt;
                statePath = (parent / reference).string();
                return SculptureSerializer::readStateFile(statePath, limits);
            };
            if (v2) {
                statePath = (parent / references->world).string();
                content = acquired->get(CaptureV2Role::World);
            } else content = acquire(references->world);
            if (content) {
                world = decodeWorld(*content);
                decodedWorld = true;
            }
            if (v2 && (!world || world->captureSubmissionSerial == 0U)) {
                std::fprintf(stderr, "[SculptureApplication] Capture v2 world rejected: %s\n", action.path.c_str());
                return;
            }
            if (!v2 && !world && references->world != references->epimode) {
                content = acquire(references->epimode);
                decodedWorld = false;
            }
        } else {
            content = SculptureSerializer::readStateFile(statePath, limits);
        }
        if (!content) {
            std::fprintf(stderr,
                "[SculptureApplication] Failed to read state from %s (limit: %zu bytes)\n",
                action.path.c_str(), limits.maxBytes);
            return;
        }
        if (!decodedWorld) world = decodeWorld(*content);
        const auto prepareCanonicalSource = [this, &canonicalMode, &canonicalSeedImageInput](const SculptureProgramParams& baseline,
                std::string_view presetId, std::string_view presetRevision)
            -> std::shared_ptr<const SculptureCanonicalFeedbackSource> {
            if (!canonicalMode) return {};
            // Prepare identity without mutating live state. Failed decoding or
            // allocation cannot invalidate the currently selected resource.
            if (canonicalSeedImageSelectionRevision_ == std::numeric_limits<std::uint64_t>::max())
                throw std::overflow_error("canonical image selection identity exhausted");
            return std::make_shared<const SculptureCanonicalFeedbackSource>(
                SculptureCanonicalFeedbackSource{std::move(*canonicalMode), baseline,
                    std::string(presetId), std::string(presetRevision), canonicalSeedImageSelectionRevision_ + 1U,
                    canonicalSeedImageInput});
        };
        // Decode before taking over playback. Rejected input must leave the
        // live session intact; successful paths replace choreography below.
        // Prefer the complete WorldMode codec before the legacy .epimode
        // decoder. This preserves capture provenance regardless of filename;
        // the fallback below remains for authored JSON recipes and parameter
        // snapshots that never carried WorldMode metadata.
        const auto takeOverParameterSource = [this, fromManifest, &statePath, &action]() noexcept {
            if (fromManifest) {
                std::fprintf(stderr,
                    "[SculptureApplication] Reopening %s from manifest %s; "
                    "volume attachments and recursive GPU history are not restored.\n",
                    statePath.c_str(), action.path.c_str());
            }
            // Composition prioritizes deck, then morph space, then preset/sequence.
            // A decoded load owns the base source; old controllers stay dormant.
            control_.sources().deactivateAlternates();
            clearCapturedSources();
        };
        // The host owns IO/diagnostics; control owns route parsing and resolved
        // defaults. A rejected new load must not retire an earlier pending one.
        const auto prepareLoadedGraph = [this, &statePath](const std::vector<std::string>& routes)
            -> std::optional<SculptureControlSession::PreparedModulationGraph> {
            try {
                ModulationRouteDiagnostic diagnostic;
                auto candidate = control_.prepareModulationGraph(routes, appState_.audioEnabled, diagnostic);
                if (!candidate) {
                    std::fprintf(stderr,
                        "[SculptureApplication] Load routing rejected (route-index=%zu, reason=%s): %s\n",
                        diagnostic.routeIndex, modulation_inspection::name(diagnostic.error).data(),
                        statePath.c_str());
                }
                return candidate;
            } catch (const std::exception& error) {
                std::fprintf(stderr, "[SculptureApplication] Load routing preparation failed: %s\n", error.what());
            } catch (...) {
                std::fprintf(stderr, "[SculptureApplication] Load routing preparation failed.\n");
            }
            return std::nullopt;
        };
        if (world) {
            if (!world->volumePresentation.valid()) return;
            auto graphCandidate = prepareLoadedGraph(world->routingMap);
            if (!graphCandidate) return;
            std::optional<SculptureCapturedWorldRecipe> capturedCandidate;
            std::shared_ptr<const SculptureCanonicalFeedbackSource> canonicalCandidate;
            std::optional<SculptureMorphChoreographer> morphCandidate;
            std::optional<SculptureMorphSpace> morphSpaceCandidate;
            bool playlistRestored = false;
            bool morphSpaceRestored = true;
            const auto& capturedMorphSpace = world->morphSpace;
            try {
                capturedCandidate = prepareCapturedWorldRecipe(*world);
                canonicalCandidate = prepareCanonicalSource(world->params, world->presetName, world->presetRevision);
                morphCandidate.emplace(world->params);
                playlistRestored = prepareLibraryPlaylistCapture(world->playlist, *morphCandidate);
                // Restoration is transactional: expected refusal already retains
                // the captured moment. Allocation failure still rejects the load.
                if (capturedMorphSpace.active || !capturedMorphSpace.activeBankName.empty()
                    || !capturedMorphSpace.waypoints.empty()) {
                    // Retain bank definitions and uncaptured motion state, but let
                    // all fallible node/path replacement affect only this candidate.
                    morphSpaceCandidate.emplace(control_.sources().morphSpace());
                    morphSpaceRestored = morphSpaceCandidate->importCapturedState(capturedMorphSpace);
                    if (!morphSpaceRestored) morphSpaceCandidate.reset();
                }
            } catch (const std::exception& error) {
                std::fprintf(stderr, "[SculptureApplication] Capture source preparation failed: %s\n", error.what());
                return;
            } catch (...) {
                std::fprintf(stderr, "[SculptureApplication] Capture source preparation failed.\n");
                return;
            }
            // Resolve descriptive provenance before taking over the live session.
            // An empty revision in a modern capture cannot enable title/alias
            // fallback; only pre-provenance formats keep that legacy behavior.
            SculpturePresetIdResolution presetIdentity;
            if (world->version >= SculptureWorldMode::kPresetProvenanceVersion) {
                presetIdentity = presetLibrary_.resolveExactId(world->presetName);
            } else if (!world->presetName.empty()) {
                if (const auto* legacyAsset = presetLibrary_.find(world->presetName)) {
                    presetIdentity = {SculpturePresetIdStatus::Found, legacyAsset};
                }
            }
            const SculpturePresetAsset* currentAsset = presetIdentity.asset;

            presentationBindings_->setVolumePresentation(world->volumePresentation);
            takeOverParameterSource();
            capturedCanonicalSource_ = std::move(canonicalCandidate);
            if (capturedCanonicalSource_) {
                canonicalSeedImageSelectionRevision_ = capturedCanonicalSource_->seedImageSelectionRevision;
                appState_.requestTemporalReset = true;
            }
            beginCompactHostStagedTransition();
            presentationOwnership_ = world->presentationOwnership;
            presentationBindings_->director().setRenderParadigm(
                presentationOwnership_ == SculpturePresentationOwnership::UnifiedCompositeField
                    ? RenderParadigm::UnifiedComposite
                    : (presentationOwnership_ == SculpturePresentationOwnership::ImplicitField
                        ? RenderParadigm::RaymarchedImplicit
                        : RenderParadigm::VolumetricInstancing));
            presentationBindings_->director().setPresentationTuning(
                world->presentationTuning.hasTuning,
                world->presentationTuning.fogDensity,
                world->presentationTuning.emissionThreshold,
                world->presentationTuning.emissionMultiplier);
            if (!world->presetName.empty() && currentAsset) {
                for (std::size_t index = 0; index < presetLibrary_.entries().size(); ++index) {
                    if (presetLibrary_.at(index) == currentAsset) {
                        static_cast<void>(control_.bindPreset(presetLibrary_, index));
                        break;
                    }
                }
                configureWorldCadence(currentAsset->worldRecipe);
            }
            if (capturedCandidate) {
                capturedWorldRecipe_ = std::move(capturedCandidate);
                configureWorldCadence(capturedWorldRecipe_->recipe, capturedWorldRecipe_->effectiveUpdateHz);
            }

            if (world->presetName.empty()) {
                std::fprintf(stderr, "[SculptureApplication] Capture has no preset provenance.\n");
            } else if (presetIdentity.status == SculpturePresetIdStatus::Ambiguous) {
                std::fprintf(stderr,
                    "[SculptureApplication] Capture preset ID '%s' is ambiguous in the current library; "
                    "retaining the current preset binding.\n",
                    world->presetName.c_str());
            } else if (!currentAsset) {
                std::fprintf(stderr,
                    "[SculptureApplication] Capture preset '%s' is not in the current library; "
                    "retaining the current preset binding.\n",
                    world->presetName.c_str());
            } else if (world->presetRevision.empty()) {
                std::fprintf(stderr,
                    "[SculptureApplication] Capture preset '%s' has no recorded revision.\n",
                    world->presetName.c_str());
            } else if (world->presetRevision == currentAsset->revision) {
                std::fprintf(stderr,
                    "[SculptureApplication] Capture provenance matches preset '%s' (%s).\n",
                    world->presetName.c_str(), world->presetRevision.c_str());
            } else {
                std::fprintf(stderr,
                    "[SculptureApplication] Capture preset '%s' has changed "
                    "(captured %s, current %s); using the current library binding.\n",
                    world->presetName.c_str(), world->presetRevision.c_str(),
                    currentAsset->revision.c_str());
            }

            // Restore the captured parameter moment. The admitted canonical input
            // owns its captured baseline; the library remains a separate binding.
            static_assert(std::is_nothrow_move_assignable_v<SculptureMorphChoreographer>);
            control_.sources().presetMorph() = std::move(*morphCandidate);
            if (playlistRestored && world->playlist.hasLibraryTargets)
                appliedPlaylistTargetIndex_.reset();
            if (!playlistRestored) {
                std::fprintf(stderr,
                    "[SculptureApplication] Playlist provenance could not be restored; "
                    "restoring the captured moment only.\n");
            } else if (world->playlist.hasLibraryTargets) {
                for (const auto& target : world->playlist.targets) {
                    if (!target.presetRevision.empty()) continue;
                    std::fprintf(stderr,
                        "[SculptureApplication] Playlist contains unversioned cues; "
                        "using current library revisions for those IDs.\n");
                    break;
                }
            }
            if (capturedCanonicalSource_ && control_.sources().presetMorph().isPlaylistActive()) {
                // The restored cue is already this source. Reconciliation should
                // clear it on a later cue change, not merely on the first update.
                appliedPlaylistTargetIndex_ = control_.sources().presetMorph().playlistIndex();
            }
            control_.setManualOffset(SculptureManualParameter::LatentW, 0.0f);
            control_.runtime().restoreCapturedWorld(world->state);
            // Transfer the complete route candidate without parsing/allocation;
            // the source stage publishes it after playlist identity reconciliation.
            appState_.loadedModulationGraph = std::move(graphCandidate);
            if (morphSpaceCandidate) {
                static_assert(std::is_nothrow_move_assignable_v<SculptureMorphSpace>);
                control_.sources().morphSpace() = std::move(*morphSpaceCandidate);
            } else if (!morphSpaceRestored) {
                // Expected identity refusal keeps the previous controller dormant;
                // allocation failure already returned before source takeover.
                std::fprintf(stderr,
                    "[SculptureApplication] Captured morph-space bank ID is missing or ambiguous; "
                    "restoring the captured moment only.\n");
            }
            appState_.stateJustLoaded = true;
            const auto* activePreset = presetLibrary_.at(control_.presetIndex());
            std::fprintf(stderr,
                "[SculptureApplication] Restored parameter moment from %s; "
                "active preset binding is '%s' (%s).\n",
                statePath.c_str(), activePreset ? activePreset->id.c_str() : "unavailable",
                !activePreset || activePreset->revision.empty()
                    ? "revision unavailable" : activePreset->revision.c_str());
            if (world->hasResolvedWorldRecipe) {
                std::fprintf(stderr, capturedWorldRecipe_
                    ? "[SculptureApplication] Admitted captured dedicated-world recipe as the effective baseline.\n"
                    : "[SculptureApplication] Captured world recipe is provenance only; "
                      "world production uses the active preset recipe and current quality policy.\n");
            }
            std::fprintf(stderr, capturedCanonicalSource_
                ? "[SculptureApplication] Restored captured canonical producer input; "
                  "recursive GPU field history and engine clocks are not restored.\n"
                : "[SculptureApplication] Preset-dependent canonical input uses the active library binding; "
                  "saved deck automation and recursive GPU field history are not restored.\n");
            return;
        }

        if (fromManifest || statePath.find(".epimode") != std::string::npos || statePath.find(".world") != std::string::npos || statePath.find(".json") != std::string::npos || statePath.find(".epi") != std::string::npos) {
            if (fromManifest && !captureEpimodeAdmitted(*content)) {
                std::fprintf(stderr, "[SculptureApplication] Capture epimode rejected: %s\n", statePath.c_str());
                return;
            }
            // Retain one owned semantic candidate through resource admission;
            // no partially decoded output becomes live application state.
            auto result = SculptureSerializer::decodePresetContentDetailed(
                *content, std::filesystem::path(statePath).stem().string());
            auto& decoded = result.preset;
            if (!decoded) {
                // Explain only this terminal refusal. Earlier format probes may
                // legitimately fail before another supported format succeeds.
                std::fprintf(stderr,
                    "[SculptureApplication] Failed to restore moment from %s [reason=%s%s%s]\n",
                    statePath.c_str(), presetDecodeErrorName(result.diagnostic.error).data(),
                    result.diagnostic.field.empty() ? "" : "; field=",
                    result.diagnostic.field.c_str());
                return;
            }
            auto graphCandidate = prepareLoadedGraph(decoded->routingMap);
            if (!graphCandidate) return;
            // Automation rejection must precede source takeover, just like
            // canonical input admission. Copy the live engine so sparse imports
            // preserve omitted lanes and its clock; publish the admitted value
            // once rather than decoding again into live state.
            std::optional<SculptureMacroAutomationEngine> macroCandidate;
            if (!decoded->macroAutomationJson.empty()) {
                try {
                    macroCandidate.emplace(control_.sources().deck().macroEngine());
                    if (!macroCandidate->deserializeJson(decoded->macroAutomationJson)) {
                        std::fprintf(stderr, "[SculptureApplication] Macro automation rejected: %s\n", statePath.c_str());
                        return;
                    }
                } catch (const std::exception& error) {
                    std::fprintf(stderr, "[SculptureApplication] Macro automation preparation failed: %s\n", error.what());
                    return;
                }
            }
            std::shared_ptr<const SculptureCanonicalFeedbackSource> canonicalCandidate;
            std::optional<SculptureMorphChoreographer> morphCandidate;
            try {
                const std::string_view revision = canonicalPresetMetadata && canonicalPresetMetadata->first == decoded->presetName
                    ? std::string_view(canonicalPresetMetadata->second) : std::string_view{};
                canonicalCandidate = prepareCanonicalSource(decoded->params, decoded->presetName, revision);
                morphCandidate.emplace(decoded->params);
            } catch (const std::exception& error) {
                std::fprintf(stderr, "[SculptureApplication] Decoded source preparation failed: %s\n", error.what());
                return;
            }
            takeOverParameterSource();
            capturedCanonicalSource_ = std::move(canonicalCandidate);
            if (capturedCanonicalSource_) {
                canonicalSeedImageSelectionRevision_ = capturedCanonicalSource_->seedImageSelectionRevision;
                appState_.requestTemporalReset = true;
            }
            if (macroCandidate) {
                control_.sources().deck().macroEngine() = std::move(*macroCandidate);
                std::fprintf(stderr, "[SculptureApplication] Restored macro automation from %s\n", statePath.c_str());
            }
            // Restore the decoded parameter moment without merging it into the
            // currently selected preset; other preset-owned inputs stay separate.
            control_.sources().presetMorph() = std::move(*morphCandidate);
            beginCompactHostStagedTransition();
            presentationOwnership_ = decoded->presentationOwnership;
            presentationBindings_->director().setRenderParadigm(
                presentationOwnership_ == SculpturePresentationOwnership::UnifiedCompositeField
                    ? RenderParadigm::UnifiedComposite
                    : (presentationOwnership_ == SculpturePresentationOwnership::ImplicitField
                        ? RenderParadigm::RaymarchedImplicit
                        : RenderParadigm::VolumetricInstancing));
            control_.setManualOffset(SculptureManualParameter::LatentW, 0.0f);
            control_.runtime().restoreCapturedWorld(decoded->state);
            appState_.loadedModulationGraph = std::move(graphCandidate);
            appState_.stateJustLoaded = true;
            std::fprintf(stderr, "[SculptureApplication] Restored decoded parameter moment from %s\n",
                         statePath.c_str());
            return;
        }

        if (auto params = SculptureSerializer::deserializeParameterState(*content)) {
            auto graphCandidate = prepareLoadedGraph({});
            if (!graphCandidate) return;
            // This constructor may allocate: prepare it before source takeover,
            // like the richer decoded-moment paths above.
            SculptureMorphChoreographer morphCandidate(*params);
            takeOverParameterSource();
            control_.sources().presetMorph() = std::move(morphCandidate);
            // Parameter-only input declares no routes. A second accepted load
            // must not consume routing left pending by an earlier imported moment.
            appState_.loadedModulationGraph = std::move(graphCandidate);
            appState_.stateJustLoaded = true;
            std::fprintf(stderr, "[SculptureApplication] Restored custom state from %s\n",
                         action.path.c_str());
        } else {
            std::fprintf(stderr, "[SculptureApplication] Failed to restore state from %s\n",
                         action.path.c_str());
        }
        return;
    }

    if (action.action == EventFileAction::Action::LoadSequence) {
        // Host policy stays explicit; the codec does not own application budgets.
        constexpr SequenceLoadLimits limits{};
        ModulationRouteDiagnostic routeDiagnostic{};
        if (control_.loadSequence(presetLibrary_, action.path, limits, &routeDiagnostic)) {
            // The admitted sequence supersedes the prepared LoadState graph. Carry
            // that moment's pending reset forward before retiring its handoff;
            // the next source stage uses the sequence's current routes directly.
            appState_.requestTemporalReset = appState_.requestTemporalReset || appState_.stateJustLoaded;
            appState_.loadedModulationGraph.reset();
            appState_.stateJustLoaded = false;
            clearCapturedSources();
            std::fprintf(stderr, "[SculptureApplication] Loaded timeline from %s\n",
                         action.path.c_str());
        } else {
            if (routeDiagnostic.error != ModulationRouteError::None) {
                std::fprintf(stderr,
                    "[SculptureApplication] Timeline routing rejected (route-index=%zu, reason=%s): %s\n",
                    routeDiagnostic.routeIndex,
                    modulation_inspection::name(routeDiagnostic.error).data(),
                    action.path.c_str());
            }
            std::fprintf(stderr,
                "[SculptureApplication] Failed to load timeline from %s (limits: %zu bytes, %zu keyframes)\n",
                action.path.c_str(), limits.maxBytes, limits.maxKeyframes);
        }
        return;
    }

    const bool isExport = action.action == EventFileAction::Action::ExportStandaloneHtml
        || action.action == EventFileAction::Action::ExportReliefMesh
        || action.action == EventFileAction::Action::ExportGeometry;
    const bool canonicalIntent = isExport
        && resolveActiveProductionDecision().requestsCanonicalFeedback();
    if (action.action == EventFileAction::Action::ExportStandaloneHtml) {
        if (canonicalIntent) {
            std::fprintf(stderr,
                "[SculptureApplication] Standalone HTML export refused: canonical feedback cannot be represented by the scalar synthesis path.\n");
            return;
        }
        ShaderSynthesisConfig synthConfig{};
        synthConfig.title = selectedPreset().title.empty() ? selectedPreset().id : selectedPreset().title;
        const std::string htmlPath = action.path.empty() ? "sculpture_export.html" : action.path;
        const bool exported = SculptureShaderSynthesizer::exportHtmlToFile(
            htmlPath, activeParams_, synthConfig);
        std::fprintf(stderr, "[SculptureApplication] %s standalone WebGL2 HTML artifact -> %s\n",
                     exported ? "Exported" : "Failed to export", htmlPath.c_str());
        return;
    }

    if (action.action == EventFileAction::Action::ExportReliefMesh) {
        // ---------------------------------------------------------------------
        // Educational Annotation: WATERTIGHT 2.5D RELIEF DIGITAL FABRICATION
        // Constructs a solid 3D manifold with side skirts and a flat base floor
        // by sampling the continuous scalar feedback field. Directly exportable
        // to slicers (STL) and CAD environments (OBJ) for physical 3D printing.
        // ---------------------------------------------------------------------
        const auto params = activeParams_;
        EpimorphicReliefConfig reliefCfg{};
        reliefCfg.gridResX = 128U;
        reliefCfg.gridResY = 128U;
        reliefCfg.heightScale = 0.6f;
        reliefCfg.closeSolidMesh = true;
        reliefCfg.solidBaseDepth = 0.25f;
        reliefCfg.computeAnalyticalNormals = true;
        reliefCfg.projection = resolveActiveReliefProjection();

        const std::string basePath = action.path.empty() ? "epimorphic_relief_3dprint" : action.path;
        const std::string objPath = basePath + ".obj";
        const std::string stlPath = basePath + ".stl";

        const EpiFeedbackFieldDelivery* canonicalField = canonicalFeedbackFields_.latest();
        if (canonicalIntent) {
            // A host field is eligible only when its completed publication still
            // belongs to the currently selected canonical source and reset epoch.
            bool eligible = canonicalField && canonicalField->isConsumable()
                && canonicalField->transport == EpiFeedbackFieldTransport::HostStaged
                && canonicalField->isHostPayloadCoherent()
                && canonicalField->descriptor.resetEpoch == canonicalFeedbackResetEpoch_
                && !appState_.requestTemporalReset
                && !appState_.temporalReset.intent().pending(SculptureProductionRoute::CanonicalFeedback);
            if (eligible) {
                const auto input = canonicalPublicationInputs_.find(*canonicalField,
                    publishedProductionReceiptSequence(SculptureProductionRoute::CanonicalFeedback,
                        canonicalField->descriptor.generation, canonicalField->descriptor.resetEpoch));
                const auto& preset = selectedPreset();
                const auto& presetId = capturedCanonicalSource_ ? capturedCanonicalSource_->presetId : preset.id;
                const auto& revision = capturedCanonicalSource_ ? capturedCanonicalSource_->presetRevision : preset.revision;
                eligible = input && input->presetId == presetId && input->presetRevision == revision;
            }
            if (!eligible) {
                std::fprintf(stderr,
                    "[SculptureApplication] Relief mesh export refused: no current completed host-staged canonical feedback field.\n");
                return;
            }
        }
        std::vector<float> field;
        std::uint32_t fieldW = reliefCfg.gridResX;
        std::uint32_t fieldH = reliefCfg.gridResY;

        // A leftover canonical cache is not the source of a compact/world
        // export. Canonical intent reached here only after full admission above.
        if (canonicalIntent) {
            field = canonicalField->hostLinearRgba;
            fieldW = canonicalField->descriptor.width;
            fieldH = canonicalField->descriptor.height;
        } else {
            field.resize(static_cast<std::size_t>(fieldW) * fieldH);
            SculptureProgram program(params);
            const float time = control_.runtime().simulationTime();
            const float morph = control_.sources().morphSpace().isActive() ? 1.0f : control_.sources().presetMorph().progress();

            for (std::uint32_t y = 0; y < fieldH; ++y) {
                const float v = (static_cast<float>(y) / static_cast<float>(fieldH - 1U)) * 2.0f - 1.0f;
                for (std::uint32_t x = 0; x < fieldW; ++x) {
                    const float u = (static_cast<float>(x) / static_cast<float>(fieldW - 1U)) * 2.0f - 1.0f;
                    field[x + y * fieldW] = program.sampleDensityField(SculptureVec3{u, v, 0.0f}, time, morph);
                }
            }
        }

        const auto reliefData = EpimorphicReliefGenerator::generate(field, fieldW, fieldH, reliefCfg);
        const bool objWritten = EpimorphicReliefGenerator::exportObj(objPath, reliefData, "EpimorphicRelief3DSolid");
        const bool stlWritten = EpimorphicReliefGenerator::exportStlBinary(stlPath, reliefData, "epiC Watertight Epimorphic Relief");
        const auto report = EpimorphicReliefGenerator::analyzeTopology(reliefData);

        std::fprintf(stderr,
            "[SculptureApplication] Watertight 3D-Print Relief Export %s [%s%s]: %s, %s (%zu triangles, watertight=%s, chi=%d, nonManifoldEdges=%zu)\n",
            objWritten && stlWritten ? "complete" : "incomplete",
            name(reliefCfg.projection),
            activeReliefProjectionOverride_.has_value() ? " (manual override)" : "",
            objPath.c_str(), stlPath.c_str(),
            reliefData.triangleCount(),
            report.isClosedManifold ? "YES" : "NO",
            report.eulerCharacteristic,
            report.nonManifoldEdgeCount);
        return;
    }

    if (action.action != EventFileAction::Action::ExportGeometry) {
        std::fprintf(stderr, "[SculptureApplication] Unsupported file action for %s\n",
                     action.path.c_str());
        return;
    }

    if (canonicalIntent) {
        std::fprintf(stderr,
            "[SculptureApplication] Geometry export refused: canonical feedback cannot be represented by the scalar isosurface path.\n");
        return;
    }

    SculptureProgram program(activeParams_);
    IsosurfaceConfig config{};
    config.resolution = 28;
    config.isoLevel = 0.35f;
    config.calculateNormals = true;
    const auto mesh = SculptureIsosurface::extractMesh(
        program, control_.runtime().simulationTime(), control_.manualOverrides().latentW,
        {}, config);
    const std::string objPath = action.path + ".obj";
    const std::string stlPath = action.path + ".stl";
    const bool objWritten = SculptureGeometryExporter::exportWavefrontObj(
        mesh, objPath, "EpimorphismSnapshot");
    const bool stlWritten = SculptureGeometryExporter::exportBinaryStl(mesh, stlPath);
    std::fprintf(stderr,
                 "[SculptureApplication] Geometry export %s: %s, %s (%zu triangles)\n",
                 objWritten && stlWritten ? "complete" : "incomplete",
                 objPath.c_str(), stlPath.c_str(), mesh.triangleCount());
}

void SculptureApplication::applyPresentationReceiptToCapture(
    SculptureWorldMode& worldMode,
    const SculpturePresentationReceipt& receipt) noexcept {
    worldMode.presentationFrameSequence = receipt.frameSequence;
    worldMode.presentationDeviceGeneration = receipt.deviceGeneration;
    worldMode.presentationSurfaceGeneration = receipt.surfaceGeneration;
    worldMode.selectedCompactGeneration = receipt.compactGeneration;
    worldMode.selectedCompactProductionReceiptSequence =
        receipt.compactProductionReceiptSequence;
    worldMode.selectedWorldGeneration = receipt.worldGeneration;
    worldMode.selectedWorldProductionReceiptSequence =
        receipt.worldProductionReceiptSequence;
    worldMode.selectedCanonicalFeedbackGeneration = receipt.canonicalFeedbackGeneration;
    worldMode.selectedCanonicalFeedbackResetEpoch = receipt.canonicalFeedbackResetEpoch;
    worldMode.selectedCanonicalFeedbackProductionReceiptSequence =
        receipt.canonicalFeedbackProductionReceiptSequence;
    worldMode.selectedImplicitGeneration = receipt.implicitGeneration;
    worldMode.selectedImplicitProductionReceiptSequence =
        receipt.implicitProductionReceiptSequence;
    worldMode.presentationOutcome = static_cast<std::uint32_t>(receipt.outcome);
    worldMode.presentationOutcomeKnown = receipt.settled;
    worldMode.presentationConfirmed = receipt.presentationConfirmed();

    // These historic names now retain their literal meaning: a generation is
    // "displayed" only after the desktop presentation path reports its stable
    // non-exceptional outcome. The selected fields above preserve the capture
    // candidate when a submitted frame required swapchain recreation.
    worldMode.compactGeneration = receipt.presentationConfirmed()
        ? receipt.compactGeneration : 0U;
    worldMode.worldGeneration = receipt.presentationConfirmed()
        ? receipt.worldGeneration : 0U;
    worldMode.implicitGeneration = receipt.presentationConfirmed()
        ? receipt.implicitGeneration : 0U;
    worldMode.canonicalFeedbackGeneration = receipt.presentationConfirmed()
        ? receipt.canonicalFeedbackGeneration : 0U;
    worldMode.canonicalFeedbackResetEpoch = receipt.presentationConfirmed()
        ? receipt.canonicalFeedbackResetEpoch : 0U;
}

void SculptureApplication::handleFrameCapture(
    FrameContext& ctx, const mgfx::Camera& captureCamera) {
    const SculptureFramePlan& framePlan = ctx.plan();
    const auto admission = appState_.capture.admit([&](SculptureCaptureAdmission::Request request) {
        if (request.kind == SculptureCaptureAdmission::Kind::Sequence) {
            const auto directory = appState_.sequenceDestination.directoryFor(request.sequenceSession);
            if (!directory) {
                std::fprintf(stderr, "[Capture] Cannot reserve sequence directory; admission deferred\n");
                return mgfx::vk::CaptureAdmission::Busy;
            }
            return presentationHost_->frameLoop().requestCaptureOutcome(SculptureCaptureAdmission::completedOnly(
                [frameIdx = request.sequenceIndex, directory = *directory](const mgfx::vk::CaptureResult& res) {
                    try {
                        withRecorderCapturePixels(res, [&](const auto& pixels, CapturePixelOrder order) {
                            SculptureStateRecorder::captureSequenceFrame(pixels, frameIdx, directory, order);
                        });
                    } catch (const std::exception& error) {
                        std::fprintf(stderr, "[Capture] Sequence frame %u conversion failed: %s\n",
                                     frameIdx, error.what());
                    } catch (...) {
                        std::fprintf(stderr, "[Capture] Sequence frame %u conversion failed: unknown error\n",
                                     frameIdx);
                    }
                }));
        }

        // Common frozen semantic intent; observed field identity is attached
        // only after this snapshot's native submission has settled.
        SculptureWorldMode worldMode = makeFrameCaptureWorldMode(framePlan);
        if (!worldMode.hasResolvedWorldRecipe)
            std::fprintf(stderr, "[Capture] Canonical fallback omits the captured world recipe from the state.\n");
        worldMode.macroAutomationJson = control_.sources().deck().macroEngine().serializeJson();

        const auto* implicitTransport = presentationBindings_->director().implicitExternalTransport();
        worldMode.implicitExternalActive = implicitTransport != nullptr;
        worldMode.implicitDiagnostic = presentationBindings_->director().implicitDiagnostic();
        if (implicitTransport) {
            worldMode.implicitQuarantineCount = implicitTransport->diagnostics().quarantineCount;
        }

        worldMode.state = captureCameraPose(worldMode.state, captureCamera);
        const CaptureStereoIntent stereoIntent{
            framePlan.presentation().vrEnabled, framePlan.presentation().stereoRig};
        std::optional<WorldProfileSummary> completedProfile;
        if (const WorldProfileSummary* summary = worldProfileDriver_.completedSummary()) {
            completedProfile = *summary;
        }
        // Only queued snapshot provenance needs to outlive the frame. Normal
        // rendering and sequence-only capture avoid these shared allocations.
        const auto& receipt = ctx.presentationReceipt;
        auto observation = std::make_shared<SculptureFrameCaptureObservation>(
            SculptureCaptureFrameIdentity{receipt.frameSequence, receipt.deviceGeneration,
                                         receipt.surfaceGeneration});
        observation->canonicalPublicationInput = ctx.canonicalPublicationInput;
        observation->fieldCandidate = ctx.fieldCandidate;
        observation->mono = !framePlan.presentation().vrEnabled;
        const auto result = presentationHost_->frameLoop().requestCaptureOutcome(
            SculptureCaptureAdmission::joinedSnapshotOnly(observation,
            [worldMode = std::move(worldMode), completedProfile, stereoIntent,
             tracker = fieldCaptureTracker_]
            (const mgfx::vk::CaptureOutcome& outcome,
             const SculptureFrameCaptureObservation& observation) mutable {
                try {
                    withRecorderCapturePixels(outcome.image, [&](const auto& pixels, CapturePixelOrder order) {
                        applyPresentationReceiptToCapture(worldMode, observation.presentation);
                        worldMode.captureSubmissionSerial = outcome.submissionSerial;
                        const SnapshotPaths paths = SculptureStateRecorder::generateV2Paths(
                            "snapshots", observation.canonicalPublicationInput
                                ? observation.canonicalPublicationInput->presetId : worldMode.presetName);
                        const auto canonical = prepareCanonicalSnapshotInput(
                            observation.canonicalPublicationInput.get(), observation.presentation,
                            observation.identity, outcome.submissionSerial, outcome.submitted, worldMode,
                            paths.pngPath);
                        if (!canonical.admitted) {
                            std::fprintf(stderr, "[Capture] Canonical input does not match joined snapshot\n");
                            return;
                        }
                        if (canonical.inputUnknown)
                            std::fprintf(stderr, "[Capture] Selected canonical input unknown; saving configuration only\n");
                        if (!SculptureStateRecorder::prepareV2SnapshotAttachments(
                                paths, completedProfile.has_value(), canonical.state.has_value())) {
                            std::fprintf(stderr,
                                "[SculptureApplication] capture generation already exists or is unavailable\n");
                            return;
                        }
                        bool causalAttached = false;
                        if (observation.causal) {
                            causalAttached = SculptureCausalExecutionJson::writeSnapshotFile(
                                paths.causalPath, *observation.causal);
                            if (!causalAttached) {
                                std::fprintf(stderr,
                                    "[SculptureApplication] failed to write causal capture sidecar %s\n",
                                    paths.causalPath.c_str());
                            }
                        }
                        if (!SculptureStateRecorder::captureFrameAndStateWithPaths(
                            pixels, worldMode, paths,
                            completedProfile ? &*completedProfile : nullptr,
                            causalAttached, canonical.state ? &*canonical.state : nullptr, {}, order, &stereoIntent, true)) {
                            std::fprintf(stderr,
                                "[SculptureApplication] failed to publish capture bundle %s\n",
                                paths.manifestPath.c_str());
                            return;
                        }
                        const auto field = qualifyFieldCapture(observation.fieldCandidate,
                            observation.fieldAccepted, observation.identity, outcome, observation.mono);
                        if (!field) {
                            std::fprintf(stderr, "[Capture] Field/view comparison unavailable for %s\n",
                                paths.manifestPath.c_str());
                        } else {
                            if (tracker->previous) {
                                std::fprintf(stderr, "[Capture] Field/view inputs %s: %s | %s (same process only; scene settings excluded)\n",
                                    matchedFieldCapture(*tracker->previous, *field) ? "match" : "differ",
                                    tracker->previousPath.c_str(), paths.manifestPath.c_str());
                            } else {
                                std::fprintf(stderr, "[Capture] Field/view reference recorded: %s (same process only)\n",
                                    paths.manifestPath.c_str());
                            }
                            tracker->previous = *field;
                            tracker->previousPath = paths.manifestPath;
                        }
                    });
                } catch (const std::exception& error) {
                    std::fprintf(stderr, "[SculptureApplication] capture persistence failed: %s\n",
                                 error.what());
                } catch (...) {
                    std::fprintf(stderr, "[SculptureApplication] capture persistence failed: unknown error\n");
                }
            }));
        // Busy/closed admission retains no frame-local observation for a request
        // owned by another frame. Only this accepted callback keeps the envelope.
        if (result == mgfx::vk::CaptureAdmission::Accepted)
            ctx.captureObservation = std::move(observation);
        return result;
    });
    if (admission && (*admission == mgfx::vk::CaptureAdmission::Closed
                      || *admission == mgfx::vk::CaptureAdmission::Invalid)) {
        std::fprintf(stderr, "[SculptureApplication] capture admission rejected (%d)\n",
                     static_cast<int>(*admission));
    }
}

} // namespace epi_sculpt
