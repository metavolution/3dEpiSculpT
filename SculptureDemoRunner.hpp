#pragma once

#include "SculptureFrameProducer.h"
#include "SculptureProducerCadence.h"
#include "SculptureLiveControls.hpp"
#include "SculptureStateRecorder.h"
#include "SculptureVulkanDeviceIdentity.h"
#include "mgfx/Engine.h"
#include "mgfx/vk/Mesh.h"
#include <GLFW/glfw3.h>
#include "SculptureAudio.h"
#include "SculptureAudioResonance.h"
#include <chrono>
#include <cstdio>
#include <exception>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: STRICT OPENCL-TO-VULKAN DEMO RUNNER
// -----------------------------------------------------------------------------
// This single header isolates all the redundant boilerplate required to stand up
// a Vulkan rendering context, a GLFW window, and the staged OpenCL producer.
// It owns the non-negotiable live-demo policy: matching OpenCL/Vulkan devices,
// no CPU fallback, a bounded compute cadence, and a 1440p Vulkan presenter.
// Individual demos only own their creative mapping (snapshot -> instances).
// -----------------------------------------------------------------------------

struct DemoConfig {
    std::string title = "epiC Demo";
    std::uint32_t maxInstances = 1024U;
    mgfx::math::vec3 cameraEye = {0.0f, 0.0f, 5.0f};
    mgfx::math::vec3 cameraTarget = {0.0f, 0.0f, 0.0f};
    float cameraFovY = 60.0f;
    // Vulkan presents the newest immutable OpenCL result every frame. 30 Hz is
    // the live default because the compact producer is mathematical state, not
    // a per-display-frame renderer. Set <= 0 only for deliberate profiling.
    float openClUpdateHz = 30.0f;
    // Audio remains an optional future control source; it must not introduce
    // capture, mock-beat, or resonance work into ordinary visual demos.
    bool enableAudioReactiveModulation = kSculptureAudioEnabledByDefault;
    mgfx::pbr::Atmosphere atmosphere{};
    mgfx::pbr::LightSourceRegistry lights{};
};

inline void runSculptureDemo(
    const DemoConfig& config,
    const std::function<mgfx::vk::Mesh(mgfx::Engine&)>& onMakeMesh,
    mgfx::pbr::Material material,
    SculptureProgramParams initialParams,
    const std::function<void(
        const gpu_abi::PackedSculptureSnapshot& snapshot,
        float time,
        const SculptureProgramParams& params,
        std::vector<mgfx::vk::InstanceData>& instances)>& onUpdateInstances,
    const std::function<void(
        GLFWwindow* window,
        mgfx::Engine& engine,
        float time,
        SculptureProgramParams& params,
        mgfx::Camera& camera)>& onUpdateState = nullptr) {

    mgfx::vk::Context context(mgfx::vk::Context::CreateInfo{
        .width = 2560,
        .height = 1440,
        .title = config.title.c_str(),
        .enableValidation = false,
    });
    mgfx::vk::Swapchain swapchain(context);
    mgfx::vk::FrameLoop frameLoop(context, swapchain);

    mgfx::Engine::Config engineConfig{};
    engineConfig.quality = mgfx::RenderQuality::basic();
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

    mgfx::vk::Mesh mesh = onMakeMesh(engine);
    const mgfx::MaterialHandle materialHandle = engine.materials()->createMaterial(material);

    engine.setAtmosphere(config.atmosphere);
    engine.setLights(config.lights);

    SculptureFrameProducer::Config producerConfig{};
#ifdef MGFX_EPI_SCULPT_KERNEL_PATH
    producerConfig.kernelPath = MGFX_EPI_SCULPT_KERNEL_PATH;
#endif
    producerConfig.preferOpenCl = true;
    producerConfig.enableCpuFailureRecovery = false;
    producerConfig.vulkanExternalMemoryFd = true;
    producerConfig.vulkanExternalSemaphoreFd = true;

    matchOpenClToVulkanDevice(producerConfig, context.device().physical());
    
    SculptureFrameProducer producer(producerConfig);
    if (!producer.usingOpenCl()) {
        throw std::runtime_error("OpenCL is required: " + std::string(producer.diagnostic()));
    }

    SculptureProgramParams params = initialParams;
    std::vector<mgfx::vk::InstanceData> instances;
    SculptureLiveControls controls(context.window().glfw());
    
    // Audio is intentionally opt-in: focused OpenCL/Vulkan demos keep their
    // mathematical state deterministic and do not open an input device unless
    // a dedicated audio-reactive application asks for it.
    SculptureAudio audio;
    const bool audioActive = config.enableAudioReactiveModulation && audio.initialize();
    if (config.enableAudioReactiveModulation && !audioActive) {
        std::fprintf(stderr, "[SculptureDemoRunner] Warning: Audio capture failed to initialize.\n");
    }
    SculptureAudioResonance resonance;
    
    const auto start = std::chrono::steady_clock::now();
    auto previousFrame = start;
    std::uint32_t generation = 0U;
    bool generationExhaustionReported = false;
    SculptureProducerCadence producerCadence{{.updateHz = config.openClUpdateHz}};
    float nextTelemetryTime = 1.0f;
    std::uint32_t renderedFrames = 0U;
    int cameraTourIndex = 0;
    
    while (!context.shouldClose()) {
        context.pollEvents();

        const auto now = std::chrono::steady_clock::now();
        const float realTime = std::chrono::duration<float>(now - start).count();
        const float realDeltaSeconds = std::chrono::duration<float>(now - previousFrame).count();
        previousFrame = now;
        controls.beginFrame(realDeltaSeconds);
        if (context.shouldClose()) continue;

        const float time = controls.simulationTime();
        const float deltaSeconds = controls.simulationDeltaSeconds();
        controls.restoreProgramBaseline(params);

        mgfx::Camera camera{
            .eye = config.cameraEye,
            .target = config.cameraTarget,
            .fovYRad = mgfx::math::radians(config.cameraFovY),
        };

        if (onUpdateState) {
            onUpdateState(context.window().glfw(), engine, time, params, camera);
        }
        
        // Dedicated audio-reactive demos may opt into live FFT-driven modulation.
        if (audioActive) {
            epi_sculpt::AudioSignalContext audioCtx = audio.getSignalContext();
            params.audioSpectrum = audio.getSpectrum();
            
            // Generate dummy geometry metrics since we don't extract them from the snapshot here
            epi_sculpt::GeometricAudioFeatures dummyGeo{};
            dummyGeo.surfaceCurvature = 0.5f;
            dummyGeo.lyapunovIndex = 0.1f;
            
            resonance.step(audioCtx, dummyGeo, epi_sculpt::SculptureBoundingVolume{}, time, deltaSeconds);
            resonance.applyResonanceModulation(params, time, deltaSeconds);
        } else {
            params.audioSpectrum = {};
        }

        controls.applyProgramOffsets(params);
        cameraTourIndex += controls.consumeTourStep();
        camera = SculptureLiveControls::composeBasicTourCamera(camera, cameraTourIndex);
        camera = controls.applyCamera(camera, realDeltaSeconds);
        if (controls.consumePresentationToggle()) {
            std::fprintf(stderr,
                "[SculptureDemoRunner] P is reserved for render-path switching in advanced demos\n");
        }

        if (controls.consumeCapture()) {
            SculptureWorldState state{};
            state.time = time;
            state.cameraAngle = static_cast<float>(cameraTourIndex)
                * mgfx::math::kTwoPi / 8.0f;
            state.cameraEye[0] = camera.eye.x; state.cameraEye[1] = camera.eye.y; state.cameraEye[2] = camera.eye.z;
            state.cameraTarget[0] = camera.target.x; state.cameraTarget[1] = camera.target.y; state.cameraTarget[2] = camera.target.z;
            state.cameraUp[0] = camera.up.x; state.cameraUp[1] = camera.up.y; state.cameraUp[2] = camera.up.z;
            const SculptureWorldMode mode = SculptureStateRecorder::makeCaptureWorldMode(
                state, "compact_demo", "demo-v1", params, {});
            frameLoop.requestCapture([mode](const mgfx::vk::CaptureResult& result) {
                SculptureStateRecorder::captureFrameAndState(result, mode);
            });
        }

        const auto proposal = producerCadence.prepare(time, params);
        if (!proposal.submit || !producerCadence.canCommit(proposal)) {
            producerCadence.defer(proposal);
        } else if (generation == std::numeric_limits<std::uint32_t>::max()) {
            producerCadence.defer(proposal);
            if (!generationExhaustionReported) {
                std::fprintf(stderr,
                    "[SculptureDemoRunner] compact generation exhausted; retaining last delivery\n");
                generationExhaustionReported = true;
            }
        } else {
            const std::uint32_t candidateGeneration = generation + 1U;
            static_cast<void>(producer.produce(
                params, time, 0.0f, proposal.simulationDeltaSeconds, candidateGeneration));
            const auto& prodTelem = producer.telemetry();
            if (prodTelem.submissionAccepted) {
                // A compile wait or full producer ring is not a simulation step.
                generation = candidateGeneration;
                producerCadence.commit(proposal);
                const float latency = prodTelem.outputFromOpenCl
                    ? prodTelem.openClCompletionMilliseconds
                    : prodTelem.cpuCallMilliseconds;
                producerCadence.observeProducerLatency(latency);
            } else {
                producerCadence.defer(proposal);
            }
        }
        if (!producer.usingOpenCl()) {
            if (producer.telemetry().submissionIndeterminate) {
                std::fprintf(stderr, "Sculpture demo: indeterminate OpenCL work; stopping before resource destruction\n");
                std::terminate();
            }
            throw std::runtime_error("OpenCL producer stopped; refusing CPU fallback: "
                + std::string(producer.diagnostic()));
        }

        const auto& delivery = producer.delivery();
        if (producer.telemetry().outputFromOpenCl && delivery.snapshot) {
            onUpdateInstances(*delivery.snapshot, time, params, instances);
        }
        
        const std::string hud = controls.hud(
            "OPENCL -> VULKAN  |  INSTANCED MESH",
            cameraTourIndex == 0 ? "Authored" : "OrbitView");
        engine.runFrame(camera, [&](mgfx::Drawable& draw) {
            if (!instances.empty()) {
                draw.drawInstanced(mesh, materialHandle, instances);
            }
            draw.drawText(hud.c_str(), mgfx::math::vec2(18.0f, 16.0f), 18.0f);
        });

        ++renderedFrames;
        if (realTime >= nextTelemetryTime) {
            std::fprintf(stderr, "[%s] fps=%u instances=%zu sim=%.2fs speed=%.2fx%s\n",
                         config.title.c_str(), renderedFrames, instances.size(),
                         time, controls.timeScale(), controls.paused() ? " paused" : "");
            renderedFrames = 0U;
            nextTelemetryTime += 1.0f;
        }
    }

    producer.waitIdle();
    engine.waitIdle();
}

} // namespace epi_sculpt
