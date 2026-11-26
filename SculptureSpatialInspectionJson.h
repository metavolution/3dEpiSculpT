#pragma once

#include "SculptureInspectionJson.h"
#include "SculpturePresetNeighborhood.h"

namespace epi_sculpt::spatial_inspection_json_detail {

// Shared domain writers consume already-admitted values. Document validation,
// precision/locale and publication belong to the enclosing serializer; the
// metric's single component visitor owns names, order and numeric meaning.
inline void componentsObject(std::ostream& out, const SculptureProgramParams& params) {
    out << '{';
    bool first = true;
    static_cast<void>(preset_neighborhood_detail::visitNumericComponents(params, params,
        [&](std::string_view name, double value, double, std::optional<std::size_t> component) {
            if (!first) out << ',';
            first = false;
            // Registry names are compiled ASCII. Array labels match text reports.
            out << '"' << name;
            if (component) out << '[' << *component << ']';
            out << "\":" << value;
            return true;
        }));
    out << '}';
}

inline void differenceArray(std::ostream& out, const SpatialPresetDistanceReport& report) {
    out << '[';
    for (std::size_t index = 0; index < report.differenceCount; ++index) {
        const auto& delta = report.differences[index];
        if (index != 0U) out << ',';
        out << "{\"parameter\":";
        inspection_json_detail::label(out, delta.parameter);
        out << ",\"component\":";
        if (delta.component) out << *delta.component;
        else out << "null";
        out << ",\"from\":" << delta.from << ",\"to\":" << delta.to
            << ",\"contribution\":" << delta.contribution << '}';
    }
    out << ']';
}

} // namespace epi_sculpt::spatial_inspection_json_detail
