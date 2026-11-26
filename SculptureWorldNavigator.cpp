#include "SculptureWorldNavigator.h"
#include "SculptureWorldStatePersistence.h"
#include "SculptureSequenceCapture.h"
#include "SculptureWorldActionDispatcher.h"
#include "SculptureFluidAtmosphereManager.h"
#include "SculptureAudioFeedDirector.h"
#include "SculptureTimelineDirector.h"
#include "SculptureWorldTourChoreographer.h"
#include "SculpturePresetLibrary.h"
#include "SculptureModeProjection.h"
#include "SculptureRaymarcher.h"
#include "SculptureWorldExternalTransport.h"
#include "mgfx/pbr/Material.h"
#include "mgfx/vk/Mesh.h"
#include <cstdio>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <charconv>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace epi_sculpt {

struct SculptureWorldPrimitiveMeshes {
    std::unique_ptr<mgfx::vk::Mesh> sphere;
    std::unique_ptr<mgfx::vk::Mesh> torusKnot;
    std::unique_ptr<mgfx::vk::Mesh> cylinder;
    std::unique_ptr<mgfx::vk::Mesh> tile;
    std::unique_ptr<mgfx::vk::Mesh> cube;
    std::unique_ptr<mgfx::vk::Mesh> pillar;
    std::unique_ptr<mgfx::vk::Mesh> branch;
    std::unique_ptr<mgfx::vk::Mesh> trussStrut;
    SculptureWorldRenderBridge::PrimitiveRegistry registry{};
};

namespace {

[[nodiscard]] SculptureProgramParams resolveRootPatternWorldParams(
    std::string_view presetName) {
    return resolveWorldProgramParams(presetName);
}

static_assert(std::is_nothrow_move_assignable_v<EpimorphicReliefData>,
    "Relief geometry publication must not fail after native ownership changes");
static_assert(std::is_nothrow_assignable_v<std::optional<ReliefMeshRequest>&,
    const ReliefMeshRequest&>);
static_assert(std::is_nothrow_assignable_v<std::optional<PolychoronMeshRequest>&,
    const PolychoronMeshRequest&>);

} // namespace

SculptureWorldNavigator::SculptureWorldNavigator(
    mgfx::Engine* engine,
    SculpturePipelineCoordinator& coordinator,
    const SculpturePresetLibrary* presetLibrary,
    std::string_view initialPresetName,
    std::filesystem::path presetDirectory
) : engine_(engine),
    coordinator_(coordinator),
    presetLibrary_(presetLibrary),
    presetDirectory_(std::move(presetDirectory)),
    tourChoreographer_(std::make_unique<SculptureWorldTourChoreographer>()) {

    // 1. Collect all catalog entries with complete, implemented routes
    for (const auto& entry : worldCatalog()) {
        if (entry.hasCompleteRoute()) {
            availableWorlds_.push_back(&entry);
        }
    }

    // An empty catalog is unavailable, not permission to invent a world route.
    if (availableWorlds_.empty()) {
        throw std::runtime_error("World catalog has no implemented routes");
    }
    const auto initial = resolveWorldSelection(initialPresetName.empty()
        ? availableWorlds_.front()->canonicalPresetId : initialPresetName);
    if (!initial) {
        throw std::invalid_argument("Unknown or unsupported initial world: "
            + std::string(initialPresetName));
    }
    activeIndex_ = initial->catalogIndex;
    const auto& entry = *availableWorlds_[activeIndex_];
    currentRecipe_ = initial->recipe;
    currentPresetName_ = initial->presetId;
    currentPresetRevision_ = initial->revision;

    // Initialize autonomous tour choreographer catalog tracking and curated thematic playlists
    tourChoreographer_->setCatalogSize(availableWorlds_.size());
    initCuratedTours();

    // Startup uses the same resolved parameters and recipe as live selection.
    currentParams_ = initial->params;
    baselineParams_ = currentParams_;

    // 4. Initialize coordinator parameters
    coordinator_.setTargetParams(currentParams_, 0.0f);
    coordinator_.snapToTarget();

    // 5. Create Vulkan material handles using authoritative PBR palette (if rendering engine is active)
    if (engine_ != nullptr && engine_->materials() != nullptr) {
        const auto initialPalette = lookupWorldMaterialPalette(currentRecipe_.generator);
        bands_.reserve(initialPalette.size());
        for (const auto& band : initialPalette) {
            mgfx::pbr::Material m{};
            m.baseColorFactor = mgfx::math::vec4(
                band.baseColor[0], band.baseColor[1], band.baseColor[2], band.baseColor[3]);
            m.metallic = band.metallic;
            m.roughness = band.roughness;
            m.emissiveFactor = mgfx::math::vec3(
                band.emission[0], band.emission[1], band.emission[2]);
            bands_.push_back(engine_->materials()->createMaterial(m));
        }
    }

    // 6. Synchronize initial camera framing with AutoDirector
    syncAutoDirectorFraming();

    // 6.1 Context-aware manifold projection selection for higher-dimensional relief recipes
    switch (currentRecipe_.generator) {
    case WorldGeneratorKind::ConformalRelief25D:
        reliefConfig_.projection = ReliefManifoldProjection::PoincareDisk;
        presentationMode_ = WorldPresentationMode::HybridVolumetric;
        break;
    case WorldGeneratorKind::ToroidalFoliation35D:
        reliefConfig_.projection = ReliefManifoldProjection::ToroidalChamber;
        presentationMode_ = WorldPresentationMode::HybridVolumetric;
        break;
    case WorldGeneratorKind::KuramotoHyperbolicCanopy35D:
        reliefConfig_.projection = ReliefManifoldProjection::KleinBeltrami;
        presentationMode_ = WorldPresentationMode::HybridVolumetric;
        break;
    case WorldGeneratorKind::HyperbolicHoneycombHorosphere4D:
        reliefConfig_.projection = ReliefManifoldProjection::HyperbolicPseudosphere;
        break;
    default:
        break;
    }

    // 7. Initialize standard primitive meshes & render bridge (if rendering engine is active)
    if (engine_ != nullptr) {
        primitiveMeshes_ = std::make_unique<SculptureWorldPrimitiveMeshes>();
        primitiveMeshes_->sphere = std::make_unique<mgfx::vk::Mesh>(
            mgfx::vk::Mesh::makeSphere(engine_->context(), 1.0f, 16, 16));
        primitiveMeshes_->torusKnot = std::make_unique<mgfx::vk::Mesh>(
            mgfx::vk::Mesh::makeTorusKnot(engine_->context(), 0.5f, 0.2f, 2, 3, 64, 16));
        primitiveMeshes_->cylinder = std::make_unique<mgfx::vk::Mesh>(
            mgfx::vk::Mesh::makeCylinder(engine_->context(), 1.0f, 1.0f, 16));
        primitiveMeshes_->tile = std::make_unique<mgfx::vk::Mesh>(
            mgfx::vk::Mesh::makeCube(engine_->context()));
        primitiveMeshes_->cube = std::make_unique<mgfx::vk::Mesh>(
            mgfx::vk::Mesh::makeCube(engine_->context()));
        primitiveMeshes_->pillar = std::make_unique<mgfx::vk::Mesh>(
            mgfx::vk::Mesh::makeCylinder(engine_->context(), 1.0f, 1.0f, 16));
        primitiveMeshes_->branch = std::make_unique<mgfx::vk::Mesh>(
            mgfx::vk::Mesh::makeCylinder(engine_->context(), 1.0f, 1.0f, 16));
        primitiveMeshes_->trussStrut = std::make_unique<mgfx::vk::Mesh>(
            mgfx::vk::Mesh::makeCylinder(engine_->context(), 0.022f, 1.35f, 12));

        primitiveMeshes_->registry.meshes[0] = primitiveMeshes_->sphere.get();
        primitiveMeshes_->registry.meshes[1] = primitiveMeshes_->torusKnot.get();
        primitiveMeshes_->registry.meshes[2] = primitiveMeshes_->cylinder.get();
        primitiveMeshes_->registry.meshes[3] = primitiveMeshes_->tile.get();
        primitiveMeshes_->registry.meshes[4] = primitiveMeshes_->cube.get();
        primitiveMeshes_->registry.meshes[5] = primitiveMeshes_->pillar.get();
        primitiveMeshes_->registry.meshes[6] = primitiveMeshes_->branch.get();
        primitiveMeshes_->registry.meshes[7] = primitiveMeshes_->trussStrut.get();
    }

    // 8. Initialize real-time Performance Monitor & Frame-Pacing Optimizer
    PerformanceMonitorConfig perfCfg{};
    perfCfg.consecutiveHitchesForThrottle = 2U;
    perfCfg.stableHeadroomFramesForRecovery = 60U;
    perfCfg.enableAutoGovernorPacing = true;
    performanceMonitor_ = std::make_unique<SculpturePerformanceMonitor>(perfCfg);
    performanceMonitor_->bindGovernor(&coordinator_.stagedGovernor());

    // 9. Initialize Multidimensional Morph Space (2D Vector Pad)
    morphSpace_ = std::make_unique<SculptureMorphSpace>();
    populateMorphSpaceFromPresetLibrary();

    std::fprintf(stderr,
        "[SculptureWorldNavigator] Initialized with %zu worlds. Active [%zu/%zu]: %s (%s)\n",
        availableWorlds_.size(), activeIndex_ + 1U, availableWorlds_.size(),
        entry.title.data(), worldGeneratorName(currentRecipe_.generator).data());
}

SculptureWorldNavigator::~SculptureWorldNavigator() {
    if (performanceMonitor_ != nullptr) {
        performanceMonitor_->unbindGovernor();
        performanceMonitor_->unbindEventBus();
    }
}

void SculptureWorldNavigator::applyRecipePalette() {
    const auto palette = lookupWorldPresentationPreset(
        currentRecipe_.generator, currentRecipe_.mirrorTopology).materials;
    SculptureWorldTransitionController::applyPaletteToEngineMaterials(
        engine_, bands_, palette);
}

void SculptureWorldNavigator::updateTransitionPalette(float progress) {
    const auto blended = SculptureWorldTransitionController::interpolatePalette(
        transitionState_.fromPalette, transitionState_.toPalette, progress);
    SculptureWorldTransitionController::applyPaletteToEngineMaterials(
        engine_, bands_, blended);
}

void SculptureWorldNavigator::syncReliefProjectionForRecipe(const WorldRecipe& recipe) {
    SculptureHybridManifoldSynthesizer::syncReliefProjectionForRecipe(
        recipe, reliefConfig_, presentationMode_);
}

void SculptureWorldNavigator::syncAutoDirectorFraming() {
    const auto hint = activeFrameHint();
    float dist = std::sqrt(hint.eyeX * hint.eyeX + hint.eyeY * hint.eyeY + hint.eyeZ * hint.eyeZ);
    if (dist < 1.0f) dist = 14.0f;

    if (coordinator_.autoDirector() != nullptr) {
        AutoDirectorConfig adCfg = coordinator_.autoDirector()->config();
        adCfg.baseCameraRadius = dist;
        coordinator_.autoDirector()->setConfig(adCfg);
        coordinator_.autoDirector()->reset();
    }
}

bool SculptureWorldNavigator::selectWorld(std::size_t index, float transitionSeconds) {
    if (index >= availableWorlds_.size()) return false;
    return selectWorld(availableWorlds_[index]->canonicalPresetId, transitionSeconds);
}

bool SculptureWorldNavigator::applyWorldSelection(
    const ResolvedWorldSelection& selection, float transitionSeconds) {
    if (!std::isfinite(transitionSeconds) || transitionSeconds < 0.0f) return false;
    const auto index = selection.catalogIndex;
    const auto& nextEntry = *availableWorlds_[index];
    const auto& nextRecipe = selection.recipe;
    const auto& nextPresetName = selection.presetId;
    const auto& nextParams = selection.params;
    hasSavedCameraOverride_ = false;

    if (transitionSeconds > 0.05f) {
        const mgfx::Camera currentCam = transitionState_.active
            ? computeFrameCamera()
            : mgfx::Camera{};
        SculptureWorldTransitionController::beginTransition(
            transitionState_,
            activeIndex_,
            index,
            currentPresetName_,
            nextPresetName,
            selection.revision,
            currentRecipe_,
            nextRecipe,
            activeFrameHint(),
            currentCam,
            transitionSeconds);

        activeIndex_ = index;
        currentParams_ = nextParams;
        baselineParams_ = nextParams;
        clearHistory();
        coordinator_.setTargetParams(currentParams_, transitionSeconds);

        notifyObservers([&](INavigatorObserver& obs) {
            obs.onWorldSelected(index, nextEntry, transitionSeconds);
            obs.onWorldTransitionStarted(transitionState_.fromIndex, transitionState_.toIndex, nextEntry, transitionSeconds);
        });
    } else {
        // Immediate switch
        transitionState_.active = false;
        activeIndex_ = index;
        currentRecipe_ = nextRecipe;
        currentPresetName_ = nextPresetName;
        currentPresetRevision_ = selection.revision;
        currentParams_ = nextParams;
        baselineParams_ = nextParams;
        clearHistory();
        coordinator_.setTargetParams(currentParams_, 0.0f);
        coordinator_.snapToTarget();
        applyRecipePalette();
        syncAutoDirectorFraming();
        syncReliefProjectionForRecipe(currentRecipe_);
        renderedGeneration_ = 0U;
        worldBridge_.reset();

        notifyObservers([&](INavigatorObserver& obs) {
            obs.onWorldSelected(index, nextEntry, 0.0f);
            obs.onWorldTransitionCompleted(index, nextEntry);
        });
    }

    std::fprintf(stderr,
        "[SculptureWorldNavigator] %s [%zu/%zu]: %s (%s) [transition: %.1fs]\n",
        transitionSeconds > 0.05f ? "Initiated transition to" : "Switched to",
        activeIndex_ + 1U, availableWorlds_.size(),
        nextEntry.title.data(), worldGeneratorName(nextRecipe.generator).data(),
        transitionSeconds);
    return true;
}

bool SculptureWorldNavigator::canConfigureMirrorTopology() const noexcept {
    return !transitionState_.active &&
           currentRecipe_.generator == WorldGeneratorKind::NeonBubblesphere;
}

bool SculptureWorldNavigator::setMirrorTopology(MirrorCellTopology topology) noexcept {
    if (!canConfigureMirrorTopology()) return false;

    switch (topology) {
    case MirrorCellTopology::Triangular:
    case MirrorCellTopology::Quadrilateral:
    case MirrorCellTopology::Pentagonal:
    case MirrorCellTopology::Hexagonal:
        break;
    default:
        return false;
    }

    if (currentRecipe_.mirrorTopology == topology) return true;

    // The history ring stores only continuous program parameters. Clearing it
    // prevents an undo entry from being misread as restoring this discrete
    // OpenCL recipe override.
    clearHistory();
    currentRecipe_.mirrorTopology = topology;
    return true;
}

bool SculptureWorldNavigator::cycleMirrorTopology() noexcept {
    if (!canConfigureMirrorTopology()) return false;

    MirrorCellTopology next = MirrorCellTopology::Triangular;
    switch (currentRecipe_.mirrorTopology) {
    case MirrorCellTopology::Triangular:
        next = MirrorCellTopology::Quadrilateral;
        break;
    case MirrorCellTopology::Quadrilateral:
        next = MirrorCellTopology::Pentagonal;
        break;
    case MirrorCellTopology::Pentagonal:
        next = MirrorCellTopology::Hexagonal;
        break;
    case MirrorCellTopology::Hexagonal:
    default:
        next = MirrorCellTopology::Triangular;
        break;
    }
    return setMirrorTopology(next);
}

void SculptureWorldNavigator::cancelTransition() noexcept {
    if (!transitionState_.active) return;
    transitionState_.active = false;
    currentRecipe_ = transitionState_.toRecipe;
    currentPresetName_ = transitionState_.toPresetName;
    currentPresetRevision_ = transitionState_.toPresetRevision;
    applyRecipePalette();
    syncAutoDirectorFraming();
    syncReliefProjectionForRecipe(currentRecipe_);
}

std::optional<SculptureWorldNavigator::ResolvedWorldSelection>
SculptureWorldNavigator::resolveWorldSelection(std::string_view presetId) {
    if (presetId.empty()) return std::nullopt;
    // CLI numeric IDs use the same bounded catalog order as keyboard cycling.
    if (presetId.find_first_not_of("0123456789") == std::string_view::npos) {
        std::size_t index = 0U;
        const auto parsed = std::from_chars(presetId.data(), presetId.data() + presetId.size(), index);
        if (parsed.ec != std::errc{} || index >= availableWorlds_.size()) return std::nullopt;
        presetId = availableWorlds_[index]->canonicalPresetId;
    }
    ensurePresetLibraryLoaded();
    const auto* asset = presetLibrary_ != nullptr ? presetLibrary_->find(presetId) : nullptr;
    const WorldCatalogEntry* route = nullptr;
    if (asset != nullptr) {
        route = findPreferredWorldCatalogEntryForPattern(asset->id);
        if (route == nullptr) route = findAvailableWorldCatalogEntry(asset->id);
        const auto compatible = [&](const WorldCatalogEntry& entry) {
            return entry.generator == asset->worldRecipe.generator
                && entry.delivery == asset->worldRecipe.delivery
                && entry.presentation == asset->worldRecipe.presentation;
        };
        if (route != nullptr && !compatible(*route)) return std::nullopt;
        if (route == nullptr) {
            // Never borrow the active route or guess from generator alone.
            // Alternate assets may vary budgets/topology, but an ambiguous
            // producer/presentation family needs an explicit catalog binding.
            for (const auto* candidate : availableWorlds_) {
                if (!compatible(*candidate)) continue;
                if (route != nullptr) return std::nullopt;
                route = candidate;
            }
        }
        const auto expectedOwnership = asset->worldRecipe.usesImplicitField()
            ? SculpturePresentationOwnership::ImplicitField
            : (asset->worldRecipe.needsDedicatedGenerator()
                ? SculpturePresentationOwnership::DedicatedWorld
                : SculpturePresentationOwnership::CompactInstances);
        if (asset->presentationOwnership != expectedOwnership) return std::nullopt;
    } else {
        // Namespaced IDs must match an actual catalog identity/alias exactly;
        // suffix matching must not turn a typo into another valid preset.
        for (const auto* candidate : availableWorlds_) {
            const bool matches = presetId.find('/') == std::string_view::npos
                ? candidate->matches(presetId)
                : (candidate->canonicalPresetId == presetId
                    || std::find(candidate->aliases.begin(), candidate->aliases.end(), presetId)
                        != candidate->aliases.end());
            if (!matches) continue;
            if (route != nullptr) return std::nullopt;
            route = candidate;
        }
    }
    if (route == nullptr) return std::nullopt;
    const auto found = std::find(availableWorlds_.begin(), availableWorlds_.end(), route);
    if (found == availableWorlds_.end()) return std::nullopt;
    ResolvedWorldSelection result;
    result.catalogIndex = static_cast<std::size_t>(found - availableWorlds_.begin());
    result.presetId = asset != nullptr ? asset->id : std::string(route->canonicalPresetId);
    result.revision = asset != nullptr ? asset->revision : "catalog-defaults";
    result.params = asset != nullptr ? asset->params
        : resolveRootPatternWorldParams(route->canonicalPresetId);
    result.recipe = asset != nullptr ? asset->worldRecipe : route->recipe();
    return result;
}

bool SculptureWorldNavigator::selectWorld(std::string_view presetId, float transitionSeconds) {
    if (!std::isfinite(transitionSeconds) || transitionSeconds < 0.0f) return false;
    const auto selection = resolveWorldSelection(presetId);
    if (!selection) {
        std::fprintf(stderr, "[SculptureWorldNavigator] Unknown or unsupported world: %.*s\n",
            static_cast<int>(presetId.size()), presetId.empty() ? "" : presetId.data());
        return false;
    }
    return applyWorldSelection(*selection, transitionSeconds);
}

void SculptureWorldNavigator::nextWorld(float transitionSeconds) {
    if (availableWorlds_.empty()) return;
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    selectWorld((activeIndex_ + 1U) % availableWorlds_.size(), transitionSeconds);
}

void SculptureWorldNavigator::prevWorld(float transitionSeconds) {
    if (availableWorlds_.empty()) return;
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    selectWorld((activeIndex_ + availableWorlds_.size() - 1U) % availableWorlds_.size(), transitionSeconds);
}

void SculptureWorldNavigator::resetCameraFraming() noexcept {
    hasSavedCameraOverride_ = false;
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    syncAutoDirectorFraming();
}

mgfx::Camera SculptureWorldNavigator::computeFrameCamera(float fovYRad) const noexcept {
    // 1. Autonomous tour flight camera (if tour custom flight is active)
    if (tourChoreographer_ != nullptr && tourChoreographer_->isEnabled()) {
        if (auto flightCam = tourChoreographer_->activeFlightCamera(fovYRad)) {
            return *flightCam;
        }
    }

    // 1.5. Continuous cinematic camera from active timeline director
    if (timelineDirector_ != nullptr && timelineDirector_->isPlaying() && timelineDirector_->hasCameraOverride()) {
        return timelineDirector_->computeCamera(fovYRad);
    }

    // 2. Geodesic camera flight during active world transitions
    if (transitionState_.active) {
        return SculptureWorldTransitionController::computeGeodesicTransitionCamera(
            transitionState_, fovYRad);
    }

    // 3. Saved snapshot camera override (if active and not in transition or tour flight)
    if (hasSavedCameraOverride_) {
        return mgfx::Camera{
            .eye = savedCameraEye_,
            .target = savedCameraTarget_,
            .up = savedCameraUp_,
            .fovYRad = fovYRad,
        };
    }

    // 4. Default camera from active recipe framing hint and coordinator AutoDirector
    const auto hint = activeFrameHint();
    mgfx::math::vec3 eye(hint.eyeX, hint.eyeY, hint.eyeZ);
    mgfx::math::vec3 target(hint.targetX, hint.targetY, hint.targetZ);

    if (coordinator_.autoDirector() != nullptr) {
        const auto cp = coordinator_.autoDirector()->cameraPosition();
        const auto ct = coordinator_.autoDirector()->cameraTarget();
        eye = mgfx::math::vec3(cp.x, cp.y, cp.z);
        target = mgfx::math::vec3(ct.x, ct.y, ct.z);
    }

    return mgfx::Camera{
        .eye = eye,
        .target = target,
        .up = mgfx::math::vec3(0.0f, 1.0f, 0.0f),
        .fovYRad = fovYRad,
    };
}

void SculptureWorldNavigator::advanceTransitionAndAutopilot(float wallDeltaSeconds) noexcept {
    // 0. Advance audio synesthesia feed and update coordinator audio context
    if (audioDirector_ != nullptr && audioDirector_->isEnabled()) {
        if (const auto* volMem = coordinator_.volumetricMemory()) {
            audioDirector_->modulateFromVolumetric(volMem->telemetry().meanDensity, volMem->telemetry().peakEnergy);
        }
        const auto audioCtx = audioDirector_->update(wallDeltaSeconds, coordinator_.simulatedTime());
        coordinator_.setAudioContext(audioCtx);
    }

    // 1. Advance deterministic macro timeline director if playing
    if (timelineDirector_ != nullptr && timelineDirector_->isPlaying()) {
        timelineDirector_->tick(*this, wallDeltaSeconds);
    }

    // 2. Advance autonomous tour choreographer (if active and not superseded by timeline)
    if (tourChoreographer_ != nullptr && tourChoreographer_->isEnabled() &&
        (timelineDirector_ == nullptr || !timelineDirector_->isPlaying())) {
        tourChoreographer_->tick(*this, wallDeltaSeconds);
    }

    // 3. Advance active world transition state
    if (transitionState_.active) {
        const auto step = SculptureWorldTransitionController::stepTransition(
            transitionState_,
            currentRecipe_,
            currentPresetName_,
            currentPresetRevision_,
            wallDeltaSeconds);

        if (step.committedMidpointRecipe) {
            syncReliefProjectionForRecipe(currentRecipe_);
        }

        updateTransitionPalette(transitionState_.progress);

        if (step.completed) {
            applyRecipePalette();
            syncAutoDirectorFraming();
            syncReliefProjectionForRecipe(currentRecipe_);

            const auto completedIdx = activeIndex_;
            const auto& completedEntry = *availableWorlds_[completedIdx];
            notifyObservers([&](INavigatorObserver& obs) {
                obs.onWorldTransitionCompleted(completedIdx, completedEntry);
            });
        } else {
            notifyObservers([&](INavigatorObserver& obs) {
                obs.onWorldTransitionProgress(transitionState_.progress, transitionState_.rawProgress);
            });
        }
    }

    // 4. Advance multidimensional barycentric & Shepard morph space vector synthesis
    if (morphSpace_ != nullptr && morphSpace_->isActive()) {
        morphSpace_->update(wallDeltaSeconds);
        currentParams_ = morphSpace_->synthesizeParams();
        coordinator_.setTargetParams(currentParams_, 0.0f);
        coordinator_.snapToTarget();
        notifyObservers([&](INavigatorObserver& obs) {
            obs.onMorphSpaceUpdated(morphSpace_->cursorX(), morphSpace_->cursorY(),
                                   morphSpace_->motionModeName());
        });
    }
}

std::optional<SculpturePipelineFrame> SculptureWorldNavigator::tick(
    SculptureFrameProducer& producer,
    float wallDeltaSeconds
) {
    if (performanceMonitor_ != nullptr) {
        performanceMonitor_->beginFrame();
    }
    advanceTransitionAndAutopilot(wallDeltaSeconds);

    try {
        auto maybeFrame = coordinator_.update(wallDeltaSeconds);
        if (!maybeFrame.has_value()) {
            if (performanceMonitor_ != nullptr) {
                performanceMonitor_->markCpuWorkEnd();
                performanceMonitor_->endFrame();
            }
            return std::nullopt;
        }

        auto frame = *maybeFrame;
        if (transitionState_.active) {
            frame.morph = transitionState_.progress;
            // Proactively pre-warm the destination world's OpenCL kernel on the
            // background compiler / binary cache thread during the crossfade.
            (void)producer.prewarmWorldKernel(transitionState_.toRecipe.generator);
        } else if (tourChoreographer_ != nullptr && tourChoreographer_->isEnabled()) {
            // Proactively pre-warm the upcoming tour waypoint's OpenCL kernel
            // while dwelling on the current world so the crossfade starts with
            // zero compilation latency.
            if (const auto nextGen = tourChoreographer_->peekNextGenerator(*this)) {
                (void)producer.prewarmWorldKernel(*nextGen);
            }
        }

        // Bind live GPU volumetric memory field for in-place OpenCL instance coupling when active
        const auto* volMem = coordinator_.volumetricMemory();
        const bool supportsGpuCoupling =
            (lookupWorldMathematicalAffinity(currentRecipe_.generator).capabilityFlags &
             world_capability::kGpuVolumetricCoupling) != 0U;
        if (volMem != nullptr && volMem->isGpuActive() &&
            volumetricCoupler_.config().enableCoupling && supportsGpuCoupling) {
            producer.bindWorldVolumetricCoupling(
                *volMem,
                volumetricCoupler_.packGpuConfig(frame.deltaSeconds, frame.time));
        } else {
            producer.clearWorldVolumetricCoupling();
        }

        // Dedicated OpenCL procedural world instance dispatch with fault-tolerant status validation
        const WorldSnapshotDelivery delivery = producer.produceWorld(
            frame.params, currentRecipe_, frame.time, frame.morph,
            frame.deltaSeconds, frame.generation
        );
        if (delivery) {
            latestDelivery_ = delivery;
        } else if (!producer.worldKernelReady()) {
            // An empty delivery is normal while OpenCL is compiling or while
            // the zero-copy ring is filling after its bootstrap submission.
            // Report only an unavailable kernel; otherwise this would label
            // healthy asynchronous startup as a production failure.
            static std::uint32_t failCount = 0U;
            if (++failCount % 60U == 1U) {
                const std::string_view diagnostic = producer.worldDiagnostic();
                std::fprintf(stderr,
                    "[SculptureWorldNavigator] Warning: OpenCL world production failed for '%s' "
                    "(generation %u)%s%.*s\n",
                    currentPresetName_.c_str(), frame.generation,
                    diagnostic.empty() ? "" : ": ",
                    static_cast<int>(diagnostic.size()), diagnostic.data());
            }
        }

        // Closed-loop latency feedback for cadence governor & performance monitor
        const auto& worldTelem = producer.worldTelemetry();
        coordinator_.observeProducerMetrics(
            worldTelem.openClCompletionMilliseconds,
            worldTelem.openClCompletionMilliseconds
        );

        if (performanceMonitor_ != nullptr) {
            performanceMonitor_->recordGpuLatency(worldTelem.openClCompletionMilliseconds);
            performanceMonitor_->markCpuWorkEnd();
            performanceMonitor_->endFrame();
            updateClosedLoopResolutionScaling();
            broadcastPerformanceMetrics();
        }

        return frame;
    } catch (const std::exception& error) {
        if (performanceMonitor_ != nullptr) {
            performanceMonitor_->markCpuWorkEnd();
            performanceMonitor_->endFrame();
            broadcastPerformanceMetrics();
        }
        std::fprintf(stderr,
            "[SculptureWorldNavigator] Warning: world tick exception: %s\n",
            error.what());
        return std::nullopt;
    }
}

SculptureWorldFrame SculptureWorldNavigator::updateFrame(
    SculptureFrameProducer& producer,
    float wallDeltaSeconds,
    float fovYRad
) {
    drainActionQueue();
    SculptureWorldFrame worldFrame{};
    worldFrame.pipelineFrame = tick(producer, wallDeltaSeconds);
    worldFrame.camera = computeFrameCamera(fovYRad);
    worldFrame.generation = renderedGeneration_;
    worldFrame.instanceCount = renderedInstanceCount();
    worldFrame.isNewGeneration = worldFrame.pipelineFrame.has_value();

    if (atmosphereManager_ != nullptr && atmosphereManager_->isEnabled()) {
        atmosphereManager_->update(
            wallDeltaSeconds,
            coordinator_.simulatedTime(),
            coordinator_.audioResonance().metrics()
        );
    }

    return worldFrame;
}

SculptureWorldFrame SculptureWorldNavigator::updateFrame(
    float wallDeltaSeconds,
    float fovYRad
) {
    drainActionQueue();
    if (performanceMonitor_ != nullptr) {
        performanceMonitor_->beginFrame();
    }
    advanceTransitionAndAutopilot(wallDeltaSeconds);

    SculptureWorldFrame worldFrame{};
    worldFrame.pipelineFrame = coordinator_.update(wallDeltaSeconds);
    worldFrame.camera = computeFrameCamera(fovYRad);
    worldFrame.generation = renderedGeneration_;
    worldFrame.instanceCount = renderedInstanceCount();
    worldFrame.isNewGeneration = worldFrame.pipelineFrame.has_value();

    if (atmosphereManager_ != nullptr && atmosphereManager_->isEnabled()) {
        atmosphereManager_->update(
            wallDeltaSeconds,
            coordinator_.simulatedTime(),
            coordinator_.audioResonance().metrics()
        );
    }

    if (performanceMonitor_ != nullptr) {
        performanceMonitor_->markCpuWorkEnd();
        performanceMonitor_->endFrame();
        updateClosedLoopResolutionScaling();
        broadcastPerformanceMetrics();
    }
    return worldFrame;
}

SculptureWorldFrame SculptureWorldNavigator::renderFrame(
    mgfx::Engine& engine,
    mgfx::vk::FrameLoop& loop,
    SculptureFrameProducer& producer,
    float wallDeltaSeconds,
    float fovYRad,
    const std::function<void(mgfx::Drawable&)>& secondaryDraw
) {
    try {
        // 1. Dynamic audio-reactive lighting
        applyDynamicLighting(engine, coordinator_.simulatedTime(), coordinator_.audioResonance().metrics());

        // 2. Closed-loop simulation tick, camera geodesics, and dynamic atmosphere update
        auto worldFrame = updateFrame(producer, wallDeltaSeconds, fovYRad);
        return renderFrameInternal(engine, loop, worldFrame, producer.delivery().generation,
                                   wallDeltaSeconds, secondaryDraw);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[SculptureWorldNavigator] Warning: Exception in renderFrame (producer): %s\n", e.what());
        return SculptureWorldFrame{};
    }
}

SculptureWorldFrame SculptureWorldNavigator::renderFrame(
    mgfx::Engine& engine,
    mgfx::vk::FrameLoop& loop,
    float wallDeltaSeconds,
    float fovYRad,
    const std::function<void(mgfx::Drawable&)>& secondaryDraw
) {
    try {
        // 1. Dynamic audio-reactive lighting
        applyDynamicLighting(engine, coordinator_.simulatedTime(), coordinator_.audioResonance().metrics());

        // 2. Closed-loop simulation tick (coordinator-driven), camera geodesics, and dynamic atmosphere update
        auto worldFrame = updateFrame(wallDeltaSeconds, fovYRad);
        return renderFrameInternal(engine, loop, worldFrame, 0U,
                                   wallDeltaSeconds, secondaryDraw);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[SculptureWorldNavigator] Warning: Exception in renderFrame: %s\n", e.what());
        return SculptureWorldFrame{};
    }
}

SculptureWorldFrame SculptureWorldNavigator::renderFrameInternal(
    mgfx::Engine& engine,
    mgfx::vk::FrameLoop& loop,
    SculptureWorldFrame worldFrame,
    std::uint32_t captureGeneration,
    float wallDeltaSeconds,
    const std::function<void(mgfx::Drawable&)>& secondaryDraw
) {
    // 3. Application-owned navigation controller override
    if (cameraController_) {
        worldFrame.camera = cameraController_(worldFrame.camera, wallDeltaSeconds);
    }

    // 4. Snapshot and video sequence recording coordination
    const bool snapshotRequested = consumeSnapshotRequest();
    if (snapshotRequested) {
        requestSnapshot(loop, worldFrame.camera, captureGeneration);
    }

    // Snapshot and sequence share one capture admission for this frame.
    if (!snapshotRequested && isSequenceRecording()) {
        const auto* recorder = coordinator_.sequenceRecorder();
        SequenceCaptureRequest request{
            recorder ? recorder->captureSession() : 0U,
            SculptureWorldStatePersistence::makeSequenceFrameMetadata(*this, worldFrame.camera, {})};
        request.metadata.origin = SequenceMetadataOrigin::CaptureRequest;
        loop.requestCapture([this, request = std::move(request)](const mgfx::vk::CaptureResult& res) {
            if (auto* current = coordinator_.sequenceRecorder()) {
                deliverSequenceCapture(*current, request, res,
                    [this](std::uint64_t session, std::uint32_t width, std::uint32_t height, const std::uint8_t* pixels,
                           const SequenceFrameMetadata& metadata, bool isBgra) {
                        return coordinator_.recordFrameForSession(session, width, height, pixels, metadata, isBgra);
                    });
            }
        });
    }

    // 5. Vulkan engine draw pass
    const auto draw = [&](mgfx::Drawable& draw) {
        render(draw, computePresentationTransform());
        if (secondaryDraw) {
            secondaryDraw(draw);
        }
        renderHud(draw);
    };

    if (stereoEnabled_) {
        const auto toTarget = worldFrame.camera.target - worldFrame.camera.eye;
        const auto forward = glm::length(toTarget) > 1e-4f ? glm::normalize(toTarget) : glm::vec3(0.0f, 0.0f, -1.0f);
        const auto right = glm::normalize(glm::cross(forward, worldFrame.camera.up));
        auto leftEye = worldFrame.camera;
        auto rightEye = worldFrame.camera;
        const float halfIpd = ipdMeters_ * 0.5f;
        leftEye.eye -= right * halfIpd;
        leftEye.target -= right * halfIpd;
        rightEye.eye += right * halfIpd;
        rightEye.target += right * halfIpd;
        engine.runStereoFrame(leftEye, rightEye, draw);
    } else {
        engine.runFrame(worldFrame.camera, draw);
    }

    return worldFrame;
}



SculptureWorldMode SculptureWorldNavigator::makeCaptureWorldMode(const mgfx::Camera& cam, std::uint32_t generation) const {
    return SculptureWorldStatePersistence::makeCaptureWorldMode(*this, cam, generation);
}

SnapshotPaths SculptureWorldNavigator::saveSnapshotBundle(
    const std::string& directory,
    const std::string& baseName,
    const mgfx::Camera* cameraOverride
) {
    return SculptureWorldStatePersistence::saveSnapshotBundle(*this, directory, baseName, cameraOverride);
}

bool SculptureWorldNavigator::loadSnapshotBundle(const std::string& filepathOrManifest) {
    return SculptureWorldStatePersistence::loadSnapshotBundle(*this, filepathOrManifest);
}

void SculptureWorldNavigator::requestSnapshot() noexcept {
    SculptureWorldStatePersistence::requestSnapshot(*this);
}

void SculptureWorldNavigator::requestSnapshot(
    mgfx::vk::FrameLoop& frameLoop,
    const mgfx::Camera& cam,
    std::uint32_t generation,
    const std::string& directory
) {
    SculptureWorldStatePersistence::requestSnapshot(*this, frameLoop, cam, generation, directory);
}

mgfx::Camera SculptureWorldNavigator::savedCameraOverride(float fovYRad) const noexcept {
    return mgfx::Camera{
        .eye = savedCameraEye_,
        .target = savedCameraTarget_,
        .up = savedCameraUp_,
        .fovYRad = fovYRad,
    };
}

bool SculptureWorldNavigator::loadSnapshot(const std::string& filepath) {
    return SculptureWorldStatePersistence::loadSnapshot(*this, filepath);
}

bool SculptureWorldNavigator::applyWorldMode(const SculptureWorldMode& worldMode) {
    return SculptureWorldStatePersistence::applyWorldMode(*this, worldMode);
}

SculptureStateDiff SculptureWorldNavigator::diffFromBaseline(float epsilon) const {
    return SculptureWorldStatePersistence::diffFromBaseline(*this, epsilon);
}

bool SculptureWorldNavigator::applyPatch(const SculptureStateDiff& diff, std::string_view tag) {
    return SculptureWorldStatePersistence::applyPatch(*this, diff, tag);
}

bool SculptureWorldNavigator::exportBaselineDiff(const std::string& filepath) const {
    return SculptureWorldStatePersistence::exportBaselineDiff(*this, filepath);
}

bool SculptureWorldNavigator::importDiffPatch(const std::string& filepath) {
    return SculptureWorldStatePersistence::importDiffPatch(*this, filepath);
}

void SculptureWorldNavigator::pushHistory(std::string_view tag) {
    SculptureWorldStatePersistence::pushHistory(*this, tag);
}

bool SculptureWorldNavigator::undo() {
    return SculptureWorldStatePersistence::undo(*this);
}

bool SculptureWorldNavigator::redo() {
    return SculptureWorldStatePersistence::redo(*this);
}

void SculptureWorldNavigator::clearHistory() noexcept {
    SculptureWorldStatePersistence::clearHistory(*this);
}

bool SculptureWorldNavigator::startSequenceRecording(std::string_view sessionName) {
    return SculptureWorldStatePersistence::startSequenceRecording(*this, sessionName);
}

bool SculptureWorldNavigator::stopSequenceRecording() {
    return SculptureWorldStatePersistence::stopSequenceRecording(*this);
}

bool SculptureWorldNavigator::toggleSequenceRecording(std::string_view sessionName) {
    if (coordinator_.isSequenceRecording()) {
        return stopSequenceRecording();
    }
    return startSequenceRecording(sessionName);
}

bool SculptureWorldNavigator::isSequenceRecording() const noexcept {
    return coordinator_.isSequenceRecording();
}

SequenceRecorderStats SculptureWorldNavigator::sequenceRecordingStats() const {
    if (const auto* rec = coordinator_.sequenceRecorder()) {
        return rec->stats();
    }
    return SequenceRecorderStats{};
}

bool SculptureWorldNavigator::recordFrame(
    const mgfx::vk::CaptureResult& res,
    const mgfx::Camera& cam,
    std::string_view customTag
) {
    return SculptureWorldStatePersistence::recordFrame(*this, res, cam, customTag);
}

bool SculptureWorldNavigator::recordFrame(
    std::uint32_t width,
    std::uint32_t height,
    const std::uint8_t* pixels,
    const mgfx::Camera& cam,
    bool isBgra,
    std::string_view customTag
) {
    return SculptureWorldStatePersistence::recordFrame(*this, width, height, pixels, cam, isBgra, customTag);
}

void SculptureWorldNavigator::formatHud(
    char* buffer,
    std::size_t size,
    std::uint32_t generation,
    std::size_t instanceCount
) const noexcept {
    SculptureWorldTelemetryFormatter::formatHud(
        *this, buffer, size, generation, instanceCount);
}

void SculptureWorldNavigator::renderHud(
    mgfx::Drawable& draw,
    const mgfx::math::vec2& position,
    float fontSize,
    const mgfx::math::vec4& color
) const noexcept {
    char buffer[1024];
    formatHud(buffer, sizeof(buffer));
    draw.drawText(buffer, position, fontSize, color);

    float yOffset = 30.0f;

    // 1. If 2D Morph Space is active, draw a compact visual 2D Vector Pad HUD overlay
    if (isMorphSpaceActive() && morphSpace_ != nullptr) {
        MorphPadWidget::render(draw, *morphSpace_,
            mgfx::math::vec2(position.x, position.y + yOffset),
            fontSize * 0.85f, mgfx::math::vec4(0.35f, 0.92f, 1.0f, 0.95f));
        yOffset += 115.0f;
    }

    // 2. If VJ Performance Deck is active, draw dual-deck crossfader console overlay
    if (isPerformanceDeckEnabled() && performanceDeck() != nullptr) {
        PerformanceDeckWidget::render(draw, *performanceDeck(),
            mgfx::math::vec2(position.x, position.y + yOffset),
            fontSize * 0.85f, mgfx::math::vec4(1.0f, 0.82f, 0.35f, 0.95f));
        yOffset += 115.0f;
    }

    // 3. If Learn Mode is active (stages isolated/bypassed), draw pedagogical decomposition table
    if (isLearnModeActive()) {
        LearnModeWidget::render(draw, stageMask(),
            mgfx::math::vec2(position.x, position.y + yOffset),
            fontSize * 0.85f, mgfx::math::vec4(0.40f, 0.95f, 0.65f, 0.95f));
        yOffset += 135.0f;
    }
}

void SculptureWorldNavigator::increaseSpeed(float factor) noexcept {
    coordinator_.increaseSpeed(factor);
}

void SculptureWorldNavigator::decreaseSpeed(float factor) noexcept {
    coordinator_.decreaseSpeed(factor);
}

void SculptureWorldNavigator::resetSpeed() noexcept {
    coordinator_.resetSpeed();
}

float SculptureWorldNavigator::speedMultiplier() const noexcept {
    return coordinator_.speedMultiplier();
}

void SculptureWorldNavigator::togglePause() noexcept {
    coordinator_.togglePause();
}

void SculptureWorldNavigator::setPaused(bool paused) noexcept {
    coordinator_.setPaused(paused);
}

bool SculptureWorldNavigator::isPaused() const noexcept {
    return coordinator_.isPaused();
}

void SculptureWorldNavigator::mutate(float mutationScale, float durationSeconds) {
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    pushHistory("Mutate");
    coordinator_.mutateParameters(mutationScale, durationSeconds);
    if (durationSeconds <= 0.0f) {
        coordinator_.snapToTarget();
    }
    currentParams_ = coordinator_.currentParams();
}

void SculptureWorldNavigator::randomizePalette(float durationSeconds) {
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    pushHistory("RandomizePalette");
    coordinator_.randomizePalette(durationSeconds);
    if (durationSeconds <= 0.0f) {
        coordinator_.snapToTarget();
    }
    currentParams_ = coordinator_.currentParams();
}

void SculptureWorldNavigator::resetParameters(float durationSeconds) {
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    pushHistory("ResetParameters");
    currentParams_ = baselineParams_;
    coordinator_.resetToBaseline(baselineParams_, durationSeconds);
    if (durationSeconds <= 0.0f) {
        coordinator_.snapToTarget();
    }
    notifyObservers([](INavigatorObserver& obs) {
        obs.onBaselineReverted();
    });
}

void SculptureWorldNavigator::cycleInnerMap(float durationSeconds) {
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    pushHistory("CycleInnerMap");
    coordinator_.cycleInnerMap(durationSeconds);
    if (durationSeconds <= 0.0f) {
        coordinator_.snapToTarget();
    }
    currentParams_ = coordinator_.currentParams();
}

void SculptureWorldNavigator::cycleReduction(float durationSeconds) {
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    pushHistory("CycleReduction");
    coordinator_.cycleReduction(durationSeconds);
    if (durationSeconds <= 0.0f) {
        coordinator_.snapToTarget();
    }
    currentParams_ = coordinator_.currentParams();
}

bool SculptureWorldNavigator::areParametersMutated() const noexcept {
    const auto& active = coordinator_.currentParams();
    return std::abs(active.pre.scaleReal - baselineParams_.pre.scaleReal) > 1e-3f
        || std::abs(active.post.scaleReal - baselineParams_.post.scaleReal) > 1e-3f
        || std::abs(active.latentW - baselineParams_.latentW) > 1e-3f
        || std::abs(active.rotorAlpha - baselineParams_.rotorAlpha) > 1e-3f
        || std::abs(active.topologyBlend - baselineParams_.topologyBlend) > 1e-3f;
}

void SculptureWorldNavigator::enableTour(bool active) {
    if (!tourChoreographer_) {
        tourChoreographer_ = std::make_unique<SculptureWorldTourChoreographer>();
    }
    tourChoreographer_->enable(active);
}

void SculptureWorldNavigator::toggleTour() noexcept {
    if (!tourChoreographer_) {
        tourChoreographer_ = std::make_unique<SculptureWorldTourChoreographer>();
    }
    tourChoreographer_->toggle();
}

bool SculptureWorldNavigator::isTourActive() const noexcept {
    return tourChoreographer_ != nullptr && tourChoreographer_->isEnabled();
}

void SculptureWorldNavigator::setTourPlaylist(
    std::vector<std::string> presetIds,
    std::string_view tourName
) {
    if (!tourChoreographer_) {
        tourChoreographer_ = std::make_unique<SculptureWorldTourChoreographer>();
    }
    tourChoreographer_->setPlaylist(std::move(presetIds), tourName);
}

void SculptureWorldNavigator::initCuratedTours() {
    curatedTours_.clear();

    // 1. Complete Implemented Catalog Sequential Tour
    curatedTours_.push_back(CuratedThematicTour{
        .id = "full_catalog",
        .title = "Full Catalog Autopilot",
        .mode = WorldTourMode::SequentialCatalog,
        .presetIds = {},
    });

    // 2. Higher-Dimensional Dedicated OpenCL Worlds (12 worlds: 180-191)
    curatedTours_.push_back(CuratedThematicTour{
        .id = "higher_dimensional_opencl",
        .title = "Higher-D OpenCL Worlds",
        .mode = WorldTourMode::CuratedPlaylist,
        .presetIds = {
            "authored/dimensional/180_clifford_hopf_multi_soliton_lattice_4d",
            "authored/dimensional/181_hecatonicosachoron_120cell_coxeter_4d",
            "authored/dimensional/182_hexacosichoron_600cell_h4_polytope_4d",
            "authored/dimensional/183_hyperbolic_icosahedral_honeycomb_horosphere_4d",
            "authored/dimensional/184_lie_e8_root_lattice_coxeter_projection_4d",
            "authored/dimensional/185_conformal_hyperbolic_rose_petal_relief_25d",
            "authored/dimensional/186_clifford_vortex_kaleidoscope_mandala_35d",
            "authored/dimensional/187_dedekind_eta_quartic_soliton_lattice_4d",
            "authored/dimensional/188_kuramoto_hyperbolic_synchronization_canopy_35d",
            "authored/dimensional/189_so4_isoclinic_penrose_twistor_foliation_4d",
            "authored/dimensional/190_dirac_monopole_quantum_tunnel",
            "authored/dimensional/191_su2_yang_mills_instanton",
        },
    });

    // 3. 4D Polytopes & Higher-Dimensional Lattices
    curatedTours_.push_back(CuratedThematicTour{
        .id = "hyperdimensional_polytopes_4d",
        .title = "4D Polytopes & Lattices",
        .mode = WorldTourMode::CuratedPlaylist,
        .presetIds = {
            "120_cell_coxeter",
            "hexacosichoron_600cell",
            "hyperbolic_honeycomb_4d",
            "lie_e8_gosset",
            "dedekind_eta",
            "penrose_twistor",
            "120_cell_quantum_hologram",
        },
    });

    // 4. Non-Euclidean Relief & Foliation Frontiers
    curatedTours_.push_back(CuratedThematicTour{
        .id = "non_euclidean_relief",
        .title = "Non-Euclidean Relief Frontiers",
        .mode = WorldTourMode::CuratedPlaylist,
        .presetIds = {
            "conformal_relief_25d",
            "toroidal_foliation_35d",
            "kuramoto_hyperbolic_synchronization_canopy_35d",
            "allen_cahn",
            "ammann_beenker_quasicrystal",
        },
    });

    // 5. Cosmological & Chaos Odysseys
    curatedTours_.push_back(CuratedThematicTour{
        .id = "cosmological_chaos",
        .title = "Cosmological & Chaos Odyssey",
        .mode = WorldTourMode::CuratedPlaylist,
        .presetIds = {
            "exp_golden_phyllotaxis_cathedral",
            "authored/dimensional/190_dirac_monopole_quantum_tunnel",
            "strange_attractor",
            "lichtenberg_discharge",
            "mandelbulb",
            "kali_set",
            "calabi_yau",
        },
    });

    // 6. Canonical Priority Benchmark Worlds (Priorities 0 through 10)
    curatedTours_.push_back(CuratedThematicTour{
        .id = "canonical_priority_benchmarks",
        .title = "Canonical Priority Benchmark Worlds",
        .mode = WorldTourMode::CuratedPlaylist,
        .presetIds = {
            "neon_bubblesphere",
            "kuramoto_synchronization_manifold",
            "hopf_fibration_drift",
            "exp_golden_phyllotaxis_cathedral",
            "sinh_rossler_spiral_ribbon",
            "120_cell_quantum_hologram",
            "ammann_beenker_quasicrystal",
            "faraday_chladni_resonance",
            "lichtenberg_discharge",
            "gyroid_maze",
            "hypercube_tesseract",
        },
    });

    // 7. Full Catalog Pseudo-Random Shuffle Exploration
    curatedTours_.push_back(CuratedThematicTour{
        .id = "full_catalog_shuffle",
        .title = "Full Catalog Shuffle",
        .mode = WorldTourMode::ShuffleCatalog,
        .presetIds = {},
    });

    // 7. Dynamic Category Tours from Preset Library
    if (presetLibrary_ != nullptr) {
        for (const auto& cat : presetLibrary_->categories()) {
            if (cat.empty()) continue;
            auto catAssets = presetLibrary_->findByCategory(cat);
            if (!catAssets.empty()) {
                std::vector<std::string> ids;
                ids.reserve(catAssets.size());
                for (const auto* a : catAssets) {
                    ids.push_back(a->id);
                }
                curatedTours_.push_back(CuratedThematicTour{
                    .id = "category_" + cat,
                    .title = "Category: " + cat,
                    .mode = WorldTourMode::CuratedPlaylist,
                    .presetIds = std::move(ids),
                });
            }
        }
    }

    activeCuratedTourIndex_ = 0U;
}

void SculptureWorldNavigator::cycleCuratedTour() noexcept {
    if (curatedTours_.empty()) return;
    const std::size_t nextIndex = (activeCuratedTourIndex_ + 1U) % curatedTours_.size();
    selectCuratedTour(nextIndex);
}

bool SculptureWorldNavigator::selectCuratedTour(std::size_t index) noexcept {
    if (index >= curatedTours_.size()) return false;
    activeCuratedTourIndex_ = index;
    const auto& tour = curatedTours_[activeCuratedTourIndex_];

    if (tour.title.rfind("Category: ", 0) == 0) {
        activeTourCategory_ = tour.title.substr(std::strlen("Category: "));
    }

    if (!tourChoreographer_) {
        tourChoreographer_ = std::make_unique<SculptureWorldTourChoreographer>();
    }
    tourChoreographer_->setCatalogSize(availableWorlds_.size());

    if (tour.mode == WorldTourMode::CuratedPlaylist && !tour.presetIds.empty()) {
        tourChoreographer_->setPlaylist(tour.presetIds, tour.title);
    } else {
        WorldTourConfig cfg = tourChoreographer_->config();
        cfg.mode = tour.mode;
        cfg.tourName = tour.title;
        tourChoreographer_->setConfig(cfg);
        tourChoreographer_->reset();
    }
    tourChoreographer_->enable(true);
    std::fprintf(stderr,
        "[SculptureWorldNavigator] Activated Curated Tour [%zu/%zu]: \"%s\" (mode: %s, waypoints: %zu)\n",
        activeCuratedTourIndex_ + 1U, curatedTours_.size(),
        tour.title.c_str(), name(tour.mode),
        tour.presetIds.empty() ? availableWorlds_.size() : tour.presetIds.size());
    return true;
}

bool SculptureWorldNavigator::selectCuratedTour(std::string_view tourId) noexcept {
    for (std::size_t i = 0; i < curatedTours_.size(); ++i) {
        if (curatedTours_[i].id == tourId) {
            return selectCuratedTour(i);
        }
    }
    return false;
}

void SculptureWorldNavigator::cycleTourMode() noexcept {
    if (!tourChoreographer_) {
        tourChoreographer_ = std::make_unique<SculptureWorldTourChoreographer>();
    }
    tourChoreographer_->cycleTourMode();
}

void SculptureWorldNavigator::stepTourNext() {
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->advanceNext(*this);
    }
}

void SculptureWorldNavigator::stepTourPrev() {
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->advancePrev(*this);
    }
}

void SculptureWorldNavigator::ensurePresetLibraryLoaded() {
    if (presetLibrary_ != nullptr) {
        return;
    }
    if (!ownedPresetLibrary_) {
        if (presetDirectory_.empty()) return;
        std::error_code ec;
        if (!std::filesystem::is_directory(presetDirectory_, ec) || ec) return;

        ownedPresetLibrary_ = std::make_unique<SculpturePresetLibrary>();
        if (!ownedPresetLibrary_->reload(presetDirectory_)) {
            ownedPresetLibrary_.reset();
            return;
        }
        presetLibrary_ = ownedPresetLibrary_.get();

        for (const auto& cat : presetLibrary_->categories()) {
            if (cat.empty()) continue;
            auto catAssets = presetLibrary_->findByCategory(cat);
            if (!catAssets.empty()) {
                std::vector<std::string> ids;
                ids.reserve(catAssets.size());
                for (const auto* a : catAssets) {
                    ids.push_back(a->id);
                }
                const std::string tourId = "category_" + cat;
                bool exists = false;
                for (const auto& ct : curatedTours_) {
                    if (ct.id == tourId) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    curatedTours_.push_back(CuratedThematicTour{
                        .id = tourId,
                        .title = "Category: " + cat,
                        .mode = WorldTourMode::CuratedPlaylist,
                        .presetIds = std::move(ids),
                    });
                }
            }
        }
        populateMorphSpaceFromPresetLibrary();
    }
}

std::vector<std::string> SculptureWorldNavigator::availableTourCategories() {
    ensurePresetLibraryLoaded();
    if (presetLibrary_ != nullptr) {
        auto cats = presetLibrary_->categories();
        std::vector<std::string> result;
        result.reserve(cats.size());
        for (auto& c : cats) {
            if (!c.empty() && !presetLibrary_->findByCategory(c).empty()) {
                result.push_back(std::move(c));
            }
        }
        return result;
    }
    return {};
}

bool SculptureWorldNavigator::selectCategoryTour(
    std::string_view category,
    float durationSeconds,
    float dwellSeconds
) {
    ensurePresetLibraryLoaded();
    if (presetLibrary_ == nullptr) return false;

    auto assets = presetLibrary_->findByCategory(category);
    std::string matchedCategory(category);
    if (assets.empty()) {
        for (const auto& cat : presetLibrary_->categories()) {
            if (cat.size() == category.size() &&
                std::equal(cat.begin(), cat.end(), category.begin(), category.end(),
                           [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); })) {
                assets = presetLibrary_->findByCategory(cat);
                matchedCategory = cat;
                break;
            }
        }
    }
    if (assets.empty()) return false;

    std::vector<std::string> ids;
    ids.reserve(assets.size());
    for (const auto* a : assets) {
        ids.push_back(a->id);
    }

    if (!tourChoreographer_) {
        tourChoreographer_ = std::make_unique<SculptureWorldTourChoreographer>();
    }
    tourChoreographer_->setCatalogSize(availableWorlds_.size());

    const std::string tourTitle = "Category: " + matchedCategory;
    tourChoreographer_->setPlaylist(ids, tourTitle);

    WorldTourConfig cfg = tourChoreographer_->config();
    cfg.mode = WorldTourMode::CuratedPlaylist;
    cfg.tourName = tourTitle;
    if (durationSeconds > 0.0f) {
        cfg.transitionSeconds = durationSeconds;
    }
    if (dwellSeconds > 0.0f) {
        cfg.dwellSeconds = dwellSeconds;
    }
    tourChoreographer_->setConfig(cfg);
    tourChoreographer_->reset();
    tourChoreographer_->enable(true);

    activeTourCategory_ = matchedCategory;
    const auto allCats = availableTourCategories();
    for (std::size_t i = 0; i < allCats.size(); ++i) {
        if (allCats[i] == activeTourCategory_) {
            activeCategoryIndex_ = i;
            break;
        }
    }

    // Keep activeCuratedTourIndex_ in sync if this category is in curatedTours_
    const std::string tourId = "category_" + matchedCategory;
    for (std::size_t i = 0; i < curatedTours_.size(); ++i) {
        if (curatedTours_[i].id == tourId || curatedTours_[i].title == tourTitle) {
            activeCuratedTourIndex_ = i;
            break;
        }
    }

    // Immediately switch to the first preset of the category tour
    if (!ids.empty()) {
        selectWorld(ids.front(), durationSeconds);
    }

    // Automatically bind 2D Morph Space to the active category
    bindMorphSpaceToCategory(matchedCategory, true);

    std::fprintf(stderr,
        "[SculptureWorldNavigator] Activated Category Tour: \"%s\" (%zu waypoints, dwell: %.1fs, flight: %.1fs)\n",
        activeTourCategory_.c_str(), ids.size(), cfg.dwellSeconds, cfg.transitionSeconds);

    return true;
}

void SculptureWorldNavigator::cycleCategoryTour() {
    const auto cats = availableTourCategories();
    if (cats.empty()) return;

    std::size_t nextIndex = 0U;
    auto it = std::find(cats.begin(), cats.end(), activeTourCategory_);
    if (it != cats.end()) {
        nextIndex = (std::distance(cats.begin(), it) + 1U) % cats.size();
    } else {
        nextIndex = activeCategoryIndex_ % cats.size();
    }
    activeCategoryIndex_ = nextIndex;
    selectCategoryTour(cats[nextIndex]);
}

bool SculptureWorldNavigator::isCategoryTourActive() const noexcept {
    if (tourChoreographer_ == nullptr || !tourChoreographer_->isEnabled()) {
        return false;
    }
    const std::string& name = tourChoreographer_->tourName();
    return name.rfind("Category: ", 0) == 0;
}

void SculptureWorldNavigator::cycleResolutionScale() noexcept {
    coordinator_.cycleResolutionScale();
}

void SculptureWorldNavigator::toggleStagedGovernor() noexcept {
    coordinator_.setStagedComputeGovernorEnabled(!coordinator_.isStagedComputeGovernorEnabled());
}

StageResolutionScale SculptureWorldNavigator::resolutionScale() const noexcept {
    return coordinator_.resolutionScale();
}

bool SculptureWorldNavigator::isStagedGovernorEnabled() const noexcept {
    return coordinator_.isStagedComputeGovernorEnabled();
}

void SculptureWorldNavigator::setVariableResolutionScale(StageResolutionScale scale) noexcept {
    if (!isValidStageResolutionScale(scale)) return;
    coordinator_.setResolutionScale(scale);
    if (performanceMonitor_ != nullptr) {
        performanceMonitor_->reset();
    }
}

void SculptureWorldNavigator::setAutoResolutionScalingEnabled(bool enabled) noexcept {
    autoResolutionScalingEnabled_ = enabled;
    coordinator_.setStagedComputeGovernorEnabled(enabled);
    if (performanceMonitor_ != nullptr) {
        auto cfg = performanceMonitor_->config();
        cfg.enableAutoGovernorPacing = enabled;
        performanceMonitor_->setConfig(cfg);
        performanceMonitor_->reset();
    }
}

bool SculptureWorldNavigator::isAutoResolutionScalingEnabled() const noexcept {
    return autoResolutionScalingEnabled_;
}

void SculptureWorldNavigator::recordPerformanceSample(float frameIntervalMs, float cpuMs, float gpuMs) noexcept {
    if (performanceMonitor_ != nullptr) {
        performanceMonitor_->recordFrameSample(frameIntervalMs, cpuMs, gpuMs);
        updateClosedLoopResolutionScaling();
        broadcastPerformanceMetrics();
    }
}

void SculptureWorldNavigator::updateClosedLoopResolutionScaling() noexcept {
    if (performanceMonitor_ == nullptr || !autoResolutionScalingEnabled_) {
        return;
    }

    const auto govScale = coordinator_.stagedGovernor().resolutionScale();
    if (coordinator_.variableResolutionStage().resolutionScale() != govScale) {
        coordinator_.setResolutionScale(govScale);
    }
}

void SculptureWorldNavigator::cycleLearnStage() noexcept {
    const auto currentMask = coordinator_.stageMask() & learn_mode::kAllStages;
    if (currentMask == learn_mode::kAllStages) {
        setLearnStageSolo(learn_mode::Stage::Transform);
        return;
    }

    for (std::uint8_t s = 0; s < 6; ++s) {
        const auto stg = static_cast<learn_mode::Stage>(s);
        if (currentMask == learn_mode::bit(stg)) {
            if (s + 1 < 6) {
                setLearnStageSolo(static_cast<learn_mode::Stage>(s + 1));
            } else {
                resetLearnMode();
            }
            return;
        }
    }

    // Custom or multi-stage mask -> cycle back to Stage 1 (Transform)
    setLearnStageSolo(learn_mode::Stage::Transform);
}

void SculptureWorldNavigator::cycleDiagnosticStage() noexcept {
    coordinator_.cycleDiagnosticStage();
}

void SculptureWorldNavigator::resetLearnMode() noexcept {
    coordinator_.resetLearnModeStages();
}

void SculptureWorldNavigator::toggleLearnStage(learn_mode::Stage stage) noexcept {
    coordinator_.toggleStage(stage);
}

void SculptureWorldNavigator::setLearnStageSolo(learn_mode::Stage stage) noexcept {
    coordinator_.setLearnStageSolo(stage);
}

void SculptureWorldNavigator::setLearnStageAccumulative(learn_mode::Stage maxStage) noexcept {
    coordinator_.setLearnStageAccumulative(maxStage);
}

std::uint32_t SculptureWorldNavigator::stageMask() const noexcept {
    return coordinator_.stageMask();
}

bool SculptureWorldNavigator::isLearnModeActive() const noexcept {
    const auto mask = coordinator_.stageMask() & learn_mode::kAllStages;
    return mask != learn_mode::kAllStages || coordinator_.diagnosticStage() != learn_mode::DiagnosticStage::FullPipeline;
}

learn_mode::DiagnosticStage SculptureWorldNavigator::diagnosticStage() const noexcept {
    return coordinator_.diagnosticStage();
}

std::string SculptureWorldNavigator::getLearnModePedagogicalReport() const {
    return coordinator_.getLearnModePedagogicalReport();
}

std::string_view SculptureWorldNavigator::activeStageName(learn_mode::Stage stage) const noexcept {
    return coordinator_.activeStageName(stage);
}

std::string_view SculptureWorldNavigator::activeStageFormula(learn_mode::Stage stage) const noexcept {
    return coordinator_.activeStageFormula(stage);
}

void SculptureWorldNavigator::toggle4DFoliationWave() noexcept {
    coordinator_.toggle4DFoliationWave();
}

void SculptureWorldNavigator::set4DFoliationWaveEnabled(bool enabled) noexcept {
    coordinator_.set4DFoliationWaveEnabled(enabled);
}

bool SculptureWorldNavigator::is4DFoliationWaveEnabled() const noexcept {
    return coordinator_.is4DFoliationWaveEnabled();
}

void SculptureWorldNavigator::cycle4DManifoldType() noexcept {
    coordinator_.cycle4DManifoldType();
    currentParams_.enable4D = coordinator_.currentParams().enable4D;
}

void SculptureWorldNavigator::set4DManifoldType(Manifold4DType type) noexcept {
    coordinator_.set4DManifoldType(type);
    currentParams_.enable4D = static_cast<std::uint32_t>(type) + 1U;
}

Manifold4DType SculptureWorldNavigator::active4DManifoldType() const noexcept {
    return coordinator_.foliationConfig().manifoldType;
}

void SculptureWorldNavigator::cycleRotorSpeed() noexcept {
    auto cfg = coordinator_.foliationConfig();
    const float speed = cfg.rotationLeftSpeed;
    if (std::abs(speed) < 1e-4f) {
        cfg.rotationLeftSpeed = 0.35f;
        cfg.rotationRightSpeed = 0.25f;
    } else if (std::abs(speed - 0.35f) < 0.05f) {
        cfg.rotationLeftSpeed = 0.70f;
        cfg.rotationRightSpeed = 0.50f;
    } else if (std::abs(speed - 0.70f) < 0.05f) {
        cfg.rotationLeftSpeed = 1.40f;
        cfg.rotationRightSpeed = 1.00f;
    } else if (std::abs(speed - 1.40f) < 0.05f) {
        cfg.rotationLeftSpeed = -0.35f;
        cfg.rotationRightSpeed = -0.25f;
    } else {
        cfg.rotationLeftSpeed = 0.0f;
        cfg.rotationRightSpeed = 0.0f;
    }
    coordinator_.setFoliationConfig(cfg);
}

float SculptureWorldNavigator::rotorSpeed() const noexcept {
    return coordinator_.foliationConfig().rotationLeftSpeed;
}

void SculptureWorldNavigator::adjustLatentW(float delta) noexcept {
    if (delta == 0.0f) return;
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    pushHistory("AdjustLatentW");
    currentParams_.latentW = SculptureParameterComposer::clampLatentW(currentParams_.latentW + delta);
    coordinator_.setTargetParams(currentParams_, 0.0f);
    coordinator_.snapToTarget();

    notifyObservers([&](INavigatorObserver& obs) {
        obs.onLatentSpaceMutated(currentParams_.latentW, currentParams_.rotorAlpha, currentParams_.rotorBeta);
    });
}

void SculptureWorldNavigator::setLatentW(float w) noexcept {
    if (std::abs(currentParams_.latentW - w) < 1e-6f) return;
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    pushHistory("SetLatentW");
    currentParams_.latentW = SculptureParameterComposer::clampLatentW(w);
    coordinator_.setTargetParams(currentParams_, 0.0f);
    coordinator_.snapToTarget();

    notifyObservers([&](INavigatorObserver& obs) {
        obs.onLatentSpaceMutated(currentParams_.latentW, currentParams_.rotorAlpha, currentParams_.rotorBeta);
    });
}

float SculptureWorldNavigator::latentW() const noexcept {
    return coordinator_.currentParams().latentW;
}

void SculptureWorldNavigator::adjustRotorAlpha(float delta) noexcept {
    if (delta == 0.0f) return;
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    pushHistory("AdjustRotorAlpha");
    currentParams_.rotorAlpha = SculptureParameterComposer::normalizeRotorAngle(currentParams_.rotorAlpha + delta);
    coordinator_.setTargetParams(currentParams_, 0.0f);
    coordinator_.snapToTarget();

    notifyObservers([&](INavigatorObserver& obs) {
        obs.onLatentSpaceMutated(currentParams_.latentW, currentParams_.rotorAlpha, currentParams_.rotorBeta);
    });
}

void SculptureWorldNavigator::setRotorAlpha(float alpha) noexcept {
    if (std::abs(currentParams_.rotorAlpha - alpha) < 1e-6f) return;
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    pushHistory("SetRotorAlpha");
    currentParams_.rotorAlpha = SculptureParameterComposer::normalizeRotorAngle(alpha);
    coordinator_.setTargetParams(currentParams_, 0.0f);
    coordinator_.snapToTarget();

    notifyObservers([&](INavigatorObserver& obs) {
        obs.onLatentSpaceMutated(currentParams_.latentW, currentParams_.rotorAlpha, currentParams_.rotorBeta);
    });
}

float SculptureWorldNavigator::rotorAlpha() const noexcept {
    return coordinator_.currentParams().rotorAlpha;
}

void SculptureWorldNavigator::adjustRotorBeta(float delta) noexcept {
    if (delta == 0.0f) return;
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    pushHistory("AdjustRotorBeta");
    currentParams_.rotorBeta = SculptureParameterComposer::normalizeRotorAngle(currentParams_.rotorBeta + delta);
    coordinator_.setTargetParams(currentParams_, 0.0f);
    coordinator_.snapToTarget();

    notifyObservers([&](INavigatorObserver& obs) {
        obs.onLatentSpaceMutated(currentParams_.latentW, currentParams_.rotorAlpha, currentParams_.rotorBeta);
    });
}

void SculptureWorldNavigator::setRotorBeta(float beta) noexcept {
    if (std::abs(currentParams_.rotorBeta - beta) < 1e-6f) return;
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    pushHistory("SetRotorBeta");
    currentParams_.rotorBeta = SculptureParameterComposer::normalizeRotorAngle(beta);
    coordinator_.setTargetParams(currentParams_, 0.0f);
    coordinator_.snapToTarget();

    notifyObservers([&](INavigatorObserver& obs) {
        obs.onLatentSpaceMutated(currentParams_.latentW, currentParams_.rotorAlpha, currentParams_.rotorBeta);
    });
}

float SculptureWorldNavigator::rotorBeta() const noexcept {
    return coordinator_.currentParams().rotorBeta;
}

void SculptureWorldNavigator::adjustRotorTheta4D(float delta) noexcept {
    if (delta == 0.0f) return;
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    pushHistory("AdjustRotorTheta4D");
    currentParams_.rotorTheta4D = SculptureParameterComposer::normalizeRotorAngle(currentParams_.rotorTheta4D + delta);
    coordinator_.setTargetParams(currentParams_, 0.0f);
    coordinator_.snapToTarget();

    notifyObservers([&](INavigatorObserver& obs) {
        obs.onLatentSpaceMutated(currentParams_.latentW, currentParams_.rotorAlpha, currentParams_.rotorBeta);
    });
}

void SculptureWorldNavigator::setRotorTheta4D(float theta) noexcept {
    if (std::abs(currentParams_.rotorTheta4D - theta) < 1e-6f) return;
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    pushHistory("SetRotorTheta4D");
    currentParams_.rotorTheta4D = SculptureParameterComposer::normalizeRotorAngle(theta);
    coordinator_.setTargetParams(currentParams_, 0.0f);
    coordinator_.snapToTarget();

    notifyObservers([&](INavigatorObserver& obs) {
        obs.onLatentSpaceMutated(currentParams_.latentW, currentParams_.rotorAlpha, currentParams_.rotorBeta);
    });
}

float SculptureWorldNavigator::rotorTheta4D() const noexcept {
    return coordinator_.currentParams().rotorTheta4D;
}

void SculptureWorldNavigator::reset4DRotors() noexcept {
    if (tourChoreographer_ != nullptr) {
        tourChoreographer_->notifyUserInteraction();
    }
    pushHistory("Reset4DRotors");
    SculptureParameterComposer::reset4DRotors(currentParams_);
    coordinator_.setTargetParams(currentParams_, 0.0f);
    coordinator_.snapToTarget();

    notifyObservers([&](INavigatorObserver& obs) {
        obs.onLatentSpaceMutated(currentParams_.latentW, currentParams_.rotorAlpha, currentParams_.rotorBeta);
    });
}

SculptureRayHit SculptureWorldNavigator::marchVolumetricRay(const SculptureRay& ray) const {
    SculptureRaymarcher raymarcher;
    SculptureRaymarchConfig cfg = raymarcher.config();
    cfg.enableVolumetricFog = true;
    raymarcher.setConfig(cfg);

    const auto p = coordinator_.currentParams();
    SculptureProgram program(p);
    return raymarcher.marchVolumetricRay(ray, program, coordinator_.simulatedTime(), 1.0f, {}, coordinator_.volumetricMemory());
}

void SculptureWorldNavigator::cycleVolumetricResolution() noexcept {
    auto* volMem = coordinator_.volumetricMemory();
    if (volMem == nullptr) return;
    StageResolutionScale nextScale = StageResolutionScale::Half;
    switch (volMem->currentResolutionScale()) {
    case StageResolutionScale::Quarter: nextScale = StageResolutionScale::Half; break;
    case StageResolutionScale::Half: nextScale = StageResolutionScale::Full; break;
    case StageResolutionScale::Full: nextScale = StageResolutionScale::Quarter; break;
    default: nextScale = StageResolutionScale::Half; break;
    }
    volMem->applyResolutionScale(nextScale);
}

void SculptureWorldNavigator::toggleVolumetricFeedback() noexcept {
    auto* volMem = coordinator_.volumetricMemory();
    if (volMem == nullptr) return;
    auto cfg = volMem->config();
    cfg.enableFeedback = !cfg.enableFeedback;
    volMem->configure(cfg);
}

void SculptureWorldNavigator::resetVolumetricMemory() noexcept {
    auto* volMem = coordinator_.volumetricMemory();
    if (volMem == nullptr) return;
    volMem->reset();
}

void SculptureWorldNavigator::toggleVolumetricWrap() noexcept {
    auto* volMem = coordinator_.volumetricMemory();
    if (volMem == nullptr) return;
    auto cfg = volMem->config();
    cfg.enableTorusWrap = !cfg.enableTorusWrap;
    volMem->configure(cfg);
}

void SculptureWorldNavigator::toggleVolumetricBackend() noexcept {
    auto* volMem = coordinator_.volumetricMemory();
    if (volMem == nullptr) return;
    volMem->setGpuEnabled(!volMem->isGpuEnabled());
}

bool SculptureWorldNavigator::isVolumetricGpuActive() const noexcept {
    const auto* volMem = coordinator_.volumetricMemory();
    return volMem != nullptr && volMem->isGpuActive();
}

bool SculptureWorldNavigator::initVolumetricGpu(
    SculptureFrameProducer& producer,
    const std::string& kernelSourceOrPath
) noexcept {
    auto* volMem = coordinator_.volumetricMemory();
    if (volMem == nullptr) return false;
    return producer.initializeVolumetricMemoryGpu(*volMem, kernelSourceOrPath);
}

void SculptureWorldNavigator::toggleVolumetricCoupling() noexcept {
    auto cfg = volumetricCoupler_.config();
    cfg.enableCoupling = !cfg.enableCoupling;
    volumetricCoupler_.configure(cfg);
    renderedGeneration_ = 0U;
}

void SculptureWorldNavigator::toggleVolumetricAdvection() noexcept {
    auto cfg = volumetricCoupler_.config();
    cfg.enableAdvection = !cfg.enableAdvection;
    volumetricCoupler_.configure(cfg);
    renderedGeneration_ = 0U;
}

void SculptureWorldNavigator::toggleAcousticSynesthesia() noexcept {
    auto* volMem = coordinator_.volumetricMemory();
    if (volMem != nullptr) {
        auto cfg = volMem->config();
        cfg.enableAcousticCoupling = !cfg.enableAcousticCoupling;
        volMem->configure(cfg);
    }
    if (audioDirector_ != nullptr) {
        audioDirector_->setVolumetricCouplingEnabled(!audioDirector_->isVolumetricCouplingEnabled());
    }
}

void SculptureWorldNavigator::setAcousticSynesthesiaEnabled(bool enabled) noexcept {
    auto* volMem = coordinator_.volumetricMemory();
    if (volMem != nullptr) {
        auto cfg = volMem->config();
        cfg.enableAcousticCoupling = enabled;
        volMem->configure(cfg);
    }
    if (audioDirector_ != nullptr) {
        audioDirector_->setVolumetricCouplingEnabled(enabled);
    }
}

bool SculptureWorldNavigator::isAcousticSynesthesiaEnabled() const noexcept {
    auto* volMem = coordinator_.volumetricMemory();
    if (volMem != nullptr) {
        return volMem->config().enableAcousticCoupling;
    }
    return audioDirector_ != nullptr ? audioDirector_->isVolumetricCouplingEnabled() : false;
}

void SculptureWorldNavigator::togglePaletteAgent() noexcept {
    coordinator_.togglePaletteAgent();
}

void SculptureWorldNavigator::setPaletteAgentEnabled(bool enabled) noexcept {
    coordinator_.setPaletteAgentEnabled(enabled);
}

bool SculptureWorldNavigator::isPaletteAgentEnabled() const noexcept {
    return coordinator_.isPaletteAgentEnabled();
}

void SculptureWorldNavigator::cyclePaletteAgentMode() noexcept {
    const auto current = static_cast<std::uint8_t>(coordinator_.paletteAgent().mode());
    const auto next = static_cast<PaletteAgentMode>((current + 1U) % 4U);
    coordinator_.paletteAgent().setMode(next);
}

PaletteAgentMode SculptureWorldNavigator::paletteAgentMode() const noexcept {
    return coordinator_.paletteAgent().mode();
}

void SculptureWorldNavigator::stimulatePaletteAgent(float intensity) {
    coordinator_.paletteAgent().stimulate(intensity);
}

SculptureGBuffer SculptureWorldNavigator::renderImplicitGBuffer(
    std::uint32_t width,
    std::uint32_t height,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    std::uint32_t threadCount,
    const SculptureVec3* prevCameraPos,
    const SculptureVec3* prevCameraTarget
) const {
    return coordinator_.renderActiveImplicitGBuffer(
        width, height, cameraPos, cameraTarget, cameraUp, fovRadians,
        threadCount, prevCameraPos, prevCameraTarget);
}

SculptureStereoGBufferPair SculptureWorldNavigator::renderStereoImplicitGBuffer(
    std::uint32_t eyeWidth,
    std::uint32_t eyeHeight,
    const SculptureStereoRig& rig,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    std::uint32_t threadCount,
    const SculptureVec3* prevCameraPos,
    const SculptureVec3* prevCameraTarget
) const {
    return coordinator_.renderActiveStereoImplicitGBuffer(
        eyeWidth, eyeHeight, rig, cameraPos, cameraTarget, cameraUp,
        fovRadians, threadCount, prevCameraPos, prevCameraTarget);
}

void SculptureWorldNavigator::setPresentationMode(WorldPresentationMode mode) noexcept {
    presentationMode_ = mode;
    if (presentationMode_ != WorldPresentationMode::ReliefManifold) {
        renderedGeneration_ = 0U;
    }
}

void SculptureWorldNavigator::cyclePresentationMode() noexcept {
    const auto current = static_cast<std::uint8_t>(presentationMode_);
    setPresentationMode(static_cast<WorldPresentationMode>((current + 1U) % 3U));
}

void SculptureWorldNavigator::setReliefProjection(ReliefManifoldProjection projection) noexcept {
    reliefConfig_.projection = projection;
}

void SculptureWorldNavigator::cycleReliefProjection() noexcept {
    const auto current = static_cast<std::uint8_t>(reliefConfig_.projection);
    setReliefProjection(static_cast<ReliefManifoldProjection>((current + 1U) % kReliefManifoldProjectionCount));
}

void SculptureWorldNavigator::setReliefConfig(const EpimorphicReliefConfig& config) noexcept {
    reliefConfig_ = config;
}

void SculptureWorldNavigator::rebuildReliefMesh() {
    reliefMeshRebuildPending_ = true;
    rebuildReliefMesh(coordinator_.describeEpimorphicRelief(reliefConfig_));
}

void SculptureWorldNavigator::rebuildReliefMesh(
    const ReliefMeshRequest& request, mgfx::Drawable* draw) {
    auto candidate = SculptureHybridManifoldSynthesizer::prepareReliefMesh(
        engine_, coordinator_, request);
    if (!candidate) {
        return;
    }
    if (draw && candidate->mesh && !draw->prepareMesh(candidate->mesh)) {
        return;
    }

    // Owner-thread commit: everything below is nonthrowing. Failed preparation
    // or frame admission leaves the previous publication available for selection.
    reliefMesh_ = std::move(candidate->mesh);
    currentReliefData_ = std::move(candidate->data);
    publishedReliefRequest_ = candidate->request;
    reliefMeshRebuildPending_ = false;
}

void SculptureWorldNavigator::formatHud(char* buffer, std::size_t size) const noexcept {
    formatHud(buffer, size, worldBridge_.generation(), worldBridge_.instanceCount());
}

std::size_t SculptureWorldNavigator::renderedInstanceCount() const noexcept {
    return worldBridge_.instanceCount();
}

void SculptureWorldNavigator::setExternalTransport(const SculptureWorldExternalTransport* transport) noexcept {
    externalTransport_ = transport;
    worldBridge_.setExternalTransport(transport);
}

const WorldSnapshotDelivery& SculptureWorldNavigator::prepareInstanceDelivery(
    const WorldSnapshotDelivery& delivery
) {
    return SculptureHybridManifoldSynthesizer::prepareInstanceDelivery(
        delivery,
        coordinator_.volumetricMemory(),
        volumetricCoupler_,
        coupledSnapshot_,
        coupledDelivery_);
}

WorldSnapshotDelivery SculptureWorldNavigator::synthesizePolychoronDelivery() const {
    return SculptureHybridManifoldSynthesizer::synthesizePolychoronDelivery(
        polychoronHomotopy_,
        currentRecipe_,
        coordinator_.proceduralGeometrySample().time,
        coordinator_.telemetry().totalFramesCoordinated,
        coupledSnapshot_,
        polychoronMetrics_,
        coupledDelivery_);
}

PolychoronMeshRequest SculptureWorldNavigator::describePolychoronMesh() const noexcept {
    return {polychoronHomotopy_.config(),
        coordinator_.proceduralGeometrySample().time, currentParams_.worldScale};
}

void SculptureWorldNavigator::rebuildPolychoronMesh() {
    polychoronMeshRebuildPending_ = true;
    rebuildPolychoronMesh(describePolychoronMesh());
}

void SculptureWorldNavigator::rebuildPolychoronMesh(
    const PolychoronMeshRequest& request, mgfx::Drawable* draw) {
    auto candidate = SculptureHybridManifoldSynthesizer::preparePolychoronMesh(
        engine_, request);
    if (!candidate) {
        return;
    }
    if (draw && candidate->mesh && !draw->prepareMesh(candidate->mesh)) {
        return;
    }

    polychoronMesh_ = std::move(candidate->mesh);
    publishedPolychoronRequest_ = candidate->request;
    polychoronMeshRebuildPending_ = false;
}

void SculptureWorldNavigator::render(
    mgfx::Drawable& draw,
    const WorldSnapshotDelivery& delivery,
    const mgfx::math::mat4& rootTransform
) {
    // 1. Procedural point-cloud primitive instanced draw (PointCloud or Hybrid)
    if (presentationMode_ == WorldPresentationMode::PointCloud ||
        presentationMode_ == WorldPresentationMode::HybridVolumetric) {
        if (primitiveMeshes_) {
            const WorldSnapshotDelivery effectiveDelivery = (!delivery && currentRecipe_.uses4DProjection)
                ? synthesizePolychoronDelivery()
                : delivery;

            if (effectiveDelivery && effectiveDelivery.generation != renderedGeneration_) {
                const auto& resolvedDelivery = prepareInstanceDelivery(effectiveDelivery);
                worldBridge_.rebuild(resolvedDelivery);
                renderedGeneration_ = worldBridge_.generation();
            }
            if (const auto policy = SculptureWorldRenderBridge::makeViewFamilyLodPushConstants(
                    draw.frameViews(), rootTransform)) {
                worldBridge_.setCompactionPushConstants(*policy);
                static_cast<void>(worldBridge_.draw(
                    draw, primitiveMeshes_->registry, bands_, rootTransform,
                    effectiveDelivery ? effectiveDelivery.generation : 0U));
            }
        }
    }

    // 2. Continuous 2.5D / 3.5D Epimorphic Relief Manifold draw (ReliefManifold or Hybrid)
    if (presentationMode_ == WorldPresentationMode::ReliefManifold ||
        presentationMode_ == WorldPresentationMode::HybridVolumetric) {
        if (!bands_.empty()) {
            const auto reliefRequest = coordinator_.describeEpimorphicRelief(reliefConfig_);
            if (reliefMeshRebuildPending_ || publishedReliefRequest_ != reliefRequest) {
                rebuildReliefMesh(reliefRequest, &draw);
            }
            if (reliefMesh_) {
                static_cast<void>(draw.tryDrawMesh(reliefMesh_, bands_[0], rootTransform,
                    mgfx::math::vec4(1.0f), mgfx::pbr::RenderMode::Full, 0U, true /* doubleSided */));
            }
        }

        // Watertight 4D Polychoron Homotopy tube mesh when 4D projection is active
        if (currentRecipe_.uses4DProjection && bands_.size() > 1U) {
            const auto polychoronRequest = describePolychoronMesh();
            if (polychoronMeshRebuildPending_ || publishedPolychoronRequest_ != polychoronRequest) {
                rebuildPolychoronMesh(polychoronRequest, &draw);
            }
            if (polychoronMesh_) {
                static_cast<void>(draw.tryDrawMesh(polychoronMesh_, bands_[1], rootTransform,
                    mgfx::math::vec4(1.0f), mgfx::pbr::RenderMode::Full, 0U, true /* doubleSided */));
            }
        }
    }

    // 3. Dynamic volumetric mist and fluid substrate render pass
    if (atmosphereManager_ != nullptr && atmosphereManager_->isEnabled()) {
        atmosphereManager_->render(draw, rootTransform);
    }
}

void SculptureWorldNavigator::render(
    mgfx::Drawable& draw,
    const mgfx::math::mat4& rootTransform
) {
    if (!latestDelivery_ && currentRecipe_.uses4DProjection) {
        render(draw, synthesizePolychoronDelivery(), rootTransform);
    } else {
        render(draw, latestDelivery_, rootTransform);
    }
}

mgfx::math::mat4 SculptureWorldNavigator::computePresentationTransform(
    float simulatedTime,
    float rotationSpeed,
    const mgfx::math::vec3& translation,
    float scale
) const noexcept {
    mgfx::math::mat4 root = glm::translate(mgfx::math::mat4(1.0f), translation);
    if (rotationSpeed != 0.0f) {
        root = glm::rotate(root, simulatedTime * rotationSpeed, mgfx::math::vec3(0.0f, 1.0f, 0.0f));
    }
    if (scale != 1.0f) {
        root = glm::scale(root, mgfx::math::vec3(scale));
    }
    return root;
}

mgfx::math::mat4 SculptureWorldNavigator::computePresentationTransform() const noexcept {
    return computePresentationTransform(coordinator_.simulatedTime());
}

SculptureDynamicLighting SculptureWorldNavigator::computeDynamicLighting(
    float simulatedTime,
    const AudioResonanceMetrics& metrics,
    const mgfx::math::vec3& lightDir
) const noexcept {
    (void)simulatedTime;
    SculptureDynamicLighting lighting{};
    lighting.atmosphere.worldRadiance = 0.0f;
    lighting.atmosphere.horizonTint = mgfx::math::vec3(0.0f);
    lighting.atmosphere.zenithTint = mgfx::math::vec3(0.0f);
    lighting.atmosphere.groundTint = mgfx::math::vec3(0.0f);

    lighting.lights.slots[0].kind = mgfx::pbr::LightKind::Directional;
    lighting.lights.slots[0].positionOrDirection = glm::normalize(lightDir);
    lighting.lights.slots[0].radiance = 4.0f + metrics.acousticEnergy * 6.0f;
    lighting.lights.slots[0].color = mgfx::math::vec3(
        1.0f,
        0.9f - metrics.kuramotoOrderR * 0.3f,
        0.7f + (metrics.spectralCentroidHz / 3000.0f) * 0.3f
    );
    return lighting;
}

void SculptureWorldNavigator::applyDynamicLighting(
    mgfx::Engine& engine,
    float simulatedTime,
    const AudioResonanceMetrics& metrics,
    const mgfx::math::vec3& lightDir
) const noexcept {
    const auto lighting = computeDynamicLighting(simulatedTime, metrics, lightDir);
    engine.setAtmosphere(lighting.atmosphere);
    engine.setLights(lighting.lights);
}

// -----------------------------------------------------------------------------
// Educational Annotation: UNIFIED COMMAND CENTER ACTION DISPATCHER
// -----------------------------------------------------------------------------
// The Command Center translates symbolic intent (NavigatorAction) into concrete
// state transductions.
//
// In mathematical terms, the interactive experience is a trajectory on a high-
// dimensional manifold:
//   X(t) = { WorldRecipe, \vec{\theta}_{4D}, SO(4)_{Rotor}, PBR_{Palette}, Camera }
//
// By channeling all discrete and continuous inputs through dispatchAction:
//  - Geometric changes preserve topological continuity (smooth transitions).
//  - 4D foliation scrubbers update the latent embedding hyperplane W.
//  - Learn Mode decomposition isolates specific stages of the 6-stage pipeline.
//  - Transport controls scale simulation time \tau = \int \omega(t) dt.
// -----------------------------------------------------------------------------
bool SculptureWorldNavigator::dispatchAction(NavigatorAction action, float value) noexcept {
    return SculptureWorldActionDispatcher::dispatchAction(*this, action, value);
}

void SculptureWorldNavigator::enqueueAction(NavigatorAction action, float value) {
    queuedActions_.push_back(ActionCommand{action, value});
}

void SculptureWorldNavigator::enqueueAction(const ActionCommand& cmd) {
    queuedActions_.push_back(cmd);
}

std::size_t SculptureWorldNavigator::drainActionQueue() noexcept {
    if (queuedActions_.empty()) return 0U;
    const std::size_t count = queuedActions_.size();
    for (const auto& cmd : queuedActions_) {
        dispatchAction(cmd.action, cmd.value);
    }
    queuedActions_.clear();
    return count;
}

std::size_t SculptureWorldNavigator::queuedActionCount() const noexcept {
    return queuedActions_.size();
}

void SculptureWorldNavigator::clearActionQueue() noexcept {
    queuedActions_.clear();
}

// -----------------------------------------------------------------------------
// Deterministic Macro Timeline Choreography
// -----------------------------------------------------------------------------
bool SculptureWorldNavigator::loadTimelineScript(std::string_view jsonOrFilePath) {
    if (timelineDirector_ == nullptr) {
        timelineDirector_ = std::make_unique<SculptureTimelineDirector>();
    }

    bool ok = false;
    // Fast check: if it starts with '{', treat as raw JSON
    const auto trimmedStart = jsonOrFilePath.find_first_not_of(" \t\r\n");
    if (trimmedStart != std::string_view::npos && jsonOrFilePath[trimmedStart] == '{') {
        ok = timelineDirector_->loadScriptFromJson(jsonOrFilePath);
    } else {
        std::error_code ec{};
        if (jsonOrFilePath.size() < 4096 && std::filesystem::exists(jsonOrFilePath, ec) && !ec) {
            ok = timelineDirector_->loadScriptFromFile(jsonOrFilePath);
        } else {
            ok = timelineDirector_->loadScriptFromJson(jsonOrFilePath);
        }
    }

    if (ok) {
        notifyObservers([&](INavigatorObserver& obs) {
            obs.onTimelineLoaded(timelineDirector_->script().title,
                                 timelineDirector_->script().durationSeconds,
                                 timelineDirector_->script().actionCues.size());
        });
    }
    return ok;
}

void SculptureWorldNavigator::playTimeline() noexcept {
    if (timelineDirector_ != nullptr) {
        timelineDirector_->play();
        notifyTimelinePlaybackChanged(timelineDirector_->isPlaying(), timelineDirector_->isPaused(),
                                      timelineDirector_->currentTime(), timelineDirector_->totalDuration());
    }
}

void SculptureWorldNavigator::pauseTimeline() noexcept {
    if (timelineDirector_ != nullptr) {
        timelineDirector_->pause();
        notifyTimelinePlaybackChanged(timelineDirector_->isPlaying(), timelineDirector_->isPaused(),
                                      timelineDirector_->currentTime(), timelineDirector_->totalDuration());
    }
}

void SculptureWorldNavigator::resumeTimeline() noexcept {
    if (timelineDirector_ != nullptr) {
        timelineDirector_->resume();
        notifyTimelinePlaybackChanged(timelineDirector_->isPlaying(), timelineDirector_->isPaused(),
                                      timelineDirector_->currentTime(), timelineDirector_->totalDuration());
    }
}

void SculptureWorldNavigator::stopTimeline() noexcept {
    if (timelineDirector_ != nullptr) {
        timelineDirector_->stop();
        notifyTimelinePlaybackChanged(timelineDirector_->isPlaying(), timelineDirector_->isPaused(),
                                      timelineDirector_->currentTime(), timelineDirector_->totalDuration());
    }
}

void SculptureWorldNavigator::toggleTimeline() noexcept {
    if (timelineDirector_ != nullptr) {
        timelineDirector_->togglePlayPause();
        notifyTimelinePlaybackChanged(timelineDirector_->isPlaying(), timelineDirector_->isPaused(),
                                      timelineDirector_->currentTime(), timelineDirector_->totalDuration());
    }
}

bool SculptureWorldNavigator::isTimelinePlaying() const noexcept {
    return timelineDirector_ != nullptr && timelineDirector_->isPlaying();
}

float SculptureWorldNavigator::timelineProgress() const noexcept {
    return timelineDirector_ != nullptr ? timelineDirector_->status().progress : 0.0f;
}

bool SculptureWorldNavigator::generateBeatSyncedTimeline(float bpm, std::uint32_t barsPerWorld) {
    if (timelineDirector_ == nullptr) {
        timelineDirector_ = std::make_unique<SculptureTimelineDirector>();
    }

    float targetBpm = bpm;
    if (targetBpm <= 0.0f && audioDirector_ != nullptr) {
        targetBpm = audioDirector_->bpm();
    }
    if (targetBpm <= 0.0f) {
        targetBpm = 120.0f;
    }

    std::vector<std::string> names;
    names.reserve(availableWorlds_.size());
    for (const auto* entry : availableWorlds_) {
        if (entry != nullptr) {
            names.emplace_back(entry->title);
        }
    }

    auto script = SculptureTimelineDirector::generateBeatSynchronizedScript(
        names, targetBpm, barsPerWorld, true
    );
    timelineDirector_->loadScript(std::move(script));
    timelineDirector_->play();

    notifyTimelinePlaybackChanged(timelineDirector_->isPlaying(), timelineDirector_->isPaused(),
                                  timelineDirector_->currentTime(), timelineDirector_->totalDuration());

    std::printf("[SculptureWorldNavigator] Generated and started beat-synced timeline (BPM: %.1f, bars: %u, worlds: %zu)\n",
                targetBpm, barsPerWorld, names.size());
    return true;
}

bool SculptureWorldNavigator::exportActiveTimeline(const std::string& filepath) const {
    if (timelineDirector_ == nullptr) {
        std::fprintf(stderr, "[SculptureWorldNavigator] Warning: No active timeline director to export.\n");
        return false;
    }

    std::string outPath = filepath;
    if (outPath.empty()) {
        outPath = "snapshots/timeline_" + currentPresetName_ + ".timeline.json";
    }

    const bool ok = timelineDirector_->saveScriptToFile(outPath);
    if (ok) {
        std::printf("[SculptureWorldNavigator] Successfully exported timeline script to '%s'\n", outPath.c_str());
    } else {
        std::fprintf(stderr, "[SculptureWorldNavigator] Failed to export timeline script to '%s'\n", outPath.c_str());
    }
    return ok;
}

// -----------------------------------------------------------------------------
// Reactive Observer & Decoupled EventBus System
// -----------------------------------------------------------------------------
void SculptureWorldNavigator::addObserver(std::shared_ptr<INavigatorObserver> observer) {
    if (observer) {
        sharedObservers_.push_back(std::move(observer));
    }
}

void SculptureWorldNavigator::addObserver(INavigatorObserver* rawObserver) {
    if (rawObserver != nullptr) {
        if (std::find(rawObservers_.begin(), rawObservers_.end(), rawObserver) == rawObservers_.end()) {
            rawObservers_.push_back(rawObserver);
        }
    }
}

void SculptureWorldNavigator::removeObserver(INavigatorObserver* rawObserver) {
    if (rawObserver != nullptr) {
        rawObservers_.erase(
            std::remove(rawObservers_.begin(), rawObservers_.end(), rawObserver),
            rawObservers_.end()
        );
        sharedObservers_.erase(
            std::remove_if(sharedObservers_.begin(), sharedObservers_.end(),
                [rawObserver](const std::shared_ptr<INavigatorObserver>& ptr) {
                    return ptr.get() == rawObserver;
                }),
            sharedObservers_.end()
        );
    }
}

std::size_t SculptureWorldNavigator::observerCount() const noexcept {
    return sharedObservers_.size() + rawObservers_.size();
}

void SculptureWorldNavigator::clearObservers() noexcept {
    sharedObservers_.clear();
    rawObservers_.clear();
}

void SculptureWorldNavigator::notifyTimelineCueFired(const TimelineActionCue& cue) {
    notifyObservers([&](INavigatorObserver& obs) {
        obs.onTimelineCueFired(cue, cue.timestampSeconds);
    });
}

void SculptureWorldNavigator::notifyTimelinePlaybackChanged(bool isPlaying, bool isPaused, float currentTime, float duration) {
    notifyObservers([&](INavigatorObserver& obs) {
        obs.onTimelinePlaybackChanged(isPlaying, isPaused, currentTime, duration);
    });
}

void SculptureWorldNavigator::broadcastPerformanceMetrics() {
    if (performanceMonitor_ != nullptr) {
        const auto& telem = performanceMonitor_->telemetry();
        notifyExternalObservers([&telem](INavigatorObserver& obs) {
            obs.onPerformanceMetricsUpdated(telem);
        });
    }
}

void SculptureWorldNavigator::bindEventBus(SculptureEventBus& eventBus) {
    SculptureWorldActionDispatcher::bindEventBus(*this, eventBus);
}

// -----------------------------------------------------------------------------
// Real-Time Performance Monitor & Adaptive Frame-Pacing Facade
// -----------------------------------------------------------------------------
void SculptureWorldNavigator::togglePerformanceMonitor() noexcept {
    if (performanceMonitor_ != nullptr) {
        performanceMonitor_->setEnabled(!performanceMonitor_->isEnabled());
    }
}

void SculptureWorldNavigator::cycleTargetFps() noexcept {
    if (performanceMonitor_ != nullptr) {
        performanceMonitor_->cycleTargetTier();
    }
}

void SculptureWorldNavigator::setTargetFpsTier(TargetFrameRateTier tier) noexcept {
    if (performanceMonitor_ != nullptr) {
        performanceMonitor_->setTargetTier(tier);
    }
}

TargetFrameRateTier SculptureWorldNavigator::targetFpsTier() const noexcept {
    return (performanceMonitor_ != nullptr) ? performanceMonitor_->targetTier() : TargetFrameRateTier::Fps60;
}

float SculptureWorldNavigator::targetFps() const noexcept {
    return (performanceMonitor_ != nullptr) ? performanceMonitor_->targetFps() : 60.0f;
}

// -----------------------------------------------------------------------------
// VJ Live Performance Deck & Master Crossfader Facade
// -----------------------------------------------------------------------------
void SculptureWorldNavigator::setPerformanceDeckEnabled(bool enabled) noexcept {
    coordinator_.setPerformanceDeckEnabled(enabled);
    if (enabled && coordinator_.performanceDeck()) {
        auto* deck = coordinator_.performanceDeck();
        if (deck->deck(DeckId::DeckA).presetId.empty()) {
            deck->loadDeck(DeckId::DeckA, currentParams_, activePresetName(), std::string(activeEntry().title));
        }
        if (deck->deck(DeckId::DeckB).presetId.empty()) {
            deck->loadDeck(DeckId::DeckB, currentParams_, activePresetName(), std::string(activeEntry().title));
        }
    }
}

bool SculptureWorldNavigator::isPerformanceDeckEnabled() const noexcept {
    return coordinator_.isPerformanceDeckEnabled();
}

void SculptureWorldNavigator::togglePerformanceDeck() noexcept {
    setPerformanceDeckEnabled(!isPerformanceDeckEnabled());
}

void SculptureWorldNavigator::setCrossfadePosition(float pos) noexcept {
    if (!coordinator_.isPerformanceDeckEnabled()) {
        setPerformanceDeckEnabled(true);
    }
    if (auto* deck = coordinator_.performanceDeck()) {
        deck->setCrossfadePosition(pos);
    }
}

float SculptureWorldNavigator::crossfadePosition() const noexcept {
    if (const auto* deck = coordinator_.performanceDeck()) {
        return deck->crossfadePosition();
    }
    return 0.0f;
}

void SculptureWorldNavigator::nudgeCrossfade(float delta) noexcept {
    if (auto* deck = coordinator_.performanceDeck()) {
        deck->nudgeCrossfade(delta);
    }
}

void SculptureWorldNavigator::triggerAutoCrossfade(float durationSeconds, std::optional<DeckId> targetDeck) noexcept {
    if (auto* deck = coordinator_.performanceDeck()) {
        deck->triggerAutoCrossfade(durationSeconds, targetDeck);
    }
}

void SculptureWorldNavigator::cancelAutoCrossfade() noexcept {
    if (auto* deck = coordinator_.performanceDeck()) {
        deck->cancelAutoCrossfade();
    }
}

bool SculptureWorldNavigator::isAutoCrossfading() const noexcept {
    if (const auto* deck = coordinator_.performanceDeck()) {
        return deck->isAutoCrossfading();
    }
    return false;
}

float SculptureWorldNavigator::autoCrossfadeProgress() const noexcept {
    if (const auto* deck = coordinator_.performanceDeck()) {
        return deck->autoCrossfadeProgress();
    }
    return 0.0f;
}

void SculptureWorldNavigator::swapPerformanceDecks() noexcept {
    if (auto* deck = coordinator_.performanceDeck()) {
        deck->swapDecks();
    }
}

void SculptureWorldNavigator::loadActiveToDeck(DeckId deckId) {
    if (auto* deck = coordinator_.performanceDeck()) {
        deck->loadDeck(deckId, currentParams_, activePresetName(), std::string(activeEntry().title));
    }
}

void SculptureWorldNavigator::loadWorldToDeck(DeckId deckId, std::size_t worldIndex) {
    if (worldIndex >= availableWorlds_.size()) return;
    if (auto* deck = coordinator_.performanceDeck()) {
        const auto* entry = availableWorlds_[worldIndex];
        if (!entry) return;
        const std::string presetName = std::string(entry->canonicalPresetId);
        const SculpturePresetAsset* asset = nullptr;
        if (presetLibrary_ != nullptr) {
            asset = presetLibrary_->find(presetName);
        }
        SculptureProgramParams params{};
        if (asset != nullptr) {
            params = asset->params;
        } else {
            params = resolveRootPatternWorldParams(presetName);
        }
        deck->loadDeck(deckId, params, presetName, std::string(entry->title));
    }
}

void SculptureWorldNavigator::loadPresetToDeck(DeckId deckId, std::string_view presetId) {
    if (!presetLibrary_) return;
    const auto* asset = presetLibrary_->find(presetId);
    if (!asset) return;
    if (auto* deck = coordinator_.performanceDeck()) {
        deck->loadDeck(deckId, *asset);
    }
}

SculpturePerformanceDeck* SculptureWorldNavigator::performanceDeck() noexcept {
    return coordinator_.performanceDeck();
}

const SculpturePerformanceDeck* SculptureWorldNavigator::performanceDeck() const noexcept {
    return coordinator_.performanceDeck();
}

// -----------------------------------------------------------------------------
// Multidimensional Morph Space (Vector Synthesis Pad)
// -----------------------------------------------------------------------------
void SculptureWorldNavigator::toggleMorphSpace() noexcept {
    if (morphSpace_ != nullptr) {
        morphSpace_->setActive(!morphSpace_->isActive());
        if (morphSpace_->isActive()) {
            currentParams_ = morphSpace_->synthesizeParams();
            coordinator_.setTargetParams(currentParams_, 0.0f);
            coordinator_.snapToTarget();
        }
    }
}

bool SculptureWorldNavigator::isMorphSpaceActive() const noexcept {
    return morphSpace_ != nullptr && morphSpace_->isActive();
}

void SculptureWorldNavigator::setMorphSpaceCursor(float x, float y) noexcept {
    if (morphSpace_ != nullptr) {
        morphSpace_->setCursor(x, y);
        if (morphSpace_->isActive()) {
            currentParams_ = morphSpace_->synthesizeParams();
            coordinator_.setTargetParams(currentParams_, 0.0f);
            coordinator_.snapToTarget();
        }
    }
}

void SculptureWorldNavigator::nudgeMorphSpaceCursor(float dx, float dy) noexcept {
    if (morphSpace_ != nullptr) {
        morphSpace_->nudgeCursor(dx, dy);
        if (morphSpace_->isActive()) {
            currentParams_ = morphSpace_->synthesizeParams();
            coordinator_.setTargetParams(currentParams_, 0.0f);
            coordinator_.snapToTarget();
        }
    }
}

void SculptureWorldNavigator::cycleMorphSpaceMotion() noexcept {
    if (morphSpace_ != nullptr) {
        morphSpace_->cycleMotionMode();
    }
}

void SculptureWorldNavigator::cycleMorphSpaceBank() noexcept {
    if (morphSpace_ != nullptr) {
        morphSpace_->cycleBank();
        if (morphSpace_->isActive()) {
            currentParams_ = morphSpace_->synthesizeParams();
            coordinator_.setTargetParams(currentParams_, 0.0f);
            coordinator_.snapToTarget();
        }
        notifyObservers([&](INavigatorObserver& obs) {
            obs.onMorphSpaceUpdated(morphSpace_->cursorX(), morphSpace_->cursorY(),
                                   morphSpace_->motionModeName());
        });
    }
}

void SculptureWorldNavigator::loadMorphSpaceBank(std::size_t index) noexcept {
    if (morphSpace_ != nullptr) {
        morphSpace_->loadBank(index);
        if (morphSpace_->isActive()) {
            currentParams_ = morphSpace_->synthesizeParams();
            coordinator_.setTargetParams(currentParams_, 0.0f);
            coordinator_.snapToTarget();
        }
        notifyObservers([&](INavigatorObserver& obs) {
            obs.onMorphSpaceUpdated(morphSpace_->cursorX(), morphSpace_->cursorY(),
                                   morphSpace_->motionModeName());
        });
    }
}

void SculptureWorldNavigator::loadMorphSpaceBank(std::string_view name) noexcept {
    if (morphSpace_ != nullptr) {
        morphSpace_->loadBank(name);
        if (morphSpace_->isActive()) {
            currentParams_ = morphSpace_->synthesizeParams();
            coordinator_.setTargetParams(currentParams_, 0.0f);
            coordinator_.snapToTarget();
        }
        notifyObservers([&](INavigatorObserver& obs) {
            obs.onMorphSpaceUpdated(morphSpace_->cursorX(), morphSpace_->cursorY(),
                                   morphSpace_->motionModeName());
        });
    }
}

std::string_view SculptureWorldNavigator::activeMorphSpaceBankName() const noexcept {
    if (morphSpace_ != nullptr) {
        return morphSpace_->activeBankName();
    }
    return "default";
}

std::string_view SculptureWorldNavigator::activeMorphSpaceBankTitle() const noexcept {
    if (morphSpace_ != nullptr) {
        return morphSpace_->activeBankTitle();
    }
    return "Default Quad";
}

void SculptureWorldNavigator::toggleMorphSpacePath() noexcept {
    if (morphSpace_ != nullptr) {
        morphSpace_->togglePathPlayback();
    }
}

bool SculptureWorldNavigator::isMorphSpacePathPlaying() const noexcept {
    return morphSpace_ != nullptr && morphSpace_->motionMode() == MorphSpaceMotion::Path;
}

void SculptureWorldNavigator::populateMorphSpaceFromPresetLibrary() noexcept {
    if (!morphSpace_ || !presetLibrary_) return;

    // 1. Synthesize dedicated quadrant banks for all categories discovered in preset library
    synthesizeCategoryMorphBanks();

    // 2. Corner references are stable IDs, not interactive titles or aliases.
    morphSpace_->refreshBankCornerParams([this](std::string_view id) -> const SculptureProgramParams* {
        const auto* asset = presetLibrary_->resolveExactId(id).asset;
        return asset ? &asset->params : nullptr;
    });

    // 3. Automatically synchronize morph space to active world's category
    syncMorphSpaceToActiveCategory();
}

bool SculptureWorldNavigator::bindMorphSpaceToCategory(std::string_view category, bool autoLoad) noexcept {
    if (!morphSpace_ || !presetLibrary_) return false;

    auto assets = presetLibrary_->findByCategory(category);
    std::string matchedCategory(category);
    if (assets.empty()) {
        for (const auto& cat : presetLibrary_->categories()) {
            if (cat.size() == category.size() &&
                std::equal(cat.begin(), cat.end(), category.begin(), category.end(),
                           [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); })) {
                assets = presetLibrary_->findByCategory(cat);
                matchedCategory = cat;
                break;
            }
        }
    }
    if (assets.empty()) return false;

    // Select 4 contrasting presets for SW, SE, NW, NE corners
    std::array<std::string, 4> cornerPresetNames;
    std::array<SculptureProgramParams, 4> cornerParams;
    const std::size_t n = assets.size();

    std::size_t idxSW = 0;
    std::size_t idxSE = (n > 1) ? std::min(n - 1, std::max<std::size_t>(1, n / 3)) : 0;
    std::size_t idxNW = (n > 2) ? std::min(n - 1, std::max<std::size_t>(2, (2 * n) / 3)) : (n > 1 ? 1 : 0);
    std::size_t idxNE = (n > 3) ? (n - 1) : (n > 2 ? 2 : (n > 1 ? 1 : 0));

    const std::array<std::size_t, 4> chosenIndices = {idxSW, idxSE, idxNW, idxNE};
    for (std::size_t c = 0; c < 4; ++c) {
        const auto* asset = assets[chosenIndices[c]];
        cornerPresetNames[c] = asset->id;
        cornerParams[c] = asset->params;
    }

    const std::string bankId = "category_" + matchedCategory;
    std::string bankTitle = matchedCategory;
    if (!bankTitle.empty()) {
        bankTitle[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(bankTitle[0])));
    }
    bankTitle += " Quad";

    return morphSpace_->tryUpsertCategoryBank(
        bankId, bankTitle, cornerPresetNames, cornerParams, autoLoad).has_value();
}

bool SculptureWorldNavigator::syncMorphSpaceToActiveCategory() noexcept {
    if (!presetLibrary_) return false;
    std::string category;
    if (const auto* asset = presetLibrary_->find(currentPresetName_)) {
        category = asset->category;
    }
    if (category.empty() && activeIndex_ < availableWorlds_.size()) {
        // Categories belong to authored assets; the world catalog owns route
        // identity and budgets. Resolve through that identity rather than
        // introducing a second category field that could drift.
        if (const auto* asset = presetLibrary_->find(
                availableWorlds_[activeIndex_]->canonicalPresetId)) {
            category = asset->category;
        }
    }
    if (category.empty()) {
        category = activeTourCategory_;
    }
    if (category.empty()) {
        category = "dimensional";
    }
    return bindMorphSpaceToCategory(category, /*autoLoad=*/true);
}

std::size_t SculptureWorldNavigator::synthesizeCategoryMorphBanks() noexcept {
    if (!morphSpace_ || !presetLibrary_) return 0U;
    const auto cats = presetLibrary_->categories();
    std::size_t count = 0U;
    for (const auto& cat : cats) {
        if (bindMorphSpaceToCategory(cat, /*autoLoad=*/false)) {
            ++count;
        }
    }
    return count;
}

void SculptureWorldNavigator::setPolychoronHomotopy(PolychoronKind source, PolychoronKind target, float progress) noexcept {
    auto cfg = polychoronHomotopy_.config();
    cfg.sourcePolytope = source;
    cfg.targetPolytope = target;
    cfg.morphProgress = std::clamp(progress, 0.0f, 1.0f);
    cfg.rotorAlpha = rotorAlpha();
    cfg.rotorBeta = rotorBeta();
    cfg.rotorTheta4D = rotorTheta4D();
    cfg.latentW = latentW();
    polychoronHomotopy_.setConfig(cfg);
    renderedGeneration_ = 0U;
}

void SculptureWorldNavigator::setPolychoronMorphProgress(float progress) noexcept {
    auto cfg = polychoronHomotopy_.config();
    cfg.morphProgress = std::clamp(progress, 0.0f, 1.0f);
    cfg.rotorAlpha = rotorAlpha();
    cfg.rotorBeta = rotorBeta();
    cfg.rotorTheta4D = rotorTheta4D();
    cfg.latentW = latentW();
    polychoronHomotopy_.setConfig(cfg);
    renderedGeneration_ = 0U;
}

float SculptureWorldNavigator::polychoronMorphProgress() const noexcept {
    return polychoronHomotopy_.config().morphProgress;
}

void SculptureWorldNavigator::cyclePolychoronSource() noexcept {
    auto cfg = polychoronHomotopy_.config();
    auto next = static_cast<std::uint8_t>((static_cast<std::uint8_t>(cfg.sourcePolytope) + 1U) % kPolychoronKindCount);
    cfg.sourcePolytope = static_cast<PolychoronKind>(next);
    cfg.rotorAlpha = rotorAlpha();
    cfg.rotorBeta = rotorBeta();
    cfg.rotorTheta4D = rotorTheta4D();
    cfg.latentW = latentW();
    polychoronHomotopy_.setConfig(cfg);
    renderedGeneration_ = 0U;
}

void SculptureWorldNavigator::cyclePolychoronTarget() noexcept {
    auto cfg = polychoronHomotopy_.config();
    auto next = static_cast<std::uint8_t>((static_cast<std::uint8_t>(cfg.targetPolytope) + 1U) % kPolychoronKindCount);
    cfg.targetPolytope = static_cast<PolychoronKind>(next);
    cfg.rotorAlpha = rotorAlpha();
    cfg.rotorBeta = rotorBeta();
    cfg.rotorTheta4D = rotorTheta4D();
    cfg.latentW = latentW();
    polychoronHomotopy_.setConfig(cfg);
    renderedGeneration_ = 0U;
}

PolychoronKind SculptureWorldNavigator::polychoronSource() const noexcept {
    return polychoronHomotopy_.config().sourcePolytope;
}

PolychoronKind SculptureWorldNavigator::polychoronTarget() const noexcept {
    return polychoronHomotopy_.config().targetPolytope;
}

std::vector<ProjectedVertex3D> SculptureWorldNavigator::samplePolychoronHomotopy() const {
    return polychoronHomotopy_.evaluateHomotopy(coordinator_.simulatedTime());
}

float SculptureWorldNavigator::coxeterOrderParameter() const noexcept {
    return polychoronHomotopy_.evaluateCoxeterOrderParameter(polychoronMorphProgress());
}

} // namespace epi_sculpt
