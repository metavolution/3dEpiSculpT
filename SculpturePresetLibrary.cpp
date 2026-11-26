#include "SculpturePresetLibrary.h"
#include "SculptureTourTiming.h"
#include "SculpturePresetRouteAdmission.h"

#include "SculptureModeProjection.h"
#include "SculptureSerializer.h"
#include "SculptureMorphChoreographer.h"
#include "SculptureModulationEngine.h"
#include "SculptureWorldRecipe.h"
#include <epi/io/bounded_text.hpp>
#include <epi/mode/mode_runtime_bridge.hpp>
#include <epi/pipeline/canonical_preset_pattern.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <unordered_set>
#include <utility>

namespace epi_sculpt {

namespace {

constexpr bool isExplicitPresetId(std::string_view id) noexcept {
    return id.starts_with("authored/") || id.starts_with("builtin/")
        || id.starts_with("pattern/") || id.starts_with("feedback/");
}

// FNV-1a is deliberately small and stable across platforms. It is provenance,
// not a security boundary: a readable source revision lets captures disclose
// exactly which authored recipe produced their embedded resolved state.
std::string contentRevision(std::string_view content) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char byte : content) {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    std::ostringstream out;
    out << "fnv1a64-" << std::hex << std::setfill('0') << std::setw(16) << hash;
    return out.str();
}

std::string readFile(const std::filesystem::path& path, const PresetLoadLimits& limits) {
    if (limits.maxBytes == 0U) return {};
    try {
        return epi::io::readBoundedText(path, limits.maxBytes, "preset");
    } catch (const std::runtime_error&) {
        return {};
    }
}

} // namespace

void SculpturePresetLibrary::appendBuiltIns() {
    for (const SculpturePreset& preset : sculpturePresets()) {
        const std::string id = "builtin/" + std::string(preset.name);
        std::string recipe = id + "\n" + std::string(preset.description) + "\n"
            + SculptureSerializer::serialize(preset.params);
        for (const std::string& route : preset.routingMap) recipe += route + "\n";
        SculpturePresetAsset asset{
            .id = id,
            .title = std::string(preset.name),
            .category = "built-in",
            .description = std::string(preset.description),
            .revision = contentRevision(recipe),
            .sourcePath = {},
            .params = preset.params,
            .worldRecipe = makeWorldRecipe(id, preset.params),
            .routingMap = preset.routingMap,
        };
        if (asset.worldRecipe.usesImplicitField()) {
            asset.presentationOwnership = SculpturePresentationOwnership::ImplicitField;
        } else if (asset.worldRecipe.needsDedicatedGenerator()) {
            const WorldCatalogEntry* catalogEntry = findWorldCatalogEntry(asset.id);
            if (catalogEntry != nullptr && !catalogEntry->hasCompleteRoute()) {
                continue;
            }
            asset.presentationOwnership = SculpturePresentationOwnership::DedicatedWorld;
        }
        entries_.push_back(std::move(asset));
    }
}

void SculpturePresetLibrary::appendRootPatternWorlds() {
    for (const PatternWorldBinding& binding : patternWorldBindings()) {
        const std::optional<epi::VisualMode> canonicalMode =
            mode_projection::fetchRootPatternMode(binding.patternId);
        const WorldCatalogEntry* const world =
            findPreferredWorldCatalogEntryForPattern(binding.patternId);
        if (!canonicalMode || world == nullptr) {
            diagnostics_.push_back(
                "root pattern world binding unavailable: " + std::string(binding.patternId));
            continue;
        }

        SculptureProgramParams projectedParams{};
        mode_projection::visualModeToSculptureParams(
            *canonicalMode, projectedParams);

        // A lightweight in-memory asset carries both truths together: exact
        // root Pattern/VisualMode mathematics plus its independent 3D spatial
        // projection. No copied .epimode or duplicate canonical schema exists.
        const std::string id = "pattern/" + std::string(binding.patternId);
        WorldRecipe worldRecipe = world->recipe();
        if (binding.mirrorTopologyOverride) {
            worldRecipe.mirrorTopology = *binding.mirrorTopologyOverride;
        }
        const std::string revisionSource = id + "\n" + std::string(world->canonicalPresetId)
            + "\n" + canonicalMode->serialize();
        SculpturePresetAsset asset{
            .id = id,
            .title = std::string(binding.patternId),
            .category = "root-pattern-world",
            .description = "root epimorphism pattern with an implemented 3D world route",
            .revision = contentRevision(revisionSource),
            .sourcePath = {},
            .params = projectedParams,
            .canonicalFeedbackMode = *canonicalMode,
            .worldRecipe = worldRecipe,
            .routingMap = {},
        };
        asset.presentationOwnership = worldRecipe.usesImplicitField()
            ? SculpturePresentationOwnership::ImplicitField
            : SculpturePresentationOwnership::DedicatedWorld;
        entries_.push_back(std::move(asset));
    }
}

void SculpturePresetLibrary::appendCanonicalFeedback() {
    for (const std::string_view key : epi::canonical_presets) {
        // Catch admission failures only: conversion, allocation and publication
        // failures must leave the existing catalog untouched.
        std::optional<epi::Pattern> pattern;
        try {
            pattern.emplace(epi::make_canonical_preset_pattern(key));
        } catch (const std::invalid_argument& error) {
            diagnostics_.push_back("canonical feedback preset skipped: " + std::string(key)
                + " [" + error.what() + "]");
            continue;
        } catch (const std::runtime_error& error) {
            diagnostics_.push_back("canonical feedback preset skipped: " + std::string(key)
                + " [" + error.what() + "]");
            continue;
        }

        epi::mode_command::RuntimeState runtime{};
        epi::VisualMode mode = epi::mode_bridge::mode_from_pattern_runtime(
            *pattern, 1, 1, runtime);
        mode.preset_name = std::string(key);
        SculptureProgramParams projectedParams{};
        mode_projection::visualModeToSculptureParams(mode, projectedParams);

        const std::string id = "feedback/" + std::string(key);
        const std::string revisionSource = id + "\ncanonical-feedback\n" + mode.serialize();
        SculpturePresetAsset asset{
            .id = id,
            .title = "Canonical feedback: " + std::string(key),
            .category = "canonical-feedback",
            .description = "admitted canonical epimorphism feedback pattern",
            .revision = contentRevision(revisionSource),
            .sourcePath = {},
            .params = projectedParams,
            .canonicalFeedbackMode = std::move(mode),
            .presentationOwnership = SculpturePresentationOwnership::CanonicalFeedback,
            // The feedback producer owns presentation; no world generator is claimed.
            .worldRecipe = {},
            .routingMap = {},
        };
        entries_.push_back(std::move(asset));
    }
}

bool SculpturePresetLibrary::reload(const std::filesystem::path& root,
                                    const std::filesystem::path& secondaryRoot) {
    return reload(root, secondaryRoot, PresetLoadLimits{});
}

bool SculpturePresetLibrary::reload(const std::filesystem::path& root,
                                    const std::filesystem::path& secondaryRoot,
                                    const PresetLoadLimits& limits) {
    SculpturePresetLibrary candidate;
    const bool discovered = candidate.loadCandidate(root, secondaryRoot, limits);
    if (discovered || entries_.empty()) {
        candidate.appendCanonicalFeedback();
    }
    if (!discovered && !entries_.empty()) {
        candidate.diagnostics_.push_back("preset reload failed; previous catalog retained");
    }
    // Admission happens before publication: failed discovery must not invalidate
    // asset pointers or replace a working catalog with an incomplete candidate.
    // Startup still needs built-ins when authored resources are unavailable.
    if (discovered || entries_.empty()) {
        entries_.swap(candidate.entries_);
        discoveredSourcePaths_.swap(candidate.discoveredSourcePaths_);
    }
    diagnostics_.swap(candidate.diagnostics_);
    return discovered;
}

bool SculpturePresetLibrary::loadCandidate(const std::filesystem::path& root,
                                           const std::filesystem::path& secondaryRoot,
                                           const PresetLoadLimits& limits) {
    appendBuiltIns();
    appendRootPatternWorlds();

    std::vector<std::filesystem::path> roots;
    std::error_code error;

    auto addRootIfValid = [&](const std::filesystem::path& candidate) {
        if (candidate.empty()) return;
        std::error_code ec;
        if (std::filesystem::is_directory(candidate, ec)) {
            for (const auto& existing : roots) {
                if (std::filesystem::equivalent(existing, candidate, ec)) return;
            }
            roots.push_back(candidate);
        }
    };

    addRootIfValid(root);
    addRootIfValid(secondaryRoot);

    if (roots.empty()) {
        diagnostics_.push_back("preset directory unavailable: " + root.string());
        return false;
    }

    struct FileItem {
        std::filesystem::path path;
        std::filesystem::path rootDir;
    };
    std::vector<FileItem> files;
    const auto limitExceeded = [&](std::string_view resource, std::size_t limit,
                                   const std::filesystem::path& context) {
        diagnostics_.push_back("preset catalog " + std::string(resource) + " limit exceeded ("
            + std::to_string(limit) + "): " + context.string());
        return false;
    };
    std::size_t visitedEntries = 0;
    for (const auto& r : roots) {
        auto discoveryFailed = [&] {
            diagnostics_.push_back("preset discovery failed in " + r.string()
                + ": " + error.message());
            return false;
        };
        // Do not hide inaccessible subdirectories: an incomplete traversal is
        // not an authoritative replacement for the currently published assets.
        std::filesystem::recursive_directory_iterator it(
            r, std::filesystem::directory_options::none, error), end;
        if (error) return discoveryFailed();
        while (it != end) {
            // Count all work before filtering by extension; a directory full of
            // unrelated files must not evade the host's traversal budget.
            if (visitedEntries == limits.maxDirectoryEntries)
                return limitExceeded("directory-entry", limits.maxDirectoryEntries, r);
            ++visitedEntries;
            if (static_cast<std::size_t>(it.depth()) > limits.maxDirectoryDepth)
                return limitExceeded("directory-depth", limits.maxDirectoryDepth, it->path());
            const bool regularFile = it->is_regular_file(error);
            if (error) return discoveryFailed();
            if (regularFile && it->path().extension() == ".epimode") {
                if (files.size() == limits.maxPresetFiles)
                    return limitExceeded("preset-file", limits.maxPresetFiles, r);
                files.push_back({it->path(), r});
            }
            it.increment(error);
            // Increment may move to end on failure, so inspect its error before
            // checking the iterator again rather than inside the next iteration.
            if (error) {
                return discoveryFailed();
            }
        }
    }
    std::sort(files.begin(), files.end(), [](const FileItem& a, const FileItem& b) {
        return a.path < b.path;
    });

    std::unordered_set<std::string> ids;
    for (const auto& entry : entries_) ids.insert(entry.id);
    // Keep the fallback separate until the entire authored candidate is admitted.
    // A late aggregate refusal at startup must not expose earlier decoded files.
    std::vector<SculpturePresetAsset> authoredEntries;
    std::size_t totalSourceBytes = 0;
    for (const auto& item : files) {
        const auto& path = item.path;
        const auto& rootDir = item.rootDir;
        const std::string source = readFile(path, limits);
        if (source.empty()) {
            diagnostics_.push_back((limits.maxBytes == 0U
                ? "preset source disabled by host policy: "
                : "preset source unreadable or exceeds host byte limit: ") + path.string());
            continue;
        }
        // Charge the owned snapshot, not a racy file-size estimate. Rejected
        // decodes and shadowed identities still consume acquisition work.
        if (source.size() > limits.maxTotalSourceBytes - totalSourceBytes)
            return limitExceeded("total-source-byte", limits.maxTotalSourceBytes, path);
        totalSourceBytes += source.size();
        // Decode and fingerprint one owned snapshot, never two opens of a live path.
        auto result = SculptureSerializer::decodePresetContentDetailed(source, path.stem().string());
        auto& decoded = result.preset;
        if (!decoded) {
            std::string message = "preset rejected: " + path.string() + " ["
                + std::string(presetDecodeErrorName(result.diagnostic.error));
            if (!result.diagnostic.field.empty()) message += "; field=" + result.diagnostic.field;
            diagnostics_.push_back(std::move(message) + "]");
            continue;
        }
        const std::filesystem::path relative = std::filesystem::relative(path, rootDir, error);
        if (error) {
            diagnostics_.push_back("preset path rejected: " + path.string());
            error.clear();
            continue;
        }
        std::filesystem::path idPath = relative;
        idPath.replace_extension();
        const std::string id = "authored/" + idPath.generic_string();
        if (!ids.insert(id).second) {
            continue;
        }
        if (decoded->presetName.empty()) decoded->presetName = path.stem().string();
        SculpturePresetAsset asset{
            .id = id,
            .title = std::move(decoded->presetName),
            .category = relative.has_parent_path() ? relative.parent_path().generic_string() : "authored",
            .description = "authored epimorphism world preset",
            .revision = contentRevision(source),
            .sourcePath = path,
            .params = decoded->params,
            .state = decoded->state,
            .presentationOwnership = decoded->presentationOwnership,
            .worldRecipe = decoded->declaredWorldRecipe ? *decoded->declaredWorldRecipe
                : makeWorldRecipe(id, decoded->params),
            .routingMap = std::move(decoded->routingMap),
            .authoredOnDisk = true,
            .macroAutomationJson = std::move(decoded->macroAutomationJson),
        };
        // Resolved WorldMode recipe/ownership is already authored intent. Only
        // legacy or recipe-absent files need catalog output-substrate inference.
        if (!decoded->declaredWorldRecipe && asset.presentationOwnership == SculpturePresentationOwnership::CompactInstances
            && asset.worldRecipe.usesImplicitField()) {
            asset.presentationOwnership = SculpturePresentationOwnership::ImplicitField;
        } else if (!decoded->declaredWorldRecipe && asset.presentationOwnership == SculpturePresentationOwnership::CompactInstances
            && asset.worldRecipe.needsDedicatedGenerator()) {
            const WorldCatalogEntry* catalogEntry = findWorldCatalogEntry(asset.id);
            if (catalogEntry != nullptr && !catalogEntry->hasCompleteRoute()) {
                diagnostics_.push_back("preset skipped (unimplemented route): " + id);
                continue;
            }
            asset.presentationOwnership = SculpturePresentationOwnership::DedicatedWorld;
        }
        authoredEntries.push_back(std::move(asset));
    }
    entries_.reserve(entries_.size() + authoredEntries.size());
    for (auto& asset : authoredEntries)
        entries_.push_back(std::move(asset));
    // Decoder admission does not grant permission to overwrite a rejected or
    // shadowed authored input. Retain the complete successful discovery at the
    // same publication boundary as the selectable entries, without rereading it.
    discoveredSourcePaths_.reserve(files.size());
    for (auto& item : files)
        discoveredSourcePaths_.push_back(std::move(item.path));
    return true;
}

const SculpturePresetAsset* SculpturePresetLibrary::at(std::size_t index) const noexcept {
    return index < entries_.size() ? &entries_[index] : nullptr;
}

std::optional<std::size_t> SculpturePresetLibrary::findIndex(
    std::string_view idTitleOrAlias) const noexcept {
    // Friendly world names are aliases of authored assets, not duplicate
    // parameter blobs. A bound root pattern wins before a generic alias so it
    // retains its own translated epimorphism math. Catalog aliases otherwise
    // preserve the canonical epimode's exact parameters, routing map, state,
    // and content revision. Explicitly namespaced queries skip alias resolution;
    // this interactive lookup still accepts titles in its final scan. Durable
    // identity consumers use resolveExactId instead.
    const bool explicitId = isExplicitPresetId(idTitleOrAlias);
    if (!explicitId && findPatternWorldBinding(idTitleOrAlias) != nullptr) {
        for (std::size_t index = 0U; index < entries_.size(); ++index) {
            const std::string& id = entries_[index].id;
            constexpr std::string_view kPatternIdPrefix = "pattern/";
            if (id.starts_with(kPatternIdPrefix)
                && std::string_view(id).substr(kPatternIdPrefix.size()) == idTitleOrAlias) {
                return index;
            }
        }
    }

    const WorldCatalogEntry* catalogEntry =
        findAvailableWorldCatalogEntry(idTitleOrAlias);
    if (catalogEntry != nullptr && !explicitId) {
        for (std::size_t index = 0U; index < entries_.size(); ++index) {
            if (entries_[index].id == catalogEntry->canonicalPresetId) return index;
        }
    }

    for (std::size_t index = 0U; index < entries_.size(); ++index) {
        const SculpturePresetAsset& entry = entries_[index];
        if (entry.id == idTitleOrAlias || entry.title == idTitleOrAlias) {
            return index;
        }
    }
    return std::nullopt;
}

const SculpturePresetAsset* SculpturePresetLibrary::find(
    std::string_view idOrTitle) const noexcept {
    const std::optional<std::size_t> index = findIndex(idOrTitle);
    return index ? at(*index) : nullptr;
}

bool SculpturePresetLibrary::loadSequenceFromFile(
    const std::filesystem::path& path,
    SculptureSequencer& sequencer) const {
    return loadSequenceFromFile(path, sequencer, SequenceLoadLimits{});
}

bool SculpturePresetLibrary::loadSequenceFromFile(
    const std::filesystem::path& path,
    SculptureSequencer& sequencer,
    const SequenceLoadLimits& limits) const {
    return SculptureSerializer::loadSequenceFromFile(
        path.string(), sequencer,
        [this](std::string_view id, SculptureProgramParams& params,
               SculptureWorldState& state, std::vector<std::string>& routingMap) {
            auto identity = resolveExactId(id);
            // Sequence references must identify one asset before supplying its
            // values. Friendly discovery cannot repair a missing namespaced ID
            // or select the first of several assets with the same stable ID.
            if (identity.status == SculpturePresetIdStatus::Missing
                && !id.empty() && !isExplicitPresetId(id)) {
                if (const auto* friendlyAsset = find(id)) {
                    identity = resolveExactId(friendlyAsset->id);
                }
            }
            const SculpturePresetAsset* asset = identity.asset;
            if (!asset) return false;
            params = asset->params;
            state = asset->state;
            routingMap = asset->routingMap;
            return true;
        }, limits);
}

std::vector<std::string> SculpturePresetLibrary::categories() const {
    std::unordered_set<std::string> uniqueCats;
    for (const auto& entry : entries_) {
        if (!entry.category.empty()) {
            uniqueCats.insert(entry.category);
        }
    }
    std::vector<std::string> result(uniqueCats.begin(), uniqueCats.end());
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<const SculpturePresetAsset*> SculpturePresetLibrary::findByCategory(std::string_view category) const {
    std::vector<const SculpturePresetAsset*> matches;
    for (const auto& entry : entries_) {
        if (entry.category == category) {
            matches.push_back(&entry);
        }
    }
    return matches;
}

std::vector<std::string> SculpturePresetLibrary::availableTours() const {
    return {
        "clifford_hopf_study",
        "hyperdimensional_fractal_odyssey",
        "higher_dimensional_soliton_odyssey",
        "non_euclidean_poincare_clifford_odyssey",
        "hyperdimensional_4d_projections_odyssey",
        "quantum_quasicrystal_foliation_odyssey",
        "higher_dimensional_fibration_odyssey",
        "higher_dimensional_calabi_yau_odyssey",
        "hyperdimensional_quantum_soliton_voyage",
        "scientific_dynamic_systems_odyssey",
        "root_pattern_mirror_chambers",
        "all_root_pattern_worlds",
        "canonical_vision_frontiers",
        "canonical_vision_presets",
        "canonical_3d_world_suite",
        "higher_dimensional_opencl_worlds",
        "dimensional_horizons",
        "topological_soliton_homotopy_odyssey",
        "hyperbolic_poincare_honeycomb_voyage",
        "hyperdimensional_homotopy_slerp_frontiers",
        "non_euclidean_relief_frontiers",
        "hyperdimensional_polytope_frontiers",
        "hyperdimensional_master_horizons",
        "hyperdimensional_weyl_mandelbulb_frontiers",
        "hyperdimensional_soliton_frontiers",
        "higher_dimensional_lie_groups",
        "quantum_topological_solitons",
        "chaotic_kinetics",
        "epimorphic_masterpieces",
        "hyperbolic_exponential_vortices",
        "cosmological_geodesics",
        "modular_conformal_masterpieces",
        "quantum_gravitational_singularities",
        "dimensional_manifolds_odyssey",
        "vision_dimensional_frontiers",
        "reaction_diffusion_and_chaos",
        "quantum_and_cosmology",
        "morphogenesis_and_chaos",
        "quasicrystals_and_aperiodic_tilings",
        "minimal_surfaces_and_solitons",
        "exceptional_geometries_and_holonomy",
        "non_orientable_and_minimal_surfaces",
        "hyperdimensional_polytopes_4d",
        "all_authored",
        "all_builtins",
    };
}

std::size_t SculptureTourInspection::readyCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(entries.begin(), entries.end(),
        [](const SculptureTourEntry& entry) {
            return entry.status == SculptureTourEntryStatus::Ready;
        }));
}

SculptureTourEntry SculpturePresetLibrary::inspectTourEntry(std::string_view presetId) const {
    auto identity = resolveExactId(presetId);
    std::string_view resolvedId = presetId;
    // Friendly discovery is allowed only before stable identity admission. An
    // exact duplicate or missing known namespace must never pick a title winner.
    if (identity.status == SculpturePresetIdStatus::Missing
        && !presetId.empty() && !isExplicitPresetId(presetId)) {
        if (const auto* friendlyAsset = find(presetId)) {
            resolvedId = friendlyAsset->id;
            identity = resolveExactId(resolvedId);
        }
    }
    if (identity.status == SculpturePresetIdStatus::Ambiguous) {
        return {std::string(resolvedId), {}, SculptureTourEntryStatus::AmbiguousAsset};
    }
    const auto* asset = identity.asset;
    if (asset == nullptr) {
        return {std::string(presetId), {}, SculptureTourEntryStatus::MissingAsset};
    }
    const auto status = hasImplementedPresetRoute(*asset)
        ? SculptureTourEntryStatus::Ready : SculptureTourEntryStatus::UnsupportedRoute;
    return {asset->id, asset->title, status};
}

SculptureTourInspection SculpturePresetLibrary::inspectCategoryTour(std::string_view category) const {
    SculptureTourInspection result;
    for (const auto* asset : findByCategory(category)) {
        result.entries.push_back(inspectTourEntry(asset->id));
    }
    result.recognized = !result.entries.empty();
    return result;
}

SculptureTourInspection SculpturePresetLibrary::inspectCuratedTour(std::string_view tourName) const {
    std::vector<std::string_view> idList;
    if (tourName == "clifford_hopf_study") {
        // A compact artistic study with matching discrete math/geometry selectors.
        // Keep the authored endpoints and the ordinary smoothstep/dwell policy.
        idList = {
            "authored/dimensional/100_clifford_torus_stereographic_projection",
            "authored/dimensional/113_so4_clifford_quantum_hopf_fibration",
        };
    } else if (tourName == "hyperdimensional_fractal_odyssey") {
        idList = {
            "authored/mathematical/44_quantum_volumetric_calabi_yau",
            "authored/mathematical/45_hyperbolic_poincare_honeycomb",
            "authored/mathematical/46_dynamic_4d_quaternion_julia",
            "authored/mathematical/47_dynamic_4d_hyper_mandelbulb",
            "authored/mathematical/26_clifford_hopf",
            "authored/mathematical/24_calabi_yau",
            "authored/mathematical/19_quat_julia",
        };
    } else if (tourName == "higher_dimensional_soliton_odyssey") {
        idList = {
            "authored/dimensional/140_so4_clifford_hopf_fibration_4d",
            "authored/dimensional/141_poincare_dodecahedral_homotopy_4d",
            "authored/dimensional/142_calabi_yau_threefold_quintic_35d",
            "authored/dimensional/143_e8_coxeter_plane_projection_4d",
            "authored/dimensional/144_belousov_scroll_wave_filament_3d",
            "authored/dimensional/145_seifert_hyperbolic_knot_soliton_35d",
            "authored/dimensional/146_poincare_hyperbolic_canopy_25d",
            "authored/dimensional/147_exponential_rosette_cascade_35d",
            "authored/dimensional/148_clifford_hopf_foliation_4d",
        };
    } else if (tourName == "non_euclidean_poincare_clifford_odyssey") {
        idList = {
            "authored/dimensional/108_poincare_hyperbolic_disk_reduction_4d",
            "authored/dimensional/109_clifford_foliation_hyper_torus_35d",
            "authored/dimensional/110_hyperbolic_to_clifford_homotopy_4d",
            "authored/dimensional/71_hyperbolic_poincare_geodesic_honeycomb",
            "authored/dimensional/80_clifford_so4_isoclinic_fibration",
        };
    } else if (tourName == "hyperdimensional_4d_projections_odyssey") {
        idList = {
            "authored/dimensional/100_clifford_torus_stereographic_projection",
            "authored/dimensional/101_calabi_yau_cross_cap_foliation",
            "authored/dimensional/102_hopf_fibration_quantum_vortex",
            "authored/dimensional/103_hyperbolic_icosahedral_honeycomb_4d",
            "authored/dimensional/104_epimorphic_complex_plane_relief_25d",
            "authored/dimensional/80_clifford_so4_isoclinic_fibration",
        };
    } else if (tourName == "quantum_quasicrystal_foliation_odyssey") {
        idList = {
            "authored/dimensional/85_ammann_beenker_quasicrystal_lattice",
            "authored/kinetic/87_chladni_quantum_membrane",
            "authored/cosmological/88_lichtenberg_discharge_forest",
            "authored/dimensional/86_120_cell_quantum_hologram",
            "authored/dimensional/80_clifford_so4_isoclinic_fibration",
            "authored/kinetic/81_kuramoto_lattice_synchronization",
        };
    } else if (tourName == "higher_dimensional_fibration_odyssey") {
        idList = {
            "authored/dimensional/93_hopf_clifford_fibration_superstructure",
            "authored/dimensional/94_calabi_yau_mirror_quintic_lattice",
            "authored/dimensional/130_calabi_yau_manifold",
            "authored/dimensional/95_quaternionic_julia_4d_hypercross",
            "authored/dimensional/96_octonionic_exceptional_e8_weyl_cathedral",
        };
    } else if (tourName == "higher_dimensional_calabi_yau_odyssey") {
        idList = {
            "authored/dimensional/89_penrose_amorphous_quasicrystal",
            "authored/dimensional/90_hyperbolic_seifert_surface",
            "authored/kinetic/91_octonionic_fano_associator",
            "authored/cosmological/92_calabi_yau_quintic_threefold",
            "authored/cosmological/93_schwarzschild_black_hole",
        };
    } else if (tourName == "hyperdimensional_quantum_soliton_voyage") {
        idList = {
            "authored/dimensional/86_120_cell_quantum_hologram",
            "authored/kinetic/87_chladni_quantum_membrane",
            "authored/cosmological/88_lichtenberg_discharge_forest",
            "authored/cosmological/89_epimorphic_gyroid_maze",
        };
    } else if (tourName == "scientific_dynamic_systems_odyssey") {
        idList = {
            "authored/cosmological/83_golden_phyllotaxis_cathedral",
            "authored/kinetic/84_rossler_lorenz_strange_attractor",
            "authored/dimensional/85_ammann_beenker_quasicrystal_lattice",
            "authored/kinetic/97_gray_scott_reaction_diffusion",
        };
    } else if (tourName == "root_pattern_mirror_chambers") {
        // A bounded live-performance arc: the same root-pattern mathematics
        // travels through the distinct finite reflection grammars carried by
        // immutable world delivery, then opens into its related 3D studies.
        idList = {
            "pattern/hyperbolic_mirror_kaleidoscope",
            "pattern/tanh_infinity_mirror",
            "pattern/tanh_honeycomb_tunnel",
            "pattern/tanh_pentagonal_kaleidoscope",
            "pattern/tanh_nonagonal_mirror_hall",
            "pattern/exp_dodecagonal_breathing_vault",
            "pattern/cube_viral_capsid_bloom",
            "pattern/weierstrass_fractal_foam",
            "pattern/schwarzschild_gravitational_lens",
        };
    } else if (tourName == "canonical_vision_frontiers") {
        idList = {
            "authored/dimensional/78_hyperbolic_canopy_petal_arcs",
            "authored/dimensional/79_exponential_radial_starburst_cascade",
            "authored/dimensional/80_clifford_so4_isoclinic_fibration",
            "authored/kinetic/81_kuramoto_lattice_synchronization",
            "authored/dimensional/82_epimorphic_torus_chamber_relief",
            "authored/dimensional/185_conformal_hyperbolic_rose_petal_relief_25d",
            "authored/dimensional/186_clifford_vortex_kaleidoscope_mandala_35d",
            "authored/dimensional/187_dedekind_eta_quartic_soliton_lattice_4d",
            "authored/dimensional/188_kuramoto_hyperbolic_synchronization_canopy_35d",
            "authored/dimensional/189_so4_isoclinic_penrose_twistor_foliation_4d",
            "authored/dimensional/190_dirac_monopole_quantum_tunnel",
            "authored/dimensional/191_su2_yang_mills_instanton",
            "authored/dimensional/192_weierstrass_fractal_foam_35d",
            "authored/dimensional/193_talbot_moire_carpet_flux_4d",
            "authored/dimensional/194_feigenbaum_attractor_cascade_35d",
            "authored/dimensional/195_schwarzschild_gravitational_lens_4d",
            "authored/dimensional/196_hofstadter_butterfly_quantum_lattice_35d",
            "authored/dimensional/197_rayleigh_benard_convection_rolls_35d",
        };
    } else if (tourName == "canonical_vision_presets") {
        idList = {
            "authored/mathematical/hyperbolic_spiral_vision",
            "authored/mathematical/exponential_fractal_vision",
            "authored/mathematical/polar_kaleidoscope_vision",
            "authored/mathematical/quantum_calabi_yau",
        };
    } else if (tourName == "canonical_3d_world_suite") {
        idList = {
            "authored/dimensional/200_neon_bubblesphere",
            "authored/dimensional/201_kuramoto_lattice",
            "authored/dimensional/202_clifford_hopf",
            "authored/dimensional/203_golden_phyllotaxis",
            "authored/dimensional/204_rossler_lanterns",
            "authored/dimensional/205_cell120_hologram",
            "authored/dimensional/206_ammann_beenker",
            "authored/dimensional/207_chladni_membrane",
            "authored/dimensional/208_lichtenberg_forest",
            "authored/dimensional/209_gyroid_maze",
            "authored/dimensional/210_hypercube_tesseract",
        };
    } else if (tourName == "higher_dimensional_opencl_worlds") {
        idList = {
            "authored/dimensional/180_clifford_hopf_multi_soliton_lattice_4d",
            "authored/dimensional/181_hecatonicosachoron_120cell_coxeter_4d",
            "authored/dimensional/182_hexacosichoron_600cell_h4_polytope_4d",
            "authored/dimensional/183_hyperbolic_icosahedral_honeycomb_horosphere_4d",
            "authored/dimensional/184_lie_e8_root_lattice_coxeter_projection_4d",
            "authored/dimensional/185_conformal_hyperbolic_rose_petal_relief_25d",
            "authored/dimensional/186_clifford_vortex_kaleidoscope_mandala_35d",
            "authored/dimensional/187_dedekind_eta_quartic_soliton_lattice_4d",
            "authored/dimensional/188_kuramoto_hyperbolic_synchronization_canopy_35d",
            "authored/dimensional/189_so4_isoclinic_penrose_twistor_foliation_4d",
            "authored/dimensional/190_dirac_monopole_quantum_tunnel",
            "authored/dimensional/191_su2_yang_mills_instanton",
            "authored/dimensional/192_weierstrass_fractal_foam_35d",
            "authored/dimensional/193_talbot_moire_carpet_flux_4d",
            "authored/dimensional/194_feigenbaum_attractor_cascade_35d",
            "authored/dimensional/195_schwarzschild_gravitational_lens_4d",
            "authored/dimensional/196_hofstadter_butterfly_quantum_lattice_35d",
            "authored/dimensional/197_rayleigh_benard_convection_rolls_35d",
        };
    } else if (tourName == "dimensional_horizons") {
        idList = {
            "authored/dimensional/01_hopf_lantern",
            "authored/dimensional/02_tesseract_bloom",
            "authored/dimensional/03_quaternion_julia_chapel",
            "authored/dimensional/04_octonion_orbital_weave",
            "authored/dimensional/10_penrose_quasicrystal_hologram",
            "authored/dimensional/11_enneper_minimal_surface",
        };
    } else if (tourName == "topological_soliton_homotopy_odyssey") {
        idList = {
            "authored/dimensional/16_so4_clifford_torus_homotopy",
            "authored/dimensional/25_hyperbolic_hopf_instanton",
            "authored/kinetic/27_quantum_hall_skyrmion_vortex",
            "authored/dimensional/42_clifford_hopf_super_soliton",
            "authored/dimensional/47_non_abelian_yang_mills_monopole",
            "authored/dimensional/48_hyperbolic_poincare_soliton_lattice",
        };
    } else if (tourName == "hyperbolic_poincare_honeycomb_voyage") {
        idList = {
            "authored/dimensional/71_hyperbolic_poincare_geodesic_honeycomb",
            "authored/dimensional/64_calabi_yau_non_euclidean_geodesic_flow",
            "authored/dimensional/65_kuramoto_hyperbolic_sync_soliton",
            "authored/dimensional/66_quaternion_hyperbolic_foliation",
            "authored/dimensional/67_hopf_clifford_fibers",
            "authored/dimensional/69_epimorphic_chamber_relief",
        };
    } else if (tourName == "hyperdimensional_homotopy_slerp_frontiers") {
        idList = {
            "authored/dimensional/72_rose_homotopy_hyper_soliton",
            "authored/dimensional/73_kaleidoscopic_chladni_quasicrystal",
            "authored/kinetic/74_wave_ring_quantum_skyrmion",
            "authored/dimensional/75_breathing_hyperbolic_canopy",
            "authored/dimensional/76_cross_seed_jacobi_elliptic_soliton",
            "authored/cosmological/77_learn_mode_six_stage_decomposition",
        };
    } else if (tourName == "non_euclidean_relief_frontiers") {
        idList = {
            "authored/dimensional/63_epimorphic_poincare_hyper_relief",
            "authored/dimensional/64_calabi_yau_non_euclidean_geodesic_flow",
            "authored/dimensional/65_kuramoto_hyperbolic_sync_soliton",
            "authored/dimensional/66_quaternion_hyperbolic_foliation",
            "authored/dimensional/67_hopf_clifford_fibers",
            "authored/kinetic/68_kuramoto_phase_lattice_sync",
            "authored/dimensional/69_epimorphic_chamber_relief",
            "authored/dimensional/70_octonion_associator_calabi_flow",
            "authored/dimensional/71_hyperbolic_poincare_geodesic_honeycomb",
        };
    } else if (tourName == "hyperdimensional_polytope_frontiers") {
        idList = {
            "authored/dimensional/46_octonionic_cayley_dickson_fibration",
            "authored/dimensional/47_non_abelian_yang_mills_monopole",
            "authored/dimensional/48_hyperbolic_poincare_soliton_lattice",
            "authored/kinetic/49_bioluminescent_morphogenetic_wave_front",
            "authored/cosmological/50_chladni_nodal_cymatic_resonator",
        };
    } else if (tourName == "hyperdimensional_weyl_mandelbulb_frontiers") {
        idList = {
            "authored/dimensional/51_quaternionic_mandelbulb_foliation",
            "authored/dimensional/52_klein_bottle_non_orientable_immersion",
            "authored/dimensional/53_lie_e8_weyl_chamber_reflections",
            "authored/kinetic/54_kuramoto_lattice_sync_attractor",
            "authored/cosmological/55_hyperbolic_beltrami_pseudosphere_vortex",
            "authored/dimensional/56_quaternionic_ribbon_braid_foliation",
        };
    } else if (tourName == "hyperdimensional_soliton_frontiers") {
        idList = {
            "authored/dimensional/57_boys_surface_non_orientable_homotopy",
            "authored/dimensional/58_roman_surface_cross_cap_singularity",
            "authored/kinetic/59_hopf_fibration_soliton_lattice",
            "authored/dimensional/60_lie_g2_exceptional_holonomy_manifold",
            "authored/cosmological/61_quaternionic_calabi_yau_3fold",
            "authored/cosmological/62_super_helical_twistor_string_cavity",
        };
    } else if (tourName == "hyperdimensional_master_horizons") {
        idList = {
            "authored/dimensional/43_calabi_yau_quintic_mirror_knot",
            "authored/kinetic/44_chladni_turing_quasiconformal_foliation",
            "authored/cosmological/45_twistor_kerr_robinson_congruence",
            "authored/dimensional/40_calabi_yau_mirror_symmetry",
            "authored/dimensional/41_hyperbolic_beltrami_pseudosphere",
            "authored/dimensional/42_clifford_hopf_super_soliton",
            "authored/cosmological/39_kerr_newman_ergosphere_singularity",
            "authored/dimensional/35_hyperbolic_canopy_sheets",
        };
    } else if (tourName == "higher_dimensional_lie_groups") {
        idList = {
            "authored/dimensional/16_so4_clifford_torus_homotopy",
            "authored/dimensional/17_g2_octonion_exceptional_fibration",
            "authored/dimensional/18_calabi_yau_quintic_threefold",
            "authored/dimensional/19_hypertorus_hopf_soliton",
            "authored/dimensional/20_seifert_hyperbolic_soliton",
            "authored/dimensional/21_calabi_yau_quintic_variety",
            "authored/dimensional/22_lie_e8_coxeter_projection",
            "authored/dimensional/24_e8_coxeter_homotopy_lattice",
        };
    } else if (tourName == "quantum_topological_solitons") {
        idList = {
            "authored/dimensional/25_hyperbolic_hopf_instanton",
            "authored/dimensional/26_noncommutative_torus_foliation",
            "authored/kinetic/27_quantum_hall_skyrmion_vortex",
            "authored/cosmological/28_kerr_newman_ergosphere_horizon",
            "authored/dimensional/29_langlands_automorphic_cusp_form",
            "authored/dimensional/30_quasicrystal_icosahedral_projection",
        };
    } else if (tourName == "chaotic_kinetics") {
        idList = {
            "authored/kinetic/05_lorenz_silk_atrium",
            "authored/kinetic/06_turing_bioluminescent_reef",
            "authored/kinetic/07_chladni_hypercube_chorus",
            "authored/kinetic/12_rossler_hyperbolic_ribbon",
            "authored/kinetic/13_feigenbaum_period_web",
        };
    } else if (tourName == "epimorphic_masterpieces") {
        idList = {
            "authored/dimensional/31_hopf_clifford_instanton",
            "authored/dimensional/32_noncommutative_calabi_yau_3fold",
            "authored/cosmological/33_schwarzschild_ergosphere_vortex",
            "authored/kinetic/34_turing_chladni_cymatic_resonance",
            "authored/dimensional/35_hyperbolic_canopy_sheets",
            "authored/dimensional/36_exponential_starburst_cascade",
            "authored/kinetic/37_four_dimensional_rose_homotopy",
            "authored/cosmological/38_logarithmic_spiral_vortex",
            "authored/dimensional/25_hyperbolic_hopf_instanton",
            "authored/dimensional/18_calabi_yau_quintic_threefold",
            "authored/dimensional/24_e8_coxeter_homotopy_lattice",
        };
    } else if (tourName == "hyperbolic_exponential_vortices") {
        idList = {
            "authored/dimensional/35_hyperbolic_canopy_sheets",
            "authored/dimensional/36_exponential_starburst_cascade",
            "authored/kinetic/37_four_dimensional_rose_homotopy",
            "authored/cosmological/38_logarithmic_spiral_vortex",
            "authored/dimensional/20_seifert_hyperbolic_soliton",
            "authored/cosmological/09_hyperbolic_mirror_weather",
        };
    } else if (tourName == "cosmological_geodesics") {
        idList = {
            "authored/cosmological/08_calabi_yau_aurora",
            "authored/cosmological/09_hyperbolic_mirror_weather",
            "authored/cosmological/14_schwarzschild_geodesic_lens",
            "authored/cosmological/15_apollonian_gasket_cathedral",
            "authored/cosmological/23_seifert_hyperbolic_aurora",
            "authored/cosmological/38_logarithmic_spiral_vortex",
        };
    } else if (tourName == "modular_conformal_masterpieces") {
        idList = {
            "authored/mathematical/105_dedekind_eta_modular_cathedral",
            "authored/cosmological/106_lin_shu_galactic_density_wave",
            "authored/mathematical/107_ramanujan_mock_theta_web",
            "authored/dimensional/108_poincare_hyperbolic_vortex_chamber",
            "authored/kinetic/109_joukowsky_conformal_airfoil_manifold",
        };
    } else if (tourName == "quantum_gravitational_singularities") {
        idList = {
            "authored/cosmological/110_schwarzschild_photon_sphere_lensing",
            "authored/kinetic/111_kuramoto_synchronization_hypermanifold",
            "authored/dimensional/112_quartic_pinwheel_attractor_relief",
            "authored/dimensional/103_hyperbolic_icosahedral_honeycomb_4d",
            "authored/dimensional/101_calabi_yau_cross_cap_foliation",
        };
    } else if (tourName == "dimensional_manifolds_odyssey") {
        idList = {
            "authored/dimensional/104_epimorphic_complex_plane_relief_25d",
            "authored/dimensional/117_talbot_moire_quantum_carpet_relief_25d",
            "authored/dimensional/119_chladni_nodal_resonance_relief_25d",
            "authored/dimensional/116_rossler_chaotic_ribbon_attractor_35d",
            "authored/dimensional/118_lin_shu_density_wave_galactic_4d",
            "authored/dimensional/120_penrose_quasicrystal_inflation_4d",
            "authored/dimensional/121_apollonian_gasket_spherepack_3d",
            "authored/dimensional/122_conformal_wing_aerofoil_relief_25d",
            "authored/dimensional/123_quartic_spiral_pinwheel_foliation_4d",
            "authored/dimensional/124_hofstadter_butterfly_quantum_lattice_35d",
            "authored/dimensional/125_chirikov_kam_island_chain_35d",
            "authored/dimensional/126_allen_cahn_domain_coarsening_25d",
            "authored/dimensional/127_cube_gluon_flux_mandala_4d",
            "authored/dimensional/128_turing_morphogenesis_labyrinth_3d",
            "authored/dimensional/129_harper_hofstadter_butterfly_lattice_4d",
            "authored/dimensional/130_schwarzschild_photon_sphere_lensing_relief_25d",
            "authored/dimensional/131_lorenz_waterwheel_strange_attractor_35d",
            "authored/dimensional/132_langlands_automorphic_cusp_form_4d",
            "authored/dimensional/133_dirac_monopole_hopf_fibration_relief_25d",
        };
    } else if (tourName == "vision_dimensional_frontiers") {
        idList = {
            "authored/dimensional/128_turing_morphogenesis_labyrinth_3d",
            "authored/dimensional/129_harper_hofstadter_butterfly_lattice_4d",
            "authored/dimensional/130_schwarzschild_photon_sphere_lensing_relief_25d",
            "authored/dimensional/121_apollonian_gasket_spherepack_3d",
            "authored/dimensional/122_conformal_wing_aerofoil_relief_25d",
            "authored/dimensional/123_quartic_spiral_pinwheel_foliation_4d",
            "authored/dimensional/124_hofstadter_butterfly_quantum_lattice_35d",
            "authored/dimensional/127_cube_gluon_flux_mandala_4d",
            "authored/dimensional/104_epimorphic_complex_plane_relief_25d",
            "authored/dimensional/100_clifford_torus_stereographic_projection",
        };
    } else if (tourName == "reaction_diffusion_and_chaos") {
        idList = {
            "authored/dimensional/115_belousov_zhabotinsky_spiral_waves",
            "authored/dimensional/116_rossler_chaotic_ribbon_attractor_35d",
            "authored/dimensional/125_chirikov_kam_island_chain_35d",
            "authored/dimensional/126_allen_cahn_domain_coarsening_25d",
            "authored/kinetic/84_rossler_lorenz_strange_attractor",
            "authored/kinetic/97_gray_scott_reaction_diffusion",
            "authored/kinetic/34_turing_chladni_cymatic_resonance",
        };
    } else if (tourName == "quantum_and_cosmology") {
        idList = {
            "authored/quantum/170_hydrogen_atom_orbitals",
            "authored/dimensional/129_harper_hofstadter_butterfly_lattice_4d",
            "authored/dimensional/130_schwarzschild_photon_sphere_lensing_relief_25d",
            "authored/dimensional/133_dirac_monopole_hopf_fibration_relief_25d",
            "authored/cosmological/110_schwarzschild_photon_sphere_lensing",
            "authored/dimensional/102_hopf_fibration_quantum_vortex",
        };
    } else if (tourName == "morphogenesis_and_chaos") {
        idList = {
            "authored/dimensional/128_turing_morphogenesis_labyrinth_3d",
            "authored/dimensional/131_lorenz_waterwheel_strange_attractor_35d",
            "authored/dimensional/132_langlands_automorphic_cusp_form_4d",
            "authored/dimensional/115_belousov_zhabotinsky_spiral_waves",
            "authored/dimensional/116_rossler_chaotic_ribbon_attractor_35d",
            "authored/dimensional/126_allen_cahn_domain_coarsening_25d",
        };
    } else if (tourName == "quasicrystals_and_aperiodic_tilings") {
        idList = {
            "authored/dimensional/134_penrose_p3_quasicrystal_inflation_4d",
            "authored/dimensional/138_ammann_beenker_quasicrystal_octagonal_4d",
            "authored/dimensional/129_harper_hofstadter_butterfly_lattice_4d",
            "authored/dimensional/103_hyperbolic_icosahedral_honeycomb_4d",
            "authored/dimensional/183_hyperbolic_icosahedral_honeycomb_horosphere_4d",
            "authored/dimensional/127_cube_gluon_flux_mandala_4d",
        };
    } else if (tourName == "minimal_surfaces_and_solitons") {
        idList = {
            "authored/dimensional/136_cube_enneper_minimal_surface_relief_25d",
            "authored/dimensional/137_laughlin_fractional_hall_vortex_soliton_4d",
            "authored/dimensional/139_beltrami_hyperbolic_pseudosphere_relief_25d",
            "authored/dimensional/105_catenoid_minimal_arc_relief_25d",
            "authored/dimensional/135_feigenbaum_period_doubling_ribbon_35d",
            "authored/dimensional/133_dirac_monopole_hopf_fibration_relief_25d",
        };
    } else if (tourName == "exceptional_geometries_and_holonomy") {
        idList = {
            "authored/dimensional/149_kummer_quartic_nodal_surface_4d",
            "authored/dimensional/150_octonionic_fibration_g2_holonomy_35d",
            "authored/dimensional/152_klein_quartic_genus3_foliation_4d",
            "authored/dimensional/153_seifert_fibered_homology_sphere_35d",
            "authored/dimensional/180_clifford_hopf_multi_soliton_lattice_4d",
            "authored/dimensional/181_hecatonicosachoron_120cell_coxeter_4d",
            "authored/dimensional/182_hexacosichoron_600cell_h4_polytope_4d",
            "authored/dimensional/184_lie_e8_root_lattice_coxeter_projection_4d",
            "authored/dimensional/101_calabi_yau_cross_cap_foliation",
            "authored/dimensional/107_dedekind_eta_modular_cathedral_35d",
        };
    } else if (tourName == "non_orientable_and_minimal_surfaces") {
        idList = {
            "authored/dimensional/151_boys_surface_projective_plane_relief_25d",
            "authored/dimensional/154_costa_minimal_surface_relief_25d",
            "authored/dimensional/136_cube_enneper_minimal_surface_relief_25d",
            "authored/dimensional/139_beltrami_hyperbolic_pseudosphere_relief_25d",
            "authored/dimensional/105_catenoid_minimal_arc_relief_25d",
            "authored/dimensional/137_laughlin_fractional_hall_vortex_soliton_4d",
        };
    } else if (tourName == "hyperdimensional_polytopes_4d") {
        idList = {
            "authored/dimensional/180_clifford_hopf_multi_soliton_lattice_4d",
            "authored/dimensional/181_hecatonicosachoron_120cell_coxeter_4d",
            "authored/dimensional/182_hexacosichoron_600cell_h4_polytope_4d",
            "authored/dimensional/183_hyperbolic_icosahedral_honeycomb_horosphere_4d",
            "authored/dimensional/184_lie_e8_root_lattice_coxeter_projection_4d",
            "authored/dimensional/100_clifford_torus_stereographic_projection",
            "authored/dimensional/103_hyperbolic_icosahedral_honeycomb_4d",
        };
    } else if (tourName == "all_authored") {
        SculptureTourInspection result{.recognized = true};
        for (const auto& entry : entries_) {
            if (entry.authoredOnDisk) {
                result.entries.push_back(inspectTourEntry(entry.id));
            }
        }
        return result;
    } else if (tourName == "all_root_pattern_worlds") {
        auto result = inspectCategoryTour("root-pattern-world");
        result.recognized = true;
        return result;
    } else if (tourName == "all_builtins") {
        auto result = inspectCategoryTour("built-in");
        result.recognized = true;
        return result;
    } else {
        return inspectCategoryTour(tourName);
    }

    SculptureTourInspection result{.recognized = true};
    result.entries.reserve(idList.size());
    for (const auto id : idList) {
        result.entries.push_back(inspectTourEntry(id));
    }
    return result;
}

bool SculpturePresetLibrary::buildInspectedTour(
    SculptureMorphChoreographer& choreographer,
    const SculptureTourInspection& inspection,
    float durationSeconds,
    float dwellSeconds,
    MorphEasing easing) const {
    const auto ready = inspection.readyCount();
    if (!inspection.recognized || ready == 0
        || !std::isfinite(durationSeconds) || durationSeconds < 0.0f
        || !std::isfinite(dwellSeconds) || dwellSeconds < 0.0f) return false;

    // Keep the accumulated spline clock representable as well as each cue.
    float total = 0.0f;
    for (std::size_t index = 0; index < ready; ++index) {
        if (!tour_timing::tryAppendCueDuration(total, durationSeconds)) return false;
    }

    std::vector<MorphTarget> targets;
    targets.reserve(ready);
    MorphEnvelopeConfig envelope{};
    envelope.easing = easing;
    for (const auto& entry : inspection.entries) {
        if (entry.status != SculptureTourEntryStatus::Ready) continue;
        // Inspection has already resolved friendly tokens. Materialization must
        // consume that exact identity without another title/alias decision.
        const auto* asset = resolveExactId(entry.presetId).asset;
        if (asset == nullptr) return false;
        // A future cue must not discover malformed authored modulation only
        // after replacing the live tour. Use the installation parser without
        // an engine or graph publication; empty maps keep host default policy.
        for (const auto& route : asset->routingMap) {
            if (!SculptureModulationEngine::inspectRouteResult(route).route) return false;
        }
        targets.push_back(MorphTarget{
            .params = asset->params,
            .name = asset->title.empty() ? asset->id : asset->title,
            .presetId = asset->id,
            .durationSeconds = durationSeconds,
            .dwellSeconds = dwellSeconds,
            .easing = easing,
            .envelope = envelope,
            .presetRevision = asset->revision,
        });
    }
    // All allocation, lookup and route admission precede this no-throw transfer.
    choreographer.replacePlaylist(std::move(targets));
    return true;
}

bool SculpturePresetLibrary::buildCategoryTour(
    SculptureMorphChoreographer& choreographer, std::string_view category,
    float durationSeconds, float dwellSeconds, MorphEasing easing) const {
    return buildInspectedTour(choreographer, inspectCategoryTour(category),
                              durationSeconds, dwellSeconds, easing);
}

bool SculpturePresetLibrary::buildCuratedTour(
    SculptureMorphChoreographer& choreographer, std::string_view tourName,
    float durationSeconds, float dwellSeconds, MorphEasing easing) const {
    return buildInspectedTour(choreographer, inspectCuratedTour(tourName),
                              durationSeconds, dwellSeconds, easing);
}

} // namespace epi_sculpt
