#pragma once

#include "SculptureModulationInspection.h"
#include "SculptureModulationRouting.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace epi_sculpt {

[[nodiscard]] constexpr float normalizeModulationZero(float value) noexcept {
    return value == 0.0f ? 0.0f : value;
}

// Compare compiled modulation instructions by mathematical/runtime effect,
// ignoring transient smoothing state (currentSmoothedValue/initialized) and
// signed zero representation differences.
[[nodiscard]] constexpr bool sameCompiledModulationRoute(
    const CompiledModulationRoute& a, const CompiledModulationRoute& b) noexcept {
    if (a.sourceId == ModulationSourceId::None || a.targetId == ModulationTargetId::None
        || b.sourceId == ModulationSourceId::None || b.targetId == ModulationTargetId::None) {
        return false;
    }
    return a.sourceId == b.sourceId
        && a.targetId == b.targetId
        && a.transferCurve == b.transferCurve
        && normalizeModulationZero(a.scale) == normalizeModulationZero(b.scale)
        && normalizeModulationZero(a.bias) == normalizeModulationZero(b.bias)
        && normalizeModulationZero(a.slewTimeSeconds) == normalizeModulationZero(b.slewTimeSeconds)
        && a.enabled == b.enabled;
}

// Format a compiled modulation route into a canonical string that round-trips
// through SculptureModulationEngine::inspectRouteResult to an equivalent route.
[[nodiscard]] inline std::optional<std::string> formatCanonicalModulationRoute(
    const CompiledModulationRoute& route) {
    const auto sourceName = modulation_inspection::name(route.sourceId);
    const auto targetName = modulation_inspection::name(route.targetId);
    const auto curveName = modulation_inspection::name(route.transferCurve);
    if (sourceName == "unknown" || targetName == "unknown" || curveName == "unknown"
        || !std::isfinite(route.scale) || !std::isfinite(route.bias)
        || !std::isfinite(route.slewTimeSeconds) || route.slewTimeSeconds < 0.0f) {
        return std::nullopt;
    }
    const float scale = normalizeModulationZero(route.scale);
    const float bias = normalizeModulationZero(route.bias);
    const float slew = normalizeModulationZero(route.slewTimeSeconds);

    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<float>::max_digits10)
        << sourceName << " -> " << curveName << " * " << scale;
    if (bias < 0.0f) {
        out << " - " << normalizeModulationZero(-bias);
    } else {
        out << " + " << bias;
    }
    if (slew > 0.0f) {
        out << " / slew:" << slew;
    }
    out << " -> " << targetName;
    return out.str();
}

struct ModulationTargetFanInEntry {
    ModulationTargetId targetId = ModulationTargetId::None;
    std::size_t routeCount = 0;
    float minContributionSum = 0.0f;
    float maxContributionSum = 0.0f;

    bool operator==(const ModulationTargetFanInEntry&) const noexcept = default;
};

struct ModulationTargetFanInSummary {
    std::vector<ModulationTargetFanInEntry> targets;
    std::size_t superposedTargetCount = 0;
    bool finite = true;

    bool operator==(const ModulationTargetFanInSummary&) const noexcept = default;
};

// -----------------------------------------------------------------------------
// Educational Annotation: Additive Target Fan-In & Superposition Analysis
// -----------------------------------------------------------------------------
// In SculptureModulationEngine::applyTarget, multiple active routes targeting
// the same ModulationTargetId sum their contributions before parameter domain
// clamping. Evaluating each route over modulation_inspection::kStandardBipolarProbes
// yields the worst-case independent additive excursion [minSum, maxSum].
// -----------------------------------------------------------------------------
[[nodiscard]] inline ModulationTargetFanInSummary inspectModulationTargetFanIn(
    std::span<const CompiledModulationRoute> routes) {
    constexpr std::size_t kTargetCount = static_cast<std::size_t>(ModulationTargetId::None);

    struct Accumulator {
        std::size_t count = 0;
        float minSum = 0.0f;
        float maxSum = 0.0f;
    };
    std::array<Accumulator, kTargetCount> accumulators{};
    ModulationTargetFanInSummary summary;

    for (const auto& route : routes) {
        if (!route.enabled) continue;
        const auto targetIndex = static_cast<std::size_t>(route.targetId);
        if (targetIndex >= kTargetCount) {
            summary.finite = false;
            continue;
        }
        const auto probes = modulation_inspection::evaluateRouteProbes(route);
        if (!probes.ready()) summary.finite = false;
        auto& acc = accumulators[targetIndex];
        ++acc.count;
        acc.minSum = normalizeModulationZero(acc.minSum + probes.minContribution);
        acc.maxSum = normalizeModulationZero(acc.maxSum + probes.maxContribution);
        if (!std::isfinite(acc.minSum) || !std::isfinite(acc.maxSum)) {
            summary.finite = false;
        }
    }

    for (std::size_t index = 0; index < kTargetCount; ++index) {
        const auto& acc = accumulators[index];
        if (acc.count == 0U) continue;
        if (acc.count >= 2U) ++summary.superposedTargetCount;
        summary.targets.push_back({
            .targetId = static_cast<ModulationTargetId>(index),
            .routeCount = acc.count,
            .minContributionSum = normalizeModulationZero(acc.minSum),
            .maxContributionSum = normalizeModulationZero(acc.maxSum),
        });
    }
    return summary;
}

enum class ModulationRouteCueRelation : std::uint8_t {
    Shared,
    Modified,
    FromOnly,
    ToOnly,
    Rejected
};

[[nodiscard]] constexpr std::string_view modulationRouteCueRelationName(
    ModulationRouteCueRelation relation) noexcept {
    switch (relation) {
    case ModulationRouteCueRelation::Shared: return "shared";
    case ModulationRouteCueRelation::Modified: return "modified";
    case ModulationRouteCueRelation::FromOnly: return "from-only";
    case ModulationRouteCueRelation::ToOnly: return "to-only";
    case ModulationRouteCueRelation::Rejected: return "rejected";
    }
    return "unknown";
}

struct ModulationRouteCueComparison {
    ModulationRouteCueRelation relation = ModulationRouteCueRelation::Rejected;
    std::optional<std::size_t> fromIndex;
    std::optional<std::size_t> toIndex;
    ModulationSourceId sourceId = ModulationSourceId::None;
    ModulationTargetId targetId = ModulationTargetId::None;
    std::string fromText;
    std::string toText;
    ModulationRouteError fromError = ModulationRouteError::None;
    ModulationRouteError toError = ModulationRouteError::None;

    bool operator==(const ModulationRouteCueComparison&) const noexcept = default;
};

struct ModulationRouteMapComparison {
    std::string_view fromOrigin = "authored";
    std::string_view toOrigin = "authored";
    std::size_t fromCount = 0;
    std::size_t toCount = 0;
    bool fromAccepted = true;
    bool toAccepted = true;
    bool authoredTextSame = false;
    bool resolvedTextSame = false;
    bool semanticSame = false;
    std::size_t sharedCount = 0;
    std::size_t modifiedCount = 0;
    std::size_t fromOnlyCount = 0;
    std::size_t toOnlyCount = 0;
    std::size_t rejectedCount = 0;
    std::vector<ModulationRouteCueComparison> cues;
    ModulationTargetFanInSummary fromFanIn;
    ModulationTargetFanInSummary toFanIn;

    bool operator==(const ModulationRouteMapComparison&) const noexcept = default;
};

namespace modulation_comparison_detail {

struct InspectedRouteEntry {
    std::string rawText;
    std::optional<CompiledModulationRoute> compiled;
    std::optional<std::string> canonicalText;
    ModulationRouteError error = ModulationRouteError::None;
    bool responseFinite = false;

    [[nodiscard]] bool accepted() const noexcept {
        return compiled.has_value() && canonicalText.has_value() && responseFinite;
    }
};

[[nodiscard]] inline std::vector<InspectedRouteEntry> inspectResolvedMap(
    std::span<const std::string> authored, bool audioEnabled, std::string_view& originOut) {
    const bool useDefaults = authored.empty();
    originOut = modulation_routing::originName(useDefaults, audioEnabled);
    const auto defaults = modulation_routing::defaults(audioEnabled);
    const std::size_t count = useDefaults ? defaults.size() : authored.size();

    std::vector<InspectedRouteEntry> entries;
    entries.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const std::string_view text = useDefaults ? defaults[index] : std::string_view{authored[index]};
        const auto parsed = SculptureModulationEngine::inspectRouteResult(text);
        InspectedRouteEntry entry{
            .rawText = std::string(text),
            .compiled = parsed.route,
            .canonicalText = parsed.route ? formatCanonicalModulationRoute(*parsed.route) : std::nullopt,
            .error = parsed.error,
            .responseFinite = parsed.route ? modulation_inspection::evaluateRouteProbes(*parsed.route).ready() : false,
        };
        entries.push_back(std::move(entry));
    }
    return entries;
}

} // namespace modulation_comparison_detail

// Compare two authored modulation route maps after resolving empty-map defaults.
// Distinguishes raw authored text equality, resolved text equality, and compiled
// semantic equivalence across source/target/curve aliases and numeric spellings.
[[nodiscard]] inline ModulationRouteMapComparison compareModulationRouteMaps(
    std::span<const std::string> fromAuthored,
    std::span<const std::string> toAuthored,
    bool audioEnabled = false) {
    ModulationRouteMapComparison report;
    report.authoredTextSame = std::equal(
        fromAuthored.begin(), fromAuthored.end(), toAuthored.begin(), toAuthored.end());

    const auto fromEntries = modulation_comparison_detail::inspectResolvedMap(
        fromAuthored, audioEnabled, report.fromOrigin);
    const auto toEntries = modulation_comparison_detail::inspectResolvedMap(
        toAuthored, audioEnabled, report.toOrigin);

    report.fromCount = fromEntries.size();
    report.toCount = toEntries.size();
    report.resolvedTextSame = std::equal(
        fromEntries.begin(), fromEntries.end(), toEntries.begin(), toEntries.end(),
        [](const auto& a, const auto& b) { return a.rawText == b.rawText; });

    std::vector<CompiledModulationRoute> fromCompiled;
    std::vector<CompiledModulationRoute> toCompiled;
    fromCompiled.reserve(fromEntries.size());
    toCompiled.reserve(toEntries.size());

    for (const auto& entry : fromEntries) {
        if (!entry.accepted()) report.fromAccepted = false;
        if (entry.compiled) fromCompiled.push_back(*entry.compiled);
    }
    for (const auto& entry : toEntries) {
        if (!entry.accepted()) report.toAccepted = false;
        if (entry.compiled) toCompiled.push_back(*entry.compiled);
    }

    report.fromFanIn = inspectModulationTargetFanIn(fromCompiled);
    report.toFanIn = inspectModulationTargetFanIn(toCompiled);
    if (!report.fromFanIn.finite) report.fromAccepted = false;
    if (!report.toFanIn.finite) report.toAccepted = false;

    report.semanticSame = report.fromAccepted && report.toAccepted
        && fromEntries.size() == toEntries.size()
        && std::equal(fromEntries.begin(), fromEntries.end(), toEntries.begin(),
                      [](const auto& a, const auto& b) {
                          return sameCompiledModulationRoute(*a.compiled, *b.compiled);
                      });

    std::vector<bool> toPaired(toEntries.size(), false);
    for (std::size_t i = 0; i < fromEntries.size(); ++i) {
        const auto& from = fromEntries[i];
        if (!from.accepted()) {
            ++report.rejectedCount;
            report.cues.push_back({
                .relation = ModulationRouteCueRelation::Rejected,
                .fromIndex = i,
                .toIndex = std::nullopt,
                .sourceId = from.compiled ? from.compiled->sourceId : ModulationSourceId::None,
                .targetId = from.compiled ? from.compiled->targetId : ModulationTargetId::None,
                .fromText = from.rawText,
                .toText = "-",
                .fromError = from.error,
                .toError = ModulationRouteError::None,
            });
            continue;
        }
        std::optional<std::size_t> exactMatch;
        for (std::size_t j = 0; j < toEntries.size(); ++j) {
            if (toPaired[j] || !toEntries[j].accepted()) continue;
            if (sameCompiledModulationRoute(*from.compiled, *toEntries[j].compiled)) {
                exactMatch = j;
                break;
            }
        }
        if (exactMatch) {
            toPaired[*exactMatch] = true;
            ++report.sharedCount;
            report.cues.push_back({
                .relation = ModulationRouteCueRelation::Shared,
                .fromIndex = i,
                .toIndex = *exactMatch,
                .sourceId = from.compiled->sourceId,
                .targetId = from.compiled->targetId,
                .fromText = *from.canonicalText,
                .toText = *toEntries[*exactMatch].canonicalText,
            });
            continue;
        }
        std::optional<std::size_t> endpointMatch;
        for (std::size_t j = 0; j < toEntries.size(); ++j) {
            if (toPaired[j] || !toEntries[j].accepted()) continue;
            if (from.compiled->sourceId == toEntries[j].compiled->sourceId
                && from.compiled->targetId == toEntries[j].compiled->targetId) {
                endpointMatch = j;
                break;
            }
        }
        if (endpointMatch) {
            toPaired[*endpointMatch] = true;
            ++report.modifiedCount;
            report.cues.push_back({
                .relation = ModulationRouteCueRelation::Modified,
                .fromIndex = i,
                .toIndex = *endpointMatch,
                .sourceId = from.compiled->sourceId,
                .targetId = from.compiled->targetId,
                .fromText = *from.canonicalText,
                .toText = *toEntries[*endpointMatch].canonicalText,
            });
            continue;
        }
        ++report.fromOnlyCount;
        report.cues.push_back({
            .relation = ModulationRouteCueRelation::FromOnly,
            .fromIndex = i,
            .toIndex = std::nullopt,
            .sourceId = from.compiled->sourceId,
            .targetId = from.compiled->targetId,
            .fromText = *from.canonicalText,
            .toText = "-",
        });
    }

    for (std::size_t j = 0; j < toEntries.size(); ++j) {
        if (toPaired[j]) continue;
        const auto& to = toEntries[j];
        if (!to.accepted()) {
            ++report.rejectedCount;
            report.cues.push_back({
                .relation = ModulationRouteCueRelation::Rejected,
                .fromIndex = std::nullopt,
                .toIndex = j,
                .sourceId = to.compiled ? to.compiled->sourceId : ModulationSourceId::None,
                .targetId = to.compiled ? to.compiled->targetId : ModulationTargetId::None,
                .fromText = "-",
                .toText = to.rawText,
                .fromError = ModulationRouteError::None,
                .toError = to.error,
            });
            continue;
        }
        ++report.toOnlyCount;
        report.cues.push_back({
            .relation = ModulationRouteCueRelation::ToOnly,
            .fromIndex = std::nullopt,
            .toIndex = j,
            .sourceId = to.compiled->sourceId,
            .targetId = to.compiled->targetId,
            .fromText = "-",
            .toText = *to.canonicalText,
        });
    }

    return report;
}

// -----------------------------------------------------------------------------
// Educational Annotation: Sequence / Timeline Modulation Continuity Analysis
// Evaluates adjacent keyframe route transitions across a sequence or tour.
// Transitions with resolvedTextSame == true preserve slew/smoothing history
// when SculptureControlSession reconciles the next keyframe's routing map.
// -----------------------------------------------------------------------------
struct ModulationSequenceTransitionStep {
    std::size_t fromKeyframeIndex = 0;
    std::size_t toKeyframeIndex = 0;
    bool wrapsLoop = false;
    ModulationRouteMapComparison comparison{};

    [[nodiscard]] bool operator==(const ModulationSequenceTransitionStep&) const noexcept = default;
};

struct ModulationSequenceTransitionReport {
    bool audioEnabled = false;
    bool loop = false;
    std::size_t keyframeCount = 0;
    bool allKeyframesValid = true;
    std::optional<std::size_t> firstInvalidKeyframeIndex{};
    std::size_t transitionCount = 0;
    std::size_t seamlessSmoothingTransitionCount = 0;
    std::size_t semanticallyEquivalentTransitionCount = 0;
    std::size_t reconfiguredTransitionCount = 0;
    std::size_t maxTargetFanIn = 0;
    std::vector<ModulationSequenceTransitionStep> steps{};

    [[nodiscard]] bool operator==(const ModulationSequenceTransitionReport&) const noexcept = default;
};

[[nodiscard]] inline ModulationSequenceTransitionReport analyzeSequenceModulationTransitions(
    std::span<const std::vector<std::string>> keyframeRoutingMaps,
    bool loop = false,
    bool audioEnabled = false) {
    ModulationSequenceTransitionReport report{};
    report.audioEnabled = audioEnabled;
    report.loop = loop;
    report.keyframeCount = keyframeRoutingMaps.size();

    for (std::size_t k = 0; k < keyframeRoutingMaps.size(); ++k) {
        const auto selfCheck = compareModulationRouteMaps(
            keyframeRoutingMaps[k], keyframeRoutingMaps[k], audioEnabled);
        for (const auto& target : selfCheck.fromFanIn.targets) {
            report.maxTargetFanIn = std::max(report.maxTargetFanIn, target.routeCount);
        }
        if (!selfCheck.fromAccepted) {
            report.allKeyframesValid = false;
            if (!report.firstInvalidKeyframeIndex) {
                report.firstInvalidKeyframeIndex = k;
            }
        }
    }

    auto appendStep = [&](std::size_t fromIndex, std::size_t toIndex, bool wrapsLoop) {
        auto comp = compareModulationRouteMaps(
            keyframeRoutingMaps[fromIndex], keyframeRoutingMaps[toIndex], audioEnabled);
        ++report.transitionCount;
        if (comp.resolvedTextSame) {
            ++report.seamlessSmoothingTransitionCount;
        } else {
            ++report.reconfiguredTransitionCount;
        }
        if (comp.semanticSame) {
            ++report.semanticallyEquivalentTransitionCount;
        }
        report.steps.push_back({
            .fromKeyframeIndex = fromIndex,
            .toKeyframeIndex = toIndex,
            .wrapsLoop = wrapsLoop,
            .comparison = std::move(comp),
        });
    };

    for (std::size_t i = 0; i + 1 < keyframeRoutingMaps.size(); ++i) {
        appendStep(i, i + 1, false);
    }
    if (loop && keyframeRoutingMaps.size() >= 2) {
        appendStep(keyframeRoutingMaps.size() - 1, 0, true);
    }
    return report;
}

template <typename Keyframe>
    requires requires(const Keyframe& kf) {
        { kf.routingMap } -> std::convertible_to<std::span<const std::string>>;
    }
[[nodiscard]] inline ModulationSequenceTransitionReport analyzeSequenceModulationTransitions(
    std::span<const Keyframe> keyframes,
    bool loop = false,
    bool audioEnabled = false) {
    std::vector<std::vector<std::string>> maps;
    maps.reserve(keyframes.size());
    for (const auto& kf : keyframes) {
        maps.emplace_back(kf.routingMap.begin(), kf.routingMap.end());
    }
    return analyzeSequenceModulationTransitions(
        std::span<const std::vector<std::string>>(maps), loop, audioEnabled);
}

} // namespace epi_sculpt
