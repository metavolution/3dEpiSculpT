#include "SculpturePresetLibrary.h"
#include "SculptureMorphSpace.h"
#include "SculptureTourComparison.h"
#include "SculpturePresetSearch.h"
#include "SculpturePresetFacets.h"
#include "SculpturePresetNeighborhoodQuery.h"
#include "SculpturePresetBridge.h"
#include "SculpturePresetBoundary.h"
#include "SculpturePresetJourney.h"
#include "SculpturePresetPath.h"
#include "SculpturePresetPathFrontier.h"
#include "SculpturePresetPathJson.h"
#include "SculpturePresetPathTour.h"
#include "SculpturePresetPathExportCheck.h"
#include "SculpturePresetPathAlternatives.h"
#include "SculpturePresetExport.h"
#include "SculpturePresetIdentityIndex.h"
#include "SculpturePresetInspectorOptions.h"
#include "SculptureResourcePaths.h"
#include "SculptureSerializer.h"
#include "SculptureModulationInspection.h"
#include "SculptureModulationRouteComparison.h"
#include "SculptureModulationRouteComparisonJson.h"
#include "SculptureModulationRouting.h"
#include "SculptureMutator.h"
#include "SculpturePresetMorph.h"
#include "SculpturePresetMorphStudy.h"
#include "SculpturePresetMorphStudyJson.h"
#include "SculpturePresetMorphStudyText.h"
#include "SculpturePresetMorphFocusText.h"
#include "SculpturePresetMorphFocusJson.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <exception>
#include <iostream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using epi_sculpt::kDefaultPresetNeighborLimit;
using epi_sculpt::kMaxPresetNeighborLimit;

using epi_sculpt::preset_inspector::InspectorMode;
using epi_sculpt::preset_inspector::InspectorParseAction;
using epi_sculpt::preset_inspector::kCommands;

void printUsage(std::string_view executable) {
    std::cerr << "Usage: " << executable
              << " [--root <preset-directory>] [--authored-only] [--selectable-only] [--category <name>] [--search <words> [--ranked] [--search-details]] [--facets]"
              << " [--neighbor-limit N] [--neighbor-min-distance D] [--neighbor-max-distance D] [--neighbor-details] [--bridge-details] [--journey-offset N] [";
    for (std::size_t i = 0; i < kCommands.size(); ++i) {
        if (i) std::cerr << " | ";
        std::cerr << kCommands[i].flag;
        if (kCommands[i].arity) std::cerr << ' ' << kCommands[i].operands;
    }
    std::cerr << "]\n"
              << "  --search WORDS  Match every word across IDs, titles, categories and descriptions\n"
              << "                  Use double quotes inside WORDS for a contiguous phrase\n"
              << "                  Prefix a word or phrase with - to exclude matches\n"
              << "                  Quote a leading - to search for it literally\n"
              << "                  Scope terms with id:, title:, category:, or description:\n"
              << "                  Example: title:\"Hopf torus\" -description:audio\n"
              << "                  Scoped terms are substrings; --category matches the whole category\n"
              << "                  ASCII case-insensitive; combine with --authored-only\n"
              << "  --category NAME  Match a complete category, ASCII case-insensitively\n"
              << "  --ranked       Order search matches by relevance; ties retain library order\n"
              << "  --search-details  Explain each distinct positive term's strongest field and score\n"
              << "                  Requires --search; unavailable with --facets\n"
              << "  --facets       Summarize matched presets by category instead of listing them\n"
              << "                  Category names group ASCII case-insensitively; ranking does not change counts\n"
              << "                  Names are quoted; control bytes use \\xNN escapes\n"
              << "  --selectable-only  Exclude empty or duplicate exact IDs in the entire loaded catalog\n"
              << "                  Checks identity only, not route or device readiness\n"
              << "  --neighbor-limit N  With --neighbors only: 1.." << kMaxPresetNeighborLimit
              << " matches (default " << kDefaultPresetNeighborLimit << ")\n"
              << "  --neighbor-details  Explain the largest distance contributions for selected neighbors\n"
              << "  --neighbor-min-distance D  With --neighbors: include distances at or above D\n"
              << "  --neighbor-max-distance D  With --neighbors: include distances at or below D\n"
              << "                  This is a spatial parameter metric, not visual or morph equivalence\n"
              << "  --bridge-details  With --bridges: explain the three largest parameter changes on each leg\n";
    std::cerr << "  --path-max-distance D  With --path: inclusive spatial step bound (default "
              << epi_sculpt::kDefaultPresetPathMaxDistance << ")\n"
              << "  --path-hops N  With --path: 1.." << epi_sculpt::kMaxPresetPathHops
              << " edges (default " << epi_sculpt::kDefaultPresetPathHops << ")\n"
              << "                  Fewest hops; ties follow catalog discovery, not least total distance\n";
    std::cerr << "  --path-details  With --path: explain each hop's three largest parameter changes\n";
    std::cerr << "  --path-json  With --path: emit one versioned read-only JSON document, also with\n"
              << "                  --path-frontier; IDs/revisions are hex bytes; cannot combine with export\n";
    std::cerr << "  --path-frontier  With --path: show hop/largest-jump trade-offs in one bounded search\n"
              << "                  Read-only unless --path-export-recommended is supplied; longer chains reduce the largest jump\n"
              << "                  Use a reported hop count (at least 1) with --path-objective gentlest to select/export it\n";
    std::cerr << "  --path-objective fewest-hops|gentlest  Default: fewest-hops; gentlest minimizes\n"
              << "                  the largest parameter step, then hops; ties use catalog scan order\n";
    std::cerr << "  --path-export FILE.epimode  Save a compatible compact path as a paused, nonlooping\n"
              << "                  library tour with a starting configuration; does not start playback\n"
              << "  --path-export-recommended FILE.epimode  With --path-frontier: save its gentlest exportable\n"
              << "                  reported route, using the same catalog snapshot; admission and file protection still apply\n"
              << "  --path-seconds S  With either path export or --path-check-export: duration 0.001..3600 (default 3)\n"
              << "  --path-dwell S  Hold each cue for 0..3600 seconds (default 1); 0 adds no dwell\n"
              << "  --path-easing NAME  Existing playback curve for every cue (default Smoothstep)\n"
              << "                  Both require path export or --path-check-export; references use the current catalog\n"
              << "                  Easing names:";
    for (std::size_t index = 0; index < epi_sculpt::kMorphEasingCount; ++index)
        std::cerr << ' ' << epi_sculpt::name(static_cast<epi_sculpt::MorphEasing>(index));
    std::cerr << '\n';
    std::cerr << "  --path-check-export  Check tour compatibility and encoding without writing a file\n"
              << "                  With --path-frontier: assess every reported route and recommend the gentlest ready one\n"
              << "                  Ready paths include an authored cue timeline and nominal total after resume\n"
              << "                  Supports --path-seconds/--path-dwell/--path-easing; destination/native readiness unassessed\n";
    std::cerr << "  --journey-offset N  With --journey only: start at absolute step N (default 0)\n";
    std::cerr << "  --mutation-seed N  With mutation preview/export: unsigned 32-bit seed (default 42)\n"
              << "  --mutation-strength S  With mutation preview/export: finite 0..1 amount (default 0.35)\n";
    std::cerr << "  --morph-position T  With morph preview/export: finite 0..1 progress (default 0.5)\n";
    std::cerr << "  --morph-samples N  With morph study/text, JSON or focus: 2..33 seed samples (default 9)\n";
    std::cerr << "  --morph-refinements N  With morph focus: 0..12 midpoint refinements (default 8)\n";
    for (const auto& command : kCommands) {
        std::cerr << "  " << command.flag;
        if (command.arity) std::cerr << ' ' << command.operands;
        std::cerr << "  " << command.description << '\n';
    }
    std::cerr << "Preset selections require exact, unique stable IDs.\n"
              << "Readiness describes loaded assets/routes, not native capability or measured performance.\n";
}

// Default terminal selection remains a CLI concern; byte quoting is shared.
void printFacetCategory(std::string_view category, std::ostream& output = std::cout) {
    epi_sculpt::quoteInspectionText(output, category);
}

void printMutationPreview(const epi_sculpt::SculpturePresetAsset& asset,
                          std::uint32_t seed, float strength,
                          const epi_sculpt::SculptureProgramParams& preview) {
    const auto precision = std::cout.precision();
    std::cout << std::setprecision(std::numeric_limits<float>::max_digits10)
              << "mutation-scope: spatial parameter candidate; not a rendered or simulated observation\n"
              << "mutation-effect: read-only; preset, live state and RNG sequence unchanged\n"
              << "mutation-preset: ";
    printFacetCategory(asset.id);
    std::cout << "\nmutation-revision: ";
    printFacetCategory(asset.revision);
    std::cout << "\nmutation-seed: " << seed << "\nmutation-strength: " << strength
              << "\nmutation-columns: parameter\tbefore\tafter\n";
    struct Field { std::string_view name; float epi_sculpt::SculptureProgramParams::* member; };
    constexpr std::array fields{
        Field{"worldScale", &epi_sculpt::SculptureProgramParams::worldScale},
        Field{"radiusScale", &epi_sculpt::SculptureProgramParams::radiusScale},
        Field{"chromaBreath", &epi_sculpt::SculptureProgramParams::chromaBreath},
        Field{"lumaDrift", &epi_sculpt::SculptureProgramParams::lumaDrift},
        Field{"hueRotation", &epi_sculpt::SculptureProgramParams::hueRotation},
        Field{"latentW", &epi_sculpt::SculptureProgramParams::latentW},
        Field{"topologyBlend", &epi_sculpt::SculptureProgramParams::topologyBlend}};
    std::size_t changed = 0;
    for (const auto& field : fields) {
        const float before = asset.params.*(field.member), after = preview.*(field.member);
        changed += before != after;
        std::cout << "mutation-parameter: " << field.name << '\t' << before << '\t' << after << '\n';
    }
    changed += asset.params.feedbackIterations != preview.feedbackIterations;
    std::cout << "mutation-parameter: feedbackIterations\t" << asset.params.feedbackIterations
              << '\t' << preview.feedbackIterations << "\nmutation-changed: " << changed << '\n';
    std::cout.precision(precision);
}

// CLI owns diagnostics; authoring preparation owns route/codec admission and
// the publisher owns destination/source admission and single-file IO.
bool publishConfiguration(const epi_sculpt::SculpturePresetLibrary& library,
                          const epi_sculpt::SculptureWorldMode& world,
                          const std::filesystem::path& output, std::string_view annotation,
                          std::string_view kind) {
    const auto result = epi_sculpt::publishPresetConfigurationDetailed(
        library.entries(), world, output, annotation, library.discoveredSourcePaths());
    if (!result.published()) {
        const auto name = kind == "mutation" ? "Mutation" : kind == "path" ? "Path" : "Morph";
        std::cerr << name << " export " << epi_sculpt::presetExportErrorMessage(result.error);
        if (result.error == epi_sculpt::PresetExportError::InvalidModulationRoute && result.routeDiagnostic) {
            const auto& diagnostic = *result.routeDiagnostic;
            std::cerr << "; route-index=" << diagnostic.routeIndex << " (zero-based); cause="
                      << epi_sculpt::modulation_inspection::name(diagnostic.error);
        }
        std::cerr << '\n';
        return false;
    }
    std::cout << kind << "-export: ";
    printFacetCategory(output.generic_string());
    std::cout << '\n' << kind << "-export-scope: " << (world.playlist.hasLibraryTargets
        ? "starting configuration and paused library-ID tour; fresh history, current-catalog references\n"
        : "authored configuration; fresh simulation history\n");
    return true;
}

// Domain eligibility stays with the author; shared preparation admits format
// capabilities/routes before publication can inspect the filesystem.
bool exportSpatialConfiguration(const epi_sculpt::SculpturePresetLibrary& library,
                    const epi_sculpt::SculpturePresetAsset& asset,
                    const epi_sculpt::SculptureProgramParams& params,
                    const std::filesystem::path& output, std::string_view annotation,
                    std::string_view kind) {
    const std::string_view name = kind == "mutation" ? "Mutation" : "Morph";
    if (!epi_sculpt::compactSpatialPreset(asset)) {
        std::cerr << name << " export supports compact spatial presets only\n";
        return false;
    }
    const auto mode = epi_sculpt::makeSpatialPresetConfiguration(asset, params);
    return publishConfiguration(library, mode, output, annotation, kind);
}

bool exportPresetPath(const epi_sculpt::SculpturePresetLibrary& library,
                       const epi_sculpt::PreparedPresetPath& path,
                       float seconds, epi_sculpt::PresetPathTourStyle style, const std::filesystem::path& output) {
    const auto prepared = epi_sculpt::preparePresetPathTour(library.entries(), path, seconds, style);
    if (!prepared.ready()) {
        std::cerr << "Path export unavailable: " << prepared.diagnostic << '\n';
        return false;
    }
    // All cues share the authored map. The common configuration publisher now
    // admits it through the installation parser for every export kind.
    std::ostringstream annotation;
    annotation << "# Authored preset path; current-catalog references, not a field checkpoint\n"
               << "# path_objective=" << epi_sculpt::presetPathObjectiveName(path.request.objective) << '\n';
    for (const auto& step : path.steps) {
        annotation << "# path_cue=";
        epi_sculpt::quoteInspectionText(annotation, step.id);
        annotation << " revision=";
        epi_sculpt::quoteInspectionText(annotation, step.revision);
        annotation << '\n';
    }
    return publishConfiguration(library, *prepared.world, output, annotation.str(), "path");
}

void printPathTimelineHeader(bool frontier) {
    const auto prefix = frontier ? "path-frontier-export-timeline" : "path-export-timeline";
    std::cout << prefix << "-scope: authored seconds after resume; includes initial transition and final dwell; frame quantization unassessed\n"
              << prefix << "-columns: " << (frontier ? "route-index\t" : "")
              << "cue-index\tpreset-id\trevision\ttransition-start\tarrival\tdwell-end\n";
}

void printPathTimeline(const epi_sculpt::PresetPathExportCheck& checked,
                       std::optional<std::size_t> route = std::nullopt) {
    if (!checked.ready()) return;
    const auto prefix = route ? "path-frontier-export-timeline" : "path-export-timeline";
    const auto precision = std::cout.precision();
    std::cout << std::setprecision(std::numeric_limits<double>::max_digits10)
              << prefix << "-total: ";
    if (route) std::cout << *route << '\t';
    std::cout << checked.nominalDurationSeconds() << '\n';
    for (std::size_t index = 0; index < checked.timeline.size(); ++index) {
        const auto& cue = checked.timeline[index];
        std::cout << prefix << "-cue: ";
        if (route) std::cout << *route << '\t';
        std::cout << index << '\t';
        epi_sculpt::quoteInspectionText(std::cout, cue.presetId);
        std::cout << '\t';
        epi_sculpt::quoteInspectionText(std::cout, cue.presetRevision);
        std::cout << '\t' << cue.transitionStartSeconds << '\t' << cue.arrivalSeconds
                  << '\t' << cue.dwellEndSeconds << '\n';
    }
    std::cout.precision(precision);
}

bool printPathExportCheck(std::span<const epi_sculpt::SculpturePresetAsset> assets,
                          const epi_sculpt::PreparedPresetPath& path, float seconds,
                          epi_sculpt::PresetPathTourStyle style) {
    const auto checked = epi_sculpt::checkPresetPathExport(assets, path, seconds, style);
    std::cout << "path-export-check: " << (checked.ready() ? "ready" : "refused")
              << "\npath-export-check-scope: configuration only; destination/native capability and field replay unassessed"
              << "\npath-export-check-cues: " << checked.cueCount
              << "\npath-export-check-seconds: " << seconds
              << "\npath-export-check-dwell: " << style.dwellSeconds
              << "\npath-export-check-easing: " << epi_sculpt::name(style.easing) << '\n';
    if (!checked.ready()) {
        std::cout << "path-export-check-reason: "
                  << (checked.error == epi_sculpt::PresetPathExportCheckError::TourRejected
                      ? checked.tourDiagnostic : epi_sculpt::presetConfigurationErrorName(checked.configurationError)) << '\n';
        if (checked.routeDiagnostic)
            std::cout << "path-export-check-route-index: " << checked.routeDiagnostic->routeIndex
                      << "\npath-export-check-route-cause: "
                      << epi_sculpt::modulation_inspection::name(checked.routeDiagnostic->error) << '\n';
    } else {
        printPathTimelineHeader(false);
        printPathTimeline(checked);
    }
    return checked.ready();
}

bool exportMutation(const epi_sculpt::SculpturePresetLibrary& library,
                    const epi_sculpt::SculpturePresetAsset& asset,
                    const epi_sculpt::SculptureProgramParams& params,
                    std::uint32_t seed, float strength, const std::filesystem::path& output) {
    std::ostringstream annotation;
    annotation << std::setprecision(std::numeric_limits<float>::max_digits10)
               << "# Seeded spatial mutation; configuration only, fresh simulation history\n"
               << "# mutation_seed=" << seed << "\n# mutation_strength=" << strength << '\n';
    return exportSpatialConfiguration(library, asset, params, output, annotation.str(), "mutation");
}

bool reportMorphFailure(
    epi_sculpt::PresetMorphError error, std::string_view operation,
    epi_sculpt::PresetMorphCompatibility compatibility = epi_sculpt::PresetMorphCompatibility::Ready) {
    using Error = epi_sculpt::PresetMorphError;
    using Compatibility = epi_sculpt::PresetMorphCompatibility;
    std::cerr << "Morph " << operation << ' ';
    switch (error) {
    case Error::InvalidPosition: std::cerr << "requires finite progress from 0 to 1"; break;
    case Error::UnsupportedEndpoint: std::cerr << "supports compact spatial presets only"; break;
    case Error::AuthoredAutomation:
        std::cerr << "cannot preserve authored automation in the current WorldMode format"; break;
    case Error::IncompatibleConfiguration:
        std::cerr << "requires matching selectors, world recipes and routing maps";
        if (compatibility == Compatibility::SelectorMismatch
            || compatibility == Compatibility::WorldRecipeMismatch
            || compatibility == Compatibility::RoutingMapMismatch) {
            std::cerr << " (" << epi_sculpt::presetMorphCompatibilityCode(compatibility) << ')';
        }
        break;
    case Error::NonfiniteEndpoint: std::cerr << "endpoints contain nonfinite spatial parameters"; break;
    case Error::NonfiniteCandidate: std::cerr << "interpolation produced nonfinite spatial parameters"; break;
    case Error::None: std::cerr << "candidate unavailable"; break;
    }
    std::cerr << '\n';
    return false;
}

bool printMorphPreview(const epi_sculpt::SculpturePresetAsset& from,
                       const epi_sculpt::SculpturePresetAsset& to, float position) {
    const auto prepared = epi_sculpt::preparePresetMorph(from, to, position);
    if (!prepared.ready()) return reportMorphFailure(prepared.error, "preview", prepared.compatibility);
    const auto first = epi_sculpt::inspectSpatialPresetDistance(from.params, *prepared.params);
    const auto second = epi_sculpt::inspectSpatialPresetDistance(*prepared.params, to.params);
    if (!first || !second) return reportMorphFailure(epi_sculpt::PresetMorphError::NonfiniteCandidate, "preview");

    const auto precision = std::cout.precision();
    std::cout << "morph-scope: compact spatial parameter candidate; not a rendered observation\n"
              << "morph-effect: read-only; no files or runtime state changed\n"
              << "morph-from: ";
    printFacetCategory(from.id);
    std::cout << "\nmorph-from-revision: ";
    printFacetCategory(from.revision);
    std::cout << "\nmorph-to: ";
    printFacetCategory(to.id);
    std::cout << "\nmorph-to-revision: ";
    printFacetCategory(to.revision);
    std::cout << std::setprecision(std::numeric_limits<float>::max_digits10)
              << "\nmorph-position: " << position << "\nmorph-state: " << (position == 1.0f ? "to" : "from")
              << "\nmorph-metric: normalized spatial parameter distance; not visual similarity\n"
              << "morph-leg-columns: leg\tdistance\tcomponent-count\tchanged-count\tshown-count\n"
              << "morph-change-columns: leg\tparameter\tbefore\tafter\tdistance-contribution\n"
              << std::setprecision(std::numeric_limits<double>::max_digits10);
    for (const auto& [leg, report] : std::array<std::pair<std::string_view, const epi_sculpt::SpatialPresetDistanceReport*>, 2>{{
            {"from", &*first}, {"to", &*second}}}) {
        std::cout << "morph-leg: " << leg << '\t' << report->distance << '\t' << report->componentCount
                  << '\t' << report->changedCount << '\t' << report->differenceCount << '\n';
        for (std::size_t i = 0; i < report->differenceCount; ++i) {
            const auto& delta = report->differences[i];
            std::cout << "morph-change: " << leg << '\t' << delta.parameter;
            if (delta.component) std::cout << '[' << *delta.component << ']';
            std::cout << '\t' << delta.from << '\t' << delta.to << '\t' << delta.contribution << '\n';
        }
    }
    std::cout.precision(precision);
    return true;
}

bool printMorphStudy(const epi_sculpt::SculpturePresetAsset& from,
                     const epi_sculpt::SculpturePresetAsset& to, std::size_t samples, bool json = false) {
    epi_sculpt::PreparedPresetMorphStudy prepared;
    try {
        prepared = epi_sculpt::preparePresetMorphStudy(from, to, samples);
    } catch (const std::exception& error) {
        std::cerr << "Morph study preparation failed: " << error.what() << '\n';
        return false;
    }
    if (!prepared.ready()) {
        if (prepared.error == epi_sculpt::PresetMorphStudyError::MorphRejected) {
            std::cerr << "Morph study rejected sample " << prepared.failedSample << ":\n";
            return reportMorphFailure(prepared.morphError, "study", prepared.compatibility);
        }
        std::cerr << "Morph study unavailable: "
                  << (prepared.error == epi_sculpt::PresetMorphStudyError::InvalidSampleCount
                      ? "sample count outside 2..33" : "sample distance unavailable") << '\n';
        return false;
    }
    const auto document = json
        ? epi_sculpt::serializePresetMorphStudyJson(from, to, prepared)
        : epi_sculpt::serializePresetMorphStudyText(from, to, prepared);
    if (!document) {
        std::cerr << "Morph study " << (json ? "JSON " : "")
                  << "unavailable: invalid prepared report\n";
        return false;
    }
    std::cout << *document;
    return true;
}

bool printMorphFocus(const epi_sculpt::SculpturePresetAsset& from,
                     const epi_sculpt::SculpturePresetAsset& to, std::size_t samples, std::size_t refinements,
                     bool json = false) {
    // Prepare and format privately; refusal cannot publish a partial trace.
    const auto prepared = epi_sculpt::preparePresetMorphFocus(from, to, samples, refinements);
    if (!prepared.ready()) {
        std::cerr << "Morph focus unavailable: ";
        if (prepared.error == epi_sculpt::PresetMorphFocusError::InvalidRefinements)
            std::cerr << "refinements outside 0..12\n";
        else {
            std::cerr << "path preparation or metric rejected\n";
            if (prepared.morphError != epi_sculpt::PresetMorphError::None)
                return reportMorphFailure(prepared.morphError, "focus", prepared.compatibility);
        }
        return false;
    }
    const auto document = json ? epi_sculpt::serializePresetMorphFocusJson(from, to, prepared)
                               : epi_sculpt::serializePresetMorphFocusText(from, to, prepared);
    if (!document) {
        std::cerr << "Morph focus unavailable: invalid prepared report\n";
        return false;
    }
    std::cout << *document;
    return true;
}

bool exportMorph(const epi_sculpt::SculpturePresetLibrary& library,
                 const epi_sculpt::SculpturePresetAsset& from,
                 const epi_sculpt::SculpturePresetAsset& to,
                 float position, const std::filesystem::path& output) {
    const auto prepared = epi_sculpt::preparePresetMorph(from, to, position);
    if (!prepared.ready()) return reportMorphFailure(prepared.error, "export", prepared.compatibility);
    std::ostringstream annotation;
    annotation << std::setprecision(std::numeric_limits<float>::max_digits10)
               << "# Spatial morph snapshot; configuration only, fresh simulation history\n";
    for (const auto& [name, text] : std::array<std::pair<std::string_view, std::string_view>, 4>{{
            {"from", from.id}, {"from_revision", from.revision},
            {"to", to.id}, {"to_revision", to.revision}}}) {
        annotation << "# morph_" << name << '=';
        printFacetCategory(text, annotation);
        annotation << '\n';
    }
    annotation << "# morph_position=" << position << "\n# morph_state="
               << (position == 1.0f ? "to" : "from") << '\n';
    return exportSpatialConfiguration(library, position == 1.0f ? to : from, *prepared.params,
                                      output, annotation.str(), "morph");
}

std::string_view entryStatus(epi_sculpt::SculptureTourEntryStatus status) {
    switch (status) {
    case epi_sculpt::SculptureTourEntryStatus::Ready: return "ready";
    case epi_sculpt::SculptureTourEntryStatus::MissingAsset: return "missing-asset";
    case epi_sculpt::SculptureTourEntryStatus::UnsupportedRoute: return "unsupported-route";
    case epi_sculpt::SculptureTourEntryStatus::AmbiguousAsset: return "ambiguous-asset";
    }
    return "unknown";
}

void printTour(std::string_view name, const epi_sculpt::SculptureTourInspection& tour,
               bool detailed) {
    const auto ready = tour.readyCount();
    std::cout << "tour: " << name << "\trecognized: " << (tour.recognized ? "yes" : "no")
              << "\tready: " << ready << "\trequested: " << tour.entries.size()
              << "\tomitted: " << tour.entries.size() - ready << '\n';
    if (detailed) {
        for (const auto& entry : tour.entries) {
            std::cout << entryStatus(entry.status) << '\t' << entry.presetId
                      << '\t' << entry.title << '\n';
        }
    }
}

std::size_t printMorphBanks(const epi_sculpt::SculpturePresetLibrary& library) {
    // Use the controller's actual definitions; category banks belong to the live
    // application's binding policy and are deliberately outside this read model.
    epi_sculpt::SculptureMorphSpace definitions;
    std::cout << "morph-bank-scope: predefined banks; excludes live category banks and custom nodes\n"
              << "morph-bank-policy: exact unique preset IDs; catalog availability, not morph or native readiness\n"
              << "morph-bank-count: " << definitions.bankCount() << '\n'
              << "morph-bank-columns: bank-id\ttitle\tresolved\trequested\n"
              << "morph-corner-columns: bank-id\tcorner\tstatus\tpreset-id\ttitle\trevision\n";
    for (std::size_t index = 0; index < definitions.bankCount(); ++index) {
        definitions.loadBank(index);
        const auto& bank = *definitions.activeBank();
        std::array<epi_sculpt::SculpturePresetIdResolution, 4> corners;
        std::size_t resolved = 0;
        for (std::size_t corner = 0; corner < corners.size(); ++corner) {
            corners[corner] = library.resolveExactId(bank.cornerPresetNames[corner]);
            if (corners[corner].asset) ++resolved;
        }
        std::cout << "morph-bank: " << bank.id << '\t' << bank.title << '\t'
                  << resolved << '\t' << corners.size() << '\n';
        for (std::size_t corner = 0; corner < corners.size(); ++corner) {
            const auto& result = corners[corner];
            const auto status = result.status == epi_sculpt::SculpturePresetIdStatus::Found ? "found"
                : result.status == epi_sculpt::SculpturePresetIdStatus::Ambiguous ? "ambiguous" : "missing";
            std::cout << "morph-corner: " << bank.id << '\t'
                      << epi_sculpt::cornerName(static_cast<epi_sculpt::MorphSpaceCorner>(corner))
                      << '\t' << status << '\t' << bank.cornerPresetNames[corner] << '\t'
                      << (result.asset ? result.asset->title : "") << '\t'
                      << (result.asset ? result.asset->revision : "") << '\n';
        }
    }
    return definitions.bankCount();
}

void printTourComparison(std::string_view from, std::string_view to,
                         const epi_sculpt::TourCueComparison& comparison) {
    std::cout << "comparison-scope: tour references and cue positions; not morph compatibility\n"
              << "from-tour: " << from << '\n'
              << "to-tour: " << to << '\n'
              << "from-requested: " << comparison.fromCount << '\n'
              << "to-requested: " << comparison.toCount << '\n'
              << "from-ready: " << comparison.fromReady << '\n'
              << "to-ready: " << comparison.toReady << '\n'
              << "shared-references: " << comparison.shared << '\n'
              << "from-only-references: " << comparison.fromOnly << '\n'
              << "to-only-references: " << comparison.toOnly << '\n'
              << "pairing-policy: exact nonempty ID; repeated references pair in occurrence order\n"
              << "cue-order: source order, then unmatched target order; indices are zero-based\n"
              << "tour-cue-columns: relation\tid\tfrom-index\tto-index\tfrom-status\tto-status\n";
    const auto printIndex = [](const std::optional<std::size_t>& index) {
        if (index) std::cout << *index;
        else std::cout << '-';
    };
    const auto printStatus = [](const std::optional<epi_sculpt::SculptureTourEntryStatus>& status) {
        std::cout << (status ? entryStatus(*status) : "-");
    };
    for (const auto& cue : comparison.cues) {
        const auto relation = cue.fromIndex ? (cue.toIndex ? "shared" : "from-only") : "to-only";
        std::cout << "tour-cue: " << relation << '\t' << cue.presetId << '\t';
        printIndex(cue.fromIndex);
        std::cout << '\t';
        printIndex(cue.toIndex);
        std::cout << '\t';
        printStatus(cue.fromStatus);
        std::cout << '\t';
        printStatus(cue.toStatus);
        std::cout << '\n';
    }
}

void printJourney(std::string_view name, const epi_sculpt::PresetJourneyReport& journey) {
    std::cout << "journey: " << name << '\n'
              << "journey-scope: requested tour adjacency; spatial study, not live morph readiness or canonical fields\n"
              << "journey-policy: preserve repeats and unavailable cues; no shortcuts or loop closure\n"
              << "journey-cue-count: " << journey.cueCount << '\n'
              << "journey-total-steps: " << journey.totalSteps << '\n'
              << "journey-step-limit: " << epi_sculpt::kMaxPresetJourneySteps << '\n'
              << "journey-shown-steps: " << journey.steps.size() << '\n'
              << "journey-first-step: " << journey.firstStep << '\n'
              << "journey-truncated: " << (journey.truncated() ? "yes" : "no") << '\n'
              << "journey-has-more: " << (journey.hasMore() ? "yes" : "no") << '\n'
              << "journey-next-offset: ";
    if (const auto next = journey.nextOffset()) std::cout << *next;
    else std::cout << '-';
    std::cout << '\n'
              << "journey-step-columns: from-index\tto-index\tfrom-id\tto-id\tfrom-status\tto-status\tkind\tdistance\tfrom-revision\tto-revision\n"
              << "journey-change-columns: from-index\tparameter\tfrom\tto\tdistance-contribution\n"
              << "journey-boundary-columns: from-index\tdomain\tfield\tfrom\tto\n"
              << "journey-boundary-scope: declared route/selector differences; not live transition admission\n"
              << "journey-metric: mean(abs(a-b)/(1+max(abs(a),abs(b)))); largest 3 nonzero contributions\n"
              << std::setprecision(std::numeric_limits<double>::max_digits10);
    for (const auto& step : journey.steps) {
        std::cout << "journey-step: " << step.fromIndex << '\t' << step.toIndex << '\t'
                  << step.fromId << '\t' << step.toId << '\t'
                  << entryStatus(step.fromStatus) << '\t' << entryStatus(step.toStatus) << '\t'
                  << epi_sculpt::presetJourneyStepName(step.kind) << '\t';
        if (step.spatial) std::cout << step.spatial->distance;
        else std::cout << '-';
        std::cout << '\t' << (step.fromRevision.empty() ? "unknown" : step.fromRevision)
                  << '\t' << (step.toRevision.empty() ? "unknown" : step.toRevision) << '\n';
        for (const auto& difference : step.boundaries) {
            std::cout << "journey-boundary: " << step.fromIndex << '\t'
                      << epi_sculpt::presetBoundaryDomainName(difference.domain)
                      << '\t' << difference.field << '\t'
                      << epi_sculpt::formatPresetBoundaryValue(difference, difference.from)
                      << '\t' << epi_sculpt::formatPresetBoundaryValue(difference, difference.to) << '\n';
        }
        if (!step.spatial) continue;
        for (std::size_t i = 0; i < step.spatial->differenceCount; ++i) {
            const auto& difference = step.spatial->differences[i];
            std::cout << "journey-change: " << step.fromIndex << '\t' << difference.parameter;
            if (difference.component) std::cout << '[' << *difference.component << ']';
            std::cout << '\t' << difference.from << '\t' << difference.to
                      << '\t' << difference.contribution << '\n';
        }
    }
}

const epi_sculpt::SculpturePresetAsset* resolveUniquePreset(
    const epi_sculpt::SculpturePresetLibrary& library, std::string_view id) {
    const auto resolution = library.resolveExactId(id);
    if (resolution.status == epi_sculpt::SculpturePresetIdStatus::Ambiguous)
        std::cerr << "Ambiguous preset ID: " << id << '\n';
    else if (resolution.status == epi_sculpt::SculpturePresetIdStatus::Missing)
        std::cerr << "Preset ID not found: " << id << '\n';
    return resolution.asset;
}

void printAffine(std::string_view label, const epi_sculpt::ComplexAffine& affine) {
    std::cout << label << ": scale=(" << affine.scaleReal << ", " << affine.scaleImag
              << ") offset=(" << affine.offsetReal << ", " << affine.offsetImag << ")\n";
}

struct RouteInspectionOutcome {
    bool parsed = false;
    bool responseReady = false;
    [[nodiscard]] bool accepted() const noexcept { return parsed && responseReady; }
};

RouteInspectionOutcome printRoute(std::string_view text, bool showResponse = false) {
    const auto result = epi_sculpt::SculptureModulationEngine::inspectRouteResult(text);
    const auto& route = result.route;
    std::cout << "route: " << text << '\n'
              << "route-status: " << (route ? "accepted" : "rejected") << '\n'
              << "inspection-scope: parsed route text; not installed state or live source values\n";
    if (!route) {
        std::cout << "route-error: " << epi_sculpt::modulation_inspection::name(result.error) << '\n';
        return {};
    }
    using epi_sculpt::modulation_inspection::name;
    std::cout << std::setprecision(std::numeric_limits<float>::max_digits10)
              << "route-source: " << name(route->sourceId) << '\n'
              << "route-target: " << name(route->targetId) << '\n'
              << "route-curve: " << name(route->transferCurve) << '\n'
              << "route-scale: " << route->scale << '\n'
              << "route-bias: " << route->bias << '\n'
              << "route-slew-seconds: " << route->slewTimeSeconds << '\n'
              << "route-formula: contribution = curve(source) * scale + bias\n"
              << "route-smoothing: " << (route->slewTimeSeconds > 0.0f
                  ? "exponential approach with the declared time constant"
                  : "immediate") << '\n';
    if (showResponse) {
        // Reuse the shared deterministic bipolar probe evaluator so single-route
        // CLI inspection and target fan-in analysis share identical math.
        const auto probes = epi_sculpt::modulation_inspection::evaluateRouteProbes(*route);
        if (!probes.ready()) {
            std::cout << "response-status: rejected\n"
                      << "response-error: nonfinite-contribution\n"
                      << "response-error-input: " << *probes.failedInput << '\n';
            return {true, false};
        }
        std::cout << "response-status: accepted\n"
                  << "response-scope: independent bipolar probes; before smoothing, target fanout and clamping\n"
                  << "response-slew: not simulated\n"
                  << "response-count: " << probes.samples.size() << '\n'
                  << "response-columns: input\tshaped\tcontribution\n";
        for (const auto& sample : probes.samples)
            std::cout << "response: " << sample.input << '\t' << sample.shaped << '\t' << sample.contribution << '\n';
    }
    return {true, true};
}

bool printPresetRoutes(const epi_sculpt::SculpturePresetAsset& asset, bool showResponse = false) {
    const bool useDefaults = asset.routingMap.empty();
    const auto defaults = epi_sculpt::modulation_routing::defaults(false);
    const auto count = useDefaults ? defaults.size() : asset.routingMap.size();
    std::cout << "preset: " << asset.id << '\n'
              << "revision: " << (asset.revision.empty() ? "unknown" : asset.revision) << '\n'
              << "route-map-origin: " << epi_sculpt::modulation_routing::originName(useDefaults, false) << '\n'
              << "route-map-policy: flagship audio-off defaults; every member must parse\n"
              << "route-map-count: " << count << '\n';
    if (showResponse)
        std::cout << "response-map-scope: independent per-route probes; no combined target or installed state\n";
    bool parsed = true;
    bool responsesReady = true;
    for (std::size_t index = 0; index < count; ++index) {
        const std::string_view text = useDefaults ? defaults[index] : std::string_view{asset.routingMap[index]};
        std::cout << "route-index: " << index << '\n';
        // Always inspect every member, even after rejection. The application
        // alone owns installation/fallback; this command explains the request.
        const auto outcome = printRoute(text, showResponse);
        parsed = outcome.parsed && parsed;
        responsesReady = outcome.accepted() && responsesReady;
    }
    std::cout << "route-map-status: " << (parsed ? "accepted" : "rejected") << '\n';
    if (showResponse)
        std::cout << "response-map-status: " << (responsesReady ? "accepted" : "rejected") << '\n';
    return parsed && responsesReady;
}

void printExplanation(const epi_sculpt::SculpturePresetAsset& asset) {
    const auto& p = asset.params;
    std::cout << "preset: " << asset.id << '\n'
              << "title: " << asset.title << '\n'
              << "category: " << asset.category << '\n'
              << "description: " << asset.description << '\n'
              << "revision: " << (asset.revision.empty() ? "unknown" : asset.revision) << '\n';
    if (!asset.sourcePath.empty()) std::cout << "source: " << asset.sourcePath.generic_string() << '\n';
    // This view describes the stored spatial projection. Retained canonical input
    // is a distinct value, never reconstructed from these projected selectors.
    std::cout << "program-scope: decoded spatial program\n"
              << "canonical-feedback-input: " << (asset.canonicalFeedbackMode ? "retained" : "absent") << '\n'
              << "authored-automation: " << (asset.macroAutomationJson.empty() ? "absent" : "declared; checked on explicit selection") << '\n'
              << "presentation: " << epi_sculpt::presentationOwnershipName(asset.presentationOwnership) << '\n';
    if (epi_sculpt::includesCompactInstances(asset.presentationOwnership)
        || asset.presentationOwnership == epi_sculpt::SculpturePresentationOwnership::DedicatedWorld) {
        std::cout << "world-generator: " << epi_sculpt::worldGeneratorName(asset.worldRecipe.generator) << '\n'
                  << "world-primitive: " << epi_sculpt::worldPrimitiveName(asset.worldRecipe.primitive) << '\n';
    }
    if (asset.presentationOwnership == epi_sculpt::SculpturePresentationOwnership::CompactInstances) {
        std::cout << "interpretation: artistic compact sampling of the authored mathematical program\n";
    }
    printAffine("pre-affine", p.pre);
    std::cout << "inner-a: " << epi_sculpt::name(p.inner) << '\n'
              << "inner-b: " << epi_sculpt::name(p.innerB) << '\n'
              << "topology-blend: " << p.topologyBlend << '\n';
    printAffine("post-affine", p.post);
    std::cout << "reduction-a: " << epi_sculpt::name(p.reduction) << '\n'
              << "reduction-b: " << epi_sculpt::name(p.reductionB) << '\n'
              << "seed-a: " << epi_sculpt::name(p.seedType) << '\n'
              << "seed-b: " << epi_sculpt::name(p.seedTypeB) << '\n'
              << "color: " << epi_sculpt::name(p.colorEffect) << '\n'
              << "output-mode-a: " << p.outputMode << '\n'
              << "output-mode-b: " << p.outputModeB << '\n'
              << "feedback-iterations: " << p.feedbackIterations << '\n';
}

void printComparison(const epi_sculpt::SculpturePresetAsset& from,
                     const epi_sculpt::SculpturePresetAsset& to) {
    constexpr float kParameterEpsilon = 1.0e-5f;
    // Reuse the codec's semantic field registry. This compares spatial params,
    // not complete canonical modes, retained fields or native route compatibility.
    const auto diff = epi_sculpt::SculptureSerializer::diff(from.params, to.params, kParameterEpsilon);
    const auto boundaries = epi_sculpt::inspectPresetBoundaries(from, to);
    const auto recipeParams = epi_sculpt::inspectWorldRecipeParameterDifferences(from.worldRecipe, to.worldRecipe);
    const auto routeCount = static_cast<std::size_t>(std::count_if(boundaries.begin(), boundaries.end(),
        [](const auto& difference) { return difference.domain == epi_sculpt::PresetBoundaryDomain::Route; }));
    const auto morphCompatibility = epi_sculpt::assessPresetMorphCompatibility(from, to);
    const auto routeComparison = epi_sculpt::compareModulationRouteMaps(from.routingMap, to.routingMap, false);
    const bool routesAccepted = routeComparison.fromAccepted && routeComparison.toAccepted;
    std::cout << "comparison-scope: decoded spatial parameters\n"
              << "from: " << from.id << '\n'
              << "to: " << to.id << '\n'
              << "from-revision: " << (from.revision.empty() ? "unknown" : from.revision) << '\n'
              << "to-revision: " << (to.revision.empty() ? "unknown" : to.revision) << '\n'
              << "from-presentation: " << epi_sculpt::presentationOwnershipName(from.presentationOwnership) << '\n'
              << "to-presentation: " << epi_sculpt::presentationOwnershipName(to.presentationOwnership) << '\n'
              << "from-canonical-input: " << (from.canonicalFeedbackMode ? "retained" : "absent") << '\n'
              << "to-canonical-input: " << (to.canonicalFeedbackMode ? "retained" : "absent") << '\n'
              << "from-authored-automation: " << (from.macroAutomationJson.empty() ? "absent" : "declared") << '\n'
              << "to-authored-automation: " << (to.macroAutomationJson.empty() ? "absent" : "declared") << '\n'
              << "world-recipe-compatibility: " << (from.worldRecipe == to.worldRecipe ? "same" : "different") << '\n'
              << "comparison-recipe-parameter-count: " << recipeParams.size() << '\n'
              << "routing-map-compatibility: " << (from.routingMap == to.routingMap ? "same" : "different") << '\n'
              << "semantic-routing-compatibility: "
              << (!routesAccepted ? "rejected" : (routeComparison.semanticSame ? "same" : "different")) << '\n'
              << "compact-morph-eligibility: " << epi_sculpt::presetMorphCompatibilityCode(morphCompatibility) << '\n'
              << "parameter-epsilon: " << kParameterEpsilon << '\n'
              << "changed-parameters: " << diff.size() << '\n'
              << "comparison-boundary-scope: declared route/selector fields; excludes canonical math, retained fields and live transition admission\n"
              << "comparison-boundary-count: " << boundaries.size() << '\n'
              << "comparison-route-boundary-count: " << routeCount << '\n'
              << "comparison-selector-boundary-count: " << boundaries.size() - routeCount << '\n'
              << "comparison-boundary-columns: domain\tfield\tfrom\tto\n"
              << "change-columns: parameter\tfrom\tto\n";
    for (const auto& delta : diff.deltas) {
        std::cout << "change: " << delta.key << '\t' << delta.oldValue << '\t' << delta.newValue << '\n';
    }
    for (const auto& difference : boundaries) {
        std::cout << "comparison-boundary: "
                  << epi_sculpt::presetBoundaryDomainName(difference.domain)
                  << '\t' << difference.field << '\t'
                  << epi_sculpt::formatPresetBoundaryValue(difference, difference.from)
                  << '\t' << epi_sculpt::formatPresetBoundaryValue(difference, difference.to) << '\n';
    }
    for (const auto& recipeDiff : recipeParams) {
        std::cout << "comparison-recipe-parameter: " << recipeDiff.field << '\t'
                  << recipeDiff.from << '\t' << recipeDiff.to << '\n';
    }
}

bool printRouteComparison(const epi_sculpt::SculpturePresetAsset& from,
                          const epi_sculpt::SculpturePresetAsset& to) {
    const auto comparison = epi_sculpt::compareModulationRouteMaps(from.routingMap, to.routingMap, false);
    const bool bothAccepted = comparison.fromAccepted && comparison.toAccepted;
    const auto precision = std::cout.precision();
    std::cout << std::setprecision(std::numeric_limits<float>::max_digits10)
              << "route-comparison-scope: resolved modulation route maps; semantic equivalence and target fan-in\n"
              << "route-comparison-policy: flagship audio-off defaults when empty; ignores transient smoothing history\n"
              << "from: " << from.id << '\n'
              << "to: " << to.id << '\n'
              << "from-revision: " << (from.revision.empty() ? "unknown" : from.revision) << '\n'
              << "to-revision: " << (to.revision.empty() ? "unknown" : to.revision) << '\n'
              << "from-origin: " << comparison.fromOrigin << '\n'
              << "to-origin: " << comparison.toOrigin << '\n'
              << "from-count: " << comparison.fromCount << '\n'
              << "to-count: " << comparison.toCount << '\n'
              << "from-status: " << (comparison.fromAccepted ? "accepted" : "rejected") << '\n'
              << "to-status: " << (comparison.toAccepted ? "accepted" : "rejected") << '\n'
              << "authored-routing-compatibility: " << (comparison.authoredTextSame ? "same" : "different") << '\n'
              << "resolved-routing-compatibility: " << (comparison.resolvedTextSame ? "same" : "different") << '\n'
              << "semantic-routing-compatibility: "
              << (!bothAccepted ? "rejected" : (comparison.semanticSame ? "same" : "different")) << '\n'
              << "shared-routes: " << comparison.sharedCount << '\n'
              << "modified-routes: " << comparison.modifiedCount << '\n'
              << "from-only-routes: " << comparison.fromOnlyCount << '\n'
              << "to-only-routes: " << comparison.toOnlyCount << '\n'
              << "rejected-routes: " << comparison.rejectedCount << '\n'
              << "from-active-targets: " << comparison.fromFanIn.targets.size() << '\n'
              << "to-active-targets: " << comparison.toFanIn.targets.size() << '\n'
              << "from-superposed-targets: " << comparison.fromFanIn.superposedTargetCount << '\n'
              << "to-superposed-targets: " << comparison.toFanIn.superposedTargetCount << '\n'
              << "route-cue-columns: relation\tfrom-index\tto-index\tsource\ttarget\tfrom-route\tto-route\n"
              << "target-fan-in-columns: side\ttarget\troute-count\tmin-contribution-sum\tmax-contribution-sum\n";
    const auto printIndex = [](const std::optional<std::size_t>& index) {
        if (index) std::cout << *index;
        else std::cout << '-';
    };
    using epi_sculpt::modulation_inspection::name;
    for (const auto& cue : comparison.cues) {
        std::cout << "route-cue: " << epi_sculpt::modulationRouteCueRelationName(cue.relation) << '\t';
        printIndex(cue.fromIndex);
        std::cout << '\t';
        printIndex(cue.toIndex);
        std::cout << '\t' << name(cue.sourceId) << '\t' << name(cue.targetId)
                  << '\t' << cue.fromText << '\t' << cue.toText << '\n';
    }
    const auto printFanIn = [](std::string_view side, const epi_sculpt::ModulationTargetFanInSummary& fanIn) {
        for (const auto& entry : fanIn.targets) {
            std::cout << "target-fan-in: " << side << '\t' << name(entry.targetId)
                      << '\t' << entry.routeCount << '\t' << entry.minContributionSum
                      << '\t' << entry.maxContributionSum << '\n';
        }
    };
    printFanIn("from", comparison.fromFanIn);
    printFanIn("to", comparison.toFanIn);
    std::cout.precision(precision);
    return bothAccepted;
}

bool printRouteComparisonJson(const epi_sculpt::SculpturePresetAsset& from,
                              const epi_sculpt::SculpturePresetAsset& to) {
    const auto comparison = epi_sculpt::compareModulationRouteMaps(from.routingMap, to.routingMap, false);
    const auto document = epi_sculpt::serializeModulationRouteComparisonJson(
        from.id, from.revision, from.routingMap, to.id, to.revision, to.routingMap, comparison, false);
    if (!document) {
        std::cerr << "Route comparison JSON unavailable: invalid prepared report\n";
        return false;
    }
    std::cout << *document;
    return comparison.fromAccepted && comparison.toAccepted;
}

std::optional<std::size_t> printBridges(
    const epi_sculpt::SculpturePresetAsset& from, const epi_sculpt::SculpturePresetAsset& to,
    const epi_sculpt::SculpturePresetLibrary& library, bool details) {
    const auto report = epi_sculpt::preparePresetBridges(from, to, library.entries(), details);
    if (!report.ready()) {
        std::cerr << "Cannot suggest spatial bridges: ";
        switch (report.error) {
        case epi_sculpt::PresetBridgeError::InvalidEndpoint: std::cerr << "empty endpoint ID"; break;
        case epi_sculpt::PresetBridgeError::CanonicalEndpoint: std::cerr << "retained canonical endpoint"; break;
        case epi_sculpt::PresetBridgeError::NonfiniteEndpoint: std::cerr << "nonfinite endpoint parameters"; break;
        case epi_sculpt::PresetBridgeError::RouteMismatch: std::cerr << "endpoint routes differ"; break;
        case epi_sculpt::PresetBridgeError::SelectorMismatch: std::cerr << "endpoint selectors/discrete controls differ"; break;
        case epi_sculpt::PresetBridgeError::UnexplainableBridge: std::cerr << "selected bridge cannot be explained"; break;
        case epi_sculpt::PresetBridgeError::None: break;
        }
        std::cerr << '\n';
        return std::nullopt;
    }
    std::cout << std::setprecision(std::numeric_limits<double>::max_digits10)
              << "bridge-scope: catalog spatial study suggestions; not canonical modes or live morph readiness\n"
              << "bridge-from: " << report.fromId << '\n'
              << "bridge-from-revision: " << (report.fromRevision.empty() ? "unknown" : report.fromRevision) << '\n'
              << "bridge-to: " << report.toId << '\n'
              << "bridge-to-revision: " << (report.toRevision.empty() ? "unknown" : report.toRevision) << '\n'
              << "bridge-metric: mean(abs(a-b)/(1+max(abs(a),abs(b)))); existing spatial numeric domain\n"
              << "bridge-policy: matching routes/selectors; unique nonempty IDs; both legs smaller than direct\n"
              << "bridge-ranking: worst leg, total distance, exact ID\n"
              << "bridge-direct-distance: " << report.directDistance << '\n'
              << "bridge-limit: " << epi_sculpt::kMaxPresetBridgeResults << '\n'
              << "bridge-match-count: " << report.matchCount << '\n'
              << "bridge-count: " << report.hits.size() << '\n'
              << "bridge-truncated: " << (report.matchCount > report.hits.size() ? "yes" : "no") << '\n'
              << "bridge-columns: preset-id\tfrom-distance\tto-distance\tworst-distance\tworst-step-improvement\trevision\ttitle\n";
    if (details) {
        std::cout << "bridge-detail-scope: top three contributions to each leg's mean distance; not percentages\n"
                  << "bridge-leg-directions: from=FROM to bridge; to=bridge to TO\n"
                  << "bridge-leg-columns: preset-id\tleg\tdistance\tcomponents\tchanged\tshown\n"
                  << "bridge-change-columns: preset-id\tleg\tparameter\tfrom\tto\tmean-distance-contribution\n";
    }
    for (const auto& hit : report.hits) {
        std::cout << "bridge: " << hit.id << '\t' << hit.fromDistance << '\t' << hit.toDistance
                  << '\t' << hit.worstDistance() << '\t' << report.directDistance - hit.worstDistance()
                  << '\t' << (hit.revision.empty() ? "unknown" : hit.revision) << '\t' << hit.title << '\n';
        if (details) {
            const auto printLeg = [&](std::string_view leg, const epi_sculpt::SpatialPresetDistanceReport& changes) {
                std::cout << "bridge-leg: " << hit.id << '\t' << leg << '\t' << changes.distance
                          << '\t' << changes.componentCount << '\t' << changes.changedCount
                          << '\t' << changes.differenceCount << '\n';
                for (std::size_t i = 0; i < changes.differenceCount; ++i) {
                    const auto& change = changes.differences[i];
                    std::cout << "bridge-change: " << hit.id << '\t' << leg << '\t' << change.parameter;
                    if (change.component) std::cout << '[' << *change.component << ']';
                    std::cout << '\t' << change.from << '\t' << change.to << '\t' << change.contribution << '\n';
                }
            };
            printLeg("from", *hit.fromReport);
            printLeg("to", *hit.toReport);
        }
    }
    return report.hits.size();
}

std::optional<std::size_t> printNeighbors(
    const epi_sculpt::SculpturePresetAsset& source,
    const epi_sculpt::SculpturePresetLibrary& library, std::size_t limit, bool details,
    std::optional<double> maxDistance, std::optional<double> minDistance) {
    const auto neighborhood = epi_sculpt::preparePresetNeighborhood(source, library.entries(), {limit, details, maxDistance, minDistance});
    if (!neighborhood.ready()) {
        switch (neighborhood.error) {
        case epi_sculpt::PresetNeighborhoodError::NonfiniteSource:
            std::cerr << "Preset has nonfinite spatial parameters: " << source.id << '\n';
            break;
        case epi_sculpt::PresetNeighborhoodError::UnexplainableNeighbor:
            std::cerr << "Could not explain spatial preset distance: " << neighborhood.errorAsset->id << '\n';
            break;
        case epi_sculpt::PresetNeighborhoodError::InvalidLimit:
        case epi_sculpt::PresetNeighborhoodError::InvalidMaxDistance:
        case epi_sculpt::PresetNeighborhoodError::InvalidMinDistance:
        case epi_sculpt::PresetNeighborhoodError::InvalidDistanceRange:
            std::cerr << "Invalid neighborhood request\n";
            break;
        case epi_sculpt::PresetNeighborhoodError::None: break;
        }
        return std::nullopt;
    }
    const auto& hits = neighborhood.hits;
    const auto shown = hits.size();
    std::cout << "neighbor-scope: decoded spatial parameters; not canonical modes or morph readiness\n"
              << "neighbor-source: " << source.id << '\n'
              << "neighbor-source-revision: " << (source.revision.empty() ? "unknown" : source.revision) << '\n'
              << "neighbor-policy: matching selectors/discrete controls/routes and canonical-input presence; unique IDs\n"
              << "neighbor-metric: mean(abs(a-b)/(1+max(abs(a),abs(b)))); registry floats, affines, audio, iterations\n"
              << "neighbor-limit: " << neighborhood.request.limit << '\n';
    if (neighborhood.request.minDistance) {
        std::cout << "neighbor-min-distance: "
                  << std::setprecision(std::numeric_limits<double>::max_digits10)
                  << *neighborhood.request.minDistance << '\n';
    }
    if (neighborhood.request.maxDistance) {
        std::cout << "neighbor-max-distance: "
                  << std::setprecision(std::numeric_limits<double>::max_digits10)
                  << *neighborhood.request.maxDistance << '\n';
    }
    std::cout
              << "neighbor-match-count: " << neighborhood.matchCount << '\n'
              << "neighbor-truncated: " << (neighborhood.matchCount > shown ? "yes" : "no") << '\n'
              << "neighbor-count: " << shown << '\n'
              << "neighbor-columns: preset-id\tdistance\trevision\n"
              << std::setprecision(std::numeric_limits<double>::max_digits10);
    if (neighborhood.request.details) {
        std::cout << "neighbor-detail-scope: largest 3 nonzero contributions to the mean distance; ties retain metric order\n"
                  << "neighbor-detail-summary-columns: preset-id\tcomponent-count\tchanged-count\tshown-count\n"
                  << "neighbor-detail-columns: preset-id\tparameter\tfrom\tto\tdistance-contribution\n";
    }
    for (std::size_t i = 0; i < shown; ++i) {
        const auto& hit = hits[i];
        std::cout << "neighbor: " << hit.asset->id << '\t' << hit.distance << '\t'
                  << (hit.asset->revision.empty() ? "unknown" : hit.asset->revision) << '\n';
        if (neighborhood.request.details) {
            const auto& report = neighborhood.reports[i];
            std::cout << "neighbor-detail-summary: " << hit.asset->id << '\t' << report.componentCount
                      << '\t' << report.changedCount << '\t' << report.differenceCount << '\n';
            for (std::size_t index = 0; index < report.differenceCount; ++index) {
                const auto& difference = report.differences[index];
                std::cout << "neighbor-detail: " << hit.asset->id << '\t' << difference.parameter;
                if (difference.component) std::cout << '[' << *difference.component << ']';
                std::cout << '\t' << difference.from << '\t' << difference.to
                          << '\t' << difference.contribution << '\n';
            }
        }
    }
    return shown;
}

} // namespace

bool printPresetPath(const epi_sculpt::PreparedPresetPath& path, bool json = false) {
    if (!path.ready()) {
        std::cerr << "Preset path unavailable: " << epi_sculpt::presetPathErrorName(path.error) << '\n';
        return false;
    }
    if (json) {
        const auto document = epi_sculpt::serializePresetPathJson(path);
        if (!document) {
            std::cerr << "Preset path JSON unavailable: invalid read model\n";
            return false;
        }
        std::cout << *document;
        return true;
    }
    const auto precision = std::cout.precision();
    std::cout << std::setprecision(std::numeric_limits<double>::max_digits10)
              << "path-scope: decoded spatial parameters; not morph readiness or rendered motion\n"
              << "path-policy: " << (path.request.objective == epi_sculpt::PresetPathObjective::GentlestSteps
                  ? "minimum largest step, then hops; catalog-scan ties" : "fewest hops; catalog-order discovery ties")
              << "; unique IDs and matching selectors/routes\n"
              << "path-objective: " << epi_sculpt::presetPathObjectiveName(path.request.objective) << '\n'
              << "path-max-distance: " << path.request.maxDistance << '\n'
              << "path-max-hops: " << path.request.maxHops << '\n'
              << "path-candidate-count: " << path.candidateCount << '\n'
              << "path-hop-count: " << path.steps.size() - 1 << '\n'
              << "path-total-distance: " << path.totalDistance << '\n'
              << "path-largest-step-distance: " << path.largestStepDistance << '\n'
              << "path-columns: preset-id\trevision\tdistance-from-previous\n";
    if (path.request.details) {
        std::cout << "path-detail-scope: largest 3 nonzero contributions to the mean; ties retain metric order\n"
                  << "path-detail-summary-columns: from-id\tto-id\tcomponent-count\tchanged-count\tshown-count\n"
                  << "path-detail-columns: from-id\tto-id\tparameter\tfrom\tto\tdistance-contribution\n";
    }
    for (std::size_t i = 0; i < path.steps.size(); ++i) {
        const auto& step = path.steps[i];
        std::cout << "path-step: "; printFacetCategory(step.id);
        std::cout << '\t'; printFacetCategory(step.revision);
        std::cout << '\t' << step.previousDistance << '\n';
        if (i != 0 && step.details) {
            const auto& report = *step.details;
            const auto endpoints = [&] {
                printFacetCategory(path.steps[i - 1].id);
                std::cout << '\t'; printFacetCategory(step.id);
            };
            std::cout << "path-detail-summary: "; endpoints();
            std::cout << '\t' << report.componentCount << '\t' << report.changedCount
                      << '\t' << report.differenceCount << '\n';
            for (std::size_t j = 0; j < report.differenceCount; ++j) {
                const auto& difference = report.differences[j];
                std::cout << "path-detail: "; endpoints();
                std::cout << '\t' << difference.parameter;
                if (difference.component) std::cout << '[' << *difference.component << ']';
                std::cout << '\t' << difference.from << '\t' << difference.to
                          << '\t' << difference.contribution << '\n';
            }
        }
    }
    std::cout.precision(precision);
    return true;
}

bool printPresetPathFrontier(const epi_sculpt::PreparedPresetPathFrontier& frontier, bool json = false) {
    if (!frontier.ready()) {
        std::cerr << "Preset path frontier unavailable: " << epi_sculpt::presetPathErrorName(frontier.error) << '\n';
        return false;
    }
    if (json) {
        const auto document = epi_sculpt::serializePresetPathFrontierJson(frontier);
        if (!document) {
            std::cerr << "Preset path frontier JSON unavailable: invalid read model\n";
            return false;
        }
        std::cout << *document;
        return true;
    }
    const auto precision = std::cout.precision();
    std::cout << std::setprecision(std::numeric_limits<double>::max_digits10)
              << "path-frontier-scope: parameter-space trade-offs; not morph readiness or rendered smoothness\n"
              << "path-frontier-policy: nondominated hops/largest-step distance; catalog-scan ties; total distance observed\n"
              << "path-frontier-max-distance: " << frontier.request.maxDistance << '\n'
              << "path-frontier-max-hops: " << frontier.request.maxHops << '\n'
              << "path-frontier-candidate-count: " << frontier.candidateCount << '\n'
              << "path-frontier-route-count: " << frontier.routes.size() << '\n'
              << "path-frontier-columns: route-index\thop-count\tlargest-step-distance\ttotal-distance\n"
              << "path-frontier-step-columns: route-index\tstep-index\tpreset-id\trevision\tdistance-from-previous\n";
    if (frontier.request.details)
        std::cout << "path-frontier-detail-columns: route-index\tstep-index\tparameter\tfrom\tto\tdistance-contribution\n";
    for (std::size_t index = 0; index < frontier.routes.size(); ++index) {
        const auto& route = frontier.routes[index];
        std::cout << "path-frontier: " << index << '\t' << route.steps.size() - 1
                  << '\t' << route.largestStepDistance << '\t' << route.totalDistance << '\n';
        for (std::size_t stepIndex = 0; stepIndex < route.steps.size(); ++stepIndex) {
            const auto& step = route.steps[stepIndex];
            std::cout << "path-frontier-step: " << index << '\t' << stepIndex << '\t';
            printFacetCategory(step.id); std::cout << '\t'; printFacetCategory(step.revision);
            std::cout << '\t' << step.previousDistance << '\n';
            if (step.details) {
                for (std::size_t difference = 0; difference < step.details->differenceCount; ++difference) {
                    const auto& value = step.details->differences[difference];
                    std::cout << "path-frontier-detail: " << index << '\t' << stepIndex << '\t' << value.parameter;
                    if (value.component) std::cout << '[' << *value.component << ']';
                    std::cout << '\t' << value.from << '\t' << value.to << '\t' << value.contribution << '\n';
                }
            }
        }
    }
    std::cout.precision(precision);
    return true;
}

bool printFrontierExportChecks(const epi_sculpt::PresetPathAlternativesExportCheck& checked,
                              const epi_sculpt::PreparedPresetPathFrontier& frontier, float seconds,
                              epi_sculpt::PresetPathTourStyle style) {
    std::cout << "path-frontier-export-check: " << (checked.ready() ? "ready" : "refused")
              << "\npath-frontier-export-scope: current configuration only; destination/native capability and field replay unassessed"
              << "\npath-frontier-export-policy: gentlest ready reported route, then fewer hops, then input order; no global optimum claim"
              << "\npath-frontier-export-seconds: " << seconds
              << "\npath-frontier-export-dwell: " << style.dwellSeconds
              << "\npath-frontier-export-easing: " << epi_sculpt::name(style.easing)
              << "\npath-frontier-export-columns: route-index\tstatus\tcues\treason\n";
    if (checked.ready()) printPathTimelineHeader(true);
    for (std::size_t index = 0; index < checked.paths.size(); ++index) {
        const auto& result = checked.paths[index];
        const auto reason = result.error == epi_sculpt::PresetPathExportCheckError::TourRejected
            ? result.tourDiagnostic : epi_sculpt::presetConfigurationErrorName(result.configurationError);
        std::cout << "path-frontier-export-route: " << index << '\t' << (result.ready() ? "ready" : "refused")
                  << '\t' << result.cueCount << '\t' << reason << '\n';
        if (result.routeDiagnostic)
            std::cout << "path-frontier-export-route-diagnostic: " << index << '\t'
                      << result.routeDiagnostic->routeIndex << '\t'
                      << epi_sculpt::modulation_inspection::name(result.routeDiagnostic->error) << '\n';
        printPathTimeline(result, index);
    }
    std::cout << "path-frontier-export-recommended: ";
    if (checked.ready()) {
        const auto index = *checked.recommendedIndex;
        std::cout << index << "\npath-frontier-export-recommended-hops: "
                  << frontier.routes[index].request.maxHops << '\n';
    } else {
        std::cout << "none\n";
    }
    return checked.ready();
}

int inspect(int argc, char** argv) {
    const auto admitted = epi_sculpt::preset_inspector::parseInspectorOptions(argc, argv);
    if (admitted.action == InspectorParseAction::Help || admitted.showUsage) printUsage(argv[0]);
    if (admitted.action == InspectorParseAction::Help) return 0;
    if (admitted.action == InspectorParseAction::Error) {
        if (!admitted.diagnostic.empty()) std::cerr << admitted.diagnostic << '\n';
        return 2;
    }
    const auto& options = *admitted.options;
    const auto& selection = options.selection;

    // Route preparation is a cold value operation, independent of catalog IO.
    if (selection.mode == InspectorMode::Route || selection.mode == InspectorMode::RouteResponse)
        return printRoute(selection.first, selection.mode == InspectorMode::RouteResponse).accepted() ? 0 : 1;

    // An explicit authoring input is independent of the native resource tree.
    // Parse help/errors first; only default discovery uses the full host recipe.
    const std::filesystem::path root = options.requestedRoot
        ? *options.requestedRoot
#ifdef MGFX_EPI_SCULPT_PRESET_DIR
        : std::filesystem::path(MGFX_EPI_SCULPT_PRESET_DIR);
#else
        : epi_sculpt::SculptureResourceLocator::resolve(
              argc > 0 && argv != nullptr ? argv[0] : nullptr).presetDirectory;
#endif
    std::error_code error;
    if (root.empty() || !std::filesystem::is_directory(root, error) || error) {
        std::cerr << "Preset directory unavailable: " << root.generic_string() << '\n';
        return 1;
    }

    epi_sculpt::SculpturePresetLibrary library;
    const bool discovered = library.reload(root);
    const std::string_view exportKind = options.requestedPathRecommendedExport ? "Recommended path"
        : options.requestedPathExport ? "Path"
        : selection.mode == InspectorMode::MutationExport ? "Mutation"
        : selection.mode == InspectorMode::MorphExport ? "Morph" : "";
    if (!exportKind.empty() && !discovered) {
        // Fallback entries do not establish a complete catalog/source set for
        // destination protection. Refuse before query or publication effects.
        std::cerr << exportKind << " export unavailable: catalog discovery failed\n";
        for (const auto& diagnostic : library.diagnostics())
            std::cerr << "[SculpturePresetLibrary] " << diagnostic << '\n';
        return 1;
    }
    if (options.pathJson && !discovered) {
        // Startup may retain built-ins after failed discovery. Do not publish a
        // success document for a command whose catalog admission failed.
        std::cerr << "Preset path JSON unavailable: catalog discovery failed\n";
        for (const auto& diagnostic : library.diagnostics())
            std::cerr << "[SculpturePresetLibrary] " << diagnostic << '\n';
        return 1;
    }
    std::size_t shown = 0;
    bool selectionReady = true;
    if (selection.mode != InspectorMode::MorphStudyJson && selection.mode != InspectorMode::MorphFocus
        && selection.mode != InspectorMode::MorphFocusJson
        && selection.mode != InspectorMode::CompareRoutesJson && !options.pathJson)
        std::cout << "preset-root: " << root.generic_string() << '\n';
    if (selection.mode == InspectorMode::MorphFocus || selection.mode == InspectorMode::MorphFocusJson) {
        const auto* from = resolveUniquePreset(library, selection.first);
        const auto* to = resolveUniquePreset(library, selection.second);
        selectionReady = from && to && printMorphFocus(*from, *to,
            options.requestedMorphSamples.value_or(epi_sculpt::kDefaultPresetMorphStudySamples),
            options.requestedMorphRefinements.value_or(epi_sculpt::kDefaultPresetMorphRefinements),
            selection.mode == InspectorMode::MorphFocusJson);
        shown = selectionReady ? 1U : 0U;
    } else if (selection.mode == InspectorMode::MorphStudy || selection.mode == InspectorMode::MorphStudyJson) {
        const auto* from = resolveUniquePreset(library, selection.first);
        const auto* to = resolveUniquePreset(library, selection.second);
        selectionReady = from && to && printMorphStudy(*from, *to,
            options.requestedMorphSamples.value_or(epi_sculpt::kDefaultPresetMorphStudySamples),
            selection.mode == InspectorMode::MorphStudyJson);
        shown = selectionReady ? 1U : 0U;
    } else if (selection.mode == InspectorMode::MorphExport || selection.mode == InspectorMode::MorphPreview) {
        const auto* from = resolveUniquePreset(library, selection.first);
        const auto* to = resolveUniquePreset(library, selection.second);
        const auto position = options.requestedMorphPosition.value_or(0.5f);
        selectionReady = from && to && (selection.mode == InspectorMode::MorphPreview
            ? printMorphPreview(*from, *to, position)
            : exportMorph(library, *from, *to, position, std::filesystem::path(selection.third)));
        shown = selectionReady ? 1U : 0U;
    } else if (selection.mode == InspectorMode::MutationPreview || selection.mode == InspectorMode::MutationExport) {
        if (const auto* asset = resolveUniquePreset(library, selection.first)) {
            const auto seed = options.requestedMutationSeed.value_or(42U);
            const auto strength = options.requestedMutationStrength.value_or(0.35f);
            const auto preview = epi_sculpt::SculptureMutator(seed).previewMutation(asset->params, strength);
            if (!preview) {
                std::cerr << "Mutation preview unavailable: nonfinite mutation inputs\n";
                selectionReady = false;
            } else if (selection.mode == InspectorMode::MutationExport) {
                selectionReady = exportMutation(library, *asset, *preview, seed, strength,
                                                std::filesystem::path(selection.second));
            } else {
                printMutationPreview(*asset, seed, strength, *preview);
            }
            shown = selectionReady ? 1U : 0U;
        } else {
            selectionReady = false;
        }
    } else if (selection.mode == InspectorMode::MorphBanks) {
        shown = printMorphBanks(library);
    } else if (selection.mode == InspectorMode::Routes || selection.mode == InspectorMode::RoutesResponse) {
        if (const auto* asset = resolveUniquePreset(library, selection.first)) {
            selectionReady = printPresetRoutes(*asset,
                                              selection.mode == InspectorMode::RoutesResponse);
            shown = 1;
        } else {
            selectionReady = false;
        }
    } else if (selection.mode == InspectorMode::Neighbors) {
        const auto* source = resolveUniquePreset(library, selection.first);
        if (!source) {
            selectionReady = false;
        } else if (const auto count = printNeighbors(
                       *source, library, options.requestedNeighborLimit.value_or(kDefaultPresetNeighborLimit),
                       options.neighborDetails, options.requestedNeighborMaxDistance,
                       options.requestedNeighborMinDistance)) {
            shown = *count;
        } else {
            selectionReady = false;
        }
    } else if (selection.mode == InspectorMode::Path) {
        const auto seconds = options.requestedPathSeconds.value_or(3.0f);
        const epi_sculpt::PresetPathTourStyle style{
            options.requestedPathDwell.value_or(1.0f),
            options.requestedPathEasing.value_or(epi_sculpt::MorphEasing::Smoothstep)};
        if (options.pathFrontier) {
            const auto frontier = epi_sculpt::preparePresetPathFrontier(library.entries(), selection.first, selection.second,
                {options.requestedPathMaxDistance.value_or(epi_sculpt::kDefaultPresetPathMaxDistance),
                 options.requestedPathHops.value_or(epi_sculpt::kDefaultPresetPathHops), options.pathDetails});
            selectionReady = printPresetPathFrontier(frontier, options.pathJson);
            shown = selectionReady ? frontier.routes.size() : 0U;
            if (selectionReady && (options.pathCheckExport || options.requestedPathRecommendedExport)) {
                const auto checked = epi_sculpt::checkPresetPathAlternativesExport(library.entries(), frontier.routes, seconds, style);
                selectionReady = printFrontierExportChecks(checked, frontier, seconds, style);
                if (selectionReady && options.requestedPathRecommendedExport) {
                    // Export the exact assessed path, not a new search by hop
                    // count. The existing tour/publisher owners re-admit it;
                    // readiness is not permission to skip file protection.
                    const auto& recommended = frontier.routes[*checked.recommendedIndex];
                    selectionReady = exportPresetPath(library, recommended, seconds, style, *options.requestedPathRecommendedExport);
                }
            }
        } else {
            const auto path = epi_sculpt::preparePresetPath(library.entries(), selection.first, selection.second,
                {options.requestedPathMaxDistance.value_or(epi_sculpt::kDefaultPresetPathMaxDistance),
                 options.requestedPathHops.value_or(epi_sculpt::kDefaultPresetPathHops), options.pathDetails,
                 options.requestedPathObjective.value_or(epi_sculpt::PresetPathObjective::FewestHops)});
            selectionReady = path.ready() && options.requestedPathExport
                ? exportPresetPath(library, path, seconds, style,
                                   *options.requestedPathExport) : path.ready();
            if (selectionReady && options.pathCheckExport)
                selectionReady = printPathExportCheck(library.entries(), path, seconds, style);
            if (selectionReady || !path.ready()) selectionReady = printPresetPath(path, options.pathJson);
            shown = selectionReady ? path.steps.size() : 0U;
        }
    } else if (selection.mode == InspectorMode::Journey) {
        const auto tour = library.inspectCuratedTour(selection.first);
        if (!tour.recognized) {
            std::cerr << "Tour not found: " << selection.first << '\n';
            selectionReady = false;
        } else {
            const auto journey = epi_sculpt::preparePresetJourney(
                library, tour, {options.requestedJourneyOffset.value_or(0)});
            if (!journey.ready()) {
                std::cerr << "Journey offset " << journey.firstStep << " exceeds final offset "
                          << journey.totalSteps << '\n';
                selectionReady = false;
            } else {
                printJourney(selection.first, journey);
                shown = journey.steps.size();
            }
        }
    } else if (selection.mode == InspectorMode::CompareTours) {
        const auto from = library.inspectCuratedTour(selection.first);
        if (!from.recognized) {
            std::cerr << "Tour not found: " << selection.first << '\n';
            selectionReady = false;
        } else {
            const auto to = library.inspectCuratedTour(selection.second);
            if (!to.recognized) {
                std::cerr << "Tour not found: " << selection.second << '\n';
                selectionReady = false;
            } else {
                printTourComparison(selection.first, selection.second,
                                    epi_sculpt::compareTourCues(from.entries, to.entries));
                shown = 2;
            }
        }
    } else if (selection.mode == InspectorMode::Compare || selection.mode == InspectorMode::CompareRoutes
               || selection.mode == InspectorMode::CompareRoutesJson
               || selection.mode == InspectorMode::Bridges) {
        const auto* from = resolveUniquePreset(library, selection.first);
        const auto* to = from ? resolveUniquePreset(library, selection.second) : nullptr;
        if (!from || !to) {
            selectionReady = false;
        } else if (selection.mode == InspectorMode::Bridges) {
            if (const auto count = printBridges(*from, *to, library, options.bridgeDetails)) shown = *count;
            else selectionReady = false;
        } else if (selection.mode == InspectorMode::CompareRoutes) {
            selectionReady = printRouteComparison(*from, *to);
            shown = 2;
        } else if (selection.mode == InspectorMode::CompareRoutesJson) {
            selectionReady = printRouteComparisonJson(*from, *to);
            shown = selectionReady ? 2U : 0U;
        } else {
            printComparison(*from, *to);
            shown = 2;
        }
    } else if (selection.mode == InspectorMode::Explain) {
        const auto* asset = resolveUniquePreset(library, selection.first);
        if (!asset) {
            selectionReady = false;
        } else {
            printExplanation(*asset);
            shown = 1;
        }
    } else if (selection.mode == InspectorMode::Tours || selection.mode == InspectorMode::Tour) {
        std::cout << "tour-policy: ready entries only; unavailable entries omitted; "
                     "readiness is loaded asset/route availability\n";
        if (selection.mode == InspectorMode::Tour) {
            const auto tour = library.inspectCuratedTour(selection.first);
            printTour(selection.first, tour, true);
            selectionReady = tour.recognized && tour.readyCount() > 0;
            shown = 1;
        } else {
            for (const auto& name : library.availableTours()) {
                printTour(name, library.inspectCuratedTour(name), false);
                ++shown;
            }
        }
    } else {
        const epi_sculpt::SculpturePresetSearchQuery query(options.requestedSearch.value_or(""));
        if (options.searchDetails) {
            std::cout << "search-detail-policy: strongest matching field per distinct positive term; "
                         "ties use id, title, category, description order; exclusions filter only\n"
                      << "search-detail-columns: preset-id\tterm\tscope\tfield\tmatch\tscore\n"
                      << "search-score-columns: preset-id\ttotal\n";
        }
        const auto printPreset = [&](const auto& preset) {
            ++shown;
            std::cout << preset.id << "\t" << preset.category << "\t"
                      << preset.title;
            if (preset.authoredOnDisk) {
                std::cout << "\t" << preset.sourcePath.generic_string();
            }
            if (options.requestedSearch) std::cout << "\t" << preset.description;
            std::cout << '\n';
            if (options.searchDetails) {
                const epi_sculpt::SculpturePresetSearchText text{
                    preset.id, preset.title, preset.category, preset.description};
                const auto fieldName = [](epi_sculpt::preset_search_detail::SearchField field) {
                    using epi_sculpt::preset_search_detail::SearchField;
                    switch (field) {
                    case SearchField::Any: return "any";
                    case SearchField::Id: return "id";
                    case SearchField::Title: return "title";
                    case SearchField::Category: return "category";
                    case SearchField::Description: return "description";
                    }
                    return "unknown";
                };
                const auto total = query.explain(text, [&](std::string_view term, auto scope, auto match) {
                    std::cout << "search-detail: ";
                    printFacetCategory(preset.id);
                    std::cout << '\t';
                    printFacetCategory(term);
                    std::cout << '\t' << fieldName(scope) << '\t' << fieldName(match.field)
                              << '\t' << (match.exact ? "exact" : "substring")
                              << '\t' << match.score << '\n';
                });
                if (total) {
                    std::cout << "search-score: ";
                    printFacetCategory(preset.id);
                    std::cout << '\t' << *total << '\n';
                }
            }
        };
        struct SearchHit {
            const epi_sculpt::SculpturePresetAsset* preset;
            std::size_t score;
        };
        std::vector<SearchHit> hits;
        std::vector<std::string_view> facetCategories;
        // Identity admission precedes filters: a duplicate hidden by search or
        // category is still ambiguous to resolveExactId. Borrowed keys remain
        // valid while the loaded library is unchanged throughout inspection.
        std::optional<epi_sculpt::SculpturePresetIdentityIndex> selectableIdentities;
        if (options.selectableOnly) selectableIdentities.emplace(library.entries());
        for (const auto& preset : library.entries()) {
            if (selectableIdentities && !selectableIdentities->isUnique(preset.id)) continue;
            if (options.authoredOnly && !preset.authoredOnDisk) continue;
            if (options.requestedCategory
                && !epi_sculpt::matchesPresetCategory(preset.category, *options.requestedCategory)) continue;
            const epi_sculpt::SculpturePresetSearchText text{
                preset.id, preset.title, preset.category, preset.description};
            if (options.facets) {
                if (query.matches(text)) facetCategories.push_back(preset.category);
            } else if (options.ranked) {
                if (const auto score = query.score(text)) hits.push_back({&preset, *score});
            } else if (query.matches(text)) {
                printPreset(preset);
            }
        }
        if (options.facets) {
            const auto facets = epi_sculpt::presetCategoryFacets(std::move(facetCategories));
            for (const auto& facet : facets) {
                std::cout << "facet: ";
                printFacetCategory(facet.category);
                std::cout << '\t' << facet.count << '\n';
                shown += facet.count;
            }
            std::cout << "facet-total: " << shown << '\n';
        } else if (options.ranked) {
            std::stable_sort(hits.begin(), hits.end(), [](const auto& a, const auto& b) {
                return a.score > b.score;
            });
            for (const auto& hit : hits) printPreset(*hit.preset);
        }
    }
    std::cerr << "[SculpturePresetLibrary] " << shown
              << (selection.mode == InspectorMode::MorphBanks ? " banks shown, "
                  : selection.mode == InspectorMode::Bridges ? " bridges shown, "
                  : selection.mode == InspectorMode::Journey ? " journey steps shown, "
                  : selection.mode == InspectorMode::Tours || selection.mode == InspectorMode::Tour
                      || selection.mode == InspectorMode::CompareTours
                      ? " tours shown, " : " entries shown, ")
              << library.diagnostics().size() << " diagnostics\n";
    for (const auto& diagnostic : library.diagnostics()) {
        std::cerr << "[SculpturePresetLibrary] " << diagnostic << '\n';
    }
    return discovered && selectionReady ? 0 : 1;
}

int main(int argc, char** argv) {
    try {
        return inspect(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Preset inspection failed: " << error.what() << '\n';
        return 1;
    }
}
