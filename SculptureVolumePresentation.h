#pragma once

#include "mgfx/ProceduralFieldMapping.h"

#include <cmath>
#include <optional>

namespace epi_sculpt {

// The OpenCL field grid uses q(i) = -2.5 + 0.1*i. Its presentation cube
// uses x = 4*q + 0.2, so those nodes land at the centers of the 50 texels.
// This is presentation geometry, not a change to the scientific recurrence.
[[nodiscard]] inline std::optional<mgfx::ProceduralFieldMapping>
makeSculptureVolumeFieldMapping(const mgfx::math::mat4& root) noexcept {
    for (unsigned c = 0; c < 4; ++c)
        for (unsigned r = 0; r < 4; ++r)
            if (!std::isfinite(root[c][r])) return std::nullopt;
    if (root[0][3] != 0.0f || root[1][3] != 0.0f || root[2][3] != 0.0f ||
        root[3][3] != 1.0f) return std::nullopt;

    const glm::dmat3 linear(root);
    const double x = glm::length(linear[0]);
    const double y = glm::length(linear[1]);
    const double z = glm::length(linear[2]);
    const double scale = (x + y + z) / 3.0;
    if (!(scale > 0.0) || !std::isfinite(scale)) return std::nullopt;
    // Allow ordinary floating-point rotation error, but not anisotropic scale,
    // shear or a reflected basis. Double intermediates avoid float overflow.
    constexpr double tolerance = 1.0e-5;
    if (std::abs(x - scale) > tolerance * scale ||
        std::abs(y - scale) > tolerance * scale ||
        std::abs(z - scale) > tolerance * scale ||
        std::abs(glm::dot(linear[0], linear[1])) > tolerance * x * y ||
        std::abs(glm::dot(linear[0], linear[2])) > tolerance * x * z ||
        std::abs(glm::dot(linear[1], linear[2])) > tolerance * y * z ||
        !(glm::determinant(linear) > 0.0)) return std::nullopt;

    glm::dmat4 localToTexture(1.0);
    localToTexture[0][0] = localToTexture[1][1] = localToTexture[2][2] = 0.05;
    localToTexture[3] = glm::dvec4(0.5, 0.5, 0.5, 1.0);
    mgfx::ProceduralFieldMapping mapping{};
    mapping.worldToTexture = mgfx::math::mat4(localToTexture * glm::inverse(glm::dmat4(root)));
    mapping.metrics = mgfx::math::vec4(static_cast<float>(20.0 * scale),
        static_cast<float>(0.1 * scale), static_cast<float>(5.0 * scale), 0.1f);
    if (!mapping.valid()) return std::nullopt;
    return mapping;
}

} // namespace epi_sculpt
