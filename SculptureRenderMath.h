#pragma once

#include <mgfx/math/Types.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: OPTIMIZED ANALYTICAL RENDER MATH
// -----------------------------------------------------------------------------
// This module provides direct, analytical constructions of 4x4 instance model
// matrices, bypassing standard chained multiplications (Translate * Rotate * Scale).
// By directly mapping known transformation vectors (e.g., position/uniform-scale,
// orientation quaternions) to matrix columns, we save ~128 FLOPs per instance.
// This is critical for scaling procedural worlds to 100,000+ instances without
// bottlenecking the CPU during render bridge batch generation.
// -----------------------------------------------------------------------------
struct SculptureRenderMath {
    // Analytically constructs a T * R * S matrix given uniform scale and quaternion.
    [[nodiscard]] static inline mgfx::math::mat4 composeTransform(
        const mgfx::math::vec3& position,
        const glm::quat& normalizedRotation,
        float uniformScale) noexcept {
        
        const float xx = normalizedRotation.x * normalizedRotation.x;
        const float yy = normalizedRotation.y * normalizedRotation.y;
        const float zz = normalizedRotation.z * normalizedRotation.z;
        const float xz = normalizedRotation.x * normalizedRotation.z;
        const float xy = normalizedRotation.x * normalizedRotation.y;
        const float yz = normalizedRotation.y * normalizedRotation.z;
        const float wx = normalizedRotation.w * normalizedRotation.x;
        const float wy = normalizedRotation.w * normalizedRotation.y;
        const float wz = normalizedRotation.w * normalizedRotation.z;

        mgfx::math::mat4 m;
        m[0][0] = uniformScale * (1.0f - 2.0f * (yy + zz));
        m[0][1] = uniformScale * (2.0f * (xy + wz));
        m[0][2] = uniformScale * (2.0f * (xz - wy));
        m[0][3] = 0.0f;
        
        m[1][0] = uniformScale * (2.0f * (xy - wz));
        m[1][1] = uniformScale * (1.0f - 2.0f * (xx + zz));
        m[1][2] = uniformScale * (2.0f * (yz + wx));
        m[1][3] = 0.0f;
        
        m[2][0] = uniformScale * (2.0f * (xz + wy));
        m[2][1] = uniformScale * (2.0f * (yz - wx));
        m[2][2] = uniformScale * (1.0f - 2.0f * (xx + yy));
        m[2][3] = 0.0f;
        
        m[3][0] = position.x;
        m[3][1] = position.y;
        m[3][2] = position.z;
        m[3][3] = 1.0f;
        return m;
    }

    // Analytically constructs a Translation * Scale matrix
    [[nodiscard]] static inline mgfx::math::mat4 composeTranslationScale(
        const mgfx::math::vec3& position,
        float uniformScale) noexcept {
        
        mgfx::math::mat4 m(1.0f); // Identity
        m[0][0] = uniformScale;
        m[1][1] = uniformScale;
        m[2][2] = uniformScale;
        m[3][0] = position.x;
        m[3][1] = position.y;
        m[3][2] = position.z;
        return m;
    }

    // Analytically constructs a matrix spanning a cylinder between two points (centered at midpoint)
    [[nodiscard]] static inline mgfx::math::mat4 composeCylinderBetween(
        const mgfx::math::vec3& start,
        const mgfx::math::vec3& end,
        float radius) noexcept {
        
        const mgfx::math::vec3 diff = end - start;
        const float length = glm::length(diff);
        if (!std::isfinite(length) || length <= 1.0e-6f || !std::isfinite(radius) || radius <= 0.0f) {
            return mgfx::math::mat4(1.0f);
        }
        
        const mgfx::math::vec3 axis = diff / length;
        const mgfx::math::vec3 localUp{0.0f, 1.0f, 0.0f};
        const float alignment = glm::dot(localUp, axis);
        
        // Exact same quaternion rotation as glm::rotation(localUp, axis)
        glm::quat rotation;
        if (alignment < -0.9999f) {
            rotation = glm::angleAxis(3.14159265359f, mgfx::math::vec3(1.0f, 0.0f, 0.0f));
        } else {
            const mgfx::math::vec3 cross = glm::cross(localUp, axis);
            rotation = glm::quat(1.0f + alignment, cross.x, cross.y, cross.z);
            rotation = glm::normalize(rotation);
        }
        
        const float xx = rotation.x * rotation.x;
        const float yy = rotation.y * rotation.y;
        const float zz = rotation.z * rotation.z;
        const float xz = rotation.x * rotation.z;
        const float xy = rotation.x * rotation.y;
        const float yz = rotation.y * rotation.z;
        const float wx = rotation.w * rotation.x;
        const float wy = rotation.w * rotation.y;
        const float wz = rotation.w * rotation.z;

        mgfx::math::mat4 m;
        m[0][0] = radius * (1.0f - 2.0f * (yy + zz));
        m[0][1] = radius * (2.0f * (xy + wz));
        m[0][2] = radius * (2.0f * (xz - wy));
        m[0][3] = 0.0f;
        
        m[1][0] = length * (2.0f * (xy - wz));
        m[1][1] = length * (1.0f - 2.0f * (xx + zz));
        m[1][2] = length * (2.0f * (yz + wx));
        m[1][3] = 0.0f;
        
        m[2][0] = radius * (2.0f * (xz + wy));
        m[2][1] = radius * (2.0f * (yz - wx));
        m[2][2] = radius * (1.0f - 2.0f * (xx + yy));
        m[2][3] = 0.0f;
        
        const mgfx::math::vec3 midpoint = (start + end) * 0.5f;
        m[3][0] = midpoint.x;
        m[3][1] = midpoint.y;
        m[3][2] = midpoint.z;
        m[3][3] = 1.0f;
        return m;
    }
};

} // namespace epi_sculpt
