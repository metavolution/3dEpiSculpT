#include "RecursiveHopfTorusMath.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string>

namespace epi_sculpt::recursive_hopf {
namespace {

constexpr double kSqrtTwo = std::numbers::sqrt2_v<double>;
constexpr double kTau = 2.0 * std::numbers::pi_v<double>;
constexpr double kTorusBound = kSqrtTwo + 1.0;

Point add(const Point& a, const Point& b) noexcept {
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

Point scaled(const Point& p, double s) noexcept {
    return {p[0] * s, p[1] * s, p[2] * s};
}

Point cross(const Point& a, const Point& b) noexcept {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]};
}

Point apply(const Basis& axes, const Point& p) noexcept {
    return add(add(scaled(axes.x, p[0]), scaled(axes.y, p[1])), scaled(axes.z, p[2]));
}

Basis compose(const Basis& a, const Basis& b) noexcept {
    return {apply(a, b.x), apply(a, b.y), apply(a, b.z)};
}

Similarity compose(const Similarity& parent, const Similarity& child) noexcept {
    return {add(parent.translation, scaled(apply(parent.axes, child.translation), parent.scale)),
            compose(parent.axes, child.axes), parent.scale * child.scale};
}

Basis rotationZ(double angle) noexcept {
    const double c = std::cos(angle), s = std::sin(angle);
    return {{c, s, 0.0}, {-s, c, 0.0}, {0.0, 0.0, 1.0}};
}

double angle(double value) {
    if (!std::isfinite(value)) throw std::invalid_argument("Hopf angle must be finite");
    return std::remainder(value, kTau);
}

double phaseAt(const PhaseMotion& motion, double time) {
    const double advance = motion.radiansPerSecond * time;
    if (!std::isfinite(advance))
        throw std::invalid_argument("Hopf phase advance is not representable");
    // Individual circles repeat at 2*pi, but labeled crossing branches use
    // half angles. Reduce at 4*pi so their positions do not jump at a phase wrap.
    return std::remainder(std::remainder(motion.initialRadians, 2.0 * kTau)
                              + std::remainder(advance, 2.0 * kTau),
                          2.0 * kTau);
}

double subtreeBound(const Recipe& recipe) noexcept {
    const double own = kTorusBound + recipe.fiberRadius + recipe.weaveAmplitude;
    const double centers = kTorusBound + std::abs(recipe.childNormalOffset);
    const double maxConformal = recipe.conformalScaleExponent > 0.0
        ? std::pow(kSqrtTwo / (kSqrtTwo - 1.0), recipe.conformalScaleExponent)
        : 1.0;
    const double effectiveScale = std::min(0.95, recipe.childScale * maxConformal);
    double result = own;
    // R_d=max(own, centers+s*R_(d-1)). This covers every child subtree;
    // it is a conservative finite bound, not an interstitial collision proof.
    for (std::uint32_t depth = 0U; depth < recipe.logicalDepth; ++depth)
        result = std::max(own, centers + effectiveScale * result);
    // Leave modest roundoff headroom for finite-depth basis composition. This
    // is still a geometric bound, not a proof of separation between solids.
    return std::nextafter(result * (1.0 + 64.0 * std::numeric_limits<double>::epsilon()
                                                * (recipe.logicalDepth + 1U)),
                          std::numeric_limits<double>::infinity());
}

std::uint64_t checkedMultiply(std::uint64_t a, std::uint64_t b) {
    if (b != 0U && a > std::numeric_limits<std::uint64_t>::max() / b)
        throw std::overflow_error("Hopf hierarchy count overflow");
    return a * b;
}

std::uint64_t checkedAdd(std::uint64_t a, std::uint64_t b) {
    if (a > std::numeric_limits<std::uint64_t>::max() - b)
        throw std::overflow_error("Hopf hierarchy identity overflow");
    return a + b;
}

HierarchyCounts hierarchyCounts(const Recipe& recipe) {
    return computeHierarchyCounts(recipe.fibersPerFamily, recipe.logicalDepth);
}

} // namespace

HierarchyCounts computeHierarchyCounts(
    std::uint32_t fibersPerFamily,
    std::uint32_t logicalDepth) {
    if (fibersPerFamily == 0U || fibersPerFamily > kMaxFibersPerFamily) {
        throw std::invalid_argument("Hopf fibers per family must be in 1..8");
    }
    if (logicalDepth > kMaxLogicalDepth) {
        throw std::invalid_argument("Hopf logical depth must be in 0..8");
    }
    const std::uint32_t branching = 2U * fibersPerFamily * fibersPerFamily;
    std::uint64_t level = 1U, nodes = 1U;
    for (std::uint32_t depth = 0U; depth < logicalDepth; ++depth) {
        level = checkedMultiply(level, branching);
        nodes = checkedAdd(nodes, level);
    }
    return {branching, nodes, checkedMultiply(nodes, 2U * fibersPerFamily)};
}

RecipeIssue validateRecipe(const Recipe& recipe) noexcept {
    if (recipe.version != kRecipeVersion) return RecipeIssue::UnsupportedVersion;
    if (recipe.fibersPerFamily == 0U || recipe.fibersPerFamily > kMaxFibersPerFamily)
        return RecipeIssue::FiberCountOutOfRange;
    if (recipe.logicalDepth > kMaxLogicalDepth) return RecipeIssue::DepthOutOfRange;
    if (recipe.placementMode != PlacementMode::UniformSimilarity &&
        recipe.placementMode != PlacementMode::ConformalMetric &&
        recipe.placementMode != PlacementMode::StrictLocalClearance) {
        return RecipeIssue::InvalidPlacementMode;
    }
    for (double value : {recipe.plus.initialRadians, recipe.plus.radiansPerSecond,
                         recipe.minus.initialRadians, recipe.minus.radiansPerSecond,
                         recipe.childScale, recipe.childTwistRadians,
                         recipe.childNormalOffset, recipe.fiberRadius,
                         recipe.conformalScaleExponent, recipe.weaveAmplitude})
        if (!std::isfinite(value)) return RecipeIssue::NonFiniteParameter;
    if (recipe.childScale <= 0.0 || recipe.childScale >= 1.0)
        return RecipeIssue::InvalidContraction;
    if (recipe.fiberRadius <= 0.0) return RecipeIssue::InvalidFiberRadius;
    if (recipe.conformalScaleExponent < 0.0 || recipe.conformalScaleExponent > 4.0)
        return RecipeIssue::InvalidConformalExponent;
    if (recipe.weaveAmplitude < 0.0 || recipe.weaveAmplitude > 0.5)
        return RecipeIssue::InvalidWeaveAmplitude;
    if (!std::isfinite(subtreeBound(recipe))) return RecipeIssue::UnrepresentableBounds;
    double deepestScale = 1.0;
    for (std::uint32_t depth = 0U; depth < recipe.logicalDepth; ++depth)
        deepestScale *= recipe.childScale;
    if (deepestScale == 0.0 || recipe.fiberRadius * deepestScale == 0.0)
        return RecipeIssue::UnrepresentableBounds;
    return RecipeIssue::None;
}

std::string_view describe(RecipeIssue issue) noexcept {
    switch (issue) {
    case RecipeIssue::None: return "valid";
    case RecipeIssue::UnsupportedVersion: return "unsupported Hopf recipe version";
    case RecipeIssue::FiberCountOutOfRange: return "fibers per family must be in 1..8";
    case RecipeIssue::DepthOutOfRange: return "logical depth must be in 0..8";
    case RecipeIssue::NonFiniteParameter: return "Hopf recipe parameters must be finite";
    case RecipeIssue::InvalidContraction: return "child scale must be strictly between zero and one";
    case RecipeIssue::InvalidFiberRadius: return "fiber radius must be positive";
    case RecipeIssue::UnrepresentableBounds: return "Hopf subtree bounds or descendant scale are not representable";
    case RecipeIssue::InvalidConformalExponent: return "conformal scale exponent must be in 0..4";
    case RecipeIssue::InvalidWeaveAmplitude: return "weave amplitude must be in 0..0.5";
    case RecipeIssue::InvalidPlacementMode: return "placement mode must be uniform, conformal, or clearance";
    }
    return "unknown Hopf recipe issue";
}

std::string_view describePlacementMode(PlacementMode mode) noexcept {
    switch (mode) {
    case PlacementMode::UniformSimilarity: return "uniform-similarity";
    case PlacementMode::ConformalMetric: return "conformal-metric";
    case PlacementMode::StrictLocalClearance: return "strict-local-clearance";
    }
    return "unknown";
}

Point torusPoint(double u, double v) {
    u = angle(u); v = angle(v);
    const double d = kSqrtTwo - std::sin(v);
    return {std::cos(u) / d, std::sin(u) / d, std::cos(v) / d};
}

Basis torusFrame(double u, double v) {
    u = angle(u); v = angle(v);
    const double cu = std::cos(u), su = std::sin(u);
    const double cv = std::cos(v), sv = std::sin(v), d = kSqrtTwo - sv;
    const Point eU{-su, cu, 0.0};
    const Point eV{cu * cv / d, su * cv / d, (1.0 - kSqrtTwo * sv) / d};
    return {eU, scaled(eV, -1.0), scaled(cross(eU, eV), -1.0)};
}

double torusImplicitResidual(const Point& p) {
    for (double coord : p) {
        if (!std::isfinite(coord))
            throw std::invalid_argument("Torus point coordinates must be finite");
    }
    const double rho = std::hypot(p[0], p[1]);
    const double dr = rho - kSqrtTwo;
    return dr * dr + p[2] * p[2] - 1.0;
}

double torusConformalFactor(double v) {
    v = angle(v);
    return kSqrtTwo / (kSqrtTwo - std::sin(v));
}

double pointToCircleDistance(const Point& p, const Circle& c) {
    for (double coord : p) {
        if (!std::isfinite(coord))
            throw std::invalid_argument("Point coordinates must be finite");
    }
    const double dx = p[0] - c.center[0];
    const double dy = p[1] - c.center[1];
    const double dz = p[2] - c.center[2];
    const double dPerp = dx * c.axes.z[0] + dy * c.axes.z[1] + dz * c.axes.z[2];
    const double inPlaneX = dx - dPerp * c.axes.z[0];
    const double inPlaneY = dy - dPerp * c.axes.z[1];
    const double inPlaneZ = dz - dPerp * c.axes.z[2];
    const double rho = std::sqrt(inPlaneX * inPlaneX + inPlaneY * inPlaneY + inPlaneZ * inPlaneZ);
    const double dRad = rho - c.radius;
    return std::hypot(dPerp, dRad);
}

Grammar Grammar::prepare(const Recipe& recipe, double domainTime) {
    const auto issue = validateRecipe(recipe);
    if (issue != RecipeIssue::None) throw std::invalid_argument(std::string(describe(issue)));
    if (!std::isfinite(domainTime) || domainTime < 0.0)
        throw std::invalid_argument("Hopf domain time must be finite and nonnegative");

    Grammar result;
    result.recipe_ = recipe;
    result.domainTime_ = domainTime;
    result.phases_ = {phaseAt(recipe.plus, domainTime), phaseAt(recipe.minus, domainTime)};
    result.counts_ = hierarchyCounts(recipe);
    result.subtreeRadius_ = subtreeBound(recipe);
    // Depth zero has no children to describe or allocate.
    if (recipe.logicalDepth != 0U) {
        result.children_.reserve(result.counts_.childrenPerNode);
        const auto twist = rotationZ(angle(recipe.childTwistRadians));
        for (std::uint32_t i = 0U; i < result.counts_.childrenPerNode; ++i) {
            const auto site = result.site(i, true);
            double siteScale = recipe.childScale;
            if (recipe.placementMode == PlacementMode::StrictLocalClearance) {
                const double clearScale = result.siteClearanceScale(i);
                siteScale = std::clamp(recipe.childScale * clearScale, 1.0e-5, 0.95);
            } else if (recipe.placementMode == PlacementMode::ConformalMetric ||
                       recipe.conformalScaleExponent > 0.0) {
                const double expVal = (recipe.placementMode == PlacementMode::ConformalMetric &&
                                       recipe.conformalScaleExponent == 0.0)
                    ? 1.0
                    : recipe.conformalScaleExponent;
                const double factor = std::pow(torusConformalFactor(site.v), expVal);
                siteScale = std::min(0.95, recipe.childScale * factor);
            }
            result.children_.push_back({site,
                {add(site.position, scaled(site.axes.z, recipe.childNormalOffset)),
                 compose(site.axes, twist), siteScale}});
        }
    }
    return result;
}

Site Grammar::site(std::uint32_t index, bool interstitial) const {
    if (index >= counts_.childrenPerNode) throw std::out_of_range("Hopf site index out of range");
    const std::uint32_t pair = index / 2U;
    const std::uint32_t k = pair / recipe_.fibersPerFamily;
    const std::uint32_t l = pair % recipe_.fibersPerFamily;
    const std::uint32_t branch = index % 2U;
    const double shift = interstitial ? std::numbers::pi_v<double> / recipe_.fibersPerFamily : 0.0;
    const double a = fiberPhase(Chirality::Plus, k) + shift;
    const double b = fiberPhase(Chirality::Minus, l) + shift;
    const double u = 0.5 * (a + b) + branch * std::numbers::pi_v<double>;
    const double v = 0.5 * (b - a) + branch * std::numbers::pi_v<double>;
    return {k, l, branch, u, v, torusPoint(u, v), torusFrame(u, v)};
}

Site Grammar::crossing(std::uint32_t index) const { return site(index, false); }
Site Grammar::interstitial(std::uint32_t index) const { return site(index, true); }

double Grammar::siteClearanceRadius(std::uint32_t index) const {
    const Site s = site(index, true);
    const std::uint32_t m = recipe_.fibersPerFamily;
    const std::uint32_t kNext = (s.plusFiber + 1U) % m;
    const std::uint32_t lNext = (s.minusFiber + 1U) % m;
    const Point center = add(s.position, scaled(s.axes.z, recipe_.childNormalOffset));

    const double d0 = pointToCircleDistance(center, circle(Chirality::Plus, s.plusFiber));
    const double d1 = pointToCircleDistance(center, circle(Chirality::Plus, kNext));
    const double d2 = pointToCircleDistance(center, circle(Chirality::Minus, s.minusFiber));
    const double d3 = pointToCircleDistance(center, circle(Chirality::Minus, lNext));
    const double minDist = std::min({d0, d1, d2, d3});
    return std::max(0.0, minDist - (recipe_.fiberRadius + recipe_.weaveAmplitude));
}

double Grammar::siteClearanceScale(std::uint32_t index) const {
    const double clearRadius = siteClearanceRadius(index);
    const double childOuterExtent = kTorusBound + recipe_.fiberRadius + recipe_.weaveAmplitude;
    return std::clamp(clearRadius / childOuterExtent, 1.0e-5, 0.95);
}

double Grammar::fiberPhase(Chirality family, std::uint32_t fiber) const {
    if (fiber >= recipe_.fibersPerFamily) throw std::out_of_range("Hopf fiber index out of range");
    double initial;
    switch (family) {
    case Chirality::Plus: initial = phases_.plus; break;
    case Chirality::Minus: initial = phases_.minus; break;
    default: throw std::invalid_argument("invalid Hopf chirality");
    }
    return initial + kTau * fiber / recipe_.fibersPerFamily;
}

Circle Grammar::circle(Chirality family, std::uint32_t fiber) const {
    const auto rotation = rotationZ(angle(fiberPhase(family, fiber)));
    const double invSqrtTwo = 1.0 / kSqrtTwo;
    const Basis plane{{invSqrtTwo, 0.0, invSqrtTwo}, {0.0, 1.0, 0.0},
                      {-invSqrtTwo, 0.0, invSqrtTwo}};
    const double centerY = family == Chirality::Plus ? 1.0 : -1.0;
    return {apply(rotation, {0.0, centerY, 0.0}), compose(rotation, plane), kSqrtTwo};
}

Point Grammar::curvePoint(Chirality family, std::uint32_t fiber, double t) const {
    const double phase = fiberPhase(family, fiber);
    t = angle(t);
    return torusPoint((family == Chirality::Plus ? t : -t) + phase, t);
}

Point Grammar::wovenCurvePoint(Chirality family, std::uint32_t fiber, double t) const {
    const double phase = fiberPhase(family, fiber);
    t = angle(t);
    const double u = (family == Chirality::Plus ? t : -t) + phase;
    const Point base = torusPoint(u, t);
    if (recipe_.weaveAmplitude <= 0.0) return base;
    const Basis frame = torusFrame(u, t);
    const double halfPhaseDiff = 0.5 * (phases_.minus - phases_.plus);
    const double sign = (family == Chirality::Plus) ? 1.0 : -1.0;
    const double disp = sign * recipe_.weaveAmplitude
        * std::cos(static_cast<double>(recipe_.fibersPerFamily) * (t - halfPhaseDiff));
    return add(base, scaled(frame.z, disp));
}

Node Grammar::node(std::span<const std::uint32_t> path) const {
    if (path.size() > recipe_.logicalDepth) throw std::out_of_range("Hopf path exceeds logical depth");
    Node result;
    for (const auto index : path) {
        if (index >= children_.size()) throw std::out_of_range("Hopf child index out of range");
        result.id = checkedAdd(checkedMultiply(result.id, counts_.childrenPerNode), 1U + index);
        result.transform = compose(result.transform, children_[index].transform);
        ++result.depth;
    }
    return result;
}

Circle Grammar::circleAt(std::span<const std::uint32_t> path,
                         Chirality family, std::uint32_t fiber) const {
    const auto transform = node(path).transform;
    const auto local = circle(family, fiber);
    return {add(transform.translation, scaled(apply(transform.axes, local.center), transform.scale)),
            compose(transform.axes, local.axes), transform.scale * local.radius};
}

TraversalResult Grammar::traverse(const TraversalBudget& budget) const {
    TraversalResult out{};
    out.effectiveMaxDepth = std::min(recipe_.logicalDepth, budget.maxDepth);
    if (budget.maxNodes == 0U || budget.maxFibers == 0U) {
        out.truncatedByBudget = true;
        return out;
    }

    const std::uint32_t m = recipe_.fibersPerFamily;
    const std::uint32_t fibersPerNode = 2U * m;
    if (budget.maxFibers < fibersPerNode) {
        out.truncatedByBudget = true;
        return out;
    }

    // Precompute root circles once; every descendant circle is just the node's
    // similarity transform applied to the root circle descriptor.
    std::vector<Circle> rootPlus(m);
    std::vector<Circle> rootMinus(m);
    for (std::uint32_t f = 0U; f < m; ++f) {
        rootPlus[f] = circle(Chirality::Plus, f);
        rootMinus[f] = circle(Chirality::Minus, f);
    }

    std::vector<Site> rootCrossings;
    if (budget.includeCrossings) {
        rootCrossings.reserve(counts_.childrenPerNode);
        for (std::uint32_t i = 0U; i < counts_.childrenPerNode; ++i) {
            rootCrossings.push_back(crossing(i));
        }
    }

    out.nodes.reserve(std::min<std::size_t>(budget.maxNodes, 1024U));
    out.fibers.reserve(std::min<std::size_t>(budget.maxFibers, 8192U));
    out.nodes.push_back(Node{0U, 0U, Similarity{}});

    std::size_t queueCursor = 0U;
    while (queueCursor < out.nodes.size()) {
        const Node current = out.nodes[queueCursor++];
        if (out.fibers.size() + fibersPerNode > budget.maxFibers) {
            out.truncatedByBudget = true;
            break;
        }

        // Emit Plus family fibers for this node
        for (std::uint32_t f = 0U; f < m; ++f) {
            const Circle& local = rootPlus[f];
            Circle world{
                add(current.transform.translation,
                    scaled(apply(current.transform.axes, local.center), current.transform.scale)),
                compose(current.transform.axes, local.axes),
                current.transform.scale * local.radius
            };
            out.fibers.push_back(ExpandedFiber{
                current.id, current.depth, Chirality::Plus, f, world, current.transform
            });
        }
        // Emit Minus family fibers for this node
        for (std::uint32_t f = 0U; f < m; ++f) {
            const Circle& local = rootMinus[f];
            Circle world{
                add(current.transform.translation,
                    scaled(apply(current.transform.axes, local.center), current.transform.scale)),
                compose(current.transform.axes, local.axes),
                current.transform.scale * local.radius
            };
            out.fibers.push_back(ExpandedFiber{
                current.id, current.depth, Chirality::Minus, f, world, current.transform
            });
        }

        if (budget.includeCrossings) {
            for (std::uint32_t i = 0U; i < counts_.childrenPerNode; ++i) {
                const Site& s = rootCrossings[i];
                out.crossings.push_back(ExpandedSite{
                    current.id,
                    current.depth,
                    i,
                    false,
                    add(current.transform.translation,
                        scaled(apply(current.transform.axes, s.position), current.transform.scale)),
                    compose(current.transform.axes, s.axes),
                    current.transform.scale
                });
            }
        }

        if (budget.includeInterstitials && !children_.empty()) {
            for (std::uint32_t i = 0U; i < children_.size(); ++i) {
                const Site& s = children_[i].site;
                out.interstitials.push_back(ExpandedSite{
                    current.id,
                    current.depth,
                    i,
                    true,
                    add(current.transform.translation,
                        scaled(apply(current.transform.axes, s.position), current.transform.scale)),
                    compose(current.transform.axes, s.axes),
                    current.transform.scale * children_[i].transform.scale
                });
            }
        }

        ++out.visitedNodes;
        out.deepestObservedDepth = std::max(out.deepestObservedDepth, current.depth);
        out.emittedFibers = static_cast<std::uint32_t>(out.fibers.size());

        // Expand children if within depth and budget
        if (current.depth < out.effectiveMaxDepth && !children_.empty()) {
            const std::uint32_t activeEyes = budget.enableViewCulling
                ? std::min<std::uint32_t>(budget.eyeCount, 2U)
                : 0U;
            for (std::uint32_t c = 0U; c < children_.size(); ++c) {
                const Similarity childWorld = compose(current.transform, children_[c].transform);
                if (childWorld.scale < budget.minWorldScale) {
                    out.truncatedByBudget = true;
                    ++out.lodCulledNodes;
                    continue;
                }

                // Coherent view-cone + projected screen-radius LOD culling across 1 (mono) or 2 (SBS stereo) eyes
                if (activeEyes > 0U) {
                    const double boundRadius = childWorld.scale * subtreeRadius_;
                    bool visibleInAnyEye = false;
                    double maxProjectedPixels = 0.0;
                    for (std::uint32_t e = 0U; e < activeEyes; ++e) {
                        const ObservationEye& eye = budget.eyes[e];
                        const double dx = childWorld.translation[0] - eye.position[0];
                        const double dy = childWorld.translation[1] - eye.position[1];
                        const double dz = childWorld.translation[2] - eye.position[2];
                        const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

                        // Close-camera / inside-sphere safety: unconditionally visible
                        if (dist <= boundRadius * 1.05 || dist <= 1.0e-6) {
                            visibleInAnyEye = true;
                            maxProjectedPixels = std::numeric_limits<double>::infinity();
                            break;
                        }

                        const double fLen = std::sqrt(
                            eye.forward[0] * eye.forward[0] +
                            eye.forward[1] * eye.forward[1] +
                            eye.forward[2] * eye.forward[2]);
                        const double cosView = (fLen > 1.0e-6)
                            ? (dx * eye.forward[0] + dy * eye.forward[1] + dz * eye.forward[2]) / (dist * fLen)
                            : 1.0;
                        const double sinSphere = std::clamp(boundRadius / dist, 0.0, 1.0);
                        const double cosSphere = std::sqrt(std::max(0.0, 1.0 - sinSphere * sinSphere));
                        const double cosFov = std::clamp(eye.cosHalfFov, -1.0, 1.0);
                        const double sinFov = std::sqrt(std::max(0.0, 1.0 - cosFov * cosFov));
                        // Sphere intersects cone iff angle(v, forward) <= fovHalf + asin(R/d)
                        // Cosine threshold for sum of angles: cos(fovHalf + sphereAngle) = cosFov*cosSphere - sinFov*sinSphere
                        const double cosLimit = (std::acos(cosFov) + std::asin(sinSphere) >= std::numbers::pi_v<double>)
                            ? -1.0
                            : (cosFov * cosSphere - sinFov * sinSphere);

                        if (cosView >= cosLimit) {
                            visibleInAnyEye = true;
                            const double projPixels = (boundRadius / dist) * std::max(1.0, eye.focalLengthPixels);
                            maxProjectedPixels = std::max(maxProjectedPixels, projPixels);
                        }
                    }

                    if (!visibleInAnyEye) {
                        ++out.frustumCulledNodes;
                        continue;
                    }
                    if (maxProjectedPixels < budget.minProjectedRadiusPixels) {
                        ++out.lodCulledNodes;
                        continue;
                    }
                }

                if (out.nodes.size() >= budget.maxNodes ||
                    (out.nodes.size() + 1U) * fibersPerNode > budget.maxFibers) {
                    out.truncatedByBudget = true;
                    out.budgetOmittedNodes += static_cast<std::uint32_t>(children_.size() - c);
                    break;
                }
                const std::uint64_t childId =
                    checkedAdd(checkedMultiply(current.id, counts_.childrenPerNode), 1U + c);
                out.nodes.push_back(Node{childId, current.depth + 1U, childWorld});
            }
        }
    }

    if (queueCursor < out.nodes.size()) {
        out.budgetOmittedNodes += static_cast<std::uint32_t>(out.nodes.size() - queueCursor);
        out.nodes.resize(queueCursor);
    }
    return out;
}

} // namespace epi_sculpt::recursive_hopf

