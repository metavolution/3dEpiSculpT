#pragma once

#include "SculptureModulationRouteComparison.h"
#include "SculptureInspectionJson.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

namespace epi_sculpt {

namespace route_comparison_json_detail {

[[nodiscard]] inline bool fanInBoundsFinite(const ModulationTargetFanInSummary& summary) noexcept {
    for (const auto& entry : summary.targets) {
        if (!std::isfinite(entry.minContributionSum) || !std::isfinite(entry.maxContributionSum)) {
            return false;
        }
    }
    return true;
}

inline void writeOptionalIndex(std::ostream& out, const std::optional<std::size_t>& index) {
    if (index) out << *index;
    else out << "null";
}

inline void writeFanInArray(std::ostream& out, const ModulationTargetFanInSummary& fanIn) {
    using inspection_json_detail::label;
    using modulation_inspection::name;
    out << '[';
    for (std::size_t i = 0; i < fanIn.targets.size(); ++i) {
        const auto& entry = fanIn.targets[i];
        if (i != 0U) out << ',';
        out << "\n{\"target\":";
        label(out, name(entry.targetId));
        out << ",\"route_count\":" << entry.routeCount
            << ",\"min_contribution_sum\":" << entry.minContributionSum
            << ",\"max_contribution_sum\":" << entry.maxContributionSum << '}';
    }
    if (!fanIn.targets.empty()) out << '\n';
    out << ']';
}

} // namespace route_comparison_json_detail

// Validate that a ModulationRouteMapComparison matches the actual resolved route
// maps and contains only finite target fan-in bounds before JSON publication.
[[nodiscard]] inline bool validModulationRouteMapComparison(
    std::span<const std::string> fromAuthored,
    std::span<const std::string> toAuthored,
    const ModulationRouteMapComparison& report,
    bool audioEnabled = false) {
    if (!route_comparison_json_detail::fanInBoundsFinite(report.fromFanIn)
        || !route_comparison_json_detail::fanInBoundsFinite(report.toFanIn)) {
        return false;
    }
    return report == compareModulationRouteMaps(fromAuthored, toAuthored, audioEnabled);
}

// Serialize a validated route-map comparison into a strict versioned JSON document.
// Uses hex byte encoding for preset IDs, revisions, and raw/canonical route text
// alongside escaped display strings so arbitrary bytes never corrupt JSON output.
[[nodiscard]] inline std::optional<std::string> serializeModulationRouteComparisonJson(
    std::string_view fromId, std::string_view fromRevision,
    std::span<const std::string> fromAuthored,
    std::string_view toId, std::string_view toRevision,
    std::span<const std::string> toAuthored,
    const ModulationRouteMapComparison& report,
    bool audioEnabled = false) {
    if (fromId.empty() || toId.empty()
        || !validModulationRouteMapComparison(fromAuthored, toAuthored, report, audioEnabled)) {
        return std::nullopt;
    }
    using inspection_json_detail::bytes;
    using inspection_json_detail::label;
    using modulation_inspection::name;

    const bool bothAccepted = report.fromAccepted && report.toAccepted;
    const std::string_view semanticStatus =
        !bothAccepted ? "rejected" : (report.semanticSame ? "same" : "different");

    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<float>::max_digits10)
        << "{\n\"schema\":\"epic-preset-route-comparison-v1\","
        << "\n\"scope\":\"resolved_modulation_route_maps\","
        << "\n\"read_only\":true,"
        << "\n\"policy\":";
    label(out, audioEnabled ? "flagship_audio_on_defaults_when_empty"
                            : "flagship_audio_off_defaults_when_empty");
    out << ",\n\"from\":{\"id_hex\":";
    bytes(out, fromId);
    out << ",\"revision_hex\":";
    bytes(out, fromRevision);
    out << ",\"origin\":";
    label(out, report.fromOrigin);
    out << ",\"count\":" << report.fromCount
        << ",\"status\":";
    label(out, report.fromAccepted ? "accepted" : "rejected");
    out << ",\"active_targets\":" << report.fromFanIn.targets.size()
        << ",\"superposed_targets\":" << report.fromFanIn.superposedTargetCount
        << "},\n\"to\":{\"id_hex\":";
    bytes(out, toId);
    out << ",\"revision_hex\":";
    bytes(out, toRevision);
    out << ",\"origin\":";
    label(out, report.toOrigin);
    out << ",\"count\":" << report.toCount
        << ",\"status\":";
    label(out, report.toAccepted ? "accepted" : "rejected");
    out << ",\"active_targets\":" << report.toFanIn.targets.size()
        << ",\"superposed_targets\":" << report.toFanIn.superposedTargetCount
        << "},\n\"authored_routing_compatibility\":";
    label(out, report.authoredTextSame ? "same" : "different");
    out << ",\n\"resolved_routing_compatibility\":";
    label(out, report.resolvedTextSame ? "same" : "different");
    out << ",\n\"semantic_routing_compatibility\":";
    label(out, semanticStatus);
    out << ",\n\"shared_routes\":" << report.sharedCount
        << ",\n\"modified_routes\":" << report.modifiedCount
        << ",\n\"from_only_routes\":" << report.fromOnlyCount
        << ",\n\"to_only_routes\":" << report.toOnlyCount
        << ",\n\"rejected_routes\":" << report.rejectedCount
        << ",\n\"cues\":[";
    for (std::size_t i = 0; i < report.cues.size(); ++i) {
        const auto& cue = report.cues[i];
        if (i != 0U) out << ',';
        out << "\n{\"relation\":";
        label(out, modulationRouteCueRelationName(cue.relation));
        out << ",\"from_index\":";
        route_comparison_json_detail::writeOptionalIndex(out, cue.fromIndex);
        out << ",\"to_index\":";
        route_comparison_json_detail::writeOptionalIndex(out, cue.toIndex);
        out << ",\"source\":";
        label(out, name(cue.sourceId));
        out << ",\"target\":";
        label(out, name(cue.targetId));
        out << ",\"from_route\":";
        label(out, cue.fromText);
        out << ",\"from_route_hex\":";
        bytes(out, cue.fromText);
        out << ",\"to_route\":";
        label(out, cue.toText);
        out << ",\"to_route_hex\":";
        bytes(out, cue.toText);
        out << ",\"from_error\":";
        label(out, name(cue.fromError));
        out << ",\"to_error\":";
        label(out, name(cue.toError));
        out << '}';
    }
    if (!report.cues.empty()) out << '\n';
    out << "],\n\"target_fan_in\":{\"from\":";
    route_comparison_json_detail::writeFanInArray(out, report.fromFanIn);
    out << ",\"to\":";
    route_comparison_json_detail::writeFanInArray(out, report.toFanIn);
    out << "}\n}\n";
    return out.str();
}

} // namespace epi_sculpt
