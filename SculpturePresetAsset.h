#pragma once

#include "SculpturePresets.h"
#include "SculpturePresentationTypes.h"
#include "SculptureWorldRecipe.h"
#include "epi/mode/visual_mode.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace epi_sculpt {

// Preset values shared by authoring and inspection. Catalog discovery, file
// admission and publication belong to SculpturePresetLibrary, not these values.
struct SculpturePresetAsset {
    std::string id{};
    std::string title{};
    std::string category{};
    std::string description{};
    // Content-addressed provenance for captures and diagnostics. This is a
    // deterministic fingerprint of the authored source (or built-in recipe),
    // not a runtime/GPU identifier.
    std::string revision{};
    std::filesystem::path sourcePath{};
    SculptureProgramParams params{};
    // Exact epi2 mathematical state when this asset originates from a root
    // Pattern. SculptureProgramParams remains the spatial/presentation
    // projection and must not replace this canonical mode.
    std::optional<epi::VisualMode> canonicalFeedbackMode{};
    SculptureWorldState state{};
    SculpturePresentationOwnership presentationOwnership = SculpturePresentationOwnership::CompactInstances;
    // Declares the requested bounded topology independently of the current
    // producer route.  The application can therefore expose only connected
    // visuals instead of silently rendering every preset as generic samples.
    WorldRecipe worldRecipe{};
    std::vector<std::string> routingMap{};
    bool authoredOnDisk = false;
    // Inert authored input, not a second automation engine/history. Explicit
    // control selection admits it through the existing numerical owner.
    std::string macroAutomationJson{};

    // Legacy parameter-only projection. Full source admission consumes the asset.
    [[nodiscard]] SculpturePreset asPreset() const noexcept {
        return {title, description, params, routingMap};
    }
};

enum class SculpturePresetIdStatus { Missing, Found, Ambiguous };

struct SculpturePresetIdResolution {
    SculpturePresetIdStatus status = SculpturePresetIdStatus::Missing;
    const SculpturePresetAsset* asset = nullptr; // Borrowed only on Found.
};

} // namespace epi_sculpt

