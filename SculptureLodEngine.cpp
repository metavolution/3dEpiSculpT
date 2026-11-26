#include "SculptureLodEngine.h"
#include <cmath>
#include <algorithm>

namespace epi_sculpt {

LodEvaluationResult SculptureLodEngine::evaluateLod(
    const SculptureBoundingVolume& bounds,
    const SculptureVec3& cameraPos,
    const SculptureLodConfig& config
) noexcept {
    LodEvaluationResult res{};

    if (!config.enableDynamicLod) {
        res.tier = SculptureLodTier::LOD0_Full;
        res.sampleStride = 1U;
        res.radiusCompScale = 1.0f;
        res.isVisible = true;
        return res;
    }

    const float dx = bounds.center.x - cameraPos.x;
    const float dy = bounds.center.y - cameraPos.y;
    const float dz = bounds.center.z - cameraPos.z;
    const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    res.distance = dist;

    const float radius = bounds.valid ? std::max(bounds.boundingRadius, 0.1f) : 2.0f;
    res.projectedRadius = radius / std::max(dist, 0.01f);

    res.tier = config.calculateTier(dist);

    switch (res.tier) {
    case SculptureLodTier::LOD0_Full:
        res.sampleStride = 1U;
        res.radiusCompScale = 1.0f;
        res.isVisible = true;
        break;
    case SculptureLodTier::LOD1_Medium:
        res.sampleStride = 2U;
        res.radiusCompScale = 1.41421356f; // sqrt(2) density compensation
        res.isVisible = true;
        break;
    case SculptureLodTier::LOD2_Low:
        res.sampleStride = 4U;
        res.radiusCompScale = 2.0f;        // sqrt(4) density compensation
        res.isVisible = true;
        break;
    case SculptureLodTier::LOD_Culled:
        res.sampleStride = 0U;
        res.radiusCompScale = 0.0f;
        res.isVisible = false;
        break;
    }

    return res;
}

std::vector<SculptureSample> SculptureLodEngine::decimateSamples(
    const std::vector<SculptureSample>& inSamples,
    const LodEvaluationResult& lod
) {
    if (!lod.isVisible || lod.sampleStride == 0U || inSamples.empty()) {
        return {};
    }
    if (lod.sampleStride == 1U) {
        return inSamples;
    }

    std::vector<SculptureSample> out;
    out.reserve(inSamples.size() / lod.sampleStride);

    for (std::size_t i = 0; i < inSamples.size(); i += lod.sampleStride) {
        auto s = inSamples[i];
        s.radius *= lod.radiusCompScale;
        out.push_back(s);
    }

    return out;
}

std::uint32_t SculptureLodEngine::calculateAdaptiveIsosurfaceResolution(
    std::uint32_t baseResolution,
    SculptureLodTier tier
) noexcept {
    switch (tier) {
    case SculptureLodTier::LOD0_Full:
        return baseResolution;
    case SculptureLodTier::LOD1_Medium:
        return std::max(12U, (baseResolution * 3U) / 4U);
    case SculptureLodTier::LOD2_Low:
        return std::max(8U, baseResolution / 2U);
    case SculptureLodTier::LOD_Culled:
        return 0U;
    }
    return baseResolution;
}

SculptureRaymarchConfig SculptureLodEngine::calculateAdaptiveRaymarchConfig(
    const SculptureRaymarchConfig& baseConfig,
    SculptureLodTier tier
) noexcept {
    SculptureRaymarchConfig config = baseConfig;
    switch (tier) {
    case SculptureLodTier::LOD0_Full:
        break;
    case SculptureLodTier::LOD1_Medium:
        config.maxSteps = std::max(32U, baseConfig.maxSteps / 2U);
        config.volumetricSteps = std::max(8U, baseConfig.volumetricSteps / 2U);
        break;
    case SculptureLodTier::LOD2_Low:
        config.maxSteps = std::max(16U, baseConfig.maxSteps / 4U);
        config.volumetricSteps = std::max(4U, baseConfig.volumetricSteps / 4U);
        break;
    case SculptureLodTier::LOD_Culled:
        config.maxSteps = 0U;
        config.volumetricSteps = 0U;
        break;
    }
    return config;
}

void SculptureLodGovernor::reset() noexcept {
    currentTier_ = SculptureLodTier::LOD0_Full;
    lastTransitionFrame_ = 0U;
    smoothedFrameTimeMs_ = config_.targetFrameTimeMs;
}

LodGovernorDecision SculptureLodGovernor::update(
    const SculptureBoundingVolume& bounds,
    const SculptureVec3& cameraPos,
    float currentFrameTimeMs,
    std::uint64_t frameIndex
) noexcept {
    // 1. Exponential Moving Average smoothing of frame time
    if (currentFrameTimeMs > 0.0f) {
        smoothedFrameTimeMs_ = 0.80f * smoothedFrameTimeMs_ + 0.20f * currentFrameTimeMs;
    }

    SculptureLodTier distanceTier = SculptureLodTier::LOD0_Full;
    if (config_.enableDistanceDrivenLod) {
        SculptureLodConfig lodCfg{};
        lodCfg.enableDynamicLod = true;
        auto eval = SculptureLodEngine::evaluateLod(bounds, cameraPos, lodCfg);
        distanceTier = eval.tier;
    }

    LodGovernorDecision decision{};

    // 2. Telemetry-driven adaptation with cooldown hysteresis
    if (config_.enableTelemetryDrivenLod) {
        const float throttleThreshold = config_.targetFrameTimeMs * config_.throttleThresholdFactor;
        const float promoteThreshold = config_.targetFrameTimeMs * config_.promoteThresholdFactor;
        const bool canTransition = (frameIndex >= lastTransitionFrame_ + config_.cooldownFrames);

        if (canTransition) {
            if (smoothedFrameTimeMs_ > throttleThreshold) {
                // Demote quality tier to shed frame load
                if (currentTier_ == SculptureLodTier::LOD0_Full) {
                    currentTier_ = SculptureLodTier::LOD1_Medium;
                    lastTransitionFrame_ = frameIndex;
                    decision.isThrottled = true;
                    decision.reason = "Telemetry: Frame Over-Budget (LOD0->LOD1)";
                } else if (currentTier_ == SculptureLodTier::LOD1_Medium) {
                    currentTier_ = SculptureLodTier::LOD2_Low;
                    lastTransitionFrame_ = frameIndex;
                    decision.isThrottled = true;
                    decision.reason = "Telemetry: Frame Over-Budget (LOD1->LOD2)";
                }
            } else if (smoothedFrameTimeMs_ < promoteThreshold) {
                // Promote quality tier if performance has sufficient headroom and distance allows
                if (currentTier_ == SculptureLodTier::LOD2_Low && distanceTier <= SculptureLodTier::LOD1_Medium) {
                    currentTier_ = SculptureLodTier::LOD1_Medium;
                    lastTransitionFrame_ = frameIndex;
                    decision.reason = "Telemetry: Quality Restored (LOD2->LOD1)";
                } else if (currentTier_ == SculptureLodTier::LOD1_Medium && distanceTier == SculptureLodTier::LOD0_Full) {
                    currentTier_ = SculptureLodTier::LOD0_Full;
                    lastTransitionFrame_ = frameIndex;
                    decision.reason = "Telemetry: Quality Restored (LOD1->LOD0)";
                }
            }
        }
    }

    // Distance tier sets an upper bound on detail (if distance requires LOD2, don't run LOD0)
    if (config_.enableDistanceDrivenLod && static_cast<std::uint8_t>(distanceTier) > static_cast<std::uint8_t>(currentTier_)) {
        decision.tier = distanceTier;
    } else {
        decision.tier = currentTier_;
    }

    decision.isThrottled = (decision.tier > SculptureLodTier::LOD0_Full);

    // 3. Derive downstream execution metrics
    switch (decision.tier) {
    case SculptureLodTier::LOD0_Full:
        decision.seedResolution = StageResolutionScale::Full;
        decision.sampleStride = 1U;
        decision.radiusCompensation = 1.0f;
        decision.marchingCubesGridResolution = 64U;
        decision.raymarchMaxSteps = 64U;
        break;
    case SculptureLodTier::LOD1_Medium:
        decision.seedResolution = StageResolutionScale::Half;
        decision.sampleStride = 2U;
        decision.radiusCompensation = 1.41421356f;
        decision.marchingCubesGridResolution = 48U;
        decision.raymarchMaxSteps = 48U;
        break;
    case SculptureLodTier::LOD2_Low:
        decision.seedResolution = StageResolutionScale::Quarter;
        decision.sampleStride = 4U;
        decision.radiusCompensation = 2.0f;
        decision.marchingCubesGridResolution = 32U;
        decision.raymarchMaxSteps = 32U;
        break;
    case SculptureLodTier::LOD_Culled:
        decision.seedResolution = StageResolutionScale::Eighth;
        decision.sampleStride = 0U;
        decision.radiusCompensation = 0.0f;
        decision.marchingCubesGridResolution = 0U;
        decision.raymarchMaxSteps = 0U;
        decision.reason = "Culled";
        break;
    }

    return decision;
}

} // namespace epi_sculpt
