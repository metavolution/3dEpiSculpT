#pragma once

#include "SculptureProgramSchema.h"
#include <vector>
#include <array>
#include <optional>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <limits>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: Axis-Aligned Bounding Box (AABB)
// -----------------------------------------------------------------------------
struct AABB {
    // Empty union identity across the entire finite float domain.
    SculptureVec3 minPoint{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()};
    SculptureVec3 maxPoint{
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()};

    void expand(const SculptureVec3& p, float radius = 0.0f) noexcept {
        minPoint.x = std::min(minPoint.x, p.x - radius);
        minPoint.y = std::min(minPoint.y, p.y - radius);
        minPoint.z = std::min(minPoint.z, p.z - radius);
        maxPoint.x = std::max(maxPoint.x, p.x + radius);
        maxPoint.y = std::max(maxPoint.y, p.y + radius);
        maxPoint.z = std::max(maxPoint.z, p.z + radius);
    }

    void expand(const AABB& other) noexcept {
        minPoint.x = std::min(minPoint.x, other.minPoint.x);
        minPoint.y = std::min(minPoint.y, other.minPoint.y);
        minPoint.z = std::min(minPoint.z, other.minPoint.z);
        maxPoint.x = std::max(maxPoint.x, other.maxPoint.x);
        maxPoint.y = std::max(maxPoint.y, other.maxPoint.y);
        maxPoint.z = std::max(maxPoint.z, other.maxPoint.z);
    }

    [[nodiscard]] bool intersectsRay(const SculptureVec3& origin, const SculptureVec3& invDir, float tMin, float tMax) const noexcept {
        return intersectsRaySlabs(origin, {invDir.x, invDir.y, invDir.z}, tMin, tMax);
    }

    // Prepare reciprocals once per ray. Infinity denotes an exactly parallel
    // axis; double keeps tiny nonzero float directions distinct from parallel.
    [[nodiscard]] bool intersectsRaySlabs(const SculptureVec3& origin, const std::array<double, 3>& invDir, double tMin, double tMax) const noexcept {
        if (!(tMin <= tMax)) return false;

        const auto clipAxis = [&](double lower, double upper, double position, double inverse) {
            if (!std::isfinite(lower) || !std::isfinite(upper) || lower > upper ||
                !std::isfinite(position) || std::isnan(inverse) || inverse == 0.0) {
                return false;
            }
            // A parallel ray on either closed slab face remains inside. Avoid
            // 0 * infinity, which would turn a boundary into a NaN interval.
            if (std::isinf(inverse)) return lower <= position && position <= upper;

            const double t0 = (lower - position) * inverse;
            const double t1 = (upper - position) * inverse;
            tMin = std::max(tMin, std::min(t0, t1));
            tMax = std::min(tMax, std::max(t0, t1));
            return tMin <= tMax;
        };

        return clipAxis(minPoint.x, maxPoint.x, origin.x, invDir[0]) &&
               clipAxis(minPoint.y, maxPoint.y, origin.y, invDir[1]) &&
               clipAxis(minPoint.z, maxPoint.z, origin.z, invDir[2]);
    }

    [[nodiscard]] bool intersectsSphere(const SculptureVec3& center, float radius) const noexcept {
        float cx = std::clamp(center.x, minPoint.x, maxPoint.x);
        float cy = std::clamp(center.y, minPoint.y, maxPoint.y);
        float cz = std::clamp(center.z, minPoint.z, maxPoint.z);
        float dx = cx - center.x;
        float dy = cy - center.y;
        float dz = cz - center.z;
        return (dx * dx + dy * dy + dz * dz) <= (radius * radius);
    }
};

// -----------------------------------------------------------------------------
// Educational Annotation: LBVH Node Hierarchy
// -----------------------------------------------------------------------------
struct LBVHNode {
    AABB bounds;
    std::int32_t leftChild = -1;  // >= 0: internal child index; -1: leaf node
    std::int32_t rightChild = -1; // >= 0: internal child index; -1: leaf node
    std::uint32_t sampleIndex = 0xFFFFFFFFU; // If leaf, index into snapshot.samples

    [[nodiscard]] bool isLeaf() const noexcept {
        return leftChild == -1 && rightChild == -1;
    }
};

// -----------------------------------------------------------------------------
// Educational Annotation: Ray Hit Result Structure
// -----------------------------------------------------------------------------
struct RayHitResult {
    bool hit = false;
    float t = 1e30f;
    std::uint32_t sampleIndex = 0xFFFFFFFFU;
    SculptureVec3 hitPoint{0.0f, 0.0f, 0.0f};
    SculptureVec3 normal{0.0f, 0.0f, 0.0f};
};

// -----------------------------------------------------------------------------
// Educational Annotation: Linear Bounding Volume Hierarchy (LBVH)
// -----------------------------------------------------------------------------
class SculptureSpatialLBVH {
public:
    SculptureSpatialLBVH() = default;

    // Indexes positive finite-radius samples with finite centers and finite
    // float-range bounds. Excluded samples remain untouched in the snapshot.
    // Active count/bounds/leaves all describe this same admitted set.
    // Borrows the snapshot; rebuild after mutation. Construction is O(N log N).
    void build(const SculptureSnapshot& snapshot);

    // Finite nonzero directions are normalized; t/maxDistance are world distances.
    // Invalid inputs return no hit. Positive infinity permits an unbounded ray.
    // Returns the nearest surface hit with a positive finite float distance
    // below maxDistance using branch-and-bound traversal. A ray beginning inside
    // a sphere finds its exit.
    [[nodiscard]] RayHitResult intersectRay(const SculptureVec3& origin, const SculptureVec3& direction, float maxDistance = 100.0f) const noexcept;

    // Queries all sample indices within spherical radius R in O(K + log N)
    [[nodiscard]] std::vector<std::uint32_t> queryRadius(const SculptureVec3& center, float radius) const;

    // Morton 30-bit encoding utility. Finite coordinates clamp to the domain;
    // invalid/inverted axes or extents <= 1e-6f contribute zero bits.
    [[nodiscard]] static std::uint32_t encodeMorton3D(float x, float y, float z, const AABB& domain) noexcept;
    [[nodiscard]] static std::uint32_t expandBits(std::uint32_t v) noexcept;

    [[nodiscard]] std::size_t nodeCount() const noexcept { return nodes_.size(); }
    [[nodiscard]] std::size_t activeSampleCount() const noexcept { return activeSampleCount_; }
    [[nodiscard]] const AABB& rootBounds() const noexcept { return rootBounds_; }

private:
    std::vector<LBVHNode> nodes_;
    std::vector<std::uint32_t> sortedSampleIndices_;
    const SculptureSnapshot* snapshot_ = nullptr;
    AABB rootBounds_;
    std::size_t activeSampleCount_ = 0;

    std::int32_t buildRecursive(std::size_t start, std::size_t end);
};

} // namespace epi_sculpt
