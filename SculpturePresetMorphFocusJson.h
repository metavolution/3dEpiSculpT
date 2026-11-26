#pragma once

#include "SculpturePresetMorphFocus.h"
#include "SculptureInspectionJson.h"
#include "SculptureSpatialInspectionJson.h"

#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <string>

namespace epi_sculpt {

// Versioned derived inspection data, not an executable preset or GPU snapshot.
// Complete admission and private formatting precede publication by the caller.
[[nodiscard]] inline std::optional<std::string> serializePresetMorphFocusJson(
    const SculpturePresetAsset& from, const SculpturePresetAsset& to,
    const PreparedPresetMorphFocus& focus) {
    if (!validPresetMorphFocus(from, to, focus)) return std::nullopt;
    const auto& final = focus.intervals.back();
    const auto changes = inspectSpatialPresetDistance(final.before, final.after);
    if (!changes) return std::nullopt;
    using inspection_json_detail::bytes;
    using inspection_json_detail::label;
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<double>::max_digits10)
        << "{\n\"schema\":\"epic-preset-morph-focus-v1\","
        << "\n\"scope\":\"local_sampled_spatial_parameters\","
        << "\n\"read_only\":true,\"parameter_scope\":\"spatial_distance_components\","
        << "\n\"search\":\"largest_seed_step_then_greater_distance_half\","
        << "\n\"tie_break\":\"earlier_interval\","
        << "\n\"metric\":\"normalized_spatial_parameter_distance\","
        << "\n\"continuous_path_certified\":false,\"global_maximum_certified\":false,"
        << "\n\"from\":{\"id_hex\":";
    bytes(out, from.id);
    out << ",\"revision_hex\":";
    bytes(out, from.revision);
    out << "},\n\"to\":{\"id_hex\":";
    bytes(out, to.id);
    out << ",\"revision_hex\":";
    bytes(out, to.revision);
    out << "},\n\"seed_sample_count\":" << focus.seed.samples.size()
        << ",\n\"seed_step_ending_index\":" << focus.seed.largestStepEndingAt
        << ",\n\"refinements\":" << focus.intervals.size() - 1U << ",\n\"intervals\":[";
    for (std::size_t level = 0; level < focus.intervals.size(); ++level) {
        const auto& interval = focus.intervals[level];
        if (level != 0U) out << ',';
        out << "\n{\"level\":" << level << ",\"begin\":" << interval.begin << ",\"end\":" << interval.end
            << ",\"distance\":" << interval.distance << '}';
    }
    out << "\n],\n\"bracket\":{\"begin\":" << final.begin << ",\"end\":" << final.end
        << ",\"distance\":" << final.distance << ",\"component_count\":" << changes->componentCount
        << ",\"changed_count\":" << changes->changedCount;
    const auto components = [&](std::string_view key, const SculptureProgramParams& params) {
        out << ",\n";
        label(out, key);
        out << ':';
        spatial_inspection_json_detail::componentsObject(out, params);
    };
    components("before_spatial_components", final.before);
    components("after_spatial_components", final.after);
    out << ",\n\"changes\":";
    spatial_inspection_json_detail::differenceArray(out, *changes);
    out << "}\n}\n";
    return out.str();
}

} // namespace epi_sculpt
