#pragma once

#include "SculptureProgramSchema.h"
#include "SculpturePaletteAgent.h"
#include "SculptureAudioResonance.h"
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: AUTONOMOUS SCULPTURAL DIRECTOR & GEODESIC NAVIGATOR
// -----------------------------------------------------------------------------
// This module provides an intelligent, hands-free generative exhibition director.
// It monitors spatial entropy, kinetic energy flux, and acoustic resonance to:
//  1. Detect visual stagnation when the sculpture settles into a static attractor.
//  2. Guide camera trajectories smoothly along geodesic orbital splines.
//  3. Autonomously orchestrate morph transitions, mutations, and palette drifts.
// -----------------------------------------------------------------------------

struct AutoDirectorConfig {
    bool enableAutonomousNavigation = true;
    bool enableStagnationIntervention = true;
    bool enableLyapunovIntervention = true;
    bool enableVillarceauCameraFlow = true;
    bool enablePaletteDrift = true;
    bool enableAudioReactivity = true;

    float stagnationThresholdSeconds = 8.0f;
    float stagnationEnergyEpsilon = 0.005f;
    float lyapunovMinThreshold = 0.04f;
    float lyapunovMaxThreshold = 2.8f;
    float maxRadiusOfGyration = 3.5f;
    float baseCameraRadius = 4.2f;
    float cameraOrbitSpeed = 0.08f;
    float cameraElevationFrequency = 0.05f;
    float cameraElevationAmplitude = 0.8f;
    float cameraDamping = 0.12f;
    float mutationScale = 0.18f;
    float morphTransitionSeconds = 3.5f;
    float morphDwellSeconds = 6.0f;

    // Musical 4/4 Beat Clock Synchronization (Performance Deck Integration)
    bool enableBeatSync = true;
    float beatOrbitSyncMultiplier = 0.35f;
    float downbeatDropImpulse = 0.25f;
    std::uint32_t musicalTourBarInterval = 16U;
};

struct AutoDirectorTelemetry {
    float currentEntropy = 0.0f;
    float currentKineticEnergy = 0.0f;
    float lyapunovIndex = 0.0f;
    float radiusOfGyration = 0.0f;
    float stagnationTimer = 0.0f;
    bool isStagnant = false;
    bool isDivergent = false;
    bool isLyapunovDegenerate = false;
    std::uint32_t totalInterventions = 0U;
    std::uint32_t totalTourAdvances = 0U;
    float cameraAzimuth = 0.0f;
    float cameraElevation = 0.0f;
    float cameraDistance = 4.2f;
    SculptureVec3 smoothedCameraPos{0.0f, 0.0f, 4.2f};
    SculptureVec3 smoothedCameraTarget{0.0f, 0.0f, 0.0f};

    // Procedural Psychogeometry State
    float psychoFlow = 0.0f;
    float psychoTension = 0.0f;
    float psychoAura = 0.0f;

    // Musical Beat Clock Synchronization Telemetry
    float musicalBeatPhase = 0.0f;
    float downbeatEnvelope = 0.0f;
    bool isMusicalDownbeat = false;
    std::uint32_t totalMusicalBars = 0U;
};

class SculptureAutoDirector {
public:
    explicit SculptureAutoDirector(AutoDirectorConfig config = {});

    [[nodiscard]] const AutoDirectorConfig& config() const noexcept { return config_; }
    void setConfig(const AutoDirectorConfig& config) noexcept { config_ = config; }

    [[nodiscard]] const AutoDirectorTelemetry& telemetry() const noexcept { return telemetry_; }

    // Resets internal state, timers, and camera positions
    void reset() noexcept;

    // Evaluates autonomous director logic for a frame
    void update(
        float deltaSeconds,
        class SculpturePipelineCoordinator& coordinator,
        const SculptureSnapshot* currentSnapshot = nullptr,
        const AudioSignalContext* audioContext = nullptr
    );

    // Forces immediate autonomous intervention (e.g. on external trigger)
    void triggerIntervention(class SculpturePipelineCoordinator& coordinator);

    // Evaluates mathematical stability (Lyapunov exponent & radius of gyration)
    [[nodiscard]] static bool isStateDegenerate(
        float lyapunovIndex,
        float radiusOfGyration,
        float minLyap = 0.04f,
        float maxRadius = 3.5f) noexcept;

    // Computes the current smoothed camera position and target
    [[nodiscard]] SculptureVec3 cameraPosition() const noexcept { return telemetry_.smoothedCameraPos; }
    [[nodiscard]] SculptureVec3 cameraTarget() const noexcept { return telemetry_.smoothedCameraTarget; }
    [[nodiscard]] SculptureVec3 cameraUp() const noexcept { return {0.0f, 1.0f, 0.0f}; }

private:
    AutoDirectorConfig config_;
    AutoDirectorTelemetry telemetry_;
    SculpturePaletteAgent paletteAgent_;
    float elapsedTime_ = 0.0f;
    float previousEnergy_ = 0.0f;
    std::uint32_t lastMusicalTriggerBar_ = 0;
};

} // namespace epi_sculpt
