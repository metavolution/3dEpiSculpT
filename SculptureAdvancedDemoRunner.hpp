#pragma once

#include "SculptureFrameProducer.h"
#include "SculptureProducerCadence.h"
#include "ImplicitObservationSession.h"
#include "SculptureLiveControls.hpp"
#include "SculpturePresentationDirector.h"
#include "SculptureRenderBridge.h"
#include "SculptureStateRecorder.h"
#include "SculptureAudio.h"
#include "SculptureAudioResonance.h"
#include "SculptureVulkanDeviceIdentity.h"
#include "mgfx/Engine.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdio>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: ADVANCED OPENCL-TO-VULKAN DEMO RUNNER
// -----------------------------------------------------------------------------
// Orchestrates the complete high-performance Vulkan/OpenCL rendering loop
// with strict OpenCL generation, Vulkan presentation, and optional implicit
// raymarched presentation. The shared policy keeps expensive compute bounded;
// individual demos supply only their mathematical program and art direction.
// -----------------------------------------------------------------------------

struct AdvancedDemoConfig {
    std::string title = "epiC Advanced Demo";
    std::uint32_t maxInstances = 4096U;
    bool disableValidation = true;
    bool enableAudio = kSculptureAudioEnabledByDefault;
    // A compact state producer is sampled at this cadence while Vulkan keeps
    // presenting cached state at display rate. Set <= 0 only for profiling.
    float openClUpdateHz = 30.0f;
    RenderParadigm initialRenderParadigm = RenderParadigm::VolumetricInstancing;
    // The desktop window remains 2560x1440. Implicit work may start below
    // output resolution and be upsampled by Vulkan while quality adapts.
    float implicitRenderScale = 0.50f;
    // The raymarcher is also state generation, not a display-rate task.
    float implicitUpdateHz = 30.0f;
    // Per-demo raymarch quality. The default governor favors sustained live
    // cadence; a showcase may deliberately pin a higher ray budget instead
    // of trading SDF silhouette quality away under load.
    ImplicitQualityConfig implicitQuality{};
    // External-memory handoff is an optional transport optimization. Demos
    // that need a guaranteed live presentation can retain the proven bounded
    // OpenCL readback + persistent Vulkan staging route while a driver-specific
    // interop path is being hardened.
    bool enableImplicitExternalTransport = true;
    mgfx::RenderQuality renderQuality = mgfx::RenderQuality::basic();
    std::string pipelineLabel = "OPENCL IMPLICIT FIELD -> VULKAN";
    bool useAuthoredOrbitCamera = false;
    float cameraOrbitRadius = 5.25f;
    float cameraOrbitHeight = 0.85f;
    float cameraTargetHeight = 0.0f;
    float cameraAngularVelocity = 0.11f;
    float cameraFovYDegrees = 48.0f;
    bool allowPresentationToggle = true;
    bool requireImplicitOpenCl = false;
};

class SculptureVJController {
public:
    struct VJState {
        bool glitch = false;
        bool matrixRain = false;
        bool swirl = false;
        bool blackHole = false;
        bool synthwave = false;
    } state;

    void update(GLFWwindow* window, mgfx::Engine& engine, const SculptureProgramParams& params, float time) {
        state.glitch = (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS);
        state.matrixRain = (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS);
        state.swirl = (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS);
        state.blackHole = (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS);
        state.synthwave = (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS);

        auto& q = engine.config().quality;
        auto& p = engine.config().postProcess;
        
        q.digitalGlitchEnabled = state.glitch;
        if (state.glitch) {
            p.digitalGlitchStrength = 1.0f + params.audioSpectrum[0] * 5.0f; // Bass spikes glitch
        }
        
        q.matrixRainEnabled = state.matrixRain;
        if (state.matrixRain) {
            p.matrixRainStrength = 0.5f + params.audioSpectrum[1] * 2.0f; // Mids push rain density
            p.matrixRainDensity = 10.0f + params.audioSpectrum[2] * 20.0f; // Treble increases density
        }

        q.swirlEnabled = state.swirl;
        if (state.swirl) {
            p.swirlStrength = params.audioSpectrum[0] * 4.0f; // Bass creates strong swirl
            p.swirlRadius = 0.3f + params.audioSpectrum[1] * 0.5f;
            p.swirlAngle = time * 2.0f;
        }

        q.blackHoleEnabled = state.blackHole;
        if (state.blackHole) {
            p.blackHoleStrength = 1.0f + params.audioSpectrum[0] * 10.0f;
            p.blackHoleMass = 0.5f + params.audioSpectrum[1];
        }

        q.synthwaveEnabled = state.synthwave;
        if (state.synthwave) {
            p.synthwaveStrength = 0.8f + params.audioSpectrum[0];
        }
    }
};

inline void runAdvancedSculptureDemo(
    const AdvancedDemoConfig& config,
    SculptureProgramParams initialParams,
    const std::function<void(
        GLFWwindow* window,
        mgfx::Engine& engine,
        SculpturePresentationDirector& presentation,
        float time,
        SculptureProgramParams& params)>& onUpdateState = nullptr) {

    mgfx::vk::Context context(mgfx::vk::Context::CreateInfo{
        .width = 2560,
        .height = 1440,
        .title = config.title.c_str(),
        .enableValidation = !config.disableValidation,
    });
    mgfx::vk::Swapchain swapchain(context);
    mgfx::vk::FrameLoop frameLoop(context, swapchain);

    mgfx::Engine::Config engineConfig{};
    engineConfig.quality = config.renderQuality;
    engineConfig.maxInstancesPerFrame = config.maxInstances;
    engineConfig.validateShaderContract = false;
#ifdef MGFX_DEMO_SHADER_DIR
    engineConfig.shaderSourceDir = MGFX_DEMO_SHADER_DIR;
#endif
#ifdef MGFX_DEMO_FONT_PATH
    engineConfig.text.fontPath = MGFX_DEMO_FONT_PATH;
    engineConfig.text.fontPixelHeight = 18.0f;
#endif
    mgfx::Engine engine(context, frameLoop, engineConfig);

    const bool compactPathAvailable =
        config.initialRenderParadigm != RenderParadigm::RaymarchedImplicit
        || config.allowPresentationToggle;
    SculptureFrameProducer::Config producerConfig{};
#ifdef MGFX_EPI_SCULPT_KERNEL_PATH
    producerConfig.kernelPath = MGFX_EPI_SCULPT_KERNEL_PATH;
#endif
    producerConfig.preferOpenCl = true;
    producerConfig.enableCpuFailureRecovery = false;
    producerConfig.vulkanExternalMemoryFd = true;
    producerConfig.vulkanExternalSemaphoreFd = true;

    matchOpenClToVulkanDevice(producerConfig, context.device().physical());

    std::unique_ptr<epi_sculpt::SculptureFrameProducer> producer;
    if (compactPathAvailable) {
        producer = std::make_unique<epi_sculpt::SculptureFrameProducer>(
            producerConfig);
    }
    if (producer && !producer->usingOpenCl()) {
        throw std::runtime_error("OpenCL is required: "
            + std::string(producer->diagnostic()));
    }
    
    epi_sculpt::SculptureRenderBridge batches;
    if (compactPathAvailable) {
        batches.initializeGpuIngest(engine, *producer);
        batches.initCompute(engine);
    }
    
    epi_sculpt::SculpturePresentationDirector presentation(engine);
    presentation.setImplicitQuality(config.implicitQuality);
    presentation.setImplicitExternalTransportEnabled(
        config.enableImplicitExternalTransport);
    presentation.setRenderParadigm(config.initialRenderParadigm);
    presentation.setImplicitRenderScale(config.implicitRenderScale);
    presentation.setImplicitUpdateHz(config.implicitUpdateHz);
    epi_sculpt::SculptureProgramParams params = initialParams;
    epi_sculpt::SculptureVJController vj;
    epi_sculpt::SculptureLiveControls controls(context.window().glfw());

    // Audio Reactive VJ Subsystem
    epi_sculpt::SculptureAudio audio;
    epi_sculpt::SculptureAudioResonance resonance;
    bool audioActive = false;
    if (config.enableAudio) {
        audioActive = audio.initialize();
        if (!audioActive) {
            std::fprintf(stderr, "[AdvancedDemoRunner] Warning: Audio capture failed to initialize.\n");
        }
    }

    if (producer) {
        std::fprintf(stderr, "[AdvancedDemoRunner] compact producer: %s (%s)\n",
                     std::string(producer->backendName()).c_str(),
                     std::string(producer->diagnostic()).c_str());
    } else {
        // A composite without a compact producer degenerates to the implicit
        // contribution. Express that explicitly instead of carrying a visual
        // mode whose second producer can never exist.
        if (presentation.renderParadigm() != epi_sculpt::RenderParadigm::RaymarchedImplicit) {
            presentation.setRenderParadigm(epi_sculpt::RenderParadigm::RaymarchedImplicit);
        }
        std::fprintf(stderr,
            "[AdvancedDemoRunner] compact producer disabled; implicit OpenCL owns the frame\n");
    }

    const auto start = std::chrono::steady_clock::now();
    auto previousFrame = start;
    std::uint32_t generation = 0U;
    bool compactGenerationExhaustionReported = false;
    float nextDiagnosticTime = 1.0f;
    epi_sculpt::SculptureProducerCadence compactProducerCadence{
        {.updateHz = config.openClUpdateHz}};
    epi_sculpt::ImplicitObservationSession implicitObservations;
    bool implicitIdentityExhaustionReported = false;

    while (!context.shouldClose()) {
        context.pollEvents();

        const auto now = std::chrono::steady_clock::now();
        const float realTime = std::chrono::duration<float>(now - start).count();
        const float realDeltaSeconds = std::chrono::duration<float>(now - previousFrame).count();
        previousFrame = now;
        controls.beginFrame(realDeltaSeconds);
        if (context.shouldClose()) continue;

        const float currentTime = controls.simulationTime();
        const float deltaSeconds = controls.simulationDeltaSeconds();
        controls.restoreProgramBaseline(params);

        if (audioActive) {
            epi_sculpt::AudioSignalContext audioCtx = audio.getSignalContext();
            const auto spectrum = audio.getSpectrum();
            params.audioSpectrum = std::array<float, 4>{spectrum[0], spectrum[1], spectrum[2], spectrum[3]};
            
            // Integrate Acoustic Resonance Physics
            epi_sculpt::GeometricAudioFeatures dummyGeo{};
            dummyGeo.surfaceCurvature = 0.5f;
            dummyGeo.lyapunovIndex = 0.2f;
            resonance.step(audioCtx, dummyGeo, batches.boundingVolume(), currentTime, deltaSeconds);
            resonance.applyResonanceModulation(params, currentTime, deltaSeconds);
        } else {
            params.audioSpectrum = {};
        }

        if (onUpdateState) {
            onUpdateState(context.window().glfw(), engine, presentation, currentTime, params);
        }

        controls.applyProgramOffsets(params);
        vj.update(context.window().glfw(), engine, params, currentTime);

        const int tourStep = controls.consumeTourStep();
        if (tourStep != 0) {
            const int current = static_cast<int>(presentation.cameraMode());
            const int count = static_cast<int>(epi_sculpt::kCameraModeCount);
            const int next = ((current + tourStep) % count + count) % count;
            presentation.setCameraMode(static_cast<epi_sculpt::CameraMode>(next));
            std::fprintf(stderr, "[AdvancedDemoRunner] camera tour: %s\n",
                         epi_sculpt::name(presentation.cameraMode()).data());
        }

        if (config.allowPresentationToggle
            && controls.consumePresentationToggle()) {
            if (presentation.renderParadigm() == epi_sculpt::RenderParadigm::VolumetricInstancing) {
                presentation.setRenderParadigm(epi_sculpt::RenderParadigm::RaymarchedImplicit);
                std::fprintf(stderr, "[AdvancedDemoRunner] Switched to RaymarchedImplicit\n");
            } else if (presentation.renderParadigm() == epi_sculpt::RenderParadigm::RaymarchedImplicit) {
                presentation.setRenderParadigm(epi_sculpt::RenderParadigm::UnifiedComposite);
                std::fprintf(stderr, "[AdvancedDemoRunner] Switched to UnifiedComposite\n");
            } else {
                presentation.setRenderParadigm(epi_sculpt::RenderParadigm::VolumetricInstancing);
                std::fprintf(stderr, "[AdvancedDemoRunner] Switched to VolumetricInstancing\n");
            }
        }

        epi_sculpt::SculptureWorldState composeState{};
        composeState.cameraAngle = currentTime * 0.16f;
        mgfx::Camera camera = presentation.composeCamera(false, 0.0f, composeState);
        if (config.useAuthoredOrbitCamera) {
            const float angle = currentTime * config.cameraAngularVelocity;
            camera.eye = mgfx::math::vec3(
                config.cameraOrbitRadius * std::sin(angle),
                config.cameraOrbitHeight + 0.16f * std::sin(currentTime * 0.19f),
                config.cameraOrbitRadius * std::cos(angle));
            camera.target = mgfx::math::vec3(
                0.0f, config.cameraTargetHeight, 0.0f);
            camera.up = mgfx::math::vec3(0.0f, 1.0f, 0.0f);
            camera.fovYRad = mgfx::math::radians(config.cameraFovYDegrees);
        }
        camera = controls.applyCamera(camera, realDeltaSeconds);

        if (controls.consumeCapture()) {
            epi_sculpt::SculptureWorldState state{};
            state.time = currentTime;
            state.morph = 0.0f;
            state.cameraAngle = currentTime * 0.16f;
            state.cameraEye[0] = camera.eye.x; state.cameraEye[1] = camera.eye.y; state.cameraEye[2] = camera.eye.z;
            state.cameraTarget[0] = camera.target.x; state.cameraTarget[1] = camera.target.y; state.cameraTarget[2] = camera.target.z;
            state.cameraUp[0] = camera.up.x; state.cameraUp[1] = camera.up.y; state.cameraUp[2] = camera.up.z;
            epi_sculpt::SculptureWorldMode mode = epi_sculpt::SculptureStateRecorder::makeCaptureWorldMode(
                state, "advanced_demo", "demo-v1", params, {});
            frameLoop.requestCapture([mode](const mgfx::vk::CaptureResult& res) {
                epi_sculpt::SculptureStateRecorder::captureFrameAndState(res, mode);
            });
        }

        const bool raymarchedImplicit = presentation.renderParadigm()
            == epi_sculpt::RenderParadigm::RaymarchedImplicit;
        const bool implicitPresentation = presentation.renderParadigm()
            != epi_sculpt::RenderParadigm::VolumetricInstancing;
        // Pure implicit presentation has no compact contribution. Composite
        // presentation deliberately keeps both producer cadences independent.
        if (raymarchedImplicit) {
            compactProducerCadence.reset();
        } else if (producer) {
            const auto proposal = compactProducerCadence.prepare(currentTime, params);
            if (!proposal.submit || !compactProducerCadence.canCommit(proposal)) {
                compactProducerCadence.defer(proposal);
            } else if (generation == std::numeric_limits<std::uint32_t>::max()) {
                // Zero is not a valid delivery identity. Keep the last frame
                // rather than wrapping an accepted-generation counter.
                compactProducerCadence.defer(proposal);
                if (!compactGenerationExhaustionReported) {
                    std::fprintf(stderr,
                        "[AdvancedDemoRunner] compact generation exhausted; retaining last delivery\n");
                    compactGenerationExhaustionReported = true;
                }
            } else {
                const std::uint32_t candidateGeneration = generation + 1U;
                static_cast<void>(producer->produce(
                    params, currentTime, 0.0f,
                    proposal.simulationDeltaSeconds, candidateGeneration));
                const auto& prodTelem = producer->telemetry();
                if (prodTelem.submissionAccepted) {
                    // Only accepted mathematical work advances the clock and
                    // generation. Compilation and backpressure leave it due.
                    generation = candidateGeneration;
                    compactProducerCadence.commit(proposal);
                    const float latency = prodTelem.outputFromOpenCl
                        ? prodTelem.openClCompletionMilliseconds
                        : prodTelem.cpuCallMilliseconds;
                    compactProducerCadence.observeProducerLatency(latency);
                } else {
                    compactProducerCadence.defer(proposal);
                }
            }
        }
        if (!implicitPresentation) {
            implicitObservations.reset();
        }
        if (producer && !producer->usingOpenCl()) {
            if (producer->telemetry().submissionIndeterminate) {
                std::fprintf(stderr, "Advanced sculpture demo: indeterminate OpenCL work; stopping before resource destruction\n");
                std::terminate();
            }
            throw std::runtime_error("OpenCL producer stopped; refusing CPU fallback: "
                + std::string(producer->diagnostic()));
        }

        const SculptureSnapshotDelivery delivery = producer
            ? producer->delivery() : SculptureSnapshotDelivery{};
        const bool hasCompactOpenClFrame = producer
            && producer->telemetry().outputFromOpenCl;
        const bool canPresent = implicitPresentation || hasCompactOpenClFrame;
        const std::string pipelineHud = implicitPresentation
            ? config.pipelineLabel + "  |  ASYNC RAYMARCH"
            : hasCompactOpenClFrame
                ? "OPENCL -> VULKAN  |  ADVANCED RUNNER  |  "
                  + std::string(epi_sculpt::name(producer->telemetry().delivery))
                  + "  |  " + epi_sculpt::renderPathName(batches.renderPath())
                : "OPENCL -> VULKAN  |  warming the asynchronous triple buffer...";
        const std::string hud = controls.hud(
            pipelineHud, epi_sculpt::name(presentation.cameraMode()));

        engine.runFrame(camera, [&](mgfx::Drawable& draw) {
            if (implicitPresentation) {
                epi_sculpt::ImplicitFrameRequest implicitRequest{};
                const std::uint32_t nextGeneration = implicitObservations.nextGeneration();
                // Preparation also polls the last accepted field. At the
                // identity limit it may be observed, but never resubmitted.
                const std::uint32_t implicitGeneration = nextGeneration != 0U
                    ? nextGeneration : implicitObservations.generation();
                if (implicitObservations.identityExhausted() && !implicitIdentityExhaustionReported) {
                    std::fprintf(stderr, "[AdvancedDemoRunner] implicit identity exhausted; retaining last delivery\n");
                    implicitIdentityExhaustionReported = true;
                }
                if (presentation.prepareImplicitObservation(
                        camera, params, currentTime, 0.0f, deltaSeconds,
                        implicitGeneration, implicitRequest)
                    && nextGeneration != 0U && !implicitObservations.identityExhausted()) {
                    const double wallSeconds = std::chrono::duration<double>(
                        std::chrono::steady_clock::now().time_since_epoch()).count();
                    if (implicitObservations.beginAttempt(
                            wallSeconds, implicitRequest, config.implicitUpdateHz)) {
                        presentation.submitImplicitObservation(
                            implicitRequest, implicitObservations.observer());
                    }
                }
            }
            if (canPresent) {
                presentation.drawScene(
                    draw, camera, batches, params, delivery,
                    currentTime, 0.0f, deltaSeconds, hud.c_str());
            } else {
                draw.drawText(hud.c_str(), mgfx::math::vec2(18.0f, 16.0f), 18.0f);
            }
        });

        if (config.requireImplicitOpenCl && implicitPresentation
            && !presentation.implicitProducer()
            && !presentation.implicitDiagnostic().empty()) {
            throw std::runtime_error(
                "implicit OpenCL is required: "
                + presentation.implicitDiagnostic());
        }

        if (realTime >= nextDiagnosticTime) {
            if (implicitPresentation && presentation.implicitProducer()) {
                const auto& implicit = presentation.implicitProducer()->telemetry();
                std::fprintf(stderr,
                    "[%s] opencl=1 implicit_kernel=%.2fms generation=%u pending=%u reused=%u speed=%.2fx%s\n",
                    config.title.c_str(), implicit.kernelMilliseconds,
                    implicit.deliveredGeneration, implicit.pendingSlots,
                    implicit.staleFrameReuses, controls.timeScale(),
                    controls.paused() ? " paused" : "");
            } else if (producer) {
                const auto& telemetry = producer->telemetry();
                std::fprintf(stderr,
                             "[%s] opencl=%d interop=%s delivery=%s instances=%zu path=%s speed=%.2fx%s\n",
                             config.title.c_str(), telemetry.outputFromOpenCl ? 1 : 0,
                             epi_sculpt::name(telemetry.interop.state).data(),
                             std::string(epi_sculpt::name(telemetry.delivery)).c_str(),
                             batches.instanceCount(),
                             epi_sculpt::renderPathName(batches.renderPath()),
                             controls.timeScale(), controls.paused() ? " paused" : "");
            } else {
                std::fprintf(stderr,
                    "[%s] implicit producer unavailable: %s\n",
                    config.title.c_str(),
                    presentation.implicitDiagnostic().c_str());
            }
            nextDiagnosticTime += 1.0f;
        }
    }

    if (producer) producer->waitIdle();
    engine.waitIdle();
}

} // namespace epi_sculpt
