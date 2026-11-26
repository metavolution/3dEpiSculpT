#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "mgfx/Engine.h"
#include "mgfx/Drawable.h"
#include "IExperimentExternalProducer.h"
#include "ExperimentResourceExternalTransport.h"
#include "ExperimentGenericExternalComputeRing.h"
#include "ExperimentResourceExternalComputeRing.h"
#include "ExperimentStepController.h"
#include "SculptureExperimentRenderBridge.h"
#include "SculptureAudioResonance.h"
#include "ExperimentMistOptics.h"

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: DECOUPLED FLUID & ATMOSPHERIC DYNAMICS CONTROLLER
// -----------------------------------------------------------------------------
// In the epiC visual computing engine (docu/project-vision.txt:103-118), procedural
// worlds are enlivened by high-performance physical micro-systems (volumetric mist,
// Rayleigh/Mie atmospheric scattering, acoustic resonance, and surface wetness).
//
// Previously, managing these systems required over 100 lines of low-level OpenCL
// semaphore scheduling, accumulator tracking, and Vulkan render-bridge wiring
// placed directly inside application main loops.
//
// SculptureFluidAtmosphereManager modularizes this entire multi-physics pipeline:
//  1. OpenCL Fluid / Mist Simulation:
//     Coordinates external experiment producers (e.g. ExperimentSprinklerProducer)
//     generating 3D density fields and wetness substrates.
//  2. Zero-Copy Vulkan-OpenCL Interop:
//     Manages ExperimentResourceExternalTransport plus the producer-appropriate
//     import/ownership adapter (generic or Sprinkler mist), scheduling Vulkan
//     binary-semaphore release/OpenCL-ready transitions without creating a
//     second scientific dispatcher.
//  3. Fault-Tolerant Fixed-Rate Accumulator:
//     Decouples variable frame deltas from physics step rates (e.g. 30 Hz),
//     capping accumulator lag to prevent the "spiral of death".
//  4. Acoustic Resonance & Mie Scattering Optics:
//     Modulates nozzle kinematics, rainbow dispersion, and solar radiance based on
//     real-time audio resonance metrics (Kuramoto phase entrainment, acoustic energy).
//  5. Unified Volumetric Render Submission:
//     Submits the final volumetric mist and substrate wetness passes to mGFX Drawables.
// -----------------------------------------------------------------------------
struct SculptureFluidAtmosphereConfig {
    std::uint32_t mistAxis = 128U;
    float fixedPhysicsTickRate = 1.0f / 30.0f;
    float maxAccumulatorCap = 0.25f;
    std::uint32_t maxSubStepsPerFrame = 4U;
    glm::vec3 rootScale = glm::vec3(6.0f);
    glm::vec3 rootTranslation = glm::vec3(0.0f);
    bool autoKinematicNozzle = true;
    bool enabled = true;
};

class SculptureFluidAtmosphereManager {
public:
    using Config = SculptureFluidAtmosphereConfig;

    explicit SculptureFluidAtmosphereManager(
        mgfx::Engine* engine,
        Config config = Config{},
        std::unique_ptr<IExperimentExternalProducer> customProducer = nullptr
    );

    SculptureFluidAtmosphereManager(
        mgfx::Engine& engine,
        Config config = Config{},
        std::unique_ptr<IExperimentExternalProducer> customProducer = nullptr
    ) : SculptureFluidAtmosphereManager(&engine, std::move(config), std::move(customProducer)) {}

    ~SculptureFluidAtmosphereManager();

    // Simulation lifecycle & frame update
    void update(
        float deltaSeconds,
        float simulatedTime,
        const AudioResonanceMetrics& metrics,
        const glm::vec3& lightDir = glm::vec3(0.3f, -0.7f, -0.6f)
    );

    // Volumetric and substrate render submission into mGFX
    void render(
        mgfx::Drawable& draw,
        const glm::mat4& parentTransform = glm::mat4(1.0f)
    );

    // Dynamic nozzle position control
    void setNozzlePosition(float x, float y, float z) noexcept;

    // Enable / Disable toggling
    void setEnabled(bool enabled) noexcept { config_.enabled = enabled; }
    [[nodiscard]] bool isEnabled() const noexcept { return config_.enabled; }

    // Diagnostic & state queries
    [[nodiscard]] bool isHardwareAccelerated() const noexcept { return transport_ != nullptr; }
    [[nodiscard]] std::uint64_t physicsStepCount() const noexcept {
        return stepController_.nextStep();
    }
    [[nodiscard]] std::uint32_t mistAxis() const noexcept { return config_.mistAxis; }
    [[nodiscard]] const Config& config() const noexcept { return config_; }
    [[nodiscard]] const ExperimentMistOpticsConfig& latestOptics() const noexcept { return latestOptics_; }

    [[nodiscard]] IExperimentExternalProducer* producer() noexcept { return producer_.get(); }
    [[nodiscard]] const IExperimentExternalProducer* producer() const noexcept { return producer_.get(); }

private:
    mgfx::Engine* engine_ = nullptr;
    Config config_{};
    std::unique_ptr<IExperimentExternalProducer> producer_;
    std::unique_ptr<ExperimentResourceExternalTransport> transport_;
    std::unique_ptr<ExperimentGenericExternalComputeRing> ring_;
    // Sprinkler needs per-step particle inputs for mist deposition, so it uses
    // the specialized ring that borrows its authoritative mist dispatcher.
    // Other external producers keep the generic no-argument dispatcher ring.
    std::unique_ptr<ExperimentResourceExternalComputeRing> sprinklerRing_;
    std::unique_ptr<SculptureExperimentRenderBridge> renderBridge_;
    // A host delivery is borrowed in update and consumed exactly once during
    // render, where the Vulkan wait/signal transaction can actually be queued.
    std::optional<GenericExternalDelivery> pendingDelivery_;
    std::uint64_t scheduledExternalGeneration_ = 0U;

    // Advances only after the producer accepts a complete fixed step. This is
    // distinct from render frames and from update attempts blocked by transport.
    ExperimentStepController stepController_{};
    float physicsAccumulator_ = 0.0f;
    ExperimentMistOpticsConfig latestOptics_{};
};

} // namespace epi_sculpt
