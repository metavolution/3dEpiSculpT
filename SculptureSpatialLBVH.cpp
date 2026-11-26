#include "SculptureSpatialLBVH.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace epi_sculpt {

namespace {

[[nodiscard]] bool isIndexableSample(const SculptureSample& sample) noexcept {
    if (!std::isfinite(sample.x) || !std::isfinite(sample.y) || !std::isfinite(sample.z) ||
        !std::isfinite(sample.radius) || sample.radius <= 0.0f) {
        return false;
    }

    // Widen before testing center +/- radius: rejecting a sample must happen
    // before float bounds overflow. Other sample attributes do not affect picking.
    const double largestCenter = std::max({
        std::abs(static_cast<double>(sample.x)),
        std::abs(static_cast<double>(sample.y)),
        std::abs(static_cast<double>(sample.z))});
    return largestCenter + static_cast<double>(sample.radius) <= std::numeric_limits<float>::max();
}

} // namespace

std::uint32_t SculptureSpatialLBVH::expandBits(std::uint32_t v) noexcept {
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}

std::uint32_t SculptureSpatialLBVH::encodeMorton3D(float x, float y, float z, const AABB& domain) noexcept {
    const auto quantizeAxis = [](float value, float lower, float upper) -> std::uint32_t {
        if (!std::isfinite(value) || !std::isfinite(lower) || !std::isfinite(upper) || lower > upper) {
            return 0U;
        }

        // A finite float domain can span more than float::max(). Keep both
        // differences wide, and admit/clamp before any floating-to-integer cast.
        const double extent = static_cast<double>(upper) - lower;
        if (extent <= 1e-6f) return 0U;
        const double coordinate = ((static_cast<double>(value) - lower) / extent) * 1023.0;
        return static_cast<std::uint32_t>(std::clamp(coordinate, 0.0, 1023.0));
    };

    const std::uint32_t ix = quantizeAxis(x, domain.minPoint.x, domain.maxPoint.x);
    const std::uint32_t iy = quantizeAxis(y, domain.minPoint.y, domain.maxPoint.y);
    const std::uint32_t iz = quantizeAxis(z, domain.minPoint.z, domain.maxPoint.z);

    return expandBits(ix) | (expandBits(iy) << 1) | (expandBits(iz) << 2);
}

void SculptureSpatialLBVH::build(const SculptureSnapshot& snapshot) {
    snapshot_ = &snapshot;
    nodes_.clear();
    sortedSampleIndices_.clear();
    rootBounds_ = AABB{};
    activeSampleCount_ = 0;

    struct SampleKey {
        std::uint32_t mortonCode;
        std::uint32_t sampleIndex;
    };

    std::vector<SampleKey> keys;
    keys.reserve(snapshot.samples.size());

    // Admit once. Bounds, keys, counts and leaves must use the same geometry,
    // rather than repeating subtly different validity policies in two passes.
    for (std::size_t i = 0; i < snapshot.samples.size(); ++i) {
        const auto& s = snapshot.samples[i];
        if (!isIndexableSample(s)) continue;
        rootBounds_.expand(SculptureVec3{s.x, s.y, s.z}, s.radius);
        keys.push_back({0U, static_cast<std::uint32_t>(i)});
    }

    activeSampleCount_ = keys.size();
    if (activeSampleCount_ == 0) return;

    // Generate Morton codes only after the complete admitted domain is known.
    for (auto& key : keys) {
        const auto& s = snapshot.samples[key.sampleIndex];
        key.mortonCode = encodeMorton3D(s.x, s.y, s.z, rootBounds_);
    }

    // Sort samples along 1D Morton space-filling curve
    std::sort(keys.begin(), keys.end(), [](const SampleKey& a, const SampleKey& b) {
        return a.mortonCode < b.mortonCode;
    });

    sortedSampleIndices_.resize(keys.size());
    for (std::size_t i = 0; i < keys.size(); ++i) {
        sortedSampleIndices_[i] = keys[i].sampleIndex;
    }

    // Reserve tree nodes (2*N - 1 upper bound)
    nodes_.reserve(keys.size() * 2);
    buildRecursive(0, sortedSampleIndices_.size());
}

std::int32_t SculptureSpatialLBVH::buildRecursive(std::size_t start, std::size_t end) {
    if (start >= end) return -1;

    std::int32_t nodeIndex = static_cast<std::int32_t>(nodes_.size());
    nodes_.emplace_back();

    if (end - start == 1) {
        // Leaf node
        std::uint32_t sampleIdx = sortedSampleIndices_[start];
        nodes_[nodeIndex].sampleIndex = sampleIdx;
        const auto& s = snapshot_->samples[sampleIdx];
        nodes_[nodeIndex].bounds.expand(SculptureVec3{s.x, s.y, s.z}, s.radius);
        return nodeIndex;
    }

    // Internal node: binary split along sorted Morton curve
    std::size_t mid = start + (end - start) / 2;
    std::int32_t left = buildRecursive(start, mid);
    std::int32_t right = buildRecursive(mid, end);

    nodes_[nodeIndex].leftChild = left;
    nodes_[nodeIndex].rightChild = right;
    
    AABB nodeBounds{};
    if (left >= 0) nodeBounds.expand(nodes_[left].bounds);
    if (right >= 0) nodeBounds.expand(nodes_[right].bounds);
    nodes_[nodeIndex].bounds = nodeBounds;

    return nodeIndex;
}

RayHitResult SculptureSpatialLBVH::intersectRay(const SculptureVec3& origin, const SculptureVec3& direction, float maxDistance) const noexcept {
    RayHitResult bestHit{};
    bestHit.t = maxDistance;

    if (nodes_.empty() || !snapshot_) return bestHit;

    if (!std::isfinite(origin.x) || !std::isfinite(origin.y) || !std::isfinite(origin.z) ||
        !std::isfinite(direction.x) || !std::isfinite(direction.y) || !std::isfinite(direction.z) ||
        !(maxDistance > 0.0f)) return bestHit;

    // Direction magnitude is not part of ray geometry. Widen before normalizing
    // so finite float extremes neither overflow nor collapse to a zero vector.
    const double dirLen = std::hypot(double(direction.x), double(direction.y), double(direction.z));
    if (dirLen == 0.0) return bestHit;

    const std::array<double, 3> dir{direction.x / dirLen, direction.y / dirLen, direction.z / dirLen};
    const auto reciprocal = [](double component) {
        return component == 0.0
            ? std::copysign(std::numeric_limits<double>::infinity(), component)
            : 1.0 / component;
    };
    const std::array<double, 3> invDir{reciprocal(dir[0]), reciprocal(dir[1]), reciprocal(dir[2])};
    double bestDistance = maxDistance;

    // Depth-first branch-and-bound stack traversal
    std::array<std::int32_t, 64> stack;
    int stackPtr = 0;
    stack[stackPtr++] = 0; // Root node index

    while (stackPtr > 0) {
        std::int32_t currIdx = stack[--stackPtr];
        const auto& node = nodes_[currIdx];

        if (!node.bounds.intersectsRaySlabs(origin, invDir, 0.0, bestDistance)) {
            continue;
        }

        if (node.isLeaf()) {
            const auto& s = snapshot_->samples[node.sampleIndex];
            SculptureVec3 center{s.x, s.y, s.z};
            const double radius = s.radius;

            const std::array<double, 3> m{
                double(origin.x) - center.x, double(origin.y) - center.y, double(origin.z) - center.z};
            const double b = m[0] * dir[0] + m[1] * dir[1] + m[2] * dir[2];
            const double c = (m[0] * m[0] + m[1] * m[1] + m[2] * m[2]) - radius * radius;

            if (c > 0.0 && b > 0.0) continue;
            const double discr = b * b - c;
            if (discr < 0.0) continue;

            double t = -b - std::sqrt(discr);
            if (t < 0.0) t = -b + std::sqrt(discr);

            // Keep pruning/comparison wide; rounding the public float distance
            // must not change which later candidate is actually nearer.
            if (t > 0.0 && t < bestDistance &&
                t >= std::numeric_limits<float>::denorm_min() &&
                t <= std::numeric_limits<float>::max()) {
                bestDistance = t;
                bestHit.hit = true;
                bestHit.t = static_cast<float>(t);
                bestHit.sampleIndex = node.sampleIndex;
                const double hitX = origin.x + t * dir[0];
                const double hitY = origin.y + t * dir[1];
                const double hitZ = origin.z + t * dir[2];
                bestHit.hitPoint = SculptureVec3{
                    static_cast<float>(hitX), static_cast<float>(hitY), static_cast<float>(hitZ)};
                if (radius > 1e-6f) {
                    bestHit.normal = SculptureVec3{
                        static_cast<float>((hitX - center.x) / radius),
                        static_cast<float>((hitY - center.y) / radius),
                        static_cast<float>((hitZ - center.z) / radius)
                    };
                } else {
                    bestHit.normal = SculptureVec3{0.0f, 1.0f, 0.0f};
                }
            }
        } else {
            if (node.leftChild >= 0) stack[stackPtr++] = node.leftChild;
            if (node.rightChild >= 0) stack[stackPtr++] = node.rightChild;
        }
    }

    return bestHit;
}

std::vector<std::uint32_t> SculptureSpatialLBVH::queryRadius(const SculptureVec3& center, float radius) const {
    std::vector<std::uint32_t> result;
    if (nodes_.empty() || !snapshot_) return result;

    const float radiusSq = radius * radius;
    std::array<std::int32_t, 64> stack;
    int stackPtr = 0;
    stack[stackPtr++] = 0;

    while (stackPtr > 0) {
        std::int32_t currIdx = stack[--stackPtr];
        const auto& node = nodes_[currIdx];

        if (!node.bounds.intersectsSphere(center, radius)) {
            continue;
        }

        if (node.isLeaf()) {
            const auto& s = snapshot_->samples[node.sampleIndex];
            float dx = s.x - center.x;
            float dy = s.y - center.y;
            float dz = s.z - center.z;
            if ((dx * dx + dy * dy + dz * dz) <= radiusSq) {
                result.push_back(node.sampleIndex);
            }
        } else {
            if (node.leftChild >= 0) stack[stackPtr++] = node.leftChild;
            if (node.rightChild >= 0) stack[stackPtr++] = node.rightChild;
        }
    }

    return result;
}

} // namespace epi_sculpt
