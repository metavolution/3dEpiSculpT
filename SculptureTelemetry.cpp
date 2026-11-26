#include "SculptureTelemetry.h"
#include <cmath>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace epi_sculpt {

SculptureStageTelemetry SculptureTelemetryProbe::extract(
    const SculptureProgramParams& params,
    const SculptureSnapshot& snapshot,
    float simulationTime
) noexcept {
    const auto startTime = std::chrono::high_resolution_clock::now();
    SculptureStageTelemetry telemetry{};
    telemetry.activeStageMask = learn_mode::normalize(params.stageMask);

    const auto stageMask = telemetry.activeStageMask;
    const bool hasStage1 = learn_mode::enabled(stageMask, learn_mode::Stage::Transform);
    const bool hasStage2 = learn_mode::enabled(stageMask, learn_mode::Stage::Reduce);
    const bool hasStage3 = learn_mode::enabled(stageMask, learn_mode::Stage::FeedbackSample);
    const bool hasStage4 = learn_mode::enabled(stageMask, learn_mode::Stage::Color);
    const bool hasStage5 = learn_mode::enabled(stageMask, learn_mode::Stage::Seed);
    const bool hasStage6 = learn_mode::enabled(stageMask, learn_mode::Stage::Blend);

    // ─────────────────────────────────────────────────────────────────────────
    // Stage 1: Coordinate Transform Metrics
    // ─────────────────────────────────────────────────────────────────────────
    if (hasStage1) {
        const float preMag = std::sqrt(params.pre.scaleReal * params.pre.scaleReal + params.pre.scaleImag * params.pre.scaleImag);
        const float postMag = std::sqrt(params.post.scaleReal * params.post.scaleReal + params.post.scaleImag * params.post.scaleImag);
        telemetry.transform.contractionRatio = preMag * postMag;
        telemetry.transform.warpDisplacementMean = std::abs(preMag - 1.0f) + std::abs(postMag - 1.0f);
        
        const float totalGain = std::max(0.0001f, preMag * postMag);
        telemetry.transform.lyapunovExponentEst = std::log(totalGain);
    } else {
        telemetry.transform.contractionRatio = 0.0f;
        telemetry.transform.warpDisplacementMean = 0.0f;
        telemetry.transform.lyapunovExponentEst = 0.0f;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Stage 2: Domain Reduction Metrics
    // ─────────────────────────────────────────────────────────────────────────
    // ─────────────────────────────────────────────────────────────────────────
    // Stage 3 & 5 Static Parameters
    // ─────────────────────────────────────────────────────────────────────────
    if (hasStage3) {
        telemetry.orbit.activeIterations = params.feedbackIterations;
        telemetry.orbit.accumulatedEnergy = params.temporalFeedbackBlend * params.temporalFade + (1.0f - params.temporalFeedbackBlend);
        telemetry.orbit.orbitCoherence = std::clamp(1.0f - (params.patternSpeed * 0.5f), 0.0f, 1.0f);
    }
    if (hasStage4) {
        telemetry.color.meanHueShift = std::fmod(std::abs(params.hueRotation + params.hueSpeed * simulationTime), 6.2831853f);
        if (params.colorEffect == ColorEffectType::ChromaticDispersion) {
            telemetry.color.dispersionSpread = 0.045f * (params.filmIOR > 1.0f ? params.filmIOR : 1.45f);
        }
    }
    if (hasStage5) {
        const float pulse = 1.0f + params.seedPulseAmplitude * std::sin(params.seedPulseFrequency * simulationTime * 6.2831853f);
        telemetry.seed.injectionPower = params.seedAlpha * std::max(0.0f, pulse);
        telemetry.seed.activeCoverage = std::clamp(params.seedWidth, 0.0f, 1.0f);
    }
    if (hasStage6) {
        telemetry.blend.feedbackRetention = std::clamp(1.0f - params.seedAlpha, 0.0f, 1.0f);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Unified Single-Pass Sample Analysis (Stages 2, 4, 6)
    // ─────────────────────────────────────────────────────────────────────────
    float accumulatedFoldDist = 0.0f;
    std::size_t foldedCount = 0;
    float matSum = 0.0f;
    float matSqSum = 0.0f;
    std::size_t validMat = 0;
    float maxR = 0.0f;
    std::size_t validPoints = 0;
    const float maxDomain = 1.0f;

    for (std::size_t i = 0; i < kSampleCount; ++i) {
        const auto& s = snapshot.samples[i];
        if (!std::isfinite(s.x) || !std::isfinite(s.y) || !std::isfinite(s.z)) continue;
        
        const float rSq = s.x * s.x + s.y * s.y + s.z * s.z;
        const float r = std::sqrt(rSq);

        if (hasStage2) {
            accumulatedFoldDist += r;
            if (std::abs(s.x) > maxDomain || std::abs(s.y) > maxDomain || std::abs(s.z) > maxDomain) {
                foldedCount++;
            }
        }
        if (hasStage4) {
            const float m = 0.299f * s.r + 0.587f * s.g + 0.114f * s.b;
            if (std::isfinite(m)) {
                matSum += m;
                matSqSum += m * m;
                validMat++;
            }
        }
        if (hasStage6) {
            if (r > maxR) maxR = r;
            validPoints++;
        }
    }

    if (hasStage2) {
        telemetry.reduction.foldFraction = static_cast<float>(foldedCount) / static_cast<float>(kSampleCount);
        telemetry.reduction.meanFoldedRadius = accumulatedFoldDist / static_cast<float>(kSampleCount);
    }
    if (hasStage4 && validMat > 0) {
        const float mean = matSum / static_cast<float>(validMat);
        telemetry.color.materialVariance = std::max(0.0f, (matSqSum / static_cast<float>(validMat)) - (mean * mean));
    }
    if (hasStage6) {
        telemetry.blend.snapshotRadius = maxR;
        telemetry.blend.finiteSampleCount = validPoints;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Multirate Staging Metrics
    // ─────────────────────────────────────────────────────────────────────────
    telemetry.multirate.seedScale = params.auxiliaryResolutionScale;
    switch (params.auxiliaryResolutionScale) {
    case StageResolutionScale::Quarter:
        telemetry.multirate.seedDivisor = 4U;
        telemetry.multirate.auxiliaryVoxelDimension = 12U;
        break;
    case StageResolutionScale::Half:
        telemetry.multirate.seedDivisor = 2U;
        telemetry.multirate.auxiliaryVoxelDimension = 25U;
        break;
    case StageResolutionScale::Eighth:
        telemetry.multirate.seedDivisor = 8U;
        telemetry.multirate.auxiliaryVoxelDimension = 6U;
        break;
    case StageResolutionScale::Full:
    default:
        telemetry.multirate.seedDivisor = 1U;
        telemetry.multirate.auxiliaryVoxelDimension = 50U;
        break;
    }
    const float fullVoxelCount = 50.0f * 50.0f * 50.0f;
    const float auxVoxelCount = static_cast<float>(telemetry.multirate.auxiliaryVoxelDimension * 
                                                   telemetry.multirate.auxiliaryVoxelDimension * 
                                                   telemetry.multirate.auxiliaryVoxelDimension);
    telemetry.multirate.effectiveCompressionRatio = fullVoxelCount / std::max(1.0f, auxVoxelCount);

    // ─────────────────────────────────────────────────────────────────────────
    // LOD Metrics
    // ─────────────────────────────────────────────────────────────────────────
    telemetry.lod.activeTier = SculptureLodTier::LOD0_Full;
    telemetry.lod.lod0Count = kSampleCount;
    telemetry.lod.lod1Count = 0U;
    telemetry.lod.lod2Count = 0U;
    telemetry.lod.lod0Ratio = 1.0f;
    telemetry.lod.lod1Ratio = 0.0f;
    telemetry.lod.lod2Ratio = 0.0f;

    const auto endTime = std::chrono::high_resolution_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
    telemetry.probeExecutionTimeUs = static_cast<float>(elapsed) / 1000.0f;

    return telemetry;
}

std::string SculptureTelemetryProbe::formatSummary(const SculptureStageTelemetry& telemetry) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "[LearnMode Stage Telemetry | Mask 0x" << std::hex << telemetry.activeStageMask << std::dec << "]\n";
    ss << "  1. Transform : Contraction=" << telemetry.transform.contractionRatio 
       << ", Lyapunov=" << telemetry.transform.lyapunovExponentEst << "\n";
    ss << "  2. Reduction : FoldFraction=" << telemetry.reduction.foldFraction 
       << ", MeanRadius=" << telemetry.reduction.meanFoldedRadius << "\n";
    ss << "  3. Orbit     : Coherence=" << telemetry.orbit.orbitCoherence 
       << ", Iterations=" << telemetry.orbit.activeIterations << "\n";
    ss << "  4. Color     : MeanHueShift=" << telemetry.color.meanHueShift 
       << ", MatVariance=" << telemetry.color.materialVariance << "\n";
    ss << "  5. Seed      : InjectionPower=" << telemetry.seed.injectionPower 
       << ", Coverage=" << telemetry.seed.activeCoverage << "\n";
    ss << "  6. Blend     : Radius=" << telemetry.blend.snapshotRadius 
       << ", FinitePoints=" << telemetry.blend.finiteSampleCount << "/" << kSampleCount << "\n";
    ss << "  7. Multirate : Scale=" << name(telemetry.multirate.seedScale) 
       << ", Divisor=" << telemetry.multirate.seedDivisor << "x"
       << ", Grid=" << telemetry.multirate.auxiliaryVoxelDimension << "^3"
       << ", Compression=" << telemetry.multirate.effectiveCompressionRatio << "x\n";
    ss << "  8. LOD Stats : Ratios[L0: " << telemetry.lod.lod0Ratio * 100.0f << "%, L1: " 
       << telemetry.lod.lod1Ratio * 100.0f << "%, L2: " << telemetry.lod.lod2Ratio * 100.0f << "%]\n";
    ss << "  Probe Execution Time: " << telemetry.probeExecutionTimeUs << " us";
    return ss.str();
}

} // namespace epi_sculpt
