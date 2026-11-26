#include "RecursiveHopfTorusGpuAbi.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace epi_sculpt::recursive_hopf::gpu_abi {

namespace {

bool allFinite(std::span<const float> values) noexcept {
    for (float v : values) {
        if (!std::isfinite(v)) return false;
    }
    return true;
}

} // namespace

RecursiveHopfRecipeGpu packRecipe(const Recipe& recipe) noexcept {
    std::uint32_t flags = 0U;
    if (recipe.weaveAmplitude > 0.0) flags |= kFlagWovenMode;
    if (recipe.conformalScaleExponent > 0.0 ||
        recipe.placementMode == PlacementMode::ConformalMetric) {
        flags |= kFlagConformalScale;
    }
    if (recipe.placementMode == PlacementMode::StrictLocalClearance) {
        flags |= kFlagStrictClearance;
    }

    return RecursiveHopfRecipeGpu{
        .abiVersion = kVersion,
        .byteSize = kRecipeBytes,
        .fibersPerFamily = recipe.fibersPerFamily,
        .logicalDepth = recipe.logicalDepth,
        .plusInitialRadians = static_cast<float>(recipe.plus.initialRadians),
        .plusRadiansPerSecond = static_cast<float>(recipe.plus.radiansPerSecond),
        .minusInitialRadians = static_cast<float>(recipe.minus.initialRadians),
        .minusRadiansPerSecond = static_cast<float>(recipe.minus.radiansPerSecond),
        .childScale = static_cast<float>(recipe.childScale),
        .childTwistRadians = static_cast<float>(recipe.childTwistRadians),
        .childNormalOffset = static_cast<float>(recipe.childNormalOffset),
        .fiberRadius = static_cast<float>(recipe.fiberRadius),
        .conformalScaleExponent = static_cast<float>(recipe.conformalScaleExponent),
        .weaveAmplitude = static_cast<float>(recipe.weaveAmplitude),
        .flags = flags,
        .placementMode = static_cast<std::uint32_t>(recipe.placementMode),
    };
}

bool isValid(const RecursiveHopfRecipeGpu& recipe) noexcept {
    if (recipe.abiVersion != kVersion || recipe.byteSize != kRecipeBytes) return false;
    if (recipe.fibersPerFamily == 0U || recipe.fibersPerFamily > kMaxFibersPerFamily) return false;
    if (recipe.logicalDepth > kMaxLogicalDepth) return false;
    if (recipe.placementMode > 2U) return false;
    const float scalars[] = {
        recipe.plusInitialRadians,
        recipe.plusRadiansPerSecond,
        recipe.minusInitialRadians,
        recipe.minusRadiansPerSecond,
        recipe.childScale,
        recipe.childTwistRadians,
        recipe.childNormalOffset,
        recipe.fiberRadius,
        recipe.conformalScaleExponent,
        recipe.weaveAmplitude,
    };
    if (!allFinite(scalars)) return false;
    if (recipe.childScale <= 0.0f || recipe.childScale >= 1.0f) return false;
    if (recipe.fiberRadius <= 0.0f) return false;
    if (recipe.conformalScaleExponent < 0.0f || recipe.conformalScaleExponent > 4.0f) return false;
    if (recipe.weaveAmplitude < 0.0f || recipe.weaveAmplitude > 0.5f) return false;
    return true;
}

RecursiveHopfGrammarGpu packGrammar(
    const Grammar& grammar,
    std::uint32_t generation) noexcept {
    RecursiveHopfGrammarGpu out{};
    const auto& recipe = grammar.recipe();
    out.recipe = packRecipe(recipe);

    const std::uint32_t m = recipe.fibersPerFamily;
    const std::uint32_t rootCircles = 2U * m;
    const auto children = grammar.children();

    out.header.abiVersion = kVersion;
    out.header.byteSize = static_cast<std::uint32_t>(sizeof(RecursiveHopfGrammarGpu));
    out.header.generation = generation;
    out.header.rootCircleCount = rootCircles;
    out.header.childrenPerNode = static_cast<std::uint32_t>(children.size());
    out.header.logicalDepth = recipe.logicalDepth;
    out.header.flags = out.recipe.flags;
    out.header.reserved0 = 0U;
    out.header.domainTime = static_cast<float>(grammar.domainTime());
    out.header.plusPhase = static_cast<float>(grammar.phases().plus);
    out.header.minusPhase = static_cast<float>(grammar.phases().minus);
    out.header.subtreeRadius = static_cast<float>(grammar.subtreeRadius());
    out.header.logicalNodes = grammar.counts().nodes;
    out.header.logicalFibers = grammar.counts().fibers;

    constexpr double kTau = 2.0 * std::numbers::pi_v<double>;
    for (std::uint32_t f = 0U; f < m && f < kMaxFibersPerFamily; ++f) {
        const auto cPlus = grammar.circle(Chirality::Plus, f);
        const double phasePlus = grammar.phases().plus + kTau * f / m;
        out.rootCircles[f] = RecursiveHopfCircleGpu{
            .centerRadius = {
                static_cast<float>(cPlus.center[0]),
                static_cast<float>(cPlus.center[1]),
                static_cast<float>(cPlus.center[2]),
                static_cast<float>(cPlus.radius),
            },
            .axisX = {
                static_cast<float>(cPlus.axes.x[0]),
                static_cast<float>(cPlus.axes.x[1]),
                static_cast<float>(cPlus.axes.x[2]),
                1.0f,
            },
            .axisY = {
                static_cast<float>(cPlus.axes.y[0]),
                static_cast<float>(cPlus.axes.y[1]),
                static_cast<float>(cPlus.axes.y[2]),
                static_cast<float>(f),
            },
            .axisZ = {
                static_cast<float>(cPlus.axes.z[0]),
                static_cast<float>(cPlus.axes.z[1]),
                static_cast<float>(cPlus.axes.z[2]),
                static_cast<float>(phasePlus),
            },
        };

        const auto cMinus = grammar.circle(Chirality::Minus, f);
        const double phaseMinus = grammar.phases().minus + kTau * f / m;
        out.rootCircles[m + f] = RecursiveHopfCircleGpu{
            .centerRadius = {
                static_cast<float>(cMinus.center[0]),
                static_cast<float>(cMinus.center[1]),
                static_cast<float>(cMinus.center[2]),
                static_cast<float>(cMinus.radius),
            },
            .axisX = {
                static_cast<float>(cMinus.axes.x[0]),
                static_cast<float>(cMinus.axes.x[1]),
                static_cast<float>(cMinus.axes.x[2]),
                -1.0f,
            },
            .axisY = {
                static_cast<float>(cMinus.axes.y[0]),
                static_cast<float>(cMinus.axes.y[1]),
                static_cast<float>(cMinus.axes.y[2]),
                static_cast<float>(f),
            },
            .axisZ = {
                static_cast<float>(cMinus.axes.z[0]),
                static_cast<float>(cMinus.axes.z[1]),
                static_cast<float>(cMinus.axes.z[2]),
                static_cast<float>(phaseMinus),
            },
        };
    }

    const std::size_t childCount = std::min<std::size_t>(children.size(), kMaxChildrenPerNode);
    for (std::size_t i = 0U; i < childCount; ++i) {
        const auto& child = children[i];
        const auto cross = grammar.crossing(static_cast<std::uint32_t>(i));
        out.children[i] = RecursiveHopfChildMapGpu{
            .translationScale = {
                static_cast<float>(child.transform.translation[0]),
                static_cast<float>(child.transform.translation[1]),
                static_cast<float>(child.transform.translation[2]),
                static_cast<float>(child.transform.scale),
            },
            .axisX = {
                static_cast<float>(child.transform.axes.x[0]),
                static_cast<float>(child.transform.axes.x[1]),
                static_cast<float>(child.transform.axes.x[2]),
                static_cast<float>(child.site.u),
            },
            .axisY = {
                static_cast<float>(child.transform.axes.y[0]),
                static_cast<float>(child.transform.axes.y[1]),
                static_cast<float>(child.transform.axes.y[2]),
                static_cast<float>(child.site.v),
            },
            .axisZ = {
                static_cast<float>(child.transform.axes.z[0]),
                static_cast<float>(child.transform.axes.z[1]),
                static_cast<float>(child.transform.axes.z[2]),
                static_cast<float>(cross.u),
            },
            .crossingPosition = {
                static_cast<float>(cross.position[0]),
                static_cast<float>(cross.position[1]),
                static_cast<float>(cross.position[2]),
                static_cast<float>(cross.v),
            },
            .plusFiber = child.site.plusFiber,
            .minusFiber = child.site.minusFiber,
            .branch = child.site.branch,
            .childIndex = static_cast<std::uint32_t>(i),
        };
    }

    return out;
}

bool isValid(const RecursiveHopfGrammarGpu& grammar) noexcept {
    if (!isValid(grammar.recipe)) return false;
    const auto& h = grammar.header;
    if (h.abiVersion != kVersion) return false;
    if (h.byteSize != sizeof(RecursiveHopfGrammarGpu)) return false;
    if (h.rootCircleCount != 2U * grammar.recipe.fibersPerFamily) return false;
    if (h.rootCircleCount > kMaxRootCircles) return false;
    const std::uint32_t expectedChildren =
        grammar.recipe.logicalDepth == 0U
            ? 0U
            : 2U * grammar.recipe.fibersPerFamily * grammar.recipe.fibersPerFamily;
    if (h.childrenPerNode != expectedChildren || h.childrenPerNode > kMaxChildrenPerNode) return false;
    const float headerFloats[] = {h.domainTime, h.plusPhase, h.minusPhase, h.subtreeRadius};
    if (!allFinite(headerFloats) || h.domainTime < 0.0f || h.subtreeRadius <= 0.0f) return false;

    for (std::uint32_t i = 0U; i < h.rootCircleCount; ++i) {
        const auto& c = grammar.rootCircles[i];
        if (!allFinite(c.centerRadius) || !allFinite(c.axisX)
            || !allFinite(c.axisY) || !allFinite(c.axisZ)) {
            return false;
        }
        if (c.centerRadius[3] <= 0.0f) return false;
    }

    for (std::uint32_t i = 0U; i < h.childrenPerNode; ++i) {
        const auto& ch = grammar.children[i];
        if (!allFinite(ch.translationScale) || !allFinite(ch.axisX)
            || !allFinite(ch.axisY) || !allFinite(ch.axisZ)
            || !allFinite(ch.crossingPosition)) {
            return false;
        }
        if (ch.translationScale[3] <= 0.0f || ch.translationScale[3] >= 1.0f) return false;
    }
    return true;
}

std::size_t packExpandedInstances(
    const Grammar& grammar,
    const TraversalResult& traversal,
    std::span<RecursiveHopfInstanceGpu> outInstances) noexcept {
    const std::size_t count = std::min(traversal.fibers.size(), outInstances.size());
    const float baseTubeRadius = static_cast<float>(grammar.recipe().fiberRadius);
    for (std::size_t i = 0U; i < count; ++i) {
        const auto& f = traversal.fibers[i];
        const float nodeScale = static_cast<float>(f.nodeTransform.scale);
        outInstances[i] = RecursiveHopfInstanceGpu{
            .centerRadius = {
                static_cast<float>(f.worldCircle.center[0]),
                static_cast<float>(f.worldCircle.center[1]),
                static_cast<float>(f.worldCircle.center[2]),
                static_cast<float>(f.worldCircle.radius),
            },
            .axisX = {
                static_cast<float>(f.worldCircle.axes.x[0]),
                static_cast<float>(f.worldCircle.axes.x[1]),
                static_cast<float>(f.worldCircle.axes.x[2]),
                nodeScale,
            },
            .axisY = {
                static_cast<float>(f.worldCircle.axes.y[0]),
                static_cast<float>(f.worldCircle.axes.y[1]),
                static_cast<float>(f.worldCircle.axes.y[2]),
                f.family == Chirality::Plus ? 1.0f : -1.0f,
            },
            .axisZ = {
                static_cast<float>(f.worldCircle.axes.z[0]),
                static_cast<float>(f.worldCircle.axes.z[1]),
                static_cast<float>(f.worldCircle.axes.z[2]),
                baseTubeRadius * nodeScale,
            },
            .nodeIdLow = static_cast<std::uint32_t>(f.nodeId & 0xffffffffULL),
            .depth = f.depth,
            .chirality = f.family == Chirality::Plus ? 0U : 1U,
            .fiberIndex = f.fiber,
        };
    }
    return count;
}

} // namespace epi_sculpt::recursive_hopf::gpu_abi
