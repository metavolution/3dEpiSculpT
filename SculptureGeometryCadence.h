// Bounded-rate geometry preparation for the renderer-neutral sculpture pipeline.
//
// Mesh extraction is substantially more expensive than presenting an already
// prepared mesh. This scheduler preserves responsive rendering while a
// smoothly evolving field refreshes CPU-generated geometry at a controlled
// simulation cadence. Explicit invalidation always wins over the cadence.

#pragma once

#include <algorithm>
#include <cmath>

namespace epi_sculpt {

struct SculptureGeometryCadenceConfig {
    // 30 Hz keeps animated extracted meshes coherent while presentation can
    // continue at the display rate. Bounds avoid accidental freezes or an
    // unbounded per-frame extraction path.
    float maximumUpdatesPerSecond = 30.0f;
};

class SculptureGeometryCadence {
public:
    explicit SculptureGeometryCadence(SculptureGeometryCadenceConfig config = {}) noexcept
        : config_(sanitize(config)) {}

    void setConfig(SculptureGeometryCadenceConfig config) noexcept {
        config_ = sanitize(config);
    }

    [[nodiscard]] const SculptureGeometryCadenceConfig& config() const noexcept {
        return config_;
    }

    void invalidate() noexcept { invalidated_ = true; }

    [[nodiscard]] bool shouldRefresh(float simulationTime, bool externallyDirty) noexcept {
        if (externallyDirty) invalidate();

        // A rewind, reset, or invalid clock must never retain future geometry.
        if (!std::isfinite(simulationTime)
            || !hasSampleTime_
            || simulationTime < lastSampleTime_) {
            lastSampleTime_ = std::isfinite(simulationTime) ? simulationTime : 0.0f;
            hasSampleTime_ = true;
            invalidated_ = false;
            return true;
        }

        if (invalidated_
            || simulationTime - lastSampleTime_ >= reciprocalRate()) {
            lastSampleTime_ = simulationTime;
            invalidated_ = false;
            return true;
        }
        return false;
    }

private:
    [[nodiscard]] static SculptureGeometryCadenceConfig sanitize(
        SculptureGeometryCadenceConfig config) noexcept {
        config.maximumUpdatesPerSecond = std::clamp(
            config.maximumUpdatesPerSecond, 1.0f, 240.0f);
        return config;
    }

    [[nodiscard]] float reciprocalRate() const noexcept {
        return 1.0f / config_.maximumUpdatesPerSecond;
    }

    SculptureGeometryCadenceConfig config_{};
    float lastSampleTime_ = 0.0f;
    bool hasSampleTime_ = false;
    bool invalidated_ = true;
};

} // namespace epi_sculpt
