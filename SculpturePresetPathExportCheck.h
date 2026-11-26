#pragma once

#include "SculpturePresetConfiguration.h"
#include "SculpturePresetPathTour.h"
#include "SculptureTourTiming.h"

#include <string>
#include <vector>

namespace epi_sculpt {

enum class PresetPathExportCheckError { None, TourRejected, ConfigurationRejected, Unprepared };

// Owned inspection of authored intervals, measured from resume at cue zero.
// Includes the first cue's transition and the last cue's dwell. Playback's
// per-update cue bound and frame quantization can extend actual elapsed time.
struct PresetPathCueTiming {
    std::string presetId, presetRevision;
    double transitionStartSeconds = 0.0;
    double arrivalSeconds = 0.0;
    double dwellEndSeconds = 0.0;
};

// Cold assessment of current catalog values. No destination admission, file
// publication, native capability or future cue/replay guarantee is implied.
struct PresetPathExportCheck {
    PresetPathExportCheckError error = PresetPathExportCheckError::Unprepared;
    std::string_view tourDiagnostic{}; // Stable literals from the tour owner.
    PresetConfigurationError configurationError = PresetConfigurationError::None;
    std::optional<ModulationRouteDiagnostic> routeDiagnostic = std::nullopt;
    std::size_t cueCount = 0;
    // At most kMaxPresetPathHops + 1 rows, present only after full admission.
    std::vector<PresetPathCueTiming> timeline;
    [[nodiscard]] bool ready() const noexcept { return error == PresetPathExportCheckError::None; }
    [[nodiscard]] double nominalDurationSeconds() const noexcept {
        return timeline.empty() ? 0.0 : timeline.back().dwellEndSeconds;
    }
};

// Reuse the real authoring boundaries; never duplicate compatibility/parser/
// codec policy or call the file publisher to simulate an export. Encoded bytes
// are transient preparation, discarded before returning owned inspection facts.
[[nodiscard]] inline PresetPathExportCheck checkPresetPathExport(
    std::span<const SculpturePresetAsset> assets, const PreparedPresetPath& path,
    float transitionSeconds = 3.0f, PresetPathTourStyle style = {}) {
    const auto tour = preparePresetPathTour(assets, path, transitionSeconds, style);
    if (!tour.ready()) return {PresetPathExportCheckError::TourRejected, tour.diagnostic};
    const auto configuration = preparePresetConfiguration(*tour.world);
    const auto cueCount = tour.world->playlist.targets.size();
    if (!configuration.ready()) {
        return {PresetPathExportCheckError::ConfigurationRejected, {}, configuration.error,
            configuration.error == PresetConfigurationError::InvalidModulationRoute
                ? std::optional{configuration.routeDiagnostic} : std::nullopt, cueCount};
    }
    PresetPathExportCheck result{
        PresetPathExportCheckError::None, {}, PresetConfigurationError::None, std::nullopt, cueCount};
    result.timeline.reserve(cueCount);
    double elapsed = 0.0;
    for (const auto& cue : tour.world->playlist.targets) {
        const double arrival = elapsed + tour_timing::effectiveCueDuration(cue.durationSeconds);
        const double end = arrival + cue.dwellSeconds;
        result.timeline.push_back({cue.presetId, cue.presetRevision, elapsed, arrival, end});
        elapsed = end;
    }
    return result;
}

} // namespace epi_sculpt
