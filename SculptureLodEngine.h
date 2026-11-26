#pragma once

#include "SculptureProgramSchema.h"
#include "SculptureIsosurface.h"
#include "SculptureRaymarcher.h"
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <algorithm>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: DYNAMIC DISTANCE-ADAPTIVE LEVEL-OF-DETAIL ENGINE
// -----------------------------------------------------------------------------
// Automatically modulates geometric density, instance counts, Marching Cubes
// voxel grids, and volumetric raymarching step budgets based on camera distance,
// bounding sphere projection, and screen-space footprint.
//
// 1. Instance Decimation & Density-Preserving Radius Compensation:
//    - As sample stride increases (16K -> 8K -> 4K instances), particle radii
//      are scaled by sqrt(stride) to maintain constant optical cross-section
//      and integrated luminance at a distance.
//
// 2. Adaptive Isosurface Voxel Downscaling:
//    - Automatically scales Marching Cubes grid resolution from full detail
//      to low-polygon proxies as distance increases, preventing GPU fill-rate
//      and vertex queue stalls.
//
// 3. Raymarching Step Budget Scaling:
//    - Proactively reduces sphere-tracing and volumetric integration step counts
//      for distant sculptures where sub-pixel details are unresolvable.
// -----------------------------------------------------------------------------

struct LodEvaluationResult {
    SculptureLodTier tier = SculptureLodTier::LOD0_Full;
    float distance = 0.0f;
    float projectedRadius = 0.0f;
    std::size_t sampleStride = 1U;
    float radiusCompScale = 1.0f;
    bool isVisible = true;
};

class SculptureLodEngine {
public:
    // Evaluates the current LOD tier and metrics for a given bounding volume and camera position
    [[nodiscard]] static LodEvaluationResult evaluateLod(
        const SculptureBoundingVolume& bounds,
        const SculptureVec3& cameraPos,
        const SculptureLodConfig& config
    ) noexcept;

    // Decimates a snapshot's samples according to LOD tier while compensating radii
    [[nodiscard]] static std::vector<SculptureSample> decimateSamples(
        const std::vector<SculptureSample>& inSamples,
        const LodEvaluationResult& lod
    );

    // Calculates adaptive Marching Cubes resolution based on LOD tier
    [[nodiscard]] static std::uint32_t calculateAdaptiveIsosurfaceResolution(
        std::uint32_t baseResolution,
        SculptureLodTier tier
    ) noexcept;

    // Adapts raymarching configuration for distant views
    [[nodiscard]] static SculptureRaymarchConfig calculateAdaptiveRaymarchConfig(
        const SculptureRaymarchConfig& baseConfig,
        SculptureLodTier tier
    ) noexcept;
};

// -----------------------------------------------------------------------------
// Educational Annotation: CLOSED-LOOP ADAPTIVE PERFORMANCE GOVERNOR
// -----------------------------------------------------------------------------
// Dynamically reconciles live frame-time telemetry with distance-based LOD tiers.
// If GPU frame-times exceed the 16.6ms (60 FPS) or 8.3ms (120 FPS) budget, the
// governor throttles sample density, seed resolution, and marching cubes detail
// to maintain target framerates. Once performance stabilizes below budget, detail
// is smoothly restored with hysteresis cooldown to eliminate visual thrashing.
// -----------------------------------------------------------------------------

struct LodGovernorConfig {
    float targetFrameTimeMs = 16.6667f;    // 60 FPS baseline (8.333ms for 120 FPS)
    float throttleThresholdFactor = 1.15f; // Throttle down when frameTime > 1.15 * target
    float promoteThresholdFactor = 0.80f;  // Promote quality when frameTime < 0.80 * target
    std::uint32_t cooldownFrames = 30U;    // Minimum frames between transitions to prevent oscillation
    bool enableTelemetryDrivenLod = true;  // Use live frame time feedback
    bool enableDistanceDrivenLod = true;   // Use camera distance & screen-space projected bounds
};

struct LodGovernorDecision {
    SculptureLodTier tier = SculptureLodTier::LOD0_Full;
    StageResolutionScale seedResolution = StageResolutionScale::Full;
    std::size_t sampleStride = 1U;
    float radiusCompensation = 1.0f;
    std::uint32_t marchingCubesGridResolution = 64U;
    std::uint32_t raymarchMaxSteps = 64U;
    bool isThrottled = false;
    const char* reason = "Nominal";
};

class SculptureLodGovernor {
public:
    SculptureLodGovernor() noexcept = default;
    explicit SculptureLodGovernor(const LodGovernorConfig& config) noexcept : config_(config) {}

    void setConfig(const LodGovernorConfig& config) noexcept { config_ = config; }
    [[nodiscard]] const LodGovernorConfig& config() const noexcept { return config_; }

    [[nodiscard]] LodGovernorDecision update(
        const SculptureBoundingVolume& bounds,
        const SculptureVec3& cameraPos,
        float currentFrameTimeMs,
        std::uint64_t frameIndex
    ) noexcept;

    void reset() noexcept;
    [[nodiscard]] float smoothedFrameTimeMs() const noexcept { return smoothedFrameTimeMs_; }
    [[nodiscard]] SculptureLodTier currentTier() const noexcept { return currentTier_; }

private:
    LodGovernorConfig config_{};
    SculptureLodTier currentTier_{SculptureLodTier::LOD0_Full};
    std::uint64_t lastTransitionFrame_{0};
    float smoothedFrameTimeMs_{16.6667f};
};

} // namespace epi_sculpt
