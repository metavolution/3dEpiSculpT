#pragma once

#include "RecursiveHopfTorusRecipe.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace epi_sculpt::recursive_hopf {

// Plain coordinates keep the reference independent of engine/SDK vector types.
using Point = std::array<double, 3>;

struct Basis {
    Point x{1.0, 0.0, 0.0};
    Point y{0.0, 1.0, 0.0};
    Point z{0.0, 0.0, 1.0};
};

struct Similarity {
    Point translation{};
    Basis axes{}; // Right-handed orthonormal columns.
    double scale = 1.0;
};

enum class Chirality : std::uint8_t { Plus, Minus };

struct Phases {
    double plus = 0.0;
    double minus = 0.0;
};

struct Site {
    std::uint32_t plusFiber = 0U;
    std::uint32_t minusFiber = 0U;
    std::uint32_t branch = 0U;
    double u = 0.0;
    double v = 0.0;
    Point position{};
    Basis axes{}; // x=e_u, y=-e_v, z=outward normal.
};

struct Circle {
    Point center{};
    Basis axes{}; // Circle lies in x/y plane; z is its plane normal.
    double radius = 0.0;
};

struct HierarchyCounts {
    std::uint32_t childrenPerNode = 0U;
    std::uint64_t nodes = 0U;
    std::uint64_t fibers = 0U;
};

struct ChildMap {
    Site site{};
    Similarity transform{};
};

struct Node {
    std::uint64_t id = 0U; // Breadth-first identity; root=0.
    std::uint32_t depth = 0U;
    Similarity transform{};
};

struct ObservationEye {
    Point position{0.0, -6.2, 2.1};
    Point forward{0.0, 1.0, 0.0}; // Unit forward vector in mathematical Z-up coordinates.
    double cosHalfFov = 0.5;      // Cosine of outer view-cone half-angle (e.g. cos(60 deg) = 0.5).
    double focalLengthPixels = 1200.0; // 0.5 * viewportHeight / tan(fovY / 2).
};

struct TraversalBudget {
    std::uint32_t maxDepth = kMaxLogicalDepth;
    std::uint32_t maxNodes = 4096U;
    std::uint32_t maxFibers = 32768U;
    double minWorldScale = 1.0e-4;
    bool includeCrossings = false;
    bool includeInterstitials = false;
    // Optional view-cone and projected pixel-radius LOD culling.
    // For stereoscopic SBS, pass eyeCount=2 for coherent union-of-views selection.
    bool enableViewCulling = false;
    std::uint32_t eyeCount = 0U;
    std::array<ObservationEye, 2> eyes{};
    double minProjectedRadiusPixels = 0.75;
};

struct ExpandedFiber {
    std::uint64_t nodeId = 0U;
    std::uint32_t depth = 0U;
    Chirality family = Chirality::Plus;
    std::uint32_t fiber = 0U;
    Circle worldCircle{};
    Similarity nodeTransform{};
};

struct ExpandedSite {
    std::uint64_t nodeId = 0U;
    std::uint32_t depth = 0U;
    std::uint32_t index = 0U;
    bool interstitial = false;
    Point worldPosition{};
    Basis worldAxes{};
    double worldScale = 1.0;
};

struct TraversalResult {
    std::uint32_t effectiveMaxDepth = 0U;
    std::uint32_t deepestObservedDepth = 0U;
    std::uint32_t visitedNodes = 0U;
    std::uint32_t emittedFibers = 0U;
    std::uint32_t frustumCulledNodes = 0U;
    std::uint32_t lodCulledNodes = 0U;
    std::uint32_t budgetOmittedNodes = 0U;
    bool truncatedByBudget = false;
    std::vector<Node> nodes{};
    std::vector<ExpandedFiber> fibers{};
    std::vector<ExpandedSite> crossings{};
    std::vector<ExpandedSite> interstitials{};
};

// Public reference functions reject non-finite coordinates. Torus derivatives
// and child frames are analytic; geometry never depends on numerical crossings.
[[nodiscard]] Point torusPoint(double u, double v);
[[nodiscard]] Basis torusFrame(double u, double v);
// Implicit Clifford torus equation residual: (sqrt(x^2 + y^2) - sqrt(2))^2 + z^2 - 1.
[[nodiscard]] double torusImplicitResidual(const Point& p);
// Normalized stereographic metric factor: sqrt(2) / (sqrt(2) - sin(v)).
[[nodiscard]] double torusConformalFactor(double v);
// Exact 3D Euclidean distance from point p to a circle descriptor.
[[nodiscard]] double pointToCircleDistance(const Point& p, const Circle& c);
// Overflow-checked B-ary hierarchy counts for m fibers per family and depth D.
[[nodiscard]] HierarchyCounts computeHierarchyCounts(
    std::uint32_t fibersPerFamily,
    std::uint32_t logicalDepth);

// A finite immutable domain description: only B=2*m*m maps are stored, even
// when the logical tree has trillions of descendants. No clocks, IO or GPU owners.
class Grammar final {
public:
    [[nodiscard]] static Grammar prepare(const Recipe& recipe, double domainTime);

    [[nodiscard]] const Recipe& recipe() const noexcept { return recipe_; }
    [[nodiscard]] const Phases& phases() const noexcept { return phases_; }
    [[nodiscard]] double domainTime() const noexcept { return domainTime_; }
    [[nodiscard]] const HierarchyCounts& counts() const noexcept { return counts_; }
    [[nodiscard]] double subtreeRadius() const noexcept { return subtreeRadius_; }
    [[nodiscard]] std::span<const ChildMap> children() const noexcept { return children_; }

    [[nodiscard]] Site crossing(std::uint32_t index) const;
    [[nodiscard]] Site interstitial(std::uint32_t index) const;
    // Exact 3D Euclidean distance from interstitial site (index) to the nearest
    // of the 4 bounding Villarceau fiber tubes (k, k+1) and (l, l+1).
    [[nodiscard]] double siteClearanceRadius(std::uint32_t index) const;
    // Maximum non-overlapping uniform scale for a child torus at interstitial site (index).
    [[nodiscard]] double siteClearanceScale(std::uint32_t index) const;

    [[nodiscard]] Circle circle(Chirality family, std::uint32_t fiber) const;
    [[nodiscard]] Point curvePoint(Chirality family, std::uint32_t fiber, double t) const;
    [[nodiscard]] Point wovenCurvePoint(Chirality family, std::uint32_t fiber, double t) const;
    [[nodiscard]] Node node(std::span<const std::uint32_t> path) const;
    [[nodiscard]] Circle circleAt(std::span<const std::uint32_t> path,
                                  Chirality family, std::uint32_t fiber) const;
    [[nodiscard]] TraversalResult traverse(const TraversalBudget& budget = {}) const;

private:
    Grammar() = default;
    [[nodiscard]] Site site(std::uint32_t index, bool interstitial) const;
    [[nodiscard]] double fiberPhase(Chirality family, std::uint32_t fiber) const;

    Recipe recipe_{};
    Phases phases_{};
    double domainTime_ = 0.0;
    HierarchyCounts counts_{};
    double subtreeRadius_ = 0.0;
    std::vector<ChildMap> children_;
};

} // namespace epi_sculpt::recursive_hopf

