#pragma once

#include "SculpturePresetPathFrontier.h"
#include "SculptureInspectionJson.h"
#include "SculptureSpatialInspectionJson.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace epi_sculpt {
namespace preset_path_json_detail {

// Validate the bounded read model before formatting. This checks shape and
// numeric consistency, not optimality or current-catalog/morph compatibility;
// those remain properties of the query and authoring owners respectively.
[[nodiscard]] inline bool validPath(const PreparedPresetPath& path) {
    if (!presetPathStructureAdmitted(path)
        || !std::isfinite(path.totalDistance) || !std::isfinite(path.largestStepDistance)) return false;
    double total = 0, peak = 0;
    for (std::size_t i = 0; i < path.steps.size(); ++i) {
        const auto& step = path.steps[i];
        if (!std::isfinite(step.previousDistance) || step.previousDistance < 0
            || step.previousDistance > path.request.maxDistance
            || (i == 0 && step.previousDistance != 0)
            || step.details.has_value() != (i != 0 && path.request.details)) return false;
        if (step.details) {
            const auto& report = *step.details;
            if (report.distance != step.previousDistance || report.componentCount == 0
                || report.changedCount > report.componentCount
                || report.differenceCount != std::min(report.changedCount, report.differences.size())) return false;
            for (std::size_t j = 0; j < report.differenceCount; ++j) {
                const auto& delta = report.differences[j];
                if (delta.parameter.empty() || !std::isfinite(delta.from) || !std::isfinite(delta.to)
                    || !std::isfinite(delta.contribution) || delta.contribution <= 0) return false;
            }
        }
        total += step.previousDistance;
        peak = std::max(peak, step.previousDistance);
    }
    return total == path.totalDistance && peak == path.largestStepDistance;
}

inline void preamble(std::ostream& out, std::string_view schema) {
    out << "{\"schema\":";
    inspection_json_detail::label(out, schema);
    out << ",\"scope\":\"decoded_spatial_parameters\",\"read_only\":true,"
        << "\"metric\":\"normalized_spatial_parameter_distance\","
        << "\"identity_encoding\":\"hex_bytes\",\"morph_readiness_certified\":false,"
        << "\"rendered_smoothness_certified\":false,\"total_distance_optimized\":false,"
        << "\"detail_policy\":\"largest_3_nonzero_mean_contributions_metric_order_ties\","
        << "\"tie_break\":\"catalog_scan\"";
}

inline void request(std::ostream& out, double distance, std::size_t hops, bool details) {
    out << "\"max_distance\":" << distance << ",\"max_hops\":" << hops
        << ",\"details\":" << (details ? "true" : "false");
}

inline void route(std::ostream& out, const PreparedPresetPath& path) {
    out << "{\"objective\":";
    inspection_json_detail::label(out, presetPathObjectiveName(path.request.objective));
    out << ",\"max_hops\":" << path.request.maxHops
        << ",\"hop_count\":" << path.steps.size() - 1
        << ",\"largest_step_distance\":" << path.largestStepDistance
        << ",\"total_distance\":" << path.totalDistance << ",\"steps\":[";
    for (std::size_t i = 0; i < path.steps.size(); ++i) {
        const auto& step = path.steps[i];
        if (i) out << ',';
        out << "{\"id_hex\":";
        inspection_json_detail::bytes(out, step.id);
        out << ",\"revision_hex\":";
        inspection_json_detail::bytes(out, step.revision);
        out << ",\"distance_from_previous\":" << step.previousDistance;
        if (step.details) {
            const auto& report = *step.details;
            out << ",\"details\":{\"component_count\":" << report.componentCount
                << ",\"changed_count\":" << report.changedCount
                << ",\"changes\":";
            spatial_inspection_json_detail::differenceArray(out, report);
            out << '}';
        }
        out << '}';
    }
    out << "]}";
}
} // namespace preset_path_json_detail

// Versioned derived documents, never executable presets or replay evidence.
// Formatting is private and uses round-trippable doubles in the classic locale.
[[nodiscard]] inline std::optional<std::string> serializePresetPathJson(const PreparedPresetPath& path) {
    if (!preset_path_json_detail::validPath(path)) return std::nullopt;
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<double>::max_digits10);
    preset_path_json_detail::preamble(out, "epic-preset-path-v1");
    out << ",\"candidate_count\":" << path.candidateCount << ",\"request\":{";
    preset_path_json_detail::request(out, path.request.maxDistance, path.request.maxHops, path.request.details);
    out << "},\"path\":";
    preset_path_json_detail::route(out, path);
    out << "}\n";
    return out.str();
}

[[nodiscard]] inline std::optional<std::string> serializePresetPathFrontierJson(
    const PreparedPresetPathFrontier& frontier) {
    if (!frontier.ready() || frontier.routes.empty() || frontier.routes.size() > kMaxPresetPathHops
        || frontier.request.maxHops == 0 || frontier.request.maxHops > kMaxPresetPathHops
        || !std::isfinite(frontier.request.maxDistance) || frontier.request.maxDistance < 0) return std::nullopt;
    const auto& first = frontier.routes.front();
    for (std::size_t i = 0; i < frontier.routes.size(); ++i) {
        const auto& path = frontier.routes[i];
        if (!preset_path_json_detail::validPath(path)
            || path.request.objective != PresetPathObjective::GentlestSteps
            || path.candidateCount != frontier.candidateCount
            || path.request.maxDistance != frontier.request.maxDistance
            || path.request.details != frontier.request.details
            || path.request.maxHops != std::max(std::size_t{1}, path.steps.size() - 1)
            || path.request.maxHops > frontier.request.maxHops
            || path.steps.front().id != first.steps.front().id
            || path.steps.front().revision != first.steps.front().revision
            || path.steps.back().id != first.steps.back().id
            || path.steps.back().revision != first.steps.back().revision) return std::nullopt;
        if (i && (path.steps.size() <= frontier.routes[i - 1].steps.size()
            || path.largestStepDistance >= frontier.routes[i - 1].largestStepDistance)) return std::nullopt;
    }
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<double>::max_digits10);
    preset_path_json_detail::preamble(out, "epic-preset-path-frontier-v1");
    out << ",\"policy\":\"nondominated_hops_largest_step\",\"candidate_count\":"
        << frontier.candidateCount << ",\"request\":{";
    preset_path_json_detail::request(out, frontier.request.maxDistance, frontier.request.maxHops, frontier.request.details);
    out << "},\"routes\":[";
    for (std::size_t i = 0; i < frontier.routes.size(); ++i) {
        if (i) out << ',';
        preset_path_json_detail::route(out, frontier.routes[i]);
    }
    out << "]}\n";
    return out.str();
}

} // namespace epi_sculpt
