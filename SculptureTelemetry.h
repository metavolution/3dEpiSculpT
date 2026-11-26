#pragma once

#include "SculptureProgramSchema.h"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace epi_sculpt {

// The stage selector is carried in bits 8..11 of `stageMask`.  Keep its
// human-facing meaning next to the canonical stage metrics so HUD, capture,
// and future remote-control surfaces do not each invent their own labels.
[[nodiscard]] constexpr std::string_view learnModeStageDescription(
    std::uint32_t stageMask) noexcept {
    return learn_mode::diagnosticDescription(learn_mode::diagnostic(stageMask));
}

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * EPI2 - SCULPTURE 6-STAGE TELEMETRY & PHASE METRICS (LEARN MODE PROBE)
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Captures real-time, non-allocating mathematical metrics for each of the 6
 * canonical stages of the epimorphic feedback loop:
 *   Stage 1: Coordinate Transform (Affine + Nonlinear Inner Warp)
 *   Stage 2: Domain Reduction (Torus / Grid / Clamp Folding)
 *   Stage 3: Orbit Feedback Sampling (Recursive Orbit Evolution)
 *   Stage 4: Color / Material Transform (HSLS / Cauchy Dispersion)
 *   Stage 5: WCA Seed Injection (Window -> Weight -> Alpha -> Seed)
 *   Stage 6: Alpha Blend & Feature Shaping (Coupled Lift Snapshot)
 */

struct StageTransformMetrics {
    float contractionRatio = 1.0f;    // |z'| / |z| mean ratio (<1: contracting, >1: expanding)
    float warpDisplacementMean = 0.0f; // Mean displacement from nonlinear inner warp
    float lyapunovExponentEst = 0.0f;  // Estimated local divergence rate ln(|J|)
};

struct StageReductionMetrics {
    float foldFraction = 0.0f;        // Fraction of points folded back into [-1, 1]^3
    float meanFoldedRadius = 0.0f;    // Average radius after domain reduction
};

struct StageOrbitMetrics {
    float orbitCoherence = 1.0f;      // Correlation between consecutive orbit iterations
    float accumulatedEnergy = 0.0f;   // RMS energy across recursive orbit steps
    std::size_t activeIterations = 3; // Evaluated iteration depth
};

struct StageColorMetrics {
    float meanHueShift = 0.0f;        // Average hue angular rotation in radians
    float materialVariance = 0.0f;    // Variance of material coordinates across samples
    float dispersionSpread = 0.0f;    // Wavelength dispersion spread (Cauchy splitting)
};

struct StageSeedMetrics {
    float activeCoverage = 0.0f;      // Fraction of samples receiving positive seed weight
    float injectionPower = 0.0f;      // Injected energy: mean(weight * pulseAlpha)
};

struct StageBlendMetrics {
    float feedbackRetention = 0.0f;   // Mean retention weight (1 - seedAlpha)
    float snapshotRadius = 0.0f;      // Final enclosing bounding sphere radius
    std::size_t finiteSampleCount = 0;// Count of valid, finite output points
};

struct StageMultirateMetrics {
    StageResolutionScale seedScale = StageResolutionScale::Full;
    std::uint32_t seedDivisor = 1U;             // 1x, 2x, 4x
    std::uint32_t auxiliaryVoxelDimension = 50U;// 50, 25, 12
    float effectiveCompressionRatio = 1.0f;     // e.g. 8.0x for Half, 64.0x for Quarter
};

struct StageLodMetrics {
    SculptureLodTier activeTier = SculptureLodTier::LOD0_Full;
    std::size_t lod0Count = 16384U;
    std::size_t lod1Count = 0U;
    std::size_t lod2Count = 0U;
    float lod0Ratio = 1.0f;
    float lod1Ratio = 0.0f;
    float lod2Ratio = 0.0f;
};

struct SculptureStageTelemetry {
    std::uint32_t activeStageMask = 0x3F; // Active stage bitmask
    StageTransformMetrics transform{};
    StageReductionMetrics reduction{};
    StageOrbitMetrics     orbit{};
    StageColorMetrics     color{};
    StageSeedMetrics      seed{};
    StageBlendMetrics     blend{};
    StageMultirateMetrics multirate{};
    StageLodMetrics       lod{};

    float probeExecutionTimeUs = 0.0f; // Microseconds spent in probe computation
};

class SculptureTelemetryProbe {
public:
    // Non-allocating probe evaluating active parameters and sample cloud
    [[nodiscard]] static SculptureStageTelemetry extract(
        const SculptureProgramParams& params,
        const SculptureSnapshot& snapshot,
        float simulationTime = 0.0f
    ) noexcept;

    // Generates a compact human-readable telemetry summary for HUD or diagnostics
    [[nodiscard]] static std::string formatSummary(const SculptureStageTelemetry& telemetry);
};

} // namespace epi_sculpt
