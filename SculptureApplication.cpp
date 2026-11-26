// Composition root and resource lifetime. The entrypoint only constructs/runs this owner.
// These translation units share one SculptureApplication owner; none is a second scheduler.

#include "SculptureApplication.h"
#include "SculpturePresentationRetirement.h"
#include "SculptureFieldAppearance.h"
#include "SculptureRuntimeOptions.h"
#include "SculptureVulkanDeviceIdentity.h"
#include "SculptureWorldProfileSidecar.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#ifdef MGFX_ENABLE_XR
#include "mgfx/xr/OpenXrBackend.h"
#endif

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace epi_sculpt {

SculptureApplication::SculptureApplication(int argc, char** argv) 
    : latentLFO_(LFOShape::Sine, 0.05f, 1.5f),
      hueLFO_(LFOShape::Triangle, 0.02f, 0.3f)
{
    // A failed constructor never invokes ~SculptureApplication. Retire
    // partially installed cross-API owners before automatic member unwind.
    struct ConstructionRetirement final {
        SculptureApplication* app;
        ~ConstructionRetirement() noexcept {
            if (app) app->retirePresentationForShutdown();
        }
    } constructionRetirement{this};
    if (std::getenv("MGFX_TELEMETRY")) (void)telemetry_.beginSession("telemetry.csv", "SculptureApp");

    const SculptureRuntimeOptions runtimeOptions =
        SculptureRuntimeOptions::parse(argc, argv);
    externalInteropRequested_ = !runtimeOptions.hostStaged;
    runSeconds_ = runtimeOptions.runSeconds;
    captureSeconds_ = runtimeOptions.captureSeconds;
    startupCameraPose_ = runtimeOptions.cameraPose;
    appState_.reliefContours.setOverride(runtimeOptions.reliefContourDensity);

    resourcePaths_ = SculptureResourceLocator::resolve(
        argc > 0 && argv != nullptr ? argv[0] : nullptr,
        runtimeOptions.resourceRoot);

    mgfx::Engine::Config cfg{};
    cfg.quality = mgfx::RenderQuality::high();
    if (runtimeOptions.sceneRenderScale) {
        cfg.sceneRenderScale = *runtimeOptions.sceneRenderScale;
    }
    cfg.gpuPassTimingEnabled = runtimeOptions.enableGpuPassTiming;
    cfg.maxInstancesPerFrame = 65536;
    cfg.validateShaderContract = false;
    if (!resourcePaths_.fontPath.empty()) {
        cfg.text.fontPixelHeight = 20.0f;
    }

    SculpturePresentationHostRecipe hostRecipe{};
    hostRecipe.width = 2560;
    hostRecipe.height = 1440;
    if (runtimeOptions.desktopSize) {
        hostRecipe.width = runtimeOptions.desktopSize->width;
        hostRecipe.height = runtimeOptions.desktopSize->height;
    }
    hostRecipe.title = "3dEpiSculpTMorph - procedural sculpture";
    hostRecipe.enableXR = !runtimeOptions.desktop && !runtimeOptions.hidden;
#ifdef MGFX_ENABLE_XR
    hostRecipe.xrBackend = &mgfx::xr::openXrBackend();
#endif
    hostRecipe.visible = !runtimeOptions.hidden;
    if (runtimeOptions.hidden || runtimeOptions.desktopSize)
        hostRecipe.desktopFullscreenWindowed = false;
    hostRecipe.engine = cfg;
    if (!resourcePaths_.shaderBinaryDirectory.empty()) {
        hostRecipe.shaderBinaryDirectory =
            resourcePaths_.shaderBinaryDirectory.string();
    }
    if (!resourcePaths_.shaderSourceDirectory.empty()) {
        hostRecipe.shaderSourceDirectory =
            resourcePaths_.shaderSourceDirectory.string();
    }
    if (!resourcePaths_.fontPath.empty()) {
        hostRecipe.fontPath = resourcePaths_.fontPath.string();
    }
    presentationHost_ = std::make_unique<SculpturePresentationHost>(
        std::move(hostRecipe));

    if (presentationHost_->context().hasXR()) {
        printf("[epiC] OpenXR Session Successfully Initialized! Headset integration active.\n");
    }

    if (resourcePaths_.hasResourceRoot()) {
        std::fprintf(stderr, "[3dEpiSculpTMorph] resource root: %s\n",
                     resourcePaths_.root.string().c_str());
    }

    SculpturePresentationScaleConfig presentationScaleConfig{};
    presentationScaleConfig.autoEnabled = runtimeOptions.enableAutoSceneScale;
    if (runtimeOptions.sceneGpuBudgetMilliseconds) {
        presentationScaleConfig.targetGpuMilliseconds =
            *runtimeOptions.sceneGpuBudgetMilliseconds;
    }
    presentationScalePolicy_.setConfig(presentationScaleConfig);
    if (presentationScalePolicy_.autoEnabled() && !presentationHost_->engine().gpuPassTimingEnabled()) {
        std::fprintf(stderr,
                     "[3dEpiSculpTMorph] auto scene scale requested but GPU pass timing "
                     "is unavailable; leaving automatic scaling disabled\n");
        presentationScalePolicy_.setAutoEnabled(false);
    }

    eventBus_ = std::make_unique<SculptureEventBus>();
    commandCenter_ = std::make_unique<SculptureCommandCenter>(presentationHost_->window().glfw(), eventBus_.get());
    liveCameraControls_ = std::make_unique<SculptureLiveControls>(
        presentationHost_->window().glfw(), true);
    ImplicitFieldProgram implicitProgram{};
    if (!resourcePaths_.implicitKernel.empty()) {
        implicitProgram.kernelPath = resourcePaths_.implicitKernel.string();
    }
    // Keyboard control is the live default. External services are opt-in so
    // ordinary visual sessions do not allocate ports, probe MIDI devices, or
    // create background threads outside the core render/process flow.
    if (runtimeOptions.enableRemoteControl) {
        remoteServer_ = std::make_unique<SculptureRemoteServer>(*eventBus_, 9000);
        remoteServer_->start();
    }
    if (runtimeOptions.enableMidiControl) {
        if (!SculptureMidiServer::backendCompiled()) {
            std::fprintf(stderr,
                "[3dEpiSculpTMorph] MIDI requested, but no device backend is compiled; continuing with keyboard/remote control.\n");
        } else {
            midiServer_ = std::make_unique<SculptureMidiServer>(*eventBus_);
            if (runtimeOptions.midiProfile) {
                const std::string& prof = *runtimeOptions.midiProfile;
                if (prof == "launchpad" || prof == "launchcontrol") {
                    midiServer_->mapper().setProfile(ControlSurfaceProfile::LaunchControlXL);
                } else if (prof == "korg" || prof == "nanokontrol") {
                    midiServer_->mapper().setProfile(ControlSurfaceProfile::KorgNano);
                } else if (prof == "apc" || prof == "akai") {
                    midiServer_->mapper().setProfile(ControlSurfaceProfile::AkaiAPC);
                } else if (prof == "keyboard" || prof == "synth") {
                    midiServer_->mapper().setProfile(ControlSurfaceProfile::GenericKeyboard);
                } else {
                    midiServer_->mapper().setProfile(ControlSurfaceProfile::StandardDJ);
                }
            }
            if (!midiServer_->start()) {
                midiServer_.reset();
            }
        }
    }
    
    SculptureFrameProducer::Config producerConfig;
    if (!resourcePaths_.sculptureKernel.empty()) {
        producerConfig.kernelPath = resourcePaths_.sculptureKernel.string();
    }
    if (!resourcePaths_.worldKernel.empty()) {
        producerConfig.worldKernelPath = resourcePaths_.worldKernel.string();
    }
    producerConfig.preferOpenCl = true;
    matchOpenClToVulkanDevice(producerConfig, presentationHost_->context().device().physical());
    producerConfig.vulkanExternalMemoryFd =
        externalInteropRequested_ && presentationHost_->context().device().supportsExternalMemoryFd();
    producerConfig.vulkanExternalSemaphoreFd =
        externalInteropRequested_ && presentationHost_->context().device().supportsExternalSemaphoreFd();
    sculptureProducer_ = std::make_unique<SculptureFrameProducer>(producerConfig);
    EpiFeedbackFieldProducer::Config canonicalFeedbackConfig{};
    canonicalFeedbackConfig.generatedKernelDirectory =
        resourcePaths_.generatedKernelDirectory.string();
    if (!resourcePaths_.canonicalFeedbackKernel.empty()) {
        canonicalFeedbackConfig.kernelPath =
            resourcePaths_.canonicalFeedbackKernel.string();
    }
    canonicalFeedbackProducerConfig_ = canonicalFeedbackConfig;
    canonicalFeedbackProducer_ = std::make_unique<EpiFeedbackFieldProducer>(
        canonicalFeedbackProducerConfig_);
    const bool canonicalFeedbackReady = canonicalFeedbackProducer_->initialize();
    if (!canonicalFeedbackReady) {
        std::fprintf(stderr,
            "[3dEpiSculpTMorph] canonical feedback unavailable: %s\n",
            canonicalFeedbackProducer_->diagnostic().c_str());
    }

    SculptureDevicePresentationBindings::Recipe bindingRecipe{};
    bindingRecipe.implicitProgram = std::move(implicitProgram);
    bindingRecipe.volumeRecipe = runtimeOptions.volumeRecipe;
    bindingRecipe.fieldAppearance = makeSculptureFieldAppearance(runtimeOptions.fieldPresentation);
    bindingRecipe.hudVisible = !runtimeOptions.noHud;
    presentationBindings_ = std::make_unique<SculptureDevicePresentationBindings>(
        presentationHost_->engine(), std::move(bindingRecipe));

    canonicalFeedbackTransportConfig_.width = canonicalFeedbackConfig.width;
    canonicalFeedbackTransportConfig_.height = canonicalFeedbackConfig.height;
    refreshExternalInteropEligibility();
    presentationBindings_->compactBridge().initializeGpuIngest(
        presentationHost_->engine(), *sculptureProducer_);

    // Native transports are optional accelerators. Startup and later logical-
    // device reconstruction intentionally use the same admission functions so
    // capability checks cannot drift between the two lifecycle paths.
    if (canonicalFeedbackReady && canonicalExternalTransportSupported_
        && !establishCanonicalExternalTransport()) {
        std::fprintf(stderr,
            "[3dEpiSculpTMorph] canonical external feedback disabled; using host-staged OpenCL delivery\n");
    }
    static_cast<void>(establishWorldExternalTransport());

    const bool presetsLoaded = presetLibrary_.reload(resourcePaths_.presetDirectory);
    std::fprintf(stderr, "[3dEpiSculpTMorph] preset library: %zu entries (%s)\n",
        presetLibrary_.entries().size(), presetsLoaded ? "authored assets loaded" : "built-ins only");
    
    // -------------------------------------------------------------------------
    // Educational Annotation: CURATED GEODESIC HOMOTOPY PLAYLIST
    // Rather than naive linear preset jumping, the choreographer builds a
    // continuous non-linear geodesic tour through higher-dimensional horizons.
    // -------------------------------------------------------------------------
    if (!presetLibrary_.buildCuratedTour(control_.sources().presetMorph(), "dimensional_horizons", 3.5f, 6.0f)) {
        for (std::size_t i = 0; i < presetLibrary_.entries().size(); ++i) {
            MorphEasing easing = (i % 3 == 1) ? MorphEasing::Exponential
                               : (i % 3 == 2) ? MorphEasing::SpringCriticallyDamped
                               : MorphEasing::Smoothstep;
            control_.sources().presetMorph().addPlaylistAsset(presetLibrary_.entries()[i], 3.5f, 6.0f, easing);
        }
    }
    populateMorphSpaceFromPresetLibrary();
    // Establish a valid baseline before authored routing can be rejected.
    applyModulationGraph({});
    selectPreset(0, 0.0f); // Snap to the first deterministic library entry.
    
    // Construct cinematic timeline with smooth Catmull-Rom & Smoothstep choreography
    control_.sources().sequence().addKeyframe(0, 5.0f, KeyframeEasing::CatmullRom);
    control_.sources().sequence().addKeyframe(1, 6.0f, KeyframeEasing::Smoothstep);
    control_.sources().sequence().addKeyframe(4, 6.0f, KeyframeEasing::CatmullRom);
    control_.sources().sequence().addKeyframe(8, 5.0f, KeyframeEasing::Smoothstep);
    control_.sources().sequence().addKeyframe(11, 6.0f, KeyframeEasing::CatmullRom); // Kinetic Flow Advection
    control_.sources().sequence().addKeyframe(12, 6.0f, KeyframeEasing::Smoothstep); // Topological Homotopy
    control_.sources().sequence().addKeyframe(13, 6.0f, KeyframeEasing::CatmullRom); // Chladni Cymatics
    control_.sources().sequence().addKeyframe(14, 6.0f, KeyframeEasing::Smoothstep); // Clifford Hopf Fibration
    control_.sources().sequence().addKeyframe(26, 7.0f, KeyframeEasing::CatmullRom); // 8D Octonionic G2 Lie Cathedral
    control_.sources().sequence().addKeyframe(27, 6.0f, KeyframeEasing::Smoothstep); // Quantum Holomorphic Potential Singularity
    control_.sources().sequence().addKeyframe(28, 8.0f, KeyframeEasing::CatmullRom); // Non-Associative Octonion G2 Associator Vortex

    std::fprintf(stderr,
        "[3dEpiSculpTMorph] audio: disabled by default (no capture/playback devices)\n");
    
    producerBackend_ = sculptureProducer_->backendName();
    std::fprintf(stderr, "[3dEpiSculpTMorph] producer: %s (%s)\n",
                 producerBackend_.c_str(),
                 std::string(sculptureProducer_->diagnostic()).c_str());
    std::fprintf(stderr, "[3dEpiSculpTMorph] dedicated world: %s\n",
                 std::string(sculptureProducer_->worldDiagnostic()).c_str());

    // Wire proactive JIT binary pre-warming callback for live VJ performance
    control_.sources().deck().setPrewarmCallback([this](DeckId /*deck*/, const SculptureProgramParams& params) {
        if (sculptureProducer_ && sculptureProducer_->usingOpenCl()) {
            sculptureProducer_->triggerAsyncRecompile(params);
        }
    });

    // Initialize VJ Performance Deck with initial library entries
    if (!presetLibrary_.entries().empty()) {
        const auto& entryA = presetLibrary_.entries()[0];
        control_.sources().deck().loadDeck(DeckId::DeckA, entryA.params, entryA.id, entryA.title);
        if (presetLibrary_.entries().size() > 1) {
            const auto& entryB = presetLibrary_.entries()[1];
            control_.sources().deck().loadDeck(DeckId::DeckB, entryB.params, entryB.id, entryB.title);
        } else {
            control_.sources().deck().loadDeck(DeckId::DeckB, entryA.params, entryA.id, entryA.title);
        }

        control_.sources().deck().setTourAdvanceCallback([this](DeckId targetDeck) {
            if (control_.sources().presetMorph().playlistSize() > 0) {
                // Advance along the active curated tour playlist
                tourPlaylistStep_ = (tourPlaylistStep_ + 1) % control_.sources().presetMorph().playlistSize();
                const auto* target = control_.sources().presetMorph().playlistTarget(tourPlaylistStep_);
                if (target) {
                    control_.sources().deck().loadDeck(targetDeck, target->params, target->name, target->name);
                    std::fprintf(stderr, "[SculptureApplication] Autonomous Bar Tour loaded '%s' into %s\n",
                        target->name.c_str(), targetDeck == DeckId::DeckA ? "Deck A" : "Deck B");
                    return;
                }
            }
            if (presetLibrary_.entries().empty()) return;
            deckLibraryStep_ = (deckLibraryStep_ + 1U) % presetLibrary_.entries().size();
            const auto* preset = presetLibrary_.at(deckLibraryStep_);
            if (preset) {
                control_.sources().deck().loadDeck(targetDeck, preset->params, preset->id, preset->title);
            }
        });
    }

    if (runtimeOptions.startupSelection) {
        if (const auto index = presetLibrary_.findIndex(*runtimeOptions.startupSelection)) {
            if (selectPreset(*index, 0.0f)) {
                std::fprintf(stderr,
                    "[3dEpiSculpTMorph] CLI applied %s '%s' as '%s'\n",
                    runtimeOptions.startupSelectionKind == SculptureStartupSelection::World
                        ? "world" : "preset",
                    runtimeOptions.startupSelection->c_str(), selectedPreset().id.c_str());
            } else {
                std::fprintf(stderr,
                    "[3dEpiSculpTMorph] CLI selection '%s' rejected; retained preset '%s'.\n",
                    runtimeOptions.startupSelection->c_str(), selectedPreset().id.c_str());
            }
        } else {
            std::fprintf(stderr,
                "[3dEpiSculpTMorph] Warning: CLI preset '%s' not found.\n",
                runtimeOptions.startupSelection->c_str());
        }
    } else if (runtimeOptions.statePath) {
        EventFileAction action{EventFileAction::Action::LoadState,
                               *runtimeOptions.statePath};
        handleFileAction(action);
    }

    // Profiling deliberately reuses the normal catalogue/preset path. It is
    // observational: selecting it cannot introduce a second world loop.
    if (runtimeOptions.profileWorld) {
        if (const auto presetIndex = presetLibrary_.findIndex(*runtimeOptions.profileWorld)) {
            // Refusal retains the old binding. Never profile that preset as if
            // the requested selection had succeeded.
            if (!selectPreset(*presetIndex, 0.0f)) {
                std::fprintf(stderr,
                    "[3dEpiSculpTMorph] profile world '%s' selection rejected; profile not started\n",
                    runtimeOptions.profileWorld->c_str());
                worldProfileExitRequested_ = true;
            } else if (presentationOwnership_ != SculpturePresentationOwnership::DedicatedWorld) {
                std::fprintf(stderr,
                    "[3dEpiSculpTMorph] profile world '%s' has no dedicated-world route\n",
                    selectedPreset().id.c_str());
                worldProfileExitRequested_ = true;
            } else if (!sculptureProducer_->worldKernelReady()) {
                std::fprintf(stderr,
                    "[3dEpiSculpTMorph] profile world '%s' unavailable: %s\n",
                    selectedPreset().id.c_str(),
                    std::string(sculptureProducer_->worldDiagnostic()).c_str());
                worldProfileExitRequested_ = true;
            } else {
                const WorldRecipe recipe = worldQualityPolicy_.apply(selectedPreset().worldRecipe);
                WorldProfileIdentity identity{};
                identity.canonicalPresetId = selectedPreset().id;
                // Profile artifacts must identify the exact authored asset,
                // just like saved world state. A clear fallback keeps
                // hand-constructed assets comparable without pretending they
                // have a content revision.
                identity.sourceRevision = selectedPreset().revision.empty()
                    ? "unversioned"
                    : selectedPreset().revision;
                identity.requestedRecipe = selectedPreset().worldRecipe;
                identity.effectiveRecipe = recipe;
                // `producerBackend_` contains the selected OpenCL device name.
                // Vulkan's UUID is the durable cross-session adapter identity;
                // never substitute its pipeline-cache UUID here.
                identity.openClDeviceId = producerBackend_;
                identity.vulkanDeviceId = describeVulkanPhysicalDevice(
                    presentationHost_->context().device().physical());
                identity.qualityTier = "high";
                identity.viewConfiguration = presentationHost_->context().hasXR() ? "openxr" : "mono";
                WorldProfileSessionConfig profileConfig{};
                if (runtimeOptions.profileSettleSeconds) {
                    profileConfig.settleSeconds = *runtimeOptions.profileSettleSeconds;
                }
                if (runtimeOptions.profileSampleSeconds) {
                    profileConfig.sampleSeconds = *runtimeOptions.profileSampleSeconds;
                }
                const auto unixMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                const std::string profilePath = SculptureWorldProfileSidecar::makeSessionPath(
                    "profiles", selectedPreset().id,
                    static_cast<std::uint64_t>(std::max<std::int64_t>(0, unixMilliseconds)),
                    1U);
                worldProfileDriver_.start(std::move(identity), profileConfig, profilePath);
                std::fprintf(stderr,
                    "[3dEpiSculpTMorph] profiling world '%s' (settle %.3fs, sample %.3fs) -> %s\n",
                    selectedPreset().id.c_str(), profileConfig.settleSeconds,
                    profileConfig.sampleSeconds, profilePath.c_str());
            }
        } else {
            std::fprintf(stderr, "[3dEpiSculpTMorph] profile world '%s' not found\n",
                         runtimeOptions.profileWorld->c_str());
            worldProfileExitRequested_ = true;
        }
    }

    eventBus_->subscribe([this](const SculptureEvent& event) { handleEvent(event); });
    constructionRetirement.app = nullptr;
}

bool SculptureApplication::reconstructPresentationDevice(const char* reason) noexcept {
    if (!presentationHost_ || !presentationHost_->hasDeviceIsland()
        || !presentationBindings_ || !sculptureProducer_) {
        return false;
    }

    const std::uint64_t retiringDeviceGeneration = presentationHost_->deviceGeneration();
    try {
        const auto bindingRecipe = presentationBindings_->recipeForReconstruction();
        // No device-owned callback or imported allocation may outlive this
        // transaction. Mathematical producers are drained but not reset.
        VkResult graphicsRetirement = VK_SUCCESS;
        const auto retirement = retirePresentationAfterDrains(
            [this] {
                cancelPendingImplicitProductions(
                    SculptureProductionDeferralReason::FrameSubmissionRejected);
                presentationHost_->frameLoop().cancelCaptures();
            },
            [this] { return sculptureProducer_->waitIdle(); },
            [this] {
                return !canonicalFeedbackProducer_
                    || canonicalFeedbackProducer_->wait_idle() != epi::QueueDrainResult::Failed;
            },
            [this, &graphicsRetirement] {
                graphicsRetirement = presentationHost_->engine().waitIdleChecked();
                return graphicsRetirement == VK_SUCCESS;
            },
            [this] {
                // External publications refer to allocations owned by the old device.
                if (const EpiFeedbackFieldDelivery* latest = canonicalFeedbackFields_.latest();
                    latest && latest->transport == EpiFeedbackFieldTransport::ExternalMemory) {
                    if (!canonicalPublicationInputs_.invalidate(
                            canonicalFeedbackFields_, canonicalFeedbackResetEpoch_)) {
                        throw std::logic_error(
                            "canonical delivery epoch exceeds device retirement epoch");
                    }
                }
                presentationBindings_->detachExternalInterop(
                    sculptureProducer_.get(), canonicalFeedbackProducer_.get());
                canonicalExternalDeliveryEnabled_ = false;
                canonicalExternalTransportSupported_ = false;
                canonicalExternalRecoveryNextAttemptGeneration_ = canonicalFeedbackGeneration_;
                ++canonicalFeedbackTransportConfig_.allocationGeneration;
                if (canonicalFeedbackTransportConfig_.allocationGeneration == 0U) {
                    canonicalFeedbackTransportConfig_.allocationGeneration = 1U;
                }
                presentationBindings_.reset();
            });
        if (retirement == PresentationRetirementResult::SculptureDrainFailed) {
            std::fprintf(stderr, "[3dEpiSculpTMorph] sculpture OpenCL drain failed; presentation reconstruction declined\n");
            return false;
        }
        if (retirement == PresentationRetirementResult::CanonicalDrainFailed) {
            std::fprintf(stderr, "[3dEpiSculpTMorph] canonical OpenCL drain failed; presentation reconstruction declined\n");
            return false;
        }
        if (retirement == PresentationRetirementResult::GraphicsDrainFailed) {
            std::fprintf(stderr,
                "[3dEpiSculpTMorph] Vulkan retirement failed (%d); presentation reconstruction declined\n",
                static_cast<int>(graphicsRetirement));
            return false;
        }

        // The platform window and mathematical producer state remain alive.
        const SculpturePresentationRecoveryResult recovery =
            presentationHost_->reconstructDevice();
        if (!recovery.recovered()) {
            std::fprintf(stderr,
                "[3dEpiSculpTMorph] Vulkan device reconstruction failed after generation %llu: %s (VkResult %d)\n",
                static_cast<unsigned long long>(retiringDeviceGeneration),
                recovery.diagnostic.c_str(), static_cast<int>(recovery.nativeResult));
            return false;
        }

        presentationBindings_ = std::make_unique<SculptureDevicePresentationBindings>(
            presentationHost_->engine(), bindingRecipe);
        refreshExternalInteropEligibility();
        presentationBindings_->compactBridge().initializeGpuIngest(
            presentationHost_->engine(), *sculptureProducer_);

        if (canonicalExternalTransportSupported_ && canonicalFeedbackProducer_
            && canonicalFeedbackProducer_->ready()) {
            if (!establishCanonicalExternalTransport()) {
                canonicalExternalRecoveryNextAttemptGeneration_ = canonicalFeedbackGeneration_
                    + kCanonicalExternalRecoveryRetryGenerations_;
            }
        }
        static_cast<void>(establishWorldExternalTransport());

        // These are observation/device-local histories, unlike the surviving
        // producer generations. Recreated passes begin with fresh native state.
        implicitObservationCadence_.reset();
        worldPresentationMetrics_ = {};

        std::fprintf(stderr,
            "[3dEpiSculpTMorph] Vulkan presentation device reconstructed %llu -> %llu (%s); mathematical producers preserved\n",
            static_cast<unsigned long long>(retiringDeviceGeneration),
            static_cast<unsigned long long>(presentationHost_->deviceGeneration()),
            reason != nullptr ? reason : "host recovery");
        return true;
    } catch (const std::exception& error) {
        std::fprintf(stderr,
            "[3dEpiSculpTMorph] presentation-device reconstruction transaction failed: %s\n",
            error.what());
    } catch (...) {
        std::fprintf(stderr,
            "[3dEpiSculpTMorph] presentation-device reconstruction transaction failed with unknown error\n");
    }
    return false;
}

SculptureApplication::~SculptureApplication() {
    for (const auto& path : pendingStateSavePaths_) {
        std::fprintf(stderr,
            "[SculptureApplication] State save canceled before a frame could be frozen: %s\n",
            path.c_str());
    }
    retirePresentationForShutdown();
}

void SculptureApplication::retirePresentationForShutdown() noexcept {
    if (midiServer_) midiServer_->stop();
    if (remoteServer_) {
        remoteServer_->stop();
    }
    const auto result = retirePresentationAfterDrains(
        [this] {
            if (presentationHost_ && presentationHost_->hasDeviceIsland() && !captureExit_.started())
                presentationHost_->frameLoop().cancelCaptures();
        },
        [this] { return !sculptureProducer_ || sculptureProducer_->waitIdle(); },
        [this] { return !canonicalFeedbackProducer_ ||
            canonicalFeedbackProducer_->wait_idle() != epi::QueueDrainResult::Failed; },
        [this] {
            if (!presentationHost_ || !presentationHost_->hasDeviceIsland()) return true;
            return captureExit_.finish(false, presentationHost_->frameLoop(),
                [this] { return presentationHost_->engine().waitIdleChecked(); }) == VK_SUCCESS;
        },
        [this] {
            if (presentationBindings_)
                presentationBindings_->detachExternalInterop(
                    sculptureProducer_.get(), canonicalFeedbackProducer_.get());
        });
    if (result != PresentationRetirementResult::Ready) {
        std::fprintf(stderr, "Sculpture shutdown: native retirement failed (%d); stopping before resource destruction\n",
                     static_cast<int>(result));
        std::terminate();
    }
}

} // namespace epi_sculpt
