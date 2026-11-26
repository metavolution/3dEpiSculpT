#pragma once

#include "SculptureProgramSchema.h"
#include <cmath>
#include <cstdint>
#include <vector>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: 2ND-ORDER CRITICALLY DAMPED SPATIAL GESTURE FILTER
// -----------------------------------------------------------------------------
// High-fidelity XR controller and optical hand tracking data exhibits micro-jitter
// and latency spikes. This filter applies an analytical second-order spring-damper
// system to 6-DOF poses and conformal gesture envelopes:
//
//   ddot{x} + 2*zeta*omega_0 * dot{x} + omega_0^2 * (x - x_target) = 0
//
// - For critical damping (zeta = 1.0), the system reaches the target pose in
//   minimal time without overshoot or resonant oscillation.
// - Velocity clamping and exponential envelope decay prevent erratic parameter jumps
//   when tracking is intermittently lost.
// -----------------------------------------------------------------------------

struct Quaternion4D {
    float w = 1.0f;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    [[nodiscard]] static constexpr Quaternion4D identity() noexcept { return {1.0f, 0.0f, 0.0f, 0.0f}; }
    [[nodiscard]] SculptureVec3 rotate(const SculptureVec3& v) const noexcept;
    [[nodiscard]] static Quaternion4D slerp(const Quaternion4D& q1, const Quaternion4D& q2, float t) noexcept;
};

struct Pose6DOF {
    SculptureVec3 position{0.0f, 0.0f, 0.0f};
    Quaternion4D orientation{1.0f, 0.0f, 0.0f, 0.0f};
    float trigger = 0.0f; // [0.0, 1.0] primary index trigger
    float grip = 0.0f;    // [0.0, 1.0] secondary palm grip
    bool valid = false;
};

enum class GestureType : std::uint8_t {
    Idle = 0,
    ConformalPinchRotate = 1, // Two-handed scaling and phase rotation
    GrabTranslate = 2,        // Single-handed spatial translation
    VortexInject = 3,         // Localized fluid curl velocity injection
    TopologySculpt = 4        // Direct continuous topology morphing
};

struct GestureFilterConfig {
    float naturalFrequency = 18.0f; // omega_0: Higher = snappier, Lower = smoother
    float dampingRatio = 1.0f;      // zeta: 1.0 = critically damped (no overshoot)
    float maxVelocity = 15.0f;      // Maximum tracking linear velocity (m/s)
    float deadbandRadius = 0.002f;  // Jitter deadband (2mm)
    float pinchThreshold = 0.75f;   // Trigger/grip threshold for pinch engagement
    float twistSensitivity = 1.5f;  // Angular twist amplification factor
};

struct FilteredSpatialHands {
    Pose6DOF leftHand{};
    Pose6DOF rightHand{};
    SculptureVec3 leftVelocity{};
    SculptureVec3 rightVelocity{};
    float bimanualDistance = 0.0f;
    float bimanualTwistAngle = 0.0f;
    float bimanualPinchRatio = 1.0f;
    GestureType recognizedGesture = GestureType::Idle;
    bool isEngaged = false;
};

class SculptureSpatialGestureFilter {
public:
    SculptureSpatialGestureFilter() noexcept;
    explicit SculptureSpatialGestureFilter(const GestureFilterConfig& config) noexcept;

    void setConfig(const GestureFilterConfig& config) noexcept { config_ = config; }
    [[nodiscard]] const GestureFilterConfig& config() const noexcept { return config_; }

    // Resets filter state to neutral origin
    void reset() noexcept;

    // Evaluates raw 6-DOF hand poses and applies 2nd-order critically damped filtering
    [[nodiscard]] FilteredSpatialHands update(
        const Pose6DOF& rawLeft,
        const Pose6DOF& rawRight,
        float deltaSeconds
    ) noexcept;

    // Mutates sculpture program parameters based on filtered bimanual gestures
    void applyToProgramParams(
        const FilteredSpatialHands& filtered,
        SculptureProgramParams& outParams,
        float deltaSeconds
    ) const noexcept;

private:
    [[nodiscard]] static SculptureVec3 filterVectorCriticallyDamped(
        const SculptureVec3& current,
        SculptureVec3& velocity,
        const SculptureVec3& target,
        float omega0,
        float maxVel,
        float deadband,
        float dt
    ) noexcept;

    GestureFilterConfig config_{};
    Pose6DOF filteredLeft_{};
    Pose6DOF filteredRight_{};
    SculptureVec3 leftVel_{};
    SculptureVec3 rightVel_{};
    float initialBimanualDist_{0.0f};
    float initialBimanualAngle_{0.0f};
    bool wasEngaged_{false};
};

} // namespace epi_sculpt
