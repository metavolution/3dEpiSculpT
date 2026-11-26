#include "SculptureMorphChoreographer.h"
#include "SculptureTourTiming.h"
#include "SculpturePresetLibrary.h"
#include "SculptureSerializer.h"
#include <algorithm>
#include <cmath>
#include <fstream>

namespace epi_sculpt {

namespace {

inline float clamp01(float t) noexcept {
    return (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);
}

inline float flerp(float a, float b, float t) noexcept {
    return a + (b - a) * clamp01(t);
}

[[nodiscard]] constexpr bool matchingSelectorBanks(
    const SculptureProgramParams& a, const SculptureProgramParams& b) noexcept {
    return a.inner == b.inner && a.innerB == b.innerB
        && a.reduction == b.reduction && a.reductionB == b.reductionB
        && a.seedType == b.seedType && a.seedTypeB == b.seedTypeB
        && a.outputMode == b.outputMode && a.outputModeB == b.outputModeB
        && a.enable4D == b.enable4D && a.enable4DB == b.enable4DB;
}

} // namespace

SculptureMorphChoreographer::SculptureMorphChoreographer() {
    sourceParams_ = SculptureProgramParams{};
    targetParams_ = SculptureProgramParams{};
    currentParams_ = SculptureProgramParams{};
}

SculptureMorphChoreographer::SculptureMorphChoreographer(const SculptureProgramParams& initialParams)
    : sourceParams_(initialParams)
    , targetParams_(initialParams)
    , currentParams_(initialParams)
    , progress_(1.0f)
    , active_(false) {}

void SculptureMorphChoreographer::setSource(const SculptureProgramParams& source) noexcept {
    sourceParams_ = source;
    currentParams_ = source;
}

void SculptureMorphChoreographer::setTarget(
    const SculptureProgramParams& target,
    float durationSeconds,
    MorphEasing easing
) noexcept {
    sourceParams_ = currentParams_;
    targetParams_ = target;
    duration_ = tour_timing::effectiveCueDuration(durationSeconds);
    easing_ = easing;
    envelopeConfig_.easing = easing;
    elapsed_ = 0.0f;
    progress_ = 0.0f;
    active_ = true;
}

void SculptureMorphChoreographer::setTarget(
    const SculptureProgramParams& target,
    float durationSeconds,
    const MorphEnvelopeConfig& envelope
) noexcept {
    sourceParams_ = currentParams_;
    targetParams_ = target;
    duration_ = tour_timing::effectiveCueDuration(durationSeconds);
    envelopeConfig_ = envelope;
    easing_ = envelope.easing;
    elapsed_ = 0.0f;
    progress_ = 0.0f;
    active_ = true;
}

void SculptureMorphChoreographer::setTargetPreset(
    const SculpturePreset& preset,
    float durationSeconds,
    MorphEasing easing
) noexcept {
    setTarget(preset.params, durationSeconds, easing);
}

void SculptureMorphChoreographer::setTargetPreset(
    const SculpturePreset& preset,
    float durationSeconds,
    const MorphEnvelopeConfig& envelope
) noexcept {
    setTarget(preset.params, durationSeconds, envelope);
}

void SculptureMorphChoreographer::snapToTarget() noexcept {
    sourceParams_ = targetParams_;
    currentParams_ = targetParams_;
    elapsed_ = duration_;
    progress_ = 1.0f;
    active_ = false;
}

float SculptureMorphChoreographer::evaluateSpring(float t, float zeta, float omega_n, float v0) noexcept {
    if (t <= 0.0f) return 0.0f;
    const float z = std::max(0.001f, zeta);
    const float w = std::max(0.001f, omega_n);

    if (std::abs(z - 1.0f) < 1e-4f) {
        // Critically Damped (zeta = 1.0): Fastest asymptotic return to equilibrium without oscillation
        const float b = w - v0;
        return 1.0f - (1.0f + b * t) * std::exp(-w * t);
    } else if (z < 1.0f) {
        // Underdamped (zeta < 1.0): Damped harmonic oscillation with decaying envelope
        const float wd = w * std::sqrt(1.0f - z * z);
        const float decay = std::exp(-z * w * t);
        const float b = (v0 - z * w) / wd;
        return 1.0f - decay * (std::cos(wd * t) - b * std::sin(wd * t));
    } else {
        // Overdamped (zeta > 1.0): Non-oscillatory sum of two decaying exponential terms
        const float r = std::sqrt(z * z - 1.0f);
        const float g1 = -w * (z - r);
        const float g2 = -w * (z + r);
        const float c1 = (v0 + g2) / (g1 - g2);
        const float c2 = -1.0f - c1;
        return 1.0f + c1 * std::exp(g1 * t) + c2 * std::exp(g2 * t);
    }
}

float SculptureMorphChoreographer::evaluateCubicBezier(float t, float p1x, float p1y, float p2x, float p2y) noexcept {
    const float u = clamp01(t);
    if (u <= 0.0f) return 0.0f;
    if (u >= 1.0f) return 1.0f;

    const float cx1 = std::clamp(p1x, 0.0f, 1.0f);
    const float cx2 = std::clamp(p2x, 0.0f, 1.0f);

    // Solve for parameter s such that B_x(s) = u using Newton-Raphson
    float s = u;
    for (int iter = 0; iter < 8; ++iter) {
        const float om_s = 1.0f - s;
        const float bx = 3.0f * om_s * om_s * s * cx1 + 3.0f * om_s * s * s * cx2 + s * s * s;
        const float err = bx - u;
        if (std::abs(err) < 1e-5f) break;

        const float dbx = 3.0f * om_s * om_s * cx1 + 6.0f * om_s * s * (cx2 - cx1) + 3.0f * s * s * (1.0f - cx2);
        if (std::abs(dbx) < 1e-6f) break;
        s = std::clamp(s - err / dbx, 0.0f, 1.0f);
    }

    const float om_s = 1.0f - s;
    return 3.0f * om_s * om_s * s * p1y + 3.0f * om_s * s * s * p2y + s * s * s;
}

float SculptureMorphChoreographer::evaluateEnvelope(float t, const MorphEnvelopeConfig& config) noexcept {
    switch (config.easing) {
    case MorphEasing::SpringUnderdamped: {
        const float z = (config.spring.dampingRatio < 1.0f && config.spring.dampingRatio > 0.0f) ? config.spring.dampingRatio : 0.5f;
        const float val = evaluateSpring(t * 3.0f, z, config.spring.naturalFrequency, config.spring.initialVelocity);
        return 1.0f + (val - 1.0f) * config.overshootScale;
    }
    case MorphEasing::SpringCriticallyDamped:
        return evaluateSpring(t * 3.0f, 1.0f, config.spring.naturalFrequency, config.spring.initialVelocity);
    case MorphEasing::SpringOverdamped: {
        const float z = (config.spring.dampingRatio > 1.0f) ? config.spring.dampingRatio : 1.5f;
        return evaluateSpring(t * 3.0f, z, config.spring.naturalFrequency, config.spring.initialVelocity);
    }
    case MorphEasing::CubicBezier:
        return evaluateCubicBezier(t, config.bezier.p1x, config.bezier.p1y, config.bezier.p2x, config.bezier.p2y);
    default:
        return evaluateEasing(t, config.easing);
    }
}

float SculptureMorphChoreographer::evaluateEasing(float t, MorphEasing easing) noexcept {
    const float u = clamp01(t);
    switch (easing) {
    case MorphEasing::Linear:
        return u;
    case MorphEasing::Smoothstep:
        return u * u * (3.0f - 2.0f * u);
    case MorphEasing::Sinusoidal:
        return 0.5f * (1.0f - std::cos(3.1415926535f * u));
    case MorphEasing::Exponential:
        return (u <= 0.0f) ? 0.0f : ((u >= 1.0f) ? 1.0f : std::pow(2.0f, 10.0f * (u - 1.0f)));
    case MorphEasing::ElasticPulse: {
        if (u <= 0.0f) return 0.0f;
        if (u >= 1.0f) return 1.0f;
        return std::pow(2.0f, -10.0f * u) * std::sin((u - 0.075f) * (2.0f * 3.1415926535f) / 0.3f) + 1.0f;
    }
    case MorphEasing::SpringUnderdamped:
        return evaluateSpring(u * 3.0f, 0.5f, 6.0f, 0.0f);
    case MorphEasing::SpringCriticallyDamped:
        return evaluateSpring(u * 3.0f, 1.0f, 6.0f, 0.0f);
    case MorphEasing::SpringOverdamped:
        return evaluateSpring(u * 3.0f, 1.5f, 6.0f, 0.0f);
    case MorphEasing::CubicBezier:
        return evaluateCubicBezier(u, 0.25f, 0.10f, 0.25f, 1.00f);
    case MorphEasing::BackOvershoot: {
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.0f;
        const float v = u - 1.0f;
        return 1.0f + c3 * v * v * v + c1 * v * v;
    }
    case MorphEasing::BounceOut: {
        const float n1 = 7.5625f;
        const float d1 = 2.75f;
        if (u < 1.0f / d1) {
            return n1 * u * u;
        } else if (u < 2.0f / d1) {
            const float v = u - 1.5f / d1;
            return n1 * v * v + 0.75f;
        } else if (u < 2.5f / d1) {
            const float v = u - 2.25f / d1;
            return n1 * v * v + 0.9375f;
        } else {
            const float v = u - 2.625f / d1;
            return n1 * v * v + 0.984375f;
        }
    }
    }
    return u;
}

float SculptureMorphChoreographer::shortestAngleDelta(float a, float b) noexcept {
    float diff = std::fmod(b - a + 0.5f, 1.0f);
    if (diff < 0.0f) diff += 1.0f;
    return diff - 0.5f;
}

float SculptureMorphChoreographer::interpolateHue(float hueA, float hueB, float t) noexcept {
    const float delta = shortestAngleDelta(hueA, hueB);
    float res = std::fmod(hueA + clamp01(t) * delta, 1.0f);
    if (res < 0.0f) res += 1.0f;
    return res;
}

float SculptureMorphChoreographer::shortestAngleDeltaRad(float a, float b) noexcept {
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kTwoPi = 2.0f * kPi;
    float diff = std::fmod(b - a + kPi, kTwoPi);
    if (diff < 0.0f) diff += kTwoPi;
    return diff - kPi;
}

float SculptureMorphChoreographer::interpolateAngleRad(float angleA, float angleB, float t) noexcept {
    const float delta = shortestAngleDeltaRad(angleA, angleB);
    return angleA + t * delta;
}

std::array<float, 4> SculptureMorphChoreographer::quaternionSlerp(
    const std::array<float, 4>& qA,
    const std::array<float, 4>& qB,
    float t
) noexcept {
    const float normA = std::sqrt(qA[0] * qA[0] + qA[1] * qA[1] + qA[2] * qA[2] + qA[3] * qA[3]);
    const float normB = std::sqrt(qB[0] * qB[0] + qB[1] * qB[1] + qB[2] * qB[2] + qB[3] * qB[3]);
    const std::array<float, 4> a = (normA > 1.0e-6f)
        ? std::array<float, 4>{qA[0] / normA, qA[1] / normA, qA[2] / normA, qA[3] / normA}
        : std::array<float, 4>{1.0f, 0.0f, 0.0f, 0.0f};
    std::array<float, 4> b = (normB > 1.0e-6f)
        ? std::array<float, 4>{qB[0] / normB, qB[1] / normB, qB[2] / normB, qB[3] / normB}
        : std::array<float, 4>{1.0f, 0.0f, 0.0f, 0.0f};

    float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    if (dot < 0.0f) {
        // Take shortest path across antipodal identification
        b[0] = -b[0];
        b[1] = -b[1];
        b[2] = -b[2];
        b[3] = -b[3];
        dot = -dot;
    }

    dot = std::clamp(dot, -1.0f, 1.0f);

    // If quaternions are closely aligned, use normalized linear interpolation (NLerp)
    if (dot > 0.9995f) {
        std::array<float, 4> result = {
            a[0] + t * (b[0] - a[0]),
            a[1] + t * (b[1] - a[1]),
            a[2] + t * (b[2] - a[2]),
            a[3] + t * (b[3] - a[3])
        };
        const float rNorm = std::sqrt(result[0] * result[0] + result[1] * result[1] + result[2] * result[2] + result[3] * result[3]);
        if (rNorm > 1.0e-6f) {
            result[0] /= rNorm; result[1] /= rNorm; result[2] /= rNorm; result[3] /= rNorm;
        }
        return result;
    }

    const float theta = std::acos(dot);
    const float sinTheta = std::sin(theta);
    const float scaleA = std::sin((1.0f - t) * theta) / sinTheta;
    const float scaleB = std::sin(t * theta) / sinTheta;

    return {
        scaleA * a[0] + scaleB * b[0],
        scaleA * a[1] + scaleB * b[1],
        scaleA * a[2] + scaleB * b[2],
        scaleA * a[3] + scaleB * b[3]
    };
}

void SculptureMorphChoreographer::interpolateRotorSO4(
    float alphaA, float betaA, float thetaA,
    float alphaB, float betaB, float thetaB,
    float t,
    float& outAlpha, float& outBeta, float& outTheta
) noexcept {
    // 1. Left isoclinic rotor (alpha)
    const std::array<float, 4> qLA = {std::cos(alphaA * 0.5f), 0.0f, 0.0f, std::sin(alphaA * 0.5f)};
    const std::array<float, 4> qLB = {std::cos(alphaB * 0.5f), 0.0f, 0.0f, std::sin(alphaB * 0.5f)};
    const auto qL = quaternionSlerp(qLA, qLB, t);
    outAlpha = 2.0f * std::atan2(qL[3], qL[0]);

    // 2. Right isoclinic rotor (beta)
    const std::array<float, 4> qRA = {std::cos(betaA * 0.5f), 0.0f, 0.0f, std::sin(betaA * 0.5f)};
    const std::array<float, 4> qRB = {std::cos(betaB * 0.5f), 0.0f, 0.0f, std::sin(betaB * 0.5f)};
    const auto qR = quaternionSlerp(qRA, qRB, t);
    outBeta = 2.0f * std::atan2(qR[3], qR[0]);

    // 3. Hyperplane rotation angle (theta)
    const std::array<float, 4> qTA = {std::cos(thetaA * 0.5f), std::sin(thetaA * 0.5f), 0.0f, 0.0f};
    const std::array<float, 4> qTB = {std::cos(thetaB * 0.5f), std::sin(thetaB * 0.5f), 0.0f, 0.0f};
    const auto qT = quaternionSlerp(qTA, qTB, t);
    outTheta = 2.0f * std::atan2(qT[1], qT[0]);
}

ComplexAffine SculptureMorphChoreographer::interpolateAffine(
    const ComplexAffine& a,
    const ComplexAffine& b,
    float t
) noexcept {
    // Geodesic Polar Interpolation: Preserves scale magnitude and allows dynamic overshoot
    const float rA = std::sqrt(a.scaleReal * a.scaleReal + a.scaleImag * a.scaleImag);
    const float rB = std::sqrt(b.scaleReal * b.scaleReal + b.scaleImag * b.scaleImag);
    const float r = std::max(0.0f, rA + (rB - rA) * t);

    const float thetaA = (rA > 1.0e-6f) ? std::atan2(a.scaleImag, a.scaleReal) : 0.0f;
    const float thetaB = (rB > 1.0e-6f) ? std::atan2(b.scaleImag, b.scaleReal) : 0.0f;

    float dTheta = std::fmod(thetaB - thetaA + 3.14159265359f, 2.0f * 3.14159265359f);
    if (dTheta < 0.0f) dTheta += 2.0f * 3.14159265359f;
    dTheta -= 3.14159265359f;

    const float theta = thetaA + t * dTheta;
    const float sReal = (r > 1.0e-6f) ? r * std::cos(theta) : 0.0f;
    const float sImag = (r > 1.0e-6f) ? r * std::sin(theta) : 0.0f;

    return ComplexAffine{
        .scaleReal = sReal,
        .scaleImag = sImag,
        .offsetReal = a.offsetReal + (b.offsetReal - a.offsetReal) * t,
        .offsetImag = a.offsetImag + (b.offsetImag - a.offsetImag) * t
    };
}

SculptureProgramParams SculptureMorphChoreographer::interpolateParams(
    const SculptureProgramParams& a,
    const SculptureProgramParams& b,
    float t
) noexcept {
    const float u = clamp01(t);
    SculptureProgramParams p = a;
    // Matching authored A/B banks already describe the same mathematical family.
    // Morph their blend weight instead of replacing B with the other preset's A:
    // that substitution changes even a preset morphed toward itself.
    const bool matchingBanks = matchingSelectorBanks(a, b);

    // 1. Affine coordinate frames (with dynamic spring overshoot)
    p.pre = interpolateAffine(a.pre, b.pre, t);
    p.post = interpolateAffine(a.post, b.post, t);

    // 2. Topological Homotopy & Inner Maps & Reduction Folds
    if (matchingBanks) {
        p.topologyBlend = std::clamp(flerp(a.topologyBlend, b.topologyBlend, u), 0.0f, 1.0f);
    } else if (u < 0.5f) {
        p.inner = a.inner;
        p.innerB = b.inner;
        p.reduction = a.reduction;
        p.reductionB = b.reduction;
        p.topologyBlend = std::clamp(a.topologyBlend * (1.0f - 2.0f * u) + 2.0f * u, 0.0f, 1.0f);
    } else {
        p.inner = b.inner;
        p.innerB = a.inner;
        p.reduction = b.reduction;
        p.reductionB = a.reduction;
        p.topologyBlend = std::clamp(b.topologyBlend * (2.0f * u - 1.0f) + (2.0f - 2.0f * u), 0.0f, 1.0f);
    }

    // 3. WCA Seed & Pulse
    if (!matchingBanks) {
        if (u < 0.5f) {
            p.seedType = a.seedType;
            p.seedTypeB = b.seedType;
        } else {
            p.seedType = b.seedType;
            p.seedTypeB = a.seedType;
        }
    }
    p.seedWidth = a.seedWidth + (b.seedWidth - a.seedWidth) * t;
    p.seedAlpha = std::clamp(a.seedAlpha + (b.seedAlpha - a.seedAlpha) * t, 0.0f, 1.0f);
    p.seedBaseline = a.seedBaseline + (b.seedBaseline - a.seedBaseline) * t;
    p.seedPulseFrequency = a.seedPulseFrequency + (b.seedPulseFrequency - a.seedPulseFrequency) * t;
    p.seedPulseAmplitude = a.seedPulseAmplitude + (b.seedPulseAmplitude - a.seedPulseAmplitude) * t;

    // 4. OKLCH / Hue & Spatial Dynamics
    p.hueRotation = interpolateHue(a.hueRotation, b.hueRotation, t);
    p.hueSpeed = a.hueSpeed + (b.hueSpeed - a.hueSpeed) * t;
    p.chromaBreath = a.chromaBreath + (b.chromaBreath - a.chromaBreath) * t;
    p.lumaDrift = a.lumaDrift + (b.lumaDrift - a.lumaDrift) * t;
    p.paletteBlend = std::clamp(a.paletteBlend + (b.paletteBlend - a.paletteBlend) * t, 0.0f, 1.0f);

    // 5. Volumetric & Optical Media
    p.volumetricDensityScale = std::max(0.0f, a.volumetricDensityScale + (b.volumetricDensityScale - a.volumetricDensityScale) * t);
    p.fogAnisotropy = std::clamp(a.fogAnisotropy + (b.fogAnisotropy - a.fogAnisotropy) * t, -0.99f, 0.99f);
    p.filmThickness = std::max(0.0f, a.filmThickness + (b.filmThickness - a.filmThickness) * t);
    p.filmIOR = std::max(1.0f, a.filmIOR + (b.filmIOR - a.filmIOR) * t);

    // 6. Global Dimensions & Rates (with dynamic spring overshoot)
    p.latentW = std::clamp(a.latentW + (b.latentW - a.latentW) * t, 0.0f, 1.0f);
    interpolateRotorSO4(
        a.rotorAlpha, a.rotorBeta, a.rotorTheta4D,
        b.rotorAlpha, b.rotorBeta, b.rotorTheta4D,
        t,
        p.rotorAlpha, p.rotorBeta, p.rotorTheta4D
    );
    p.patternSpeed = a.patternSpeed + (b.patternSpeed - a.patternSpeed) * t;
    p.worldScale = std::max(0.01f, a.worldScale + (b.worldScale - a.worldScale) * t);
    p.radiusScale = std::max(0.01f, a.radiusScale + (b.radiusScale - a.radiusScale) * t);
    p.advectionSpeed = a.advectionSpeed + (b.advectionSpeed - a.advectionSpeed) * t;
    p.curlStrength = a.curlStrength + (b.curlStrength - a.curlStrength) * t;
    p.trailDecay = std::clamp(a.trailDecay + (b.trailDecay - a.trailDecay) * t, 0.0f, 1.0f);
    p.kineticViscosity = std::max(0.0f, a.kineticViscosity + (b.kineticViscosity - a.kineticViscosity) * t);

    for (std::size_t i = 0; i < 4; ++i) {
        p.audioSpectrum[i] = std::max(0.0f, a.audioSpectrum[i] + (b.audioSpectrum[i] - a.audioSpectrum[i]) * t);
    }

    // 7. Temporal Feedback & Cadence
    p.temporalFeedbackBlend = std::clamp(a.temporalFeedbackBlend + (b.temporalFeedbackBlend - a.temporalFeedbackBlend) * t, 0.0f, 1.0f);
    p.temporalFade = std::clamp(a.temporalFade + (b.temporalFade - a.temporalFade) * t, 0.0f, 1.0f);
    p.temporalFixedDelta = a.temporalFixedDelta + (b.temporalFixedDelta - a.temporalFixedDelta) * t;
    p.temporalReset = (u < 0.5f) ? a.temporalReset : b.temporalReset;

    // 8. Discrete Threshold Stepping & Geometric Topology Modes
    if (!matchingBanks) {
        if (u < 0.5f) {
            p.outputMode = a.outputMode;
            p.outputModeB = b.outputMode;
            p.enable4D = a.enable4D;
            p.enable4DB = b.enable4D;
        } else {
            p.outputMode = b.outputMode;
            p.outputModeB = a.outputMode;
            p.enable4D = b.enable4D;
            p.enable4DB = a.enable4D;
        }
    }
    p.attractorMode = (u < 0.5f) ? a.attractorMode : b.attractorMode;
    p.attractorStrength = a.attractorStrength + (b.attractorStrength - a.attractorStrength) * t;
    p.instanceMeshType = (u < 0.5f) ? a.instanceMeshType : b.instanceMeshType;
    p.colorEffect = (u < 0.5f) ? a.colorEffect : b.colorEffect;
    p.feedbackIterations = (u < 0.5f) ? a.feedbackIterations : b.feedbackIterations;
    p.stageMask = (u < 0.5f) ? a.stageMask : b.stageMask;
    p.auxiliaryResolutionScale = (u < 0.5f) ? a.auxiliaryResolutionScale : b.auxiliaryResolutionScale;

    return p;
}

float SculptureMorphChoreographer::evaluateCatmullRomSpline(
    float p0, float p1, float p2, float p3,
    float t,
    CatmullRomType type
) noexcept {
    const float u = clamp01(t);
    if (type == CatmullRomType::Uniform) {
        return 0.5f * (
            (2.0f * p1) +
            (-p0 + p2) * u +
            (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * (u * u) +
            (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * (u * u * u)
        );
    }
    const float alpha = (type == CatmullRomType::Centripetal) ? 0.5f : 1.0f;
    const float dt0 = std::pow(std::max(1e-4f, std::abs(p1 - p0)), alpha);
    const float dt1 = std::pow(std::max(1e-4f, std::abs(p2 - p1)), alpha);
    const float dt2 = std::pow(std::max(1e-4f, std::abs(p3 - p2)), alpha);

    const float t0 = 0.0f;
    const float t1 = t0 + dt0;
    const float t2 = t1 + dt1;
    const float t3 = t2 + dt2;

    const float evalT = t1 + u * dt1;

    const float a1 = (t1 - evalT) / (t1 - t0) * p0 + (evalT - t0) / (t1 - t0) * p1;
    const float a2 = (t2 - evalT) / (t2 - t1) * p1 + (evalT - t1) / (t2 - t1) * p2;
    const float a3 = (t3 - evalT) / (t3 - t2) * p2 + (evalT - t2) / (t3 - t2) * p3;

    const float b1 = (t2 - evalT) / (t2 - t0) * a1 + (evalT - t0) / (t2 - t0) * a2;
    const float b2 = (t3 - evalT) / (t3 - t1) * a2 + (evalT - t1) / (t3 - t1) * a3;

    return (t2 - evalT) / (t2 - t1) * b1 + (evalT - t1) / (t2 - t1) * b2;
}

ComplexAffine SculptureMorphChoreographer::interpolateAffineSpline(
    const ComplexAffine& a0, const ComplexAffine& a1,
    const ComplexAffine& a2, const ComplexAffine& a3,
    float t
) noexcept {
    return ComplexAffine{
        .scaleReal = evaluateCatmullRomSpline(a0.scaleReal, a1.scaleReal, a2.scaleReal, a3.scaleReal, t),
        .scaleImag = evaluateCatmullRomSpline(a0.scaleImag, a1.scaleImag, a2.scaleImag, a3.scaleImag, t),
        .offsetReal = evaluateCatmullRomSpline(a0.offsetReal, a1.offsetReal, a2.offsetReal, a3.offsetReal, t),
        .offsetImag = evaluateCatmullRomSpline(a0.offsetImag, a1.offsetImag, a2.offsetImag, a3.offsetImag, t)
    };
}

namespace {

inline std::array<float, 4> quatNormalize(const std::array<float, 4>& q) noexcept {
    const float n = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (n < 1.0e-7f) return {1.0f, 0.0f, 0.0f, 0.0f};
    return {q[0] / n, q[1] / n, q[2] / n, q[3] / n};
}

inline std::array<float, 4> quatMultiply(const std::array<float, 4>& p, const std::array<float, 4>& q) noexcept {
    return {
        p[0] * q[0] - p[1] * q[1] - p[2] * q[2] - p[3] * q[3],
        p[0] * q[1] + p[1] * q[0] + p[2] * q[3] - p[3] * q[2],
        p[0] * q[2] - p[1] * q[3] + p[2] * q[0] + p[3] * q[1],
        p[0] * q[3] + p[1] * q[2] - p[2] * q[1] + p[3] * q[0]
    };
}

inline std::array<float, 4> quatInverse(const std::array<float, 4>& q) noexcept {
    const float n2 = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
    if (n2 < 1.0e-7f) return {1.0f, 0.0f, 0.0f, 0.0f};
    return {q[0] / n2, -q[1] / n2, -q[2] / n2, -q[3] / n2};
}

inline std::array<float, 4> quatLog(const std::array<float, 4>& q) noexcept {
    const float vLen = std::sqrt(q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (vLen < 1.0e-6f) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    const float theta = std::atan2(vLen, q[0]);
    const float scale = theta / vLen;
    return {0.0f, q[1] * scale, q[2] * scale, q[3] * scale};
}

inline std::array<float, 4> quatExp(const std::array<float, 4>& v) noexcept {
    const float theta = std::sqrt(v[1] * v[1] + v[2] * v[2] + v[3] * v[3]);
    if (theta < 1.0e-6f) {
        return {std::cos(theta), 0.0f, 0.0f, 0.0f};
    }
    const float scale = std::sin(theta) / theta;
    return {std::cos(theta), v[1] * scale, v[2] * scale, v[3] * scale};
}

} // namespace

std::array<float, 4> SculptureMorphChoreographer::quaternionSquad(
    const std::array<float, 4>& q0,
    const std::array<float, 4>& q1,
    const std::array<float, 4>& q2,
    const std::array<float, 4>& q3,
    float t
) noexcept {
    const auto n0 = quatNormalize(q0);
    const auto n1 = quatNormalize(q1);
    const auto n2 = quatNormalize(q2);
    const auto n3 = quatNormalize(q3);

    // Inner control point s1 for q1
    const auto invN1 = quatInverse(n1);
    const auto logN1N0 = quatLog(quatMultiply(invN1, n0));
    const auto logN1N2 = quatLog(quatMultiply(invN1, n2));
    const std::array<float, 4> avgLog1 = {
        0.0f,
        -0.25f * (logN1N0[1] + logN1N2[1]),
        -0.25f * (logN1N0[2] + logN1N2[2]),
        -0.25f * (logN1N0[3] + logN1N2[3])
    };
    const auto s1 = quatNormalize(quatMultiply(n1, quatExp(avgLog1)));

    // Inner control point s2 for q2
    const auto invN2 = quatInverse(n2);
    const auto logN2N1 = quatLog(quatMultiply(invN2, n1));
    const auto logN2N3 = quatLog(quatMultiply(invN2, n3));
    const std::array<float, 4> avgLog2 = {
        0.0f,
        -0.25f * (logN2N1[1] + logN2N3[1]),
        -0.25f * (logN2N1[2] + logN2N3[2]),
        -0.25f * (logN2N1[3] + logN2N3[3])
    };
    const auto s2 = quatNormalize(quatMultiply(n2, quatExp(avgLog2)));

    const auto slerpQ = quaternionSlerp(n1, n2, t);
    const auto slerpS = quaternionSlerp(s1, s2, t);
    return quaternionSlerp(slerpQ, slerpS, 2.0f * t * (1.0f - t));
}

void SculptureMorphChoreographer::interpolateRotorSO4Squad(
    const std::array<float, 3>& r0,
    const std::array<float, 3>& r1,
    const std::array<float, 3>& r2,
    const std::array<float, 3>& r3,
    float t,
    float& outAlpha, float& outBeta, float& outTheta
) noexcept {
    // 1. Left isoclinic rotor (alpha)
    const std::array<float, 4> qL0 = {std::cos(r0[0] * 0.5f), 0.0f, 0.0f, std::sin(r0[0] * 0.5f)};
    const std::array<float, 4> qL1 = {std::cos(r1[0] * 0.5f), 0.0f, 0.0f, std::sin(r1[0] * 0.5f)};
    const std::array<float, 4> qL2 = {std::cos(r2[0] * 0.5f), 0.0f, 0.0f, std::sin(r2[0] * 0.5f)};
    const std::array<float, 4> qL3 = {std::cos(r3[0] * 0.5f), 0.0f, 0.0f, std::sin(r3[0] * 0.5f)};
    const auto qL = quaternionSquad(qL0, qL1, qL2, qL3, t);
    outAlpha = 2.0f * std::atan2(qL[3], qL[0]);

    // 2. Right isoclinic rotor (beta)
    const std::array<float, 4> qR0 = {std::cos(r0[1] * 0.5f), 0.0f, 0.0f, std::sin(r0[1] * 0.5f)};
    const std::array<float, 4> qR1 = {std::cos(r1[1] * 0.5f), 0.0f, 0.0f, std::sin(r1[1] * 0.5f)};
    const std::array<float, 4> qR2 = {std::cos(r2[1] * 0.5f), 0.0f, 0.0f, std::sin(r2[1] * 0.5f)};
    const std::array<float, 4> qR3 = {std::cos(r3[1] * 0.5f), 0.0f, 0.0f, std::sin(r3[1] * 0.5f)};
    const auto qR = quaternionSquad(qR0, qR1, qR2, qR3, t);
    outBeta = 2.0f * std::atan2(qR[3], qR[0]);

    // 3. Hyperplane rotation angle (theta)
    const std::array<float, 4> qT0 = {std::cos(r0[2] * 0.5f), std::sin(r0[2] * 0.5f), 0.0f, 0.0f};
    const std::array<float, 4> qT1 = {std::cos(r1[2] * 0.5f), std::sin(r1[2] * 0.5f), 0.0f, 0.0f};
    const std::array<float, 4> qT2 = {std::cos(r2[2] * 0.5f), std::sin(r2[2] * 0.5f), 0.0f, 0.0f};
    const std::array<float, 4> qT3 = {std::cos(r3[2] * 0.5f), std::sin(r3[2] * 0.5f), 0.0f, 0.0f};
    const auto qT = quaternionSquad(qT0, qT1, qT2, qT3, t);
    outTheta = 2.0f * std::atan2(qT[1], qT[0]);
}

SculptureProgramParams SculptureMorphChoreographer::interpolateParamsSpline(
    const SculptureProgramParams& p0,
    const SculptureProgramParams& p1,
    const SculptureProgramParams& p2,
    const SculptureProgramParams& p3,
    float t
) noexcept {
    const float u = clamp01(t);
    SculptureProgramParams p = p1;
    // The central pair owns this segment's discrete math, while the outer
    // control points shape its continuous curves. Share the pairwise bank rule.
    const bool matchingBanks = matchingSelectorBanks(p1, p2);

    // 1. Affine Splines
    p.pre = interpolateAffineSpline(p0.pre, p1.pre, p2.pre, p3.pre, u);
    p.post = interpolateAffineSpline(p0.post, p1.post, p2.post, p3.post, u);

    // 2. Homotopy & Inner Maps & Reduction Folds
    if (matchingBanks) {
        p.topologyBlend = std::clamp(evaluateCatmullRomSpline(
            p0.topologyBlend, p1.topologyBlend, p2.topologyBlend, p3.topologyBlend, u),
            0.0f, 1.0f);
    } else if (u < 0.5f) {
        p.inner = p1.inner;
        p.innerB = p2.inner;
        p.reduction = p1.reduction;
        p.reductionB = p2.reduction;
        p.topologyBlend = std::clamp(p1.topologyBlend * (1.0f - 2.0f * u) + 2.0f * u, 0.0f, 1.0f);
    } else {
        p.inner = p2.inner;
        p.innerB = p1.inner;
        p.reduction = p2.reduction;
        p.reductionB = p1.reduction;
        p.topologyBlend = std::clamp(p2.topologyBlend * (2.0f * u - 1.0f) + (2.0f - 2.0f * u), 0.0f, 1.0f);
    }

    // 3. Seed & Pulse
    if (!matchingBanks) {
        if (u < 0.5f) {
            p.seedType = p1.seedType;
            p.seedTypeB = p2.seedType;
        } else {
            p.seedType = p2.seedType;
            p.seedTypeB = p1.seedType;
        }
    }
    p.seedWidth = evaluateCatmullRomSpline(p0.seedWidth, p1.seedWidth, p2.seedWidth, p3.seedWidth, u);
    p.seedAlpha = std::clamp(evaluateCatmullRomSpline(p0.seedAlpha, p1.seedAlpha, p2.seedAlpha, p3.seedAlpha, u), 0.0f, 1.0f);
    p.seedBaseline = evaluateCatmullRomSpline(p0.seedBaseline, p1.seedBaseline, p2.seedBaseline, p3.seedBaseline, u);
    p.seedPulseFrequency = evaluateCatmullRomSpline(p0.seedPulseFrequency, p1.seedPulseFrequency, p2.seedPulseFrequency, p3.seedPulseFrequency, u);
    p.seedPulseAmplitude = evaluateCatmullRomSpline(p0.seedPulseAmplitude, p1.seedPulseAmplitude, p2.seedPulseAmplitude, p3.seedPulseAmplitude, u);

    // 4. OKLCH / Hue & Spatial Dynamics
    p.hueRotation = interpolateHue(p1.hueRotation, p2.hueRotation, u);
    p.hueSpeed = evaluateCatmullRomSpline(p0.hueSpeed, p1.hueSpeed, p2.hueSpeed, p3.hueSpeed, u);
    p.chromaBreath = evaluateCatmullRomSpline(p0.chromaBreath, p1.chromaBreath, p2.chromaBreath, p3.chromaBreath, u);
    p.lumaDrift = evaluateCatmullRomSpline(p0.lumaDrift, p1.lumaDrift, p2.lumaDrift, p3.lumaDrift, u);
    p.paletteBlend = std::clamp(evaluateCatmullRomSpline(p0.paletteBlend, p1.paletteBlend, p2.paletteBlend, p3.paletteBlend, u), 0.0f, 1.0f);

    // 5. Volumetric & Optical Media
    p.volumetricDensityScale = std::max(0.0f, evaluateCatmullRomSpline(p0.volumetricDensityScale, p1.volumetricDensityScale, p2.volumetricDensityScale, p3.volumetricDensityScale, u));
    p.fogAnisotropy = std::clamp(evaluateCatmullRomSpline(p0.fogAnisotropy, p1.fogAnisotropy, p2.fogAnisotropy, p3.fogAnisotropy, u), -0.99f, 0.99f);
    p.filmThickness = std::max(0.0f, evaluateCatmullRomSpline(p0.filmThickness, p1.filmThickness, p2.filmThickness, p3.filmThickness, u));
    p.filmIOR = std::max(1.0f, evaluateCatmullRomSpline(p0.filmIOR, p1.filmIOR, p2.filmIOR, p3.filmIOR, u));

    // 6. Global Dimensions & SO(4) Rotors
    p.latentW = std::clamp(evaluateCatmullRomSpline(p0.latentW, p1.latentW, p2.latentW, p3.latentW, u), 0.0f, 1.0f);
    const std::array<float, 3> r0 = {p0.rotorAlpha, p0.rotorBeta, p0.rotorTheta4D};
    const std::array<float, 3> r1 = {p1.rotorAlpha, p1.rotorBeta, p1.rotorTheta4D};
    const std::array<float, 3> r2 = {p2.rotorAlpha, p2.rotorBeta, p2.rotorTheta4D};
    const std::array<float, 3> r3 = {p3.rotorAlpha, p3.rotorBeta, p3.rotorTheta4D};
    interpolateRotorSO4Squad(r0, r1, r2, r3, u, p.rotorAlpha, p.rotorBeta, p.rotorTheta4D);

    p.patternSpeed = evaluateCatmullRomSpline(p0.patternSpeed, p1.patternSpeed, p2.patternSpeed, p3.patternSpeed, u);
    p.worldScale = std::max(0.01f, evaluateCatmullRomSpline(p0.worldScale, p1.worldScale, p2.worldScale, p3.worldScale, u));
    p.radiusScale = std::max(0.01f, evaluateCatmullRomSpline(p0.radiusScale, p1.radiusScale, p2.radiusScale, p3.radiusScale, u));
    p.advectionSpeed = evaluateCatmullRomSpline(p0.advectionSpeed, p1.advectionSpeed, p2.advectionSpeed, p3.advectionSpeed, u);
    p.curlStrength = evaluateCatmullRomSpline(p0.curlStrength, p1.curlStrength, p2.curlStrength, p3.curlStrength, u);
    p.trailDecay = std::clamp(evaluateCatmullRomSpline(p0.trailDecay, p1.trailDecay, p2.trailDecay, p3.trailDecay, u), 0.0f, 1.0f);
    p.kineticViscosity = std::max(0.0f, evaluateCatmullRomSpline(p0.kineticViscosity, p1.kineticViscosity, p2.kineticViscosity, p3.kineticViscosity, u));

    for (std::size_t i = 0; i < 4; ++i) {
        p.audioSpectrum[i] = std::max(0.0f, evaluateCatmullRomSpline(p0.audioSpectrum[i], p1.audioSpectrum[i], p2.audioSpectrum[i], p3.audioSpectrum[i], u));
    }

    // 7. Temporal Feedback
    p.temporalFeedbackBlend = std::clamp(evaluateCatmullRomSpline(p0.temporalFeedbackBlend, p1.temporalFeedbackBlend, p2.temporalFeedbackBlend, p3.temporalFeedbackBlend, u), 0.0f, 1.0f);
    p.temporalFade = std::clamp(evaluateCatmullRomSpline(p0.temporalFade, p1.temporalFade, p2.temporalFade, p3.temporalFade, u), 0.0f, 1.0f);
    p.temporalFixedDelta = evaluateCatmullRomSpline(p0.temporalFixedDelta, p1.temporalFixedDelta, p2.temporalFixedDelta, p3.temporalFixedDelta, u);
    p.temporalReset = (u < 0.5f) ? p1.temporalReset : p2.temporalReset;

    // 8. Discrete Modes
    if (!matchingBanks) {
        if (u < 0.5f) {
            p.outputMode = p1.outputMode;
            p.outputModeB = p2.outputMode;
            p.enable4D = p1.enable4D;
            p.enable4DB = p2.enable4D;
        } else {
            p.outputMode = p2.outputMode;
            p.outputModeB = p1.outputMode;
            p.enable4D = p2.enable4D;
            p.enable4DB = p1.enable4D;
        }
    }
    p.attractorMode = (u < 0.5f) ? p1.attractorMode : p2.attractorMode;
    p.attractorStrength = evaluateCatmullRomSpline(p0.attractorStrength, p1.attractorStrength, p2.attractorStrength, p3.attractorStrength, u);
    p.instanceMeshType = (u < 0.5f) ? p1.instanceMeshType : p2.instanceMeshType;

    p.colorEffect = (u < 0.5f) ? p1.colorEffect : p2.colorEffect;
    p.feedbackIterations = (u < 0.5f) ? p1.feedbackIterations : p2.feedbackIterations;
    p.stageMask = (u < 0.5f) ? p1.stageMask : p2.stageMask;
    p.auxiliaryResolutionScale = (u < 0.5f) ? p1.auxiliaryResolutionScale : p2.auxiliaryResolutionScale;

    return p;
}

void SculptureMorphChoreographer::update(float deltaSeconds) noexcept {
    // Pause the entire active tour, including an in-flight StepAndDwell morph.
    // A standalone morph remains independent of the inactive playlist's flag.
    if (playlistActive_ && playlistPaused_) return;

    const float dt = std::max(0.0f, deltaSeconds);

    // Continuous Spline tour playback
    if (playlistActive_ && !playlist_.empty() && tourMode_ == TourPlaybackMode::ContinuousSpline) {
        if (!playlistPaused_) {
            const std::size_t N = playlist_.size();
            splineGlobalTime_ += dt;
            const float totalDur = splineTotalDuration();
            if (totalDur > 1e-4f) {
                if (playlistLoop_) {
                    splineGlobalTime_ = std::fmod(splineGlobalTime_, totalDur);
                    if (splineGlobalTime_ < 0.0f) splineGlobalTime_ += totalDur;
                } else if (splineGlobalTime_ >= totalDur) {
                    splineGlobalTime_ = totalDur;
                    playlistIndex_ = N - 1U;
                    playlistActive_ = false;
                    // Spline evaluation bypassed the ordinary morph clock.
                    // Settle its target too, so later updates cannot revive it.
                    targetParams_ = playlist_.back().params;
                    snapToTarget();
                    return;
                }
            }

            if (N == 1) {
                currentParams_ = playlist_[0].params;
                progress_ = 1.0f;
                return;
            }

            // Find active segment k
            float accumulated = 0.0f;
            std::size_t k = 0;
            float localNormT = 0.0f;
            for (std::size_t i = 0; i < N; ++i) {
                const float segDur = tour_timing::effectiveCueDuration(playlist_[i].durationSeconds);
                if (splineGlobalTime_ < accumulated + segDur || i == N - 1) {
                    k = i;
                    localNormT = std::clamp((splineGlobalTime_ - accumulated) / segDur, 0.0f, 1.0f);
                    break;
                }
                accumulated += segDur;
            }
            playlistIndex_ = k;

            // 4 Bounding control points for Catmull-Rom: i0, i1, i2, i3
            std::size_t i1 = k;
            std::size_t i2 = (k + 1) % N;
            std::size_t i0 = (k + N - 1) % N;
            std::size_t i3 = (k + 2) % N;
            if (!playlistLoop_) {
                if (k == 0) i0 = 0;
                if (k + 1 >= N) i2 = N - 1;
                if (k + 2 >= N) i3 = N - 1;
            }

            const auto& p0 = playlist_[i0].params;
            const auto& p1 = playlist_[i1].params;
            const auto& p2 = playlist_[i2].params;
            const auto& p3 = playlist_[i3].params;

            currentParams_ = interpolateParamsSpline(p0, p1, p2, p3, localNormT);
            progress_ = localNormT;
        }
        return;
    }

    // Morph and dwell share one update budget; a finishing morph must not also
    // charge its consumed time to the following dwell stage.
    float dwellDelta = dt;
    if (active_) {
        const float remainingMorphTime = std::max(0.0f, duration_ - elapsed_);
        const float morphDelta = std::min(dt, remainingMorphTime);
        elapsed_ += morphDelta;
        dwellDelta -= morphDelta;
        const float normT = (duration_ > 0.0f) ? std::min(elapsed_ / duration_, 1.0f) : 1.0f;
        progress_ = evaluateEnvelope(normT, envelopeConfig_);
        currentParams_ = interpolateParams(sourceParams_, targetParams_, progress_);

        if (normT >= 1.0f) {
            progress_ = 1.0f;
            active_ = false;
            currentParams_ = targetParams_;
        }
    }

    // Automated playlist sequencer (StepAndDwell)
    if (playlistActive_ && !playlist_.empty()) {
        if (!active_ && !playlistPaused_) {
            playlistDwellTimer_ += dwellDelta;
            const auto& currentTarget = playlist_[playlistIndex_];
            if (playlistDwellTimer_ >= currentTarget.dwellSeconds) {
                playlistDwellTimer_ = 0.0f;
                playlistIndex_++;
                if (playlistIndex_ >= playlist_.size()) {
                    if (playlistLoop_) {
                        playlistIndex_ = 0;
                    } else {
                        // Capture and inspection retain the completed cue's identity.
                        playlistIndex_ = playlist_.size() - 1U;
                        playlistActive_ = false;
                        return;
                    }
                }
                enterPlaylistCue(playlistIndex_, true);
            }
        }
    }
}

void SculptureMorphChoreographer::clearPlaylist() noexcept {
    playlist_.clear();
    playlistIndex_ = 0;
    playlistDwellTimer_ = 0.0f;
    splineGlobalTime_ = 0.0f;
    playlistActive_ = false;
    playlistPaused_ = false;
}

void SculptureMorphChoreographer::replacePlaylist(std::vector<MorphTarget> targets) noexcept {
    clearPlaylist();
    playlist_.swap(targets);
}

void SculptureMorphChoreographer::addPlaylistPreset(
    const SculpturePreset& preset,
    float durationSeconds,
    float dwellSeconds,
    MorphEasing easing
) {
    MorphEnvelopeConfig env{};
    env.easing = easing;
    playlist_.push_back(MorphTarget{
        .params = preset.params,
        .name = std::string(preset.name),
        .durationSeconds = durationSeconds,
        .dwellSeconds = dwellSeconds,
        .easing = easing,
        .envelope = env
    });
}

void SculptureMorphChoreographer::addPlaylistPreset(
    const SculpturePreset& preset,
    float durationSeconds,
    float dwellSeconds,
    const MorphEnvelopeConfig& envelope
) {
    playlist_.push_back(MorphTarget{
        .params = preset.params,
        .name = std::string(preset.name),
        .durationSeconds = durationSeconds,
        .dwellSeconds = dwellSeconds,
        .easing = envelope.easing,
        .envelope = envelope
    });
}

void SculptureMorphChoreographer::addPlaylistAsset(
    const SculpturePresetAsset& asset,
    float durationSeconds,
    float dwellSeconds,
    MorphEasing easing
) {
    MorphEnvelopeConfig env{};
    env.easing = easing;
    playlist_.push_back(MorphTarget{
        .params = asset.params,
        .name = asset.title.empty() ? asset.id : asset.title,
        .presetId = asset.id,
        .durationSeconds = durationSeconds,
        .dwellSeconds = dwellSeconds,
        .easing = easing,
        .envelope = env,
        .presetRevision = asset.revision
    });
}

void SculptureMorphChoreographer::addPlaylistAsset(
    const SculpturePresetAsset& asset,
    float durationSeconds,
    float dwellSeconds,
    const MorphEnvelopeConfig& envelope
) {
    playlist_.push_back(MorphTarget{
        .params = asset.params,
        .name = asset.title.empty() ? asset.id : asset.title,
        .presetId = asset.id,
        .durationSeconds = durationSeconds,
        .dwellSeconds = dwellSeconds,
        .easing = envelope.easing,
        .envelope = envelope,
        .presetRevision = asset.revision
    });
}

std::size_t SculptureMorphChoreographer::addPlaylistDirectory(
    const std::filesystem::path& directoryPath,
    float durationSeconds,
    float dwellSeconds,
    MorphEasing easing
) {
    std::error_code ec;
    if (!std::filesystem::is_directory(directoryPath, ec)) return 0U;

    std::vector<std::filesystem::path> files;
    for (std::filesystem::directory_iterator it(directoryPath, std::filesystem::directory_options::skip_permission_denied, ec), end;
         it != end; it.increment(ec)) {
        if (ec) { ec.clear(); continue; }
        if (it->is_regular_file(ec) && it->path().extension() == ".epimode") {
            files.push_back(it->path());
        }
        ec.clear();
    }
    std::sort(files.begin(), files.end());

    std::size_t addedCount = 0U;
    for (const auto& file : files) {
        SculptureWorldState state{};
        SculptureProgramParams params{};
        std::string title;
        std::vector<std::string> routes;
        if (SculptureSerializer::loadEpimodeFromFile(file.string(), state, title, params, routes)) {
            MorphEnvelopeConfig env{};
            env.easing = easing;
            playlist_.push_back(MorphTarget{
                .params = params,
                .name = title.empty() ? file.stem().string() : title,
                .durationSeconds = durationSeconds,
                .dwellSeconds = dwellSeconds,
                .easing = easing,
                .envelope = env
            });
            ++addedCount;
        }
    }
    return addedCount;
}

void SculptureMorphChoreographer::startPlaylist(bool loop) noexcept {
    (void)tryStartPlaylistFrom(0, loop);
}

void SculptureMorphChoreographer::startPlaylistFrom(std::size_t index, bool loop) noexcept {
    (void)tryStartPlaylistFrom(index, loop);
}

bool SculptureMorphChoreographer::tryStartPlaylistFrom(std::size_t index, bool loop) noexcept {
    // Mode can be selected before targets are authored. Check the populated
    // timeline here even when the mode itself has not changed since then.
    if (playlist_.empty() || !isPlaylistTimingValid(tourMode_)) return false;
    playlistLoop_ = loop;
    playlistActive_ = true;
    playlistPaused_ = false;
    enterPlaylistCue(index % playlist_.size(), true);
    return true;
}

void SculptureMorphChoreographer::enterPlaylistCue(
    std::size_t index, bool startTransition) noexcept {
    // Callers supply a valid cue in a nonempty playlist. Keep playback policy
    // (active/paused/looping) with them; this method prepares only cue-local state.
    playlistIndex_ = index;
    playlistDwellTimer_ = 0.0f;
    // The spline evaluator derives its cue from this clock on every update.
    // Every explicit entry path must agree on its time origin.
    splineGlobalTime_ = 0.0f;
    if (tourMode_ == TourPlaybackMode::ContinuousSpline) {
        for (std::size_t i = 0; i < playlistIndex_; ++i) {
            splineGlobalTime_ += tour_timing::effectiveCueDuration(playlist_[i].durationSeconds);
        }
    }

    const auto& target = playlist_[playlistIndex_];
    if (startTransition) {
        setTarget(target.params, target.durationSeconds, target.envelope);
    } else {
        // Settle all endpoints at the selected cue, including later snap readers.
        targetParams_ = target.params;
        snapToTarget();
    }
}

void SculptureMorphChoreographer::stopPlaylist() noexcept {
    if (playlistActive_) {
        // Spline playback bypasses the ordinary morph clock. Settle both paths
        // at the visible value so neither update nor a later snap revives it.
        targetParams_ = currentParams_;
        snapToTarget();
    }
    playlistActive_ = false;
    playlistPaused_ = false;
}

void SculptureMorphChoreographer::stepPlaylistNext(bool startTransition) noexcept {
    if (playlist_.empty() || !isPlaylistTimingValid(tourMode_)) return;
    enterPlaylistCue((playlistIndex_ + 1U) % playlist_.size(), startTransition);
}

void SculptureMorphChoreographer::stepPlaylistPrevious(bool startTransition) noexcept {
    if (playlist_.empty() || !isPlaylistTimingValid(tourMode_)) return;
    enterPlaylistCue((playlistIndex_ + playlist_.size() - 1U) % playlist_.size(),
                     startTransition);
}

void SculptureMorphChoreographer::togglePlaylistPause() noexcept {
    playlistPaused_ = !playlistPaused_;
}

void SculptureMorphChoreographer::setPlaylistPaused(bool paused) noexcept {
    playlistPaused_ = paused;
}

void SculptureMorphChoreographer::setTourPlaybackMode(TourPlaybackMode mode) noexcept {
    (void)trySetTourPlaybackMode(mode);
}

bool SculptureMorphChoreographer::trySetTourPlaybackMode(TourPlaybackMode mode) noexcept {
    if (mode == tourMode_) return true;
    if (playlistActive_ && playlistIndex_ >= playlist_.size()) return false;
    if (!isPlaylistTimingValid(mode)) return false;

    tourMode_ = mode;
    if (playlistActive_) {
        // Share seek's cue-local clock/parameter setup without restarting the
        // playlist or changing pause/loop flags. Current values remain until update.
        enterPlaylistCue(playlistIndex_, true);
    }
    return true;
}

bool SculptureMorphChoreographer::isPlaylistTimingValid(TourPlaybackMode mode) const noexcept {
    if (mode == TourPlaybackMode::StepAndDwell) return true;
    if (mode != TourPlaybackMode::ContinuousSpline) return false;
    float total = 0.0f;
    for (const auto& target : playlist_) {
        if (!tour_timing::tryAppendCueDuration(total, target.durationSeconds)) return false;
    }
    return true;
}

void SculptureMorphChoreographer::restorePlaylistRuntime(
    std::size_t index, bool loop, bool paused, float dwellSeconds, bool active) noexcept {
    if (playlist_.empty()) return;
    if (active) {
        // A rejected start must not be followed by dwell/pause mutations below.
        if (!tryStartPlaylistFrom(index, loop)) return;
    } else {
        // Captured values can lie between authored endpoints. Restore only the
        // stopped selection metadata, never replace that moment with cue params.
        playlistIndex_ = index % playlist_.size();
        playlistLoop_ = loop;
        playlistActive_ = false;
        splineGlobalTime_ = 0.0f;
        targetParams_ = currentParams_;
        snapToTarget();
    }
    const float maxDwell = std::max(playlist_[playlistIndex_].dwellSeconds, 0.0f);
    playlistDwellTimer_ = std::clamp(dwellSeconds, 0.0f, maxDwell);
    playlistPaused_ = paused;
}

std::string_view SculptureMorphChoreographer::currentPlaylistTargetName() const noexcept {
    if (playlist_.empty() || playlistIndex_ >= playlist_.size()) {
        return "";
    }
    return playlist_[playlistIndex_].name;
}

float SculptureMorphChoreographer::playlistDwellProgress() const noexcept {
    if (playlist_.empty() || playlistIndex_ >= playlist_.size()) {
        return 0.0f;
    }
    const float dwell = playlist_[playlistIndex_].dwellSeconds;
    if (dwell <= 0.0f) return 1.0f;
    return std::clamp(playlistDwellTimer_ / dwell, 0.0f, 1.0f);
}

float SculptureMorphChoreographer::splineTotalDuration() const noexcept {
    float total = 0.0f;
    for (const auto& target : playlist_) {
        total += tour_timing::effectiveCueDuration(target.durationSeconds);
    }
    return total;
}

} // namespace epi_sculpt
