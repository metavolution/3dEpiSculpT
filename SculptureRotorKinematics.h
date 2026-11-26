#pragma once

#include <cmath>
#include <initializer_list>

namespace epi_sculpt {

namespace rotor_kinematics_detail {
inline constexpr float kTwoPi = 6.28318530718f;

[[nodiscard]] inline float wrap(float angle) noexcept {
    // Preserve the ordinary no-wrap/one-wrap arithmetic and inclusive signed
    // endpoints. Large inputs use remainder reduction: subtracting one turn
    // repeatedly can make no progress once a float's spacing exceeds 2*pi.
    if (angle >= -kTwoPi && angle <= kTwoPi) return angle;
    if (angle > kTwoPi && angle <= 2.0f * kTwoPi) return angle - kTwoPi;
    if (angle < -kTwoPi && angle >= -2.0f * kTwoPi) return angle + kTwoPi;
    const float remainder = std::fmod(angle, kTwoPi);
    return remainder == 0.0f ? std::copysign(kTwoPi, angle) : remainder;
}
} // namespace rotor_kinematics_detail

// SO(4) uses commuting left/right isoclinic angles; the third rate is their mean.
// Prepare all four scalar outputs before committing. Invalid inputs/overflow
// leave caller-owned state unchanged; lifecycle/cadence policy stays with callers.
[[nodiscard]] inline bool tryEvaluateContinuousRotorKinematics(
    float deltaSeconds,
    float rotSpeedL,
    float rotSpeedR,
    float entrainment,
    float tapeFactor,
    float& outContinuousRotorAlpha,
    float& outContinuousRotorBeta,
    float& outContinuousRotorTheta,
    float& outLatentW,
    float time
) noexcept {
    for (float value : {deltaSeconds, rotSpeedL, rotSpeedR, entrainment, tapeFactor, time,
                        outContinuousRotorAlpha, outContinuousRotorBeta, outContinuousRotorTheta, outLatentW}) {
        if (!std::isfinite(value)) return false;
    }
    float alpha = outContinuousRotorAlpha + rotSpeedL * entrainment * tapeFactor * deltaSeconds;
    float beta = outContinuousRotorBeta + rotSpeedR * entrainment * tapeFactor * deltaSeconds;
    float theta = outContinuousRotorTheta + 0.5f * (rotSpeedL + rotSpeedR) * entrainment * tapeFactor * deltaSeconds;
    if (!std::isfinite(alpha) || !std::isfinite(beta) || !std::isfinite(theta)) return false;

    alpha = rotor_kinematics_detail::wrap(alpha);
    beta = rotor_kinematics_detail::wrap(beta);
    theta = rotor_kinematics_detail::wrap(theta);
    const float latentW = outLatentW + 0.25f * std::sin(0.45f * time + theta);
    if (!std::isfinite(latentW)) return false;

    outContinuousRotorAlpha = alpha;
    outContinuousRotorBeta = beta;
    outContinuousRotorTheta = theta;
    outLatentW = latentW;
    return true;
}

// Existing scalar consumers keep their hold-on-refusal API. Transactional
// callers use the checked entry point above; both execute the same mathematics.
inline void evaluateContinuousRotorKinematics(
    float deltaSeconds,
    float rotSpeedL,
    float rotSpeedR,
    float entrainment,
    float tapeFactor,
    float& outContinuousRotorAlpha,
    float& outContinuousRotorBeta,
    float& outContinuousRotorTheta,
    float& outLatentW,
    float time
) noexcept {
    static_cast<void>(tryEvaluateContinuousRotorKinematics(
        deltaSeconds, rotSpeedL, rotSpeedR, entrainment, tapeFactor,
        outContinuousRotorAlpha, outContinuousRotorBeta, outContinuousRotorTheta,
        outLatentW, time));
}

} // namespace epi_sculpt
