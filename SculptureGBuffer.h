#pragma once

// Stored observation data shared by producers and derived inspection.
// This contract deliberately has no mathematical evaluator or native backend.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace epi_sculpt {

namespace detail {
template<std::size_t Channels>
inline std::size_t checkedRaymarchElementCount(std::uint32_t width, std::uint32_t height) {
    static_assert(Channels > 0);
    // Divide before multiplying: both size_t overflow and vector's smaller
    // element limit must be rejected before allocation or field observation.
    const auto maxPixels = std::vector<float>{}.max_size() / Channels;
    if (height != 0 && static_cast<std::size_t>(width) > maxPixels / height) {
        throw std::length_error("raymarch output exceeds float-vector element limit");
    }
    return static_cast<std::size_t>(width) * height * Channels;
}
} // namespace detail

// -----------------------------------------------------------------------------
// Educational Annotation: Implicit Field Geometry Buffer (G-Buffer)
// Enables deep composition of continuous mathematical raymarched manifolds
// with rasterized Vulkan instanced geometry (polytopes, relief meshes, mist).
// Outputs 5 orthogonal channels:
// 1. Radiance (RGBA32F): RGB linear radiance + A optical transmittance
// 2. Depth (R32F): Planar view-space linear camera depth for hardware Z-testing
// 3. Normal (RGBA32F): World surface normal (Nx, Ny, Nz) + Material/TrapDensity in W
// 4. Velocity (RG32F): Screen-space motion vector delta (Vx, Vy) in NDC [-1, 1]
// 5. Attributes (RGBA32F): AO, Orbit Energy, Step Count effort, Hit Flag
// -----------------------------------------------------------------------------
struct SculptureGBuffer {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::vector<float> radiance;   // RGBA32F (R, G, B, Transmittance/Alpha)
    std::vector<float> depth;      // R32F (Linear camera view depth along ray forward)
    std::vector<float> normal;     // RGBA32F (Nx, Ny, Nz, Material/TrapDensity in W)
    std::vector<float> velocity;   // RG32F (Screen-space motion delta Vx, Vy in NDC [-1, 1])
    std::vector<float> attributes; // RGBA32F (AO, Orbit Energy, Normalized Step Count, Hit Flag)

    // Rejection/allocation failure preserves dimensions, sizes and values.
    // Reserve may still change capacities and invalidate borrowed data pointers.
    void resize(std::uint32_t w, std::uint32_t h) {
        const auto rgbaCount = detail::checkedRaymarchElementCount<4>(w, h);
        reserveOutput(rgbaCount);
        commitOutput(w, h, rgbaCount);
    }

    [[nodiscard]] bool empty() const noexcept {
        return width == 0U || height == 0U;
    }

    void clear() noexcept {
        width = 0U;
        height = 0U;
        radiance.clear();
        depth.clear();
        normal.clear();
        velocity.clear();
        attributes.clear();
    }

private:
    friend struct SculptureStereoGBufferPair;

    // Reserve may relocate storage, but preserves values/sizes on failure.
    // Finish every channel/eye reservation before publishing new output shape.
    void reserveOutput(std::size_t rgbaCount) {
        radiance.reserve(rgbaCount);
        depth.reserve(rgbaCount / 4U);
        normal.reserve(rgbaCount);
        velocity.reserve(rgbaCount / 2U);
        attributes.reserve(rgbaCount);
    }

    void commitOutput(std::uint32_t w, std::uint32_t h, std::size_t rgbaCount) noexcept {
        // All capacities are sufficient and float initialization cannot throw.
        // Repeated extents reuse storage without a second full-image snapshot.
        radiance.resize(rgbaCount);
        depth.resize(rgbaCount / 4U);
        normal.resize(rgbaCount);
        velocity.resize(rgbaCount / 2U);
        attributes.resize(rgbaCount);
        std::fill(radiance.begin(), radiance.end(), 0.0f);
        std::fill(depth.begin(), depth.end(), 1e6f);
        std::fill(normal.begin(), normal.end(), 0.0f);
        std::fill(velocity.begin(), velocity.end(), 0.0f);
        std::fill(attributes.begin(), attributes.end(), 0.0f);
        width = w;
        height = h;
    }
};

struct SculptureStereoGBufferPair {
    SculptureGBuffer left;
    SculptureGBuffer right;

    // Both eyes reserve successfully before either eye's output is replaced.
    void resize(std::uint32_t w, std::uint32_t h) {
        const auto rgbaCount = detail::checkedRaymarchElementCount<4>(w, h);
        left.reserveOutput(rgbaCount);
        right.reserveOutput(rgbaCount);
        left.commitOutput(w, h, rgbaCount);
        right.commitOutput(w, h, rgbaCount);
    }

    [[nodiscard]] bool empty() const noexcept {
        return left.empty() || right.empty();
    }

    void clear() noexcept {
        left.clear();
        right.clear();
    }
};

} // namespace epi_sculpt
