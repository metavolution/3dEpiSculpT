#pragma once

#include "SculpturePresetMorphStudy.h"
#include "SculptureInspectionJson.h"
#include "SculptureSpatialInspectionJson.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace epi_sculpt {
// Serialize the copied inspection view, never a runtime state/checkpoint. Invalid
// public report values reject before output; allocation failures propagate before
// a complete owned document can be published by the caller.
[[nodiscard]] inline std::optional<std::string> serializePresetMorphStudyJson(
    const SculpturePresetAsset& from, const SculpturePresetAsset& to,
    const PreparedPresetMorphStudy& study) {
    if (!validPresetMorphStudy(from, to, study)) return std::nullopt;
    using inspection_json_detail::bytes;
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<double>::max_digits10)
        << "{\n\"schema\":\"epic-preset-morph-study-v1\","
        << "\n\"scope\":\"sampled_spatial_parameters\","
        << "\n\"sampling\":\"uniform_progress\",\"continuous_path_certified\":false,"
        << "\n\"metric\":\"normalized_spatial_parameter_distance\","
        << "\n\"parameter_scope\":\"spatial_distance_components\",\n\"from\":{\"id_hex\":";
    bytes(out, from.id);
    out << ",\"revision_hex\":";
    bytes(out, from.revision);
    out << "},\n\"to\":{\"id_hex\":";
    bytes(out, to.id);
    out << ",\"revision_hex\":";
    bytes(out, to.revision);
    out << "},\n\"sample_count\":" << study.samples.size() << ",\n\"samples\":[";
    for (std::size_t index = 0; index < study.samples.size(); ++index) {
        const auto& sample = study.samples[index];
        if (index != 0U) out << ',';
        out << "\n{\"index\":" << index << ",\"position\":" << sample.position
            << ",\"previous_distance\":" << sample.previousDistance
            << ",\"from_distance\":" << sample.fromDistance << ",\"to_distance\":" << sample.toDistance
            << ",\"spatial_components\":";
        spatial_inspection_json_detail::componentsObject(out, sample.params);
        out << '}';
    }
    out << "\n],\n\"sampled_distance\":" << study.sampledDistance
        << ",\n\"largest_step\":{\"ending_index\":" << study.largestStepEndingAt
        << ",\"distance\":" << study.largestStep.distance
        << ",\"component_count\":" << study.largestStep.componentCount
        << ",\"changed_count\":" << study.largestStep.changedCount << ",\"changes\":";
    spatial_inspection_json_detail::differenceArray(out, study.largestStep);
    out << "}\n}\n";
    return out.str();
}

} // namespace epi_sculpt
