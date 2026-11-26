#pragma once

#include "SculptureInspectionText.h"
#include "SculpturePresetMorphStudy.h"

#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <string>

namespace epi_sculpt {

// Prepare a complete document without changing caller-owned streams. The same
// copied view and validation feed JSON and text; CLI publication stays outside.
[[nodiscard]] inline std::optional<std::string> serializePresetMorphStudyText(
    const SculpturePresetAsset& from, const SculpturePresetAsset& to,
    const PreparedPresetMorphStudy& study) {
    if (!validPresetMorphStudy(from, to, study)) return std::nullopt;
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "morph-study-scope: sampled spatial parameter path; not a rendered observation\n"
        << "morph-study-effect: read-only; no files or runtime state changed\n"
        << "morph-study-sampling: uniform progress; unsampled intervals are not certified\n"
        << "morph-study-metric: normalized spatial parameter distance; not visual similarity\n"
        << "morph-study-from: ";
    quoteInspectionText(out, from.id);
    out << "\nmorph-study-from-revision: ";
    quoteInspectionText(out, from.revision);
    out << "\nmorph-study-to: ";
    quoteInspectionText(out, to.id);
    out << "\nmorph-study-to-revision: ";
    quoteInspectionText(out, to.revision);
    out << std::setprecision(std::numeric_limits<double>::max_digits10)
        << "\nmorph-study-samples: " << study.samples.size()
        << "\nmorph-study-columns: index\tprogress\tprevious-distance\tfrom-distance\tto-distance\n";
    for (std::size_t index = 0; index < study.samples.size(); ++index) {
        const auto& sample = study.samples[index];
        out << "morph-study-sample: " << index << '\t' << sample.position << '\t'
            << sample.previousDistance << '\t' << sample.fromDistance << '\t' << sample.toDistance << '\n';
    }
    out << "morph-study-sampled-distance: " << study.sampledDistance
        << "\nmorph-study-largest-step: " << study.largestStepEndingAt << '\t'
        << study.largestStep.distance
        << "\nmorph-study-change-columns: parameter\tbefore\tafter\tdistance-contribution\n";
    for (std::size_t index = 0; index < study.largestStep.differenceCount; ++index) {
        const auto& delta = study.largestStep.differences[index];
        out << "morph-study-change: " << delta.parameter;
        if (delta.component) out << '[' << *delta.component << ']';
        out << '\t' << delta.from << '\t' << delta.to << '\t' << delta.contribution << '\n';
    }
    return out.str();
}

} // namespace epi_sculpt
