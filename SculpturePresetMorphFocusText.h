#pragma once

#include "SculpturePresetMorphFocus.h"
#include "SculptureInspectionText.h"

#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <string>

namespace epi_sculpt {

[[nodiscard]] inline std::optional<std::string> serializePresetMorphFocusText(
    const SculpturePresetAsset& from, const SculpturePresetAsset& to,
    const PreparedPresetMorphFocus& focus) {
    if (!validPresetMorphFocus(from, to, focus)) return std::nullopt;
    const auto& final = focus.intervals.back();
    const auto changes = inspectSpatialPresetDistance(final.before, final.after);
    if (!changes) return std::nullopt;
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "morph-focus-scope: local sampled spatial parameter search; not a rendered observation\n"
        << "morph-focus-effect: read-only; no files or runtime state changed\n"
        << "morph-focus-search: largest seed step, then greater-distance half; earlier half wins ties\n"
        << "morph-focus-limit: unsampled intervals and global maxima are not certified\n"
        << "morph-focus-from: ";
    quoteInspectionText(out, from.id);
    out << "\nmorph-focus-from-revision: ";
    quoteInspectionText(out, from.revision);
    out << "\nmorph-focus-to: ";
    quoteInspectionText(out, to.id);
    out << "\nmorph-focus-to-revision: ";
    quoteInspectionText(out, to.revision);
    out << std::setprecision(std::numeric_limits<double>::max_digits10)
        << "\nmorph-focus-seed-samples: " << focus.seed.samples.size()
        << "\nmorph-focus-refinements: " << focus.intervals.size() - 1U
        << "\nmorph-focus-columns: level\tbegin\tend\tdistance\n";
    for (std::size_t level = 0; level < focus.intervals.size(); ++level) {
        const auto& interval = focus.intervals[level];
        out << "morph-focus-interval: " << level << '\t' << interval.begin << '\t'
            << interval.end << '\t' << interval.distance << '\n';
    }
    out << "morph-focus-bracket: " << final.begin << '\t' << final.end
        << "\nmorph-focus-change-columns: parameter\tbefore\tafter\tdistance-contribution\n";
    for (std::size_t index = 0; index < changes->differenceCount; ++index) {
        const auto& delta = changes->differences[index];
        out << "morph-focus-change: " << delta.parameter;
        if (delta.component) out << '[' << *delta.component << ']';
        out << '\t' << delta.from << '\t' << delta.to << '\t' << delta.contribution << '\n';
    }
    return out.str();
}

} // namespace epi_sculpt
