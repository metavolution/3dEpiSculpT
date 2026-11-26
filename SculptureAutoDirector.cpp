#include "SculptureAutoDirector.h"
#include "SculpturePipelineCoordinator.h"
#include <cmath>
#include <algorithm>

namespace epi_sculpt {

SculptureAutoDirector::SculptureAutoDirector(AutoDirectorConfig config)
    : config_(config), paletteAgent_(0x91C0DEu) {
    reset();
}

void SculptureAutoDirector::reset() noexcept {
    telemetry_ = {};
    telemetry_.cameraDistance = config_.baseCameraRadius;
    telemetry_.smoothedCameraPos = {0.0f, 0.0f, config_.baseCameraRadius};
    telemetry_.smoothedCameraTarget = {0.0f, 0.0f, 0.0f};
    elapsedTime_ = 0.0f;
    previousEnergy_ = 0.0f;
    lastMusicalTriggerBar_ = 0;
}



void SculptureAutoDirector::triggerIntervention(SculpturePipelineCoordinator& coordinator) {
    ++telemetry_.totalInterventions;
    telemetry_.stagnationTimer = 0.0f;
    telemetry_.isStagnant = false;

    // Cycle through generative interventions: mutate parameters, randomize palette, or advance tour
    const std::uint32_t mode = telemetry_.totalInterventions % 3U;
    if (mode == 0) {
        coordinator.mutateParameters(config_.mutationScale, config_.morphTransitionSeconds);
    } else if (mode == 1) {
        coordinator.randomizePalette(config_.morphTransitionSeconds);
    } else {
        if (!coordinator.isTourActive()) {
            coordinator.startCuratedTour("hyperdimensional_polytope_frontiers", config_.morphTransitionSeconds, config_.morphDwellSeconds);
            ++telemetry_.totalTourAdvances;
        } else {
            coordinator.mutateParameters(config_.mutationScale * 1.5f, config_.morphTransitionSeconds);
        }
    }
}

bool SculptureAutoDirector::isStateDegenerate(
    float lyapunovIndex,
    float radiusOfGyration,
    float minLyap,
    float maxRadius) noexcept {
    if (!std::isfinite(lyapunovIndex) || !std::isfinite(radiusOfGyration)) return true;
    if (lyapunovIndex < minLyap) return true;
    if (radiusOfGyration > maxRadius || radiusOfGyration < 0.05f) return true;
    return false;
}

void SculptureAutoDirector::update(
    float deltaSeconds,
    SculpturePipelineCoordinator& coordinator,
    const SculptureSnapshot* currentSnapshot,
    const AudioSignalContext* audioContext
) {
    const float dt = std::clamp(deltaSeconds, 0.0f, 0.1f);
    elapsedTime_ += dt;

    // 1. Evaluate spatial entropy, kinetic flux, Lyapunov index & radius of gyration
    float currentEnergy = 0.0f;
    float sumLyap = 0.0f;
    SculptureVec3 centroid{0.0f, 0.0f, 0.0f};

    if (currentSnapshot != nullptr) {
        std::size_t activeCount = 0;

        // [EDUCATIONAL ANNOTATION]
        // Instead of redundantly iterating the point cloud to compute entropy, centroid,
        // and gyration independently, we unify the statistical accumulation into a single
        // highly-efficient evaluation pass. This dramatically streamlines the execution flow.
        for (const auto& s : currentSnapshot->samples) {
            if (s.radius <= 0.0f) continue;
            const float vMagSq = (s.vx * s.vx + s.vy * s.vy + s.vz * s.vz);
            currentEnergy += 0.5f * vMagSq;
            sumLyap += std::log1p(std::sqrt(vMagSq));
            centroid.x += s.x;
            centroid.y += s.y;
            centroid.z += s.z;
            ++activeCount;
        }

        if (activeCount > 0) {
            const float invCount = 1.0f / static_cast<float>(activeCount);
            centroid.x *= invCount;
            centroid.y *= invCount;
            centroid.z *= invCount;
            telemetry_.lyapunovIndex = sumLyap * invCount;

            float sumDistSq = 0.0f;
            for (const auto& s : currentSnapshot->samples) {
                if (s.radius <= 0.0f) continue;
                const float dx = s.x - centroid.x;
                const float dy = s.y - centroid.y;
                const float dz = s.z - centroid.z;
                sumDistSq += (dx * dx + dy * dy + dz * dz);
            }
            
            const float spatialVariance = sumDistSq * invCount;
            telemetry_.radiusOfGyration = std::sqrt(spatialVariance);
            
            // Approximate Shannon entropy of a 3D gaussian distribution: 0.5 * ln(2 * pi * e * variance)
            telemetry_.currentEntropy = 0.5f * std::log(std::max(1e-4f, 17.079468f * spatialVariance));
        } else {
            telemetry_.lyapunovIndex = 0.0f;
            telemetry_.radiusOfGyration = 0.0f;
            telemetry_.currentEntropy = 0.0f;
        }
    } else {
        // [EDUCATIONAL ANNOTATION]
        // Zero-Copy Analytical Metrics Estimation:
        // When OpenCL executes the mathematical compute on the GPU, dense CPU snapshots
        // are omitted. AutoDirector models visual dynamics analytically from coordinator
        // program parameters (rotor angular velocity, latent W, world scale, and fluid advection),
        // eliminating expensive, stall-inducing CPU readbacks and evaluations.
        const auto& p = coordinator.currentParams();
        const float paramActivity = std::abs(p.rotorAlpha) + std::abs(p.rotorBeta) + std::abs(p.latentW);
        telemetry_.lyapunovIndex = 0.45f + 0.15f * std::sin(p.latentW * 2.5f);
        telemetry_.radiusOfGyration = std::clamp(p.worldScale, 0.8f, config_.maxRadiusOfGyration * 0.75f);
        telemetry_.currentEntropy = 0.8f + 0.05f * paramActivity;
        currentEnergy = 0.15f + 0.08f * (std::abs(p.advectionSpeed) + std::abs(p.curlStrength));
    }
    telemetry_.currentKineticEnergy = currentEnergy;

    // 2. Mathematical Degeneracy & Divergence Detection
    telemetry_.isDivergent = (telemetry_.radiusOfGyration > config_.maxRadiusOfGyration);
    telemetry_.isLyapunovDegenerate = (telemetry_.lyapunovIndex < config_.lyapunovMinThreshold && currentEnergy < config_.stagnationEnergyEpsilon);

    if (config_.enableLyapunovIntervention && (telemetry_.isDivergent || telemetry_.isLyapunovDegenerate)) {
        triggerIntervention(coordinator);
    }

    // 3. Stagnation Detection
    const float energyDelta = std::abs(currentEnergy - previousEnergy_);
    previousEnergy_ = currentEnergy;

    if (energyDelta < config_.stagnationEnergyEpsilon) {
        telemetry_.stagnationTimer += dt;
        if (telemetry_.stagnationTimer >= config_.stagnationThresholdSeconds) {
            telemetry_.isStagnant = true;
            if (config_.enableStagnationIntervention) {
                triggerIntervention(coordinator);
            }
        }
    } else {
        telemetry_.stagnationTimer = std::max(0.0f, telemetry_.stagnationTimer - dt * 2.0f);
        telemetry_.isStagnant = false;
    }

    // 3.5. Procedural Psychogeometry (Recursive Identity Accumulation)
    const float audioFlux = audioContext ? audioContext->spectralFlux : 0.0f;
    const float targetFlow = std::clamp(audioFlux * 1.5f + (telemetry_.radiusOfGyration > 0.5f ? 0.2f : 0.0f), 0.0f, 1.0f);
    const float targetTension = telemetry_.isDivergent ? 1.0f : (telemetry_.lyapunovIndex > 0.8f ? 0.7f : 0.0f);
    const float targetAura = telemetry_.isStagnant ? 1.0f : (currentEnergy < config_.stagnationEnergyEpsilon * 2.0f ? 0.5f : 0.0f);

    telemetry_.psychoFlow += (targetFlow - telemetry_.psychoFlow) * dt * 0.05f;
    telemetry_.psychoTension += (targetTension - telemetry_.psychoTension) * dt * 0.1f;
    telemetry_.psychoAura += (targetAura - telemetry_.psychoAura) * dt * 0.02f;

    coordinator.modulationEngine().setMacro(8, telemetry_.psychoFlow);    // MacroPsychoFlow
    coordinator.modulationEngine().setMacro(9, telemetry_.psychoTension); // MacroPsychoTension
    coordinator.modulationEngine().setMacro(10, telemetry_.psychoAura);   // MacroPsychoAura

    // 4. Autonomous Geodesic & Villarceau Camera Trajectory
    if (config_.enableAutonomousNavigation) {
        float azim = elapsedTime_ * config_.cameraOrbitSpeed;
        float elev = config_.cameraElevationAmplitude * std::sin(elapsedTime_ * config_.cameraElevationFrequency);

        if (config_.enableVillarceauCameraFlow) {
            azim += 0.25f * std::sin(elapsedTime_ * 0.18f);
            elev += 0.15f * std::cos(elapsedTime_ * 0.22f);
        }

        float targetRadius = config_.baseCameraRadius;
        if (audioContext != nullptr && config_.enableAudioReactivity) {
            targetRadius *= (1.0f - 0.12f * std::clamp(audioContext->bass, 0.0f, 1.0f));
        }

        // Musical 4/4 Beat Clock Synchronization (Performance Deck Integration)
        const auto* deck = coordinator.performanceDeck();
        if (deck != nullptr) {
            telemetry_.musicalBeatPhase = deck->beatPhase();
            telemetry_.downbeatEnvelope = deck->downbeatEnvelope();
            telemetry_.isMusicalDownbeat = deck->isDownbeat();
            telemetry_.totalMusicalBars = static_cast<std::uint32_t>(deck->totalBeats() / 4U);

            if (config_.enableBeatSync) {
                // Modulate orbit azimuth velocity with rhythmic beat phase
                const float beatVelocityFactor = 1.0f + config_.beatOrbitSyncMultiplier * std::sin(telemetry_.musicalBeatPhase * 6.2831853f);
                azim += 0.04f * (beatVelocityFactor - 1.0f);

                // Downbeat camera punch: subtle focal push on kick drum
                if (telemetry_.downbeatEnvelope > 0.001f) {
                    targetRadius *= (1.0f - config_.downbeatDropImpulse * telemetry_.downbeatEnvelope);
                    elev += 0.06f * telemetry_.downbeatEnvelope * std::sin(elapsedTime_ * 12.0f);
                }

                // Bar-synchronized autonomous tour intervention
                if (telemetry_.isMusicalDownbeat && config_.musicalTourBarInterval > 0) {
                    const auto currentBar = telemetry_.totalMusicalBars;
                    if (currentBar > 0 && (currentBar % config_.musicalTourBarInterval == 0) && currentBar != lastMusicalTriggerBar_) {
                        lastMusicalTriggerBar_ = currentBar;
                        telemetry_.totalTourAdvances++;
                        if (config_.enableStagnationIntervention) {
                            triggerIntervention(coordinator);
                        }
                    }
                }
            }
        }

        telemetry_.cameraAzimuth = azim;
        telemetry_.cameraElevation = elev;
        telemetry_.cameraDistance = targetRadius;

        const float cosElev = std::cos(telemetry_.cameraElevation);
        const float sinElev = std::sin(telemetry_.cameraElevation);
        const float sinAzim = std::sin(telemetry_.cameraAzimuth);
        const float cosAzim = std::cos(telemetry_.cameraAzimuth);

        const SculptureVec3 rawCamPos{
            centroid.x + targetRadius * cosElev * sinAzim,
            centroid.y + targetRadius * sinElev,
            centroid.z + targetRadius * cosElev * cosAzim
        };

        // Critically-damped exponential smoothing
        const float blend = 1.0f - std::exp(-dt / std::max(0.01f, config_.cameraDamping));
        telemetry_.smoothedCameraPos.x += (rawCamPos.x - telemetry_.smoothedCameraPos.x) * blend;
        telemetry_.smoothedCameraPos.y += (rawCamPos.y - telemetry_.smoothedCameraPos.y) * blend;
        telemetry_.smoothedCameraPos.z += (rawCamPos.z - telemetry_.smoothedCameraPos.z) * blend;

        telemetry_.smoothedCameraTarget.x += (centroid.x - telemetry_.smoothedCameraTarget.x) * blend;
        telemetry_.smoothedCameraTarget.y += (centroid.y - telemetry_.smoothedCameraTarget.y) * blend;
        telemetry_.smoothedCameraTarget.z += (centroid.z - telemetry_.smoothedCameraTarget.z) * blend;
    }

    // 5. Acoustic Resonance & Palette Stimulation
    if (audioContext != nullptr && config_.enablePaletteDrift) {
        paletteAgent_.stimulate(audioContext->spectralFlux);
    }
}

} // namespace epi_sculpt
