#pragma once

#include "SculpturePresetInspectionLimits.h"
#include "SculpturePresetSearch.h"
#include "SculptureMorphEasing.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace epi_sculpt::preset_inspector {

enum class InspectorMode {
    Listing, Tours, Tour, Journey, CompareTours, Explain, Compare, CompareRoutes, CompareRoutesJson, Neighbors, Bridges, Path, MorphBanks,
    Route, Routes, RouteResponse, RoutesResponse, MutationPreview, MutationExport, MorphExport, MorphPreview, MorphStudy, MorphStudyJson, MorphFocus, MorphFocusJson
};

struct InspectorCommand {
    std::string_view flag;
    InspectorMode mode;
    unsigned arity;
    std::string_view operands;
    std::string_view description;
    std::string_view emptyError;
};

// One declaration drives argument admission and help. This is only CLI intent;
// catalog facts, mathematical state and runtime commands keep their own owners.
inline constexpr std::array kCommands{
    InspectorCommand{"--morph-banks", InspectorMode::MorphBanks, 0, "",
        "Show predefined morph banks and exact corner preset availability", ""},
    InspectorCommand{"--tours", InspectorMode::Tours, 0, "",
        "List tour definitions and ready/requested counts", ""},
    InspectorCommand{"--tour", InspectorMode::Tour, 1, "NAME",
        "Show ordered entries, including missing assets/routes", "Tour name must not be empty"},
    InspectorCommand{"--journey", InspectorMode::Journey, 1, "TOUR",
        "Study adjacent tour cues, boundaries and their largest spatial changes (up to 64 steps)", "Tour name must not be empty"},
    InspectorCommand{"--compare-tours", InspectorMode::CompareTours, 2, "FROM TO",
        "Compare ordered tour ID references and availability, retaining repeated cues", "Comparison tour names must not be empty"},
    InspectorCommand{"--explain", InspectorMode::Explain, 1, "ID",
        "Explain decoded math, route and provenance for an exact stable preset ID", "Preset ID must not be empty"},
    InspectorCommand{"--mutation-preview", InspectorMode::MutationPreview, 1, "ID",
        "Preview one seeded spatial parameter mutation without changing the preset", "Preset ID must not be empty"},
    InspectorCommand{"--mutation-export", InspectorMode::MutationExport, 2, "ID FILE.epimode",
        "Export a seeded compact spatial variant as a reopenable authored configuration", "Preset ID and output path must not be empty"},
    InspectorCommand{"--morph-export", InspectorMode::MorphExport, 3, "FROM TO FILE.epimode",
        "Save an intermediate morph between compatible compact spatial presets", "Morph endpoint IDs and output path must not be empty"},
    InspectorCommand{"--morph-preview", InspectorMode::MorphPreview, 2, "FROM TO",
        "Inspect a read-only spatial morph and its largest changes from/to endpoints", "Morph endpoint IDs must not be empty"},
    InspectorCommand{"--morph-study", InspectorMode::MorphStudy, 2, "FROM TO",
        "Sample a spatial morph path and explain its largest sampled parameter step", "Morph endpoint IDs must not be empty"},
    InspectorCommand{"--morph-study-json", InspectorMode::MorphStudyJson, 2, "FROM TO",
        "Emit a versioned read-only morph study as one JSON document", "Morph endpoint IDs must not be empty"},
    InspectorCommand{"--morph-focus", InspectorMode::MorphFocus, 2, "FROM TO",
        "Refine the largest sampled morph step into a local parameter-change bracket", "Morph endpoint IDs must not be empty"},
    InspectorCommand{"--morph-focus-json", InspectorMode::MorphFocusJson, 2, "FROM TO",
        "Emit a versioned local morph-focus trace and final spatial components as JSON", "Morph endpoint IDs must not be empty"},
    InspectorCommand{"--compare", InspectorMode::Compare, 2, "FROM TO",
        "Show directional spatial changes and declared route/selector boundaries for exact IDs", "Comparison preset IDs must not be empty"},
    InspectorCommand{"--compare-routes", InspectorMode::CompareRoutes, 2, "FROM TO",
        "Compare resolved modulation route maps, semantic equivalence and target fan-in for exact IDs", "Comparison preset IDs must not be empty"},
    InspectorCommand{"--compare-routes-json", InspectorMode::CompareRoutesJson, 2, "FROM TO",
        "Emit a versioned read-only modulation route comparison and target fan-in study as JSON", "Comparison preset IDs must not be empty"},
    InspectorCommand{"--neighbors", InspectorMode::Neighbors, 1, "ID",
        "Show nearby spatial presets with matching selectors/routes (default limit 5)", "Preset ID must not be empty"},
    InspectorCommand{"--bridges", InspectorMode::Bridges, 2, "FROM TO",
        "Suggest up to five existing spatial presets that reduce the larger of two study steps", "Bridge endpoint IDs must not be empty"},
    InspectorCommand{"--path", InspectorMode::Path, 2, "FROM TO",
        "Find bounded spatial stepping stones using fewest hops or gentlest parameter steps", "Path endpoint IDs must not be empty"},
    InspectorCommand{"--route", InspectorMode::Route, 1, "TEXT",
        "Explain one modulation route using the real parser; no catalog required", "Route text must not be empty"},
    InspectorCommand{"--routes", InspectorMode::Routes, 1, "ID",
        "Explain a preset's complete route map, using visual defaults when empty", "Preset ID must not be empty"},
    InspectorCommand{"--route-response", InspectorMode::RouteResponse, 1, "TEXT",
        "Sample the real transfer curve and unsmoothed contribution", "Route text must not be empty"},
    InspectorCommand{"--routes-response", InspectorMode::RoutesResponse, 1, "ID",
        "Study each route of a named preset with independent probes", "Preset ID must not be empty"},
};

struct InspectorSelection {
    InspectorMode mode = InspectorMode::Listing;
    std::string_view first;
    std::string_view second;
    std::string_view third;
};

struct InspectorOptions {
    std::optional<std::filesystem::path> requestedRoot;
    bool authoredOnly = false;
    bool ranked = false;
    bool facets = false;
    bool searchDetails = false;
    bool selectableOnly = false;
    bool neighborDetails = false;
    bool bridgeDetails = false;
    bool pathDetails = false;
    bool pathFrontier = false;
    bool pathJson = false;
    bool pathCheckExport = false;
    std::optional<std::string_view> requestedSearch;
    std::optional<std::string_view> requestedCategory;
    std::optional<std::size_t> requestedNeighborLimit;
    std::optional<double> requestedNeighborMaxDistance;
    std::optional<double> requestedNeighborMinDistance;
    std::optional<std::size_t> requestedJourneyOffset;
    std::optional<double> requestedPathMaxDistance;
    std::optional<std::size_t> requestedPathHops;
    std::optional<PresetPathObjective> requestedPathObjective;
    std::optional<std::filesystem::path> requestedPathExport;
    std::optional<std::filesystem::path> requestedPathRecommendedExport;
    std::optional<float> requestedPathSeconds;
    std::optional<float> requestedPathDwell;
    std::optional<MorphEasing> requestedPathEasing;
    std::optional<std::uint32_t> requestedMutationSeed;
    std::optional<float> requestedMutationStrength;
    std::optional<float> requestedMorphPosition;
    std::optional<std::size_t> requestedMorphSamples;
    std::optional<std::size_t> requestedMorphRefinements;
    InspectorSelection selection;
};

enum class InspectorParseAction { Run, Help, Error };

struct InspectorParseResult {
    InspectorParseAction action = InspectorParseAction::Error;
    std::optional<InspectorOptions> options;
    std::string diagnostic;
    bool showUsage = false;
};

namespace detail {
[[nodiscard]] inline std::optional<std::size_t> unsignedOption(
    std::string_view text, std::size_t minimum = 0,
    std::size_t maximum = std::numeric_limits<std::size_t>::max()) noexcept {
    std::size_t value = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()
        || value < minimum || value > maximum) return std::nullopt;
    return value;
}
[[nodiscard]] inline std::optional<double> nonnegativeFiniteOption(std::string_view text) noexcept {
    double value = 0.0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()
        || !std::isfinite(value) || value < 0.0) return std::nullopt;
    return value;
}
}

// This is a cold value boundary: no catalog/resource discovery or stream output.
// Only Run publishes options. IDs/search operands borrow argv until inspection
// completes; root path storage is owned. Normal process argc/argv rules apply.
[[nodiscard]] inline InspectorParseResult parseInspectorOptions(int argc, const char* const* argv) {
    InspectorOptions candidate;
    const auto failure = [](std::string message = {}, bool usage = false) -> InspectorParseResult {
        return {.diagnostic = std::move(message), .showUsage = usage};
    };
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        if (argument == "--root" && index + 1 < argc) {
            candidate.requestedRoot = argv[++index];
            if (candidate.requestedRoot->empty()) return failure("Preset directory must not be empty");
        } else if (argument == "--authored-only") {
            candidate.authoredOnly = true;
        } else if (argument == "--ranked") {
            candidate.ranked = true;
        } else if (argument == "--facets") {
            if (candidate.facets) return failure("Specify --facets once");
            candidate.facets = true;
        } else if (argument == "--search-details") {
            if (candidate.searchDetails) return failure("Specify --search-details once");
            candidate.searchDetails = true;
        } else if (argument == "--selectable-only") {
            if (candidate.selectableOnly) return failure("Specify --selectable-only once");
            candidate.selectableOnly = true;
        } else if (argument == "--path-check-export") {
            if (candidate.pathCheckExport) return failure("Specify --path-check-export once");
            candidate.pathCheckExport = true;
        } else if (argument == "--path-json") {
            if (candidate.pathJson) return failure("Specify --path-json once");
            candidate.pathJson = true;
        } else if (argument == "--path-frontier") {
            if (candidate.pathFrontier) return failure("Specify --path-frontier once");
            candidate.pathFrontier = true;
        } else if (argument == "--path-details") {
            if (candidate.pathDetails) return failure("Specify --path-details once");
            candidate.pathDetails = true;
        } else if (argument == "--path-max-distance") {
            if (candidate.requestedPathMaxDistance || index + 1 == argc)
                return failure("Specify --path-max-distance once with a nonnegative finite number");
            const auto value = detail::nonnegativeFiniteOption(argv[++index]);
            if (!value) return failure("Path maximum distance must be a complete nonnegative finite number");
            candidate.requestedPathMaxDistance = *value;
        } else if (argument == "--path-export-recommended") {
            if (candidate.requestedPathRecommendedExport || index + 1 == argc)
                return failure("Specify --path-export-recommended once with a FILE.epimode output");
            candidate.requestedPathRecommendedExport = argv[++index];
            if (candidate.requestedPathRecommendedExport->empty()
                || candidate.requestedPathRecommendedExport->extension() != ".epimode")
                return failure("Recommended path export output must have the .epimode extension");
        } else if (argument == "--path-export") {
            if (candidate.requestedPathExport || index + 1 == argc)
                return failure("Specify --path-export once with a FILE.epimode output");
            candidate.requestedPathExport = argv[++index];
            if (candidate.requestedPathExport->empty()
                || candidate.requestedPathExport->extension() != ".epimode")
                return failure("Path export output must have the .epimode extension");
        } else if (argument == "--path-seconds") {
            if (candidate.requestedPathSeconds || index + 1 == argc)
                return failure("Specify --path-seconds once with a finite transition duration from 0.001 to 3600");
            const auto value = detail::nonnegativeFiniteOption(argv[++index]);
            if (!value || *value < 0.001 || *value > 3600)
                return failure("Path transition duration must be from 0.001 to 3600 seconds");
            candidate.requestedPathSeconds = static_cast<float>(*value);
        } else if (argument == "--path-dwell") {
            if (candidate.requestedPathDwell || index + 1 == argc)
                return failure("Specify --path-dwell once with a finite duration from 0 to 3600");
            const auto value = detail::nonnegativeFiniteOption(argv[++index]);
            if (!value || *value > 3600)
                return failure("Path dwell duration must be from 0 to 3600 seconds");
            candidate.requestedPathDwell = static_cast<float>(*value);
        } else if (argument == "--path-easing") {
            if (candidate.requestedPathEasing || index + 1 == argc)
                return failure("Specify --path-easing once with an easing name from --help");
            const auto value = parseMorphEasing(argv[++index]);
            if (!value) return failure("Unknown path easing; use an exact easing name from --help");
            candidate.requestedPathEasing = *value;
        } else if (argument == "--path-objective") {
            if (candidate.requestedPathObjective || index + 1 == argc)
                return failure("Specify --path-objective once with fewest-hops or gentlest");
            const std::string_view value = argv[++index];
            if (value == "fewest-hops") candidate.requestedPathObjective = PresetPathObjective::FewestHops;
            else if (value == "gentlest") candidate.requestedPathObjective = PresetPathObjective::GentlestSteps;
            else return failure("Path objective must be fewest-hops or gentlest");
        } else if (argument == "--path-hops") {
            if (candidate.requestedPathHops || index + 1 == argc)
                return failure("Specify --path-hops once with an integer from 1 to " + std::to_string(kMaxPresetPathHops));
            const auto value = detail::unsignedOption(argv[++index], 1, kMaxPresetPathHops);
            if (!value) return failure("Path hop limit must be an integer from 1 to " + std::to_string(kMaxPresetPathHops));
            candidate.requestedPathHops = *value;
        } else if (argument == "--mutation-seed") {
            if (candidate.requestedMutationSeed || index + 1 == argc)
                return failure("Specify --mutation-seed once with an unsigned 32-bit decimal integer");
            const auto value = detail::unsignedOption(argv[++index], 0, std::numeric_limits<std::uint32_t>::max());
            if (!value) return failure("Mutation seed must be a complete unsigned 32-bit decimal integer");
            candidate.requestedMutationSeed = static_cast<std::uint32_t>(*value);
        } else if (argument == "--mutation-strength") {
            if (candidate.requestedMutationStrength || index + 1 == argc)
                return failure("Specify --mutation-strength once with a finite number from 0 to 1");
            const auto value = detail::nonnegativeFiniteOption(argv[++index]);
            if (!value || *value > 1.0) return failure("Mutation strength must be a complete finite number from 0 to 1");
            candidate.requestedMutationStrength = static_cast<float>(*value);
        } else if (argument == "--morph-refinements") {
            if (candidate.requestedMorphRefinements || index + 1 == argc)
                return failure("Specify --morph-refinements once with an unsigned integer from 0 to 12");
            const auto value = detail::unsignedOption(argv[++index], 0, kMaxPresetMorphRefinements);
            if (!value) return failure("Morph refinement count must be an unsigned integer from 0 to 12");
            candidate.requestedMorphRefinements = *value;
        } else if (argument == "--morph-samples") {
            if (candidate.requestedMorphSamples || index + 1 == argc)
                return failure("Specify --morph-samples once with an unsigned integer from 2 to 33");
            const auto value = detail::unsignedOption(argv[++index], kMinPresetMorphStudySamples,
                                                       kMaxPresetMorphStudySamples);
            if (!value) return failure("Morph sample count must be an unsigned integer from 2 to 33");
            candidate.requestedMorphSamples = *value;
        } else if (argument == "--morph-position") {
            if (candidate.requestedMorphPosition || index + 1 == argc)
                return failure("Specify --morph-position once with a finite number from 0 to 1");
            const auto value = detail::nonnegativeFiniteOption(argv[++index]);
            if (!value || *value > 1.0) return failure("Morph position must be a complete finite number from 0 to 1");
            candidate.requestedMorphPosition = static_cast<float>(*value);
        } else if (argument == "--journey-offset") {
            if (candidate.requestedJourneyOffset || index + 1 == argc)
                return failure("Specify --journey-offset once with an unsigned decimal integer");
            const auto value = detail::unsignedOption(argv[++index]);
            if (!value) return failure("Journey offset must be a complete unsigned decimal integer");
            candidate.requestedJourneyOffset = *value;
        } else if (argument == "--neighbor-details") {
            if (candidate.neighborDetails) return failure("Specify --neighbor-details once");
            candidate.neighborDetails = true;
        } else if (argument == "--bridge-details") {
            if (candidate.bridgeDetails) return failure("Specify --bridge-details once");
            candidate.bridgeDetails = true;
        } else if (argument == "--neighbor-min-distance") {
            if (candidate.requestedNeighborMinDistance || index + 1 == argc)
                return failure("Specify --neighbor-min-distance once with a nonnegative finite number");
            const auto value = detail::nonnegativeFiniteOption(argv[++index]);
            if (!value) return failure("Neighbor minimum distance must be a complete nonnegative finite number");
            candidate.requestedNeighborMinDistance = *value;
        } else if (argument == "--neighbor-max-distance") {
            if (candidate.requestedNeighborMaxDistance || index + 1 == argc)
                return failure("Specify --neighbor-max-distance once with a nonnegative finite number");
            const auto value = detail::nonnegativeFiniteOption(argv[++index]);
            if (!value) return failure("Neighbor maximum distance must be a complete nonnegative finite number");
            candidate.requestedNeighborMaxDistance = *value;
        } else if (argument == "--neighbor-limit") {
            if (candidate.requestedNeighborLimit || index + 1 == argc)
                return failure("Specify --neighbor-limit once with an integer from 1 to "
                    + std::to_string(kMaxPresetNeighborLimit));
            const auto value = detail::unsignedOption(argv[++index], 1, kMaxPresetNeighborLimit);
            if (!value) return failure("Neighbor limit must be an unsigned decimal integer from 1 to "
                + std::to_string(kMaxPresetNeighborLimit));
            candidate.requestedNeighborLimit = *value;
        } else if (argument == "--category") {
            if (candidate.requestedCategory || index + 1 == argc)
                return failure("Specify --category once with a category name");
            candidate.requestedCategory = argv[++index];
            if (!hasPresetSearchTerms(*candidate.requestedCategory))
                return failure("Category must contain at least one non-whitespace character");
        } else if (argument == "--search" && index + 1 < argc) {
            candidate.requestedSearch = argv[++index];
            if (!hasPresetSearchTerms(*candidate.requestedSearch))
                return failure("Search must contain at least one word");
        } else if (argument == "--help" || argument == "-h") {
            return {.action = InspectorParseAction::Help};
        } else {
            const auto command = std::find_if(kCommands.begin(), kCommands.end(),
                [&](const auto& entry) { return entry.flag == argument; });
            if (command == kCommands.end()
                || static_cast<unsigned>(argc - index - 1) < command->arity) return failure({}, true);
            if (candidate.selection.mode != InspectorMode::Listing) return failure("Choose one detail command");
            InspectorSelection selected{command->mode, {}, {}};
            if (command->arity > 0) selected.first = argv[++index];
            if (command->arity > 1) selected.second = argv[++index];
            if (command->arity > 2) selected.third = argv[++index];
            if ((command->arity > 0 && selected.first.empty())
                || (command->arity > 1 && selected.second.empty())
                || (command->arity > 2 && selected.third.empty())) return failure(std::string(command->emptyError));
            candidate.selection = selected;
        }
    }
    if (candidate.ranked && !candidate.requestedSearch) return failure("--ranked requires --search WORDS");
    if (candidate.searchDetails && !candidate.requestedSearch)
        return failure("--search-details requires --search WORDS");
    if (candidate.searchDetails && candidate.facets)
        return failure("--search-details cannot be combined with --facets");
    if ((candidate.authoredOnly || candidate.requestedSearch || candidate.requestedCategory
         || candidate.facets || candidate.selectableOnly)
        && candidate.selection.mode != InspectorMode::Listing)
        return failure("Choose preset filters (--authored-only/--selectable-only/--search/--category/--facets) or one detail command");
    if (candidate.requestedNeighborLimit && candidate.selection.mode != InspectorMode::Neighbors)
        return failure("--neighbor-limit requires --neighbors ID");
    if (candidate.requestedNeighborMaxDistance && candidate.selection.mode != InspectorMode::Neighbors)
        return failure("--neighbor-max-distance requires --neighbors ID");
    if (candidate.requestedNeighborMinDistance && candidate.selection.mode != InspectorMode::Neighbors)
        return failure("--neighbor-min-distance requires --neighbors ID");
    if (candidate.requestedNeighborMinDistance && candidate.requestedNeighborMaxDistance
        && *candidate.requestedNeighborMinDistance > *candidate.requestedNeighborMaxDistance)
        return failure("Neighbor minimum distance must not exceed maximum distance");
    if (candidate.neighborDetails && candidate.selection.mode != InspectorMode::Neighbors)
        return failure("--neighbor-details requires --neighbors ID");
    if (candidate.bridgeDetails && candidate.selection.mode != InspectorMode::Bridges)
        return failure("--bridge-details requires --bridges FROM TO");
    if ((candidate.requestedPathMaxDistance || candidate.requestedPathHops || candidate.pathDetails || candidate.pathFrontier || candidate.pathJson
         || candidate.requestedPathObjective || candidate.requestedPathExport || candidate.requestedPathRecommendedExport
         || candidate.requestedPathSeconds || candidate.requestedPathDwell || candidate.requestedPathEasing
         || candidate.pathCheckExport)
        && candidate.selection.mode != InspectorMode::Path)
        return failure("Path options require --path FROM TO");
    if (candidate.requestedPathSeconds && !candidate.requestedPathExport
        && !candidate.requestedPathRecommendedExport && !candidate.pathCheckExport)
        return failure("--path-seconds requires --path-export, --path-export-recommended or --path-check-export");
    if ((candidate.requestedPathDwell || candidate.requestedPathEasing) && !candidate.requestedPathExport
        && !candidate.requestedPathRecommendedExport && !candidate.pathCheckExport)
        return failure("--path-dwell/--path-easing require --path-export, --path-export-recommended or --path-check-export");
    if (candidate.requestedPathRecommendedExport
        && (!candidate.pathFrontier || candidate.requestedPathExport || candidate.pathCheckExport || candidate.pathJson))
        return failure("--path-export-recommended requires --path-frontier; omit --path-export, --path-check-export and --path-json");
    if (candidate.pathCheckExport && candidate.requestedPathExport)
        return failure("--path-check-export is read-only; omit --path-export");
    if (candidate.pathCheckExport && candidate.pathJson)
        return failure("--path-check-export reports text assessments; omit --path-json");
    if (candidate.pathFrontier && candidate.requestedPathExport)
        return failure("--path-frontier is read-only; choose one hop budget before exporting a path");
    if (candidate.pathJson && candidate.requestedPathExport)
        return failure("--path-json is read-only; omit --path-export");
    if (candidate.pathFrontier && candidate.requestedPathObjective
        && *candidate.requestedPathObjective != PresetPathObjective::GentlestSteps)
        return failure("--path-frontier uses gentlest hop/jump trade-offs; omit --path-objective or use gentlest");
    if (candidate.requestedJourneyOffset && candidate.selection.mode != InspectorMode::Journey)
        return failure("--journey-offset requires --journey TOUR");
    const bool mutation = candidate.selection.mode == InspectorMode::MutationPreview
        || candidate.selection.mode == InspectorMode::MutationExport;
    if (candidate.requestedMutationSeed && !mutation)
        return failure("--mutation-seed requires --mutation-preview ID or --mutation-export ID FILE.epimode");
    if (candidate.requestedMutationStrength && !mutation)
        return failure("--mutation-strength requires --mutation-preview ID or --mutation-export ID FILE.epimode");
    if (candidate.selection.mode == InspectorMode::MutationExport
        && std::filesystem::path(candidate.selection.second).extension() != ".epimode")
        return failure("Mutation export output must have the .epimode extension");
    if (candidate.requestedMorphSamples && candidate.selection.mode != InspectorMode::MorphStudy
        && candidate.selection.mode != InspectorMode::MorphStudyJson && candidate.selection.mode != InspectorMode::MorphFocus
        && candidate.selection.mode != InspectorMode::MorphFocusJson)
        return failure("--morph-samples requires --morph-study, --morph-study-json, --morph-focus or --morph-focus-json FROM TO");
    if (candidate.requestedMorphRefinements && candidate.selection.mode != InspectorMode::MorphFocus
        && candidate.selection.mode != InspectorMode::MorphFocusJson)
        return failure("--morph-refinements requires --morph-focus or --morph-focus-json FROM TO");
    if (candidate.requestedMorphPosition && candidate.selection.mode != InspectorMode::MorphExport
        && candidate.selection.mode != InspectorMode::MorphPreview)
        return failure("--morph-position requires --morph-preview FROM TO or --morph-export FROM TO FILE.epimode");
    if (candidate.selection.mode == InspectorMode::MorphExport
        && std::filesystem::path(candidate.selection.third).extension() != ".epimode")
        return failure("Morph export output must have the .epimode extension");
    return {.action = InspectorParseAction::Run, .options = std::move(candidate)};
}

} // namespace epi_sculpt::preset_inspector
