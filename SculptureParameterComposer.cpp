#include "SculptureParameterComposer.h"
#include "SculptureMorphChoreographer.h"
#include "SculpturePresets.h"
#include "SculptureMacroAutomation.h"
#include "SculpturePerformanceDeck.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace epi_sculpt {

float SculptureParameterComposer::normalizeRotorAngle(float radians) noexcept {
    if (!std::isfinite(radians)) {
        return 0.0f;
    }
    float wrapped = std::fmod(radians, kTwoPi);
    if (wrapped < 0.0f) {
        wrapped += kTwoPi;
    }
    return wrapped;
}

float SculptureParameterComposer::clampLatentW(
    float w,
    float minW,
    float maxW) noexcept {
    if (!std::isfinite(w)) {
        return 0.0f;
    }
    return std::clamp(w, minW, maxW);
}

void SculptureParameterComposer::reset4DRotors(SculptureProgramParams& params) noexcept {
    params.latentW = 0.0f;
    params.rotorAlpha = 0.5f;
    params.rotorBeta = 0.5f;
    params.rotorTheta4D = 0.0f;
}

SculptureProgramParams SculptureParameterComposer::composeMasterProgramParams(
    const SculptureMorphChoreographer& choreographer,
    const SculptureMultiTrackChoreographer* multiTrack,
    bool multiTrackEnabled,
    const SculpturePerformanceDeck* performanceDeck,
    bool performanceDeckEnabled,
    std::uint32_t stageMask) noexcept {
    SculptureProgramParams params = choreographer.currentParams();
    if (multiTrackEnabled && multiTrack != nullptr) {
        params = multiTrack->composeProgramParams(params);
    }
    if (performanceDeckEnabled && performanceDeck != nullptr) {
        params = performanceDeck->composeMasterParams();
        applyDownbeatKickModulation(params, performanceDeck->downbeatEnvelope());
    }
    params.stageMask = stageMask;
    return params;
}

void SculptureParameterComposer::stepKuramotoFoliationWave4D(
    SculptureProgramParams& params,
    const Foliation4DConfig& foliationConfig,
    float simulatedTime,
    float deltaSeconds,
    float couplingK,
    float audioDrive) noexcept {
    if (params.enable4D == 0U) {
        return;
    }

    const std::vector<SculptureVec3> probe = {{0.0f, 0.0f, 0.0f}};
    const auto wOffsets = Sculpture4DFoliation::evaluateKuramotoFoliationWaveField(
        probe, simulatedTime, couplingK, 2.0f);

    if (!wOffsets.empty()) {
        const float waveMod = wOffsets[0];
        params.latentW = clampLatentW(params.latentW + waveMod * deltaSeconds * 0.5f, -2.0f, 2.0f);
    }

    evaluateContinuousRotorKinematics(
        deltaSeconds,
        foliationConfig.rotationLeftSpeed,
        foliationConfig.rotationRightSpeed,
        audioDrive,
        1.0f,
        params.rotorAlpha,
        params.rotorBeta,
        params.rotorTheta4D,
        params.latentW,
        simulatedTime);
}

void SculptureParameterComposer::sanitizeProgramParams(SculptureProgramParams& params) noexcept {
    auto sanitize = [](float& val, float fallback, float minVal, float maxVal) noexcept {
        if (!std::isfinite(val)) {
            val = fallback;
        } else {
            val = std::clamp(val, minVal, maxVal);
        }
    };
    sanitize(params.pre.scaleReal, 1.0f, -20.0f, 20.0f);
    sanitize(params.pre.scaleImag, 0.0f, -20.0f, 20.0f);
    sanitize(params.pre.offsetReal, 0.0f, -20.0f, 20.0f);
    sanitize(params.pre.offsetImag, 0.0f, -20.0f, 20.0f);
    sanitize(params.post.scaleReal, 1.0f, -20.0f, 20.0f);
    sanitize(params.post.scaleImag, 0.0f, -20.0f, 20.0f);
    sanitize(params.post.offsetReal, 0.0f, -20.0f, 20.0f);
    sanitize(params.post.offsetImag, 0.0f, -20.0f, 20.0f);
    sanitize(params.patternSpeed, 0.075f, -20.0f, 20.0f);
    sanitize(params.latentW, 0.0f, -4.0f, 4.0f);
    sanitize(params.topologyBlend, 0.0f, 0.0f, 1.0f);
    sanitize(params.rotorAlpha, 0.0f, -62.83f, 62.83f);
    sanitize(params.rotorBeta, 0.0f, -62.83f, 62.83f);
    sanitize(params.rotorTheta4D, 0.0f, -62.83f, 62.83f);
    sanitize(params.attractorStrength, 0.0f, 0.0f, 10.0f);
    sanitize(params.seedWidth, 0.72f, 0.01f, 20.0f);
    sanitize(params.seedAlpha, 0.30f, 0.0f, 1.0f);
    sanitize(params.seedBaseline, 0.055f, 0.0f, 5.0f);
    sanitize(params.seedPulseFrequency, 0.19f, 0.0f, 50.0f);
    sanitize(params.seedPulseAmplitude, 0.35f, 0.0f, 5.0f);
    sanitize(params.hueRotation, 0.04f, -100.0f, 100.0f);
    sanitize(params.hueSpeed, 0.035f, -50.0f, 50.0f);
    sanitize(params.worldScale, 1.0f, 0.1f, 10.0f);
    sanitize(params.radiusScale, 1.0f, 0.1f, 10.0f);
    sanitize(params.chromaBreath, 0.0f, -5.0f, 5.0f);
    sanitize(params.lumaDrift, 0.0f, -5.0f, 5.0f);
    sanitize(params.paletteBlend, 0.0f, 0.0f, 1.0f);
    sanitize(params.volumetricDensityScale, 0.65f, 0.0f, 10.0f);
    sanitize(params.fogAnisotropy, 0.45f, -0.99f, 0.99f);
    sanitize(params.advectionSpeed, 0.0f, 0.0f, 20.0f);
    sanitize(params.curlStrength, 0.0f, 0.0f, 20.0f);
    sanitize(params.trailDecay, 0.95f, 0.0f, 1.0f);
    sanitize(params.kineticViscosity, 0.1f, 0.0f, 10.0f);
    sanitize(params.filmThickness, 480.0f, 50.0f, 2000.0f);
    sanitize(params.filmIOR, 1.45f, 1.0f, 3.5f);
    sanitize(params.temporalFeedbackBlend, 0.0f, 0.0f, 0.99f);
    sanitize(params.temporalFade, 0.98f, 0.01f, 1.0f);
}

} // namespace epi_sculpt
