#include "SculptureSpatialGestureFilter.h"
#include <algorithm>
#include <cmath>

namespace epi_sculpt {

namespace {

inline float length(const SculptureVec3& v) noexcept {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline SculptureVec3 clampLength(const SculptureVec3& v, float maxLen) noexcept {
    const float len = length(v);
    if (len > maxLen && len > 1e-6f) {
        const float factor = maxLen / len;
        return {v.x * factor, v.y * factor, v.z * factor};
    }
    return v;
}

} // namespace

SculptureSpatialGestureFilter::SculptureSpatialGestureFilter() noexcept
    : SculptureSpatialGestureFilter(GestureFilterConfig{}) {}

SculptureSpatialGestureFilter::SculptureSpatialGestureFilter(const GestureFilterConfig& config) noexcept
    : config_(config) {
    reset();
}

void SculptureSpatialGestureFilter::reset() noexcept {
    filteredLeft_ = Pose6DOF{};
    filteredRight_ = Pose6DOF{};
    leftVel_ = SculptureVec3{};
    rightVel_ = SculptureVec3{};
    initialBimanualDist_ = 0.0f;
    initialBimanualAngle_ = 0.0f;
    wasEngaged_ = false;
}

SculptureVec3 SculptureSpatialGestureFilter::filterVectorCriticallyDamped(
    const SculptureVec3& current,
    SculptureVec3& velocity,
    const SculptureVec3& target,
    float omega0,
    float maxVel,
    float deadband,
    float dt
) noexcept {
    if (dt <= 0.0f) return current;

    SculptureVec3 error{
        target.x - current.x,
        target.y - current.y,
        target.z - current.z
    };

    const float errLen = length(error);
    if (errLen < deadband) {
        error = {0.0f, 0.0f, 0.0f};
    }

    // 2nd-order critically damped ODE: acceleration = omega_0^2 * error - 2 * omega_0 * velocity
    const float omegaSq = omega0 * omega0;
    const float twoOmega = 2.0f * omega0;

    const SculptureVec3 accel{
        omegaSq * error.x - twoOmega * velocity.x,
        omegaSq * error.y - twoOmega * velocity.y,
        omegaSq * error.z - twoOmega * velocity.z
    };

    velocity.x += accel.x * dt;
    velocity.y += accel.y * dt;
    velocity.z += accel.z * dt;
    velocity = clampLength(velocity, maxVel);

    return {
        current.x + velocity.x * dt,
        current.y + velocity.y * dt,
        current.z + velocity.z * dt
    };
}

FilteredSpatialHands SculptureSpatialGestureFilter::update(
    const Pose6DOF& rawLeft,
    const Pose6DOF& rawRight,
    float deltaSeconds
) noexcept {
    FilteredSpatialHands result{};
    const float dt = std::clamp(deltaSeconds, 0.0001f, 0.1f);

    // 1. Filter Left Hand Pose
    if (rawLeft.valid) {
        if (!filteredLeft_.valid) {
            filteredLeft_ = rawLeft;
            leftVel_ = {0.0f, 0.0f, 0.0f};
        } else {
            filteredLeft_.position = filterVectorCriticallyDamped(
                filteredLeft_.position, leftVel_, rawLeft.position,
                config_.naturalFrequency, config_.maxVelocity, config_.deadbandRadius, dt
            );
            filteredLeft_.orientation = Quaternion4D::slerp(
                filteredLeft_.orientation, rawLeft.orientation, std::clamp(config_.naturalFrequency * dt, 0.0f, 1.0f)
            );
            filteredLeft_.trigger = 0.8f * filteredLeft_.trigger + 0.2f * rawLeft.trigger;
            filteredLeft_.grip = 0.8f * filteredLeft_.grip + 0.2f * rawLeft.grip;
        }
        filteredLeft_.valid = true;
    } else {
        filteredLeft_.valid = false;
        leftVel_ = {0.0f, 0.0f, 0.0f};
    }

    // 2. Filter Right Hand Pose
    if (rawRight.valid) {
        if (!filteredRight_.valid) {
            filteredRight_ = rawRight;
            rightVel_ = {0.0f, 0.0f, 0.0f};
        } else {
            filteredRight_.position = filterVectorCriticallyDamped(
                filteredRight_.position, rightVel_, rawRight.position,
                config_.naturalFrequency, config_.maxVelocity, config_.deadbandRadius, dt
            );
            filteredRight_.orientation = Quaternion4D::slerp(
                filteredRight_.orientation, rawRight.orientation, std::clamp(config_.naturalFrequency * dt, 0.0f, 1.0f)
            );
            filteredRight_.trigger = 0.8f * filteredRight_.trigger + 0.2f * rawRight.trigger;
            filteredRight_.grip = 0.8f * filteredRight_.grip + 0.2f * rawRight.grip;
        }
        filteredRight_.valid = true;
    } else {
        filteredRight_.valid = false;
        rightVel_ = {0.0f, 0.0f, 0.0f};
    }

    result.leftHand = filteredLeft_;
    result.rightHand = filteredRight_;
    result.leftVelocity = leftVel_;
    result.rightVelocity = rightVel_;

    // 3. Evaluate Bimanual Conformal Gesture Metrics
    if (filteredLeft_.valid && filteredRight_.valid) {
        const float dx = filteredRight_.position.x - filteredLeft_.position.x;
        const float dy = filteredRight_.position.y - filteredLeft_.position.y;
        const float dz = filteredRight_.position.z - filteredLeft_.position.z;
        const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        const float angle = std::atan2(dy, dx);

        result.bimanualDistance = dist;
        result.bimanualTwistAngle = angle;

        const bool isDualPinching = (filteredLeft_.trigger > config_.pinchThreshold &&
                                     filteredRight_.trigger > config_.pinchThreshold);

        if (isDualPinching) {
            if (!wasEngaged_) {
                initialBimanualDist_ = std::max(dist, 0.01f);
                initialBimanualAngle_ = angle;
                wasEngaged_ = true;
            }

            result.bimanualPinchRatio = dist / initialBimanualDist_;
            result.bimanualTwistAngle = config_.twistSensitivity * (angle - initialBimanualAngle_);
            result.recognizedGesture = GestureType::ConformalPinchRotate;
            result.isEngaged = true;
        } else {
            wasEngaged_ = false;
            result.bimanualPinchRatio = 1.0f;
            result.bimanualTwistAngle = 0.0f;

            // Single-hand gesture fallbacks
            if (filteredRight_.grip > 0.75f || filteredLeft_.grip > 0.75f) {
                result.recognizedGesture = GestureType::GrabTranslate;
                result.isEngaged = true;
            } else if (length(leftVel_) > 1.5f || length(rightVel_) > 1.5f) {
                result.recognizedGesture = GestureType::VortexInject;
                result.isEngaged = true;
            } else {
                result.recognizedGesture = GestureType::Idle;
                result.isEngaged = false;
            }
        }
    } else {
        wasEngaged_ = false;
        result.recognizedGesture = GestureType::Idle;
        result.isEngaged = false;
    }

    return result;
}

void SculptureSpatialGestureFilter::applyToProgramParams(
    const FilteredSpatialHands& filtered,
    SculptureProgramParams& outParams,
    float deltaSeconds
) const noexcept {
    if (!filtered.isEngaged) return;

    switch (filtered.recognizedGesture) {
    case GestureType::ConformalPinchRotate: {
        const float scaleRate = (filtered.bimanualPinchRatio - 1.0f) * 1.5f;
        outParams.pre.scaleReal = std::clamp(outParams.pre.scaleReal * (1.0f + scaleRate * deltaSeconds), 0.1f, 10.0f);
        outParams.hueRotation += filtered.bimanualTwistAngle * deltaSeconds * 0.5f;
        outParams.seedWidth = std::clamp(outParams.seedWidth * (1.0f + (filtered.bimanualPinchRatio - 1.0f) * deltaSeconds), 0.05f, 2.5f);
        break;
    }
    case GestureType::GrabTranslate: {
        if (filtered.rightHand.valid && filtered.rightHand.grip > 0.75f) {
            outParams.pre.offsetReal += filtered.rightVelocity.x * deltaSeconds * 0.5f;
            outParams.pre.offsetImag += filtered.rightVelocity.y * deltaSeconds * 0.5f;
        } else if (filtered.leftHand.valid && filtered.leftHand.grip > 0.75f) {
            outParams.post.offsetReal += filtered.leftVelocity.x * deltaSeconds * 0.5f;
            outParams.post.offsetImag += filtered.leftVelocity.y * deltaSeconds * 0.5f;
        }
        break;
    }
    case GestureType::VortexInject: {
        const float maxSpd = std::max(length(filtered.leftVelocity), length(filtered.rightVelocity));
        outParams.curlStrength = std::clamp(outParams.curlStrength + maxSpd * deltaSeconds * 0.2f, 0.0f, 2.0f);
        outParams.advectionSpeed = std::clamp(outParams.advectionSpeed + maxSpd * deltaSeconds * 0.1f, 0.0f, 1.0f);
        break;
    }
    default:
        break;
    }
}

} // namespace epi_sculpt
