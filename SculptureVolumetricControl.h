#pragma once

#include "SculptureEventBus.h"
#include "SculptureVolumetricMemory.h"

#include <algorithm>
#include <cmath>

namespace epi_sculpt {

// Event policy belongs at the adapter boundary, not in the field's math owner.
// Both hosts use this translation; subscription, action observers and optional
// audio effects remain with their hosts. Nothing here schedules a volume step.
inline void applyVolumetricControl(
    SculptureVolumetricMemory& volume,
    const EventVolumetricControl& event
) {
    using Action = EventVolumetricControl::Action;
    auto config = volume.config();
    switch (event.action) {
    case Action::ToggleFeedback:
        config.enableFeedback = !config.enableFeedback;
        break;
    case Action::ToggleTorusWrap:
        config.enableTorusWrap = !config.enableTorusWrap;
        break;
    case Action::ToggleWcaSeed:
        config.enableWcaSeed = !config.enableWcaSeed;
        break;
    case Action::ToggleAcousticCoupling:
        config.enableAcousticCoupling = !config.enableAcousticCoupling;
        break;
    case Action::SetDecayFactor:
        // clamp(NaN, lo, hi) is still NaN. Reject before publishing a descriptor;
        // finite UI requests retain their established artistic control range.
        if (!std::isfinite(event.decayFactor)) return;
        config.decayFactor = std::clamp(event.decayFactor, 0.5f, 0.999f);
        break;
    case Action::CycleResolution: {
        StageResolutionScale next = StageResolutionScale::Half;
        switch (volume.currentResolutionScale()) {
        case StageResolutionScale::Quarter: next = StageResolutionScale::Half; break;
        case StageResolutionScale::Half: next = StageResolutionScale::Full; break;
        case StageResolutionScale::Full: next = StageResolutionScale::Quarter; break;
        default: break;
        }
        volume.applyResolutionScale(next);
        return;
    }
    case Action::SetResolution:
        volume.applyResolutionScale(static_cast<StageResolutionScale>(event.resolutionTier));
        return;
    case Action::ResetMemory:
        volume.reset();
        return;
    case Action::ToggleBackend:
        volume.setGpuEnabled(!volume.isGpuEnabled());
        return;
    default:
        return;
    }
    volume.configure(config);
}

// Observation is derived from the owner after handling, including rejected
// requests. The caller alone decides whether/where to publish this value.
[[nodiscard]] inline EventVolumetricTelemetry makeVolumetricTelemetryEvent(
    const SculptureVolumetricMemory& volume
) noexcept {
    const auto& telemetry = volume.telemetry();
    return EventVolumetricTelemetry{
        .activeDimension = telemetry.activeDimension,
        .activeVoxelCount = telemetry.activeVoxelCount,
        .bufferBytes = telemetry.bufferBytes,
        .stepCount = telemetry.stepCount,
        .simulationTime = telemetry.simulationTime,
        .meanDensity = telemetry.meanDensity,
        .peakEnergy = telemetry.peakEnergy,
        .macroOccupancyRatio = volume.macroOccupancyRatio(),
        .resolutionTier = static_cast<std::uint8_t>(volume.currentResolutionScale()),
        .feedbackEnabled = volume.config().enableFeedback,
        .gpuAccelerated = volume.isGpuActive(),
        .backendName = telemetry.backendName
    };
}

} // namespace epi_sculpt
