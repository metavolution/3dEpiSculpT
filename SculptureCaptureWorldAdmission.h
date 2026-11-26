#pragma once

#include "SculptureParameterRegistry.h"
#include "SculptureSerializer.h"

#include <array>
#include <charconv>
#include <string_view>
#include <unordered_map>

namespace epi_sculpt {

// A complete recorder-shaped WorldMode (v20 onward) must not silently restore defaults for
// missing moment fields. The real decoder still owns value and version checks.
// Older versions keep their historical optional-field policy, but an ambiguous
// version declaration cannot be used to select that policy.
[[nodiscard]] inline bool captureWorldModeAdmitted(std::string_view text, int version) {
    static constexpr auto required = std::to_array<std::string_view>({
        "worldmode_version", "preset", "preset_revision", "presentation",
        "world_recipe_present", "playlist_library_present", "tuning_present",
        "world_version", "world_time", "world_morph", "world_cameraAngle",
        "world_cameraEye", "world_cameraTarget", "world_cameraUp", "world_camera_override",
        "routing_count", "morphspace_active", "morphspace_cursor", "morphspace_motion",
        "morphspace_speed", "morphspace_power", "morphspace_bank", "morphspace_path_looping",
        "morphspace_waypoint_count", "pre", "post", "audioSpectrum", "inner", "innerB",
        "reduction", "reductionB", "seedType", "seedTypeB", "colorEffect",
        "auxiliaryResolutionScale", "temporalReset"
    });
    // Conditional and provenance fields may be absent, but a second value
    // must not silently override the value recorded by the writer.
    static constexpr auto optional = std::to_array<std::string_view>({
        "world_recipe_generator", "world_recipe_primitive", "world_recipe_delivery",
        "world_recipe_presentation", "world_recipe_instance_budget", "world_recipe_generator_hz",
        "world_recipe_material_bands", "world_recipe_uses_4d", "world_recipe_mirror_topology",
        "playlist_active", "playlist_paused", "playlist_loop", "playlist_playback_mode",
        "playlist_target_index", "playlist_dwell_seconds", "playlist_target_count",
        "tuning_fog_density", "tuning_emission_threshold", "tuning_emission_multiplier",
        "world_quality_requested_budget", "world_quality_effective_budget", "world_quality_effective_hz",
        "canonical_feedback_generation", "canonical_feedback_reset_epoch", "compact_generation",
        "world_generation", "implicit_generation", "presentation_frame_sequence",
        "presentation_device_generation", "presentation_surface_generation", "capture_submission_serial",
        "selected_compact_generation", "selected_compact_production_receipt_sequence",
        "selected_world_generation", "selected_world_production_receipt_sequence",
        "selected_canonical_feedback_generation", "selected_canonical_feedback_reset_epoch",
        "selected_canonical_feedback_production_receipt_sequence", "selected_implicit_generation",
        "selected_implicit_production_receipt_sequence", "presentation_outcome",
        "presentation_outcome_known", "presentation_confirmed", "implicit_external_active",
        "implicit_diagnostic", "implicit_quarantine_count"
    });
    static constexpr auto volumeKeys = std::to_array<std::string_view>({
        "volume_geometry_recipe", "field_color_channel", "field_low_color_and_minimum",
        "field_high_color_and_maximum", "field_optics"
    });
    std::array<unsigned, volumeKeys.size()> volumeCounts{};
    std::unordered_map<std::string_view, unsigned> counts;
    counts.reserve(required.size() + kFloatParams.size() + kUintParams.size());
    for (auto key : required) counts.emplace(key, 0U);
    for (const auto& field : kFloatParams) counts.emplace(field.name, 0U);
    for (const auto& field : kUintParams) counts.emplace(field.name, 0U);
    std::unordered_map<std::string_view, unsigned> optionalCounts;
    optionalCounts.reserve(optional.size());
    for (auto key : optional) optionalCounts.emplace(key, 0U);
    std::array<bool, 256> routes{}, playlistTargets{}, playlistRevisions{};
    std::array<bool, 128> waypoints{};
    bool duplicateIndexedField = false;
    const auto trackIndex = [&duplicateIndexedField](std::string_view key,
                                                       std::string_view prefix, auto& seen) {
        if (!key.starts_with(prefix)) return;
        const auto suffix = key.substr(prefix.size());
        if (suffix.empty() || (suffix.size() > 1 && suffix.front() == '0')) return;
        std::size_t index = 0;
        const auto parsed = std::from_chars(suffix.data(), suffix.data() + suffix.size(), index);
        if (parsed.ec != std::errc{} || parsed.ptr != suffix.data() + suffix.size()
            || index >= seen.size()) return;
        if (seen[index]) duplicateIndexedField = true;
        seen[index] = true;
    };
    while (!text.empty()) {
        const auto newline = text.find('\n');
        const auto line = text.substr(0, newline);
        const auto equals = line.find('=');
        if (!line.empty() && line.front() != '#' && equals != std::string_view::npos) {
            const auto key = line.substr(0, equals);
            for (std::size_t i = 0; i < volumeKeys.size(); ++i)
                if (key == volumeKeys[i]) ++volumeCounts[i];
            const auto found = counts.find(key);
            if (found != counts.end()) ++found->second;
            if (version >= SculptureWorldMode::kCompleteCaptureVersion) {
                const auto other = optionalCounts.find(key);
                if (other != optionalCounts.end()) ++other->second;
                trackIndex(key, "routing_", routes);
                trackIndex(key, "playlist_target_", playlistTargets);
                trackIndex(key, "playlist_target_revision_hex_", playlistRevisions);
                trackIndex(key, "morphspace_wp_", waypoints);
            }
        }
        if (newline == std::string_view::npos) break;
        text.remove_prefix(newline + 1);
    }
    if (counts.at("worldmode_version") != 1U) return false;
    if (version >= SculptureWorldMode::kVolumePresentationVersion)
        for (unsigned count : volumeCounts) if (count != 1U) return false;
    if (version < SculptureWorldMode::kCompleteCaptureVersion) return true;
    for (const auto& entry : counts) if (entry.second != 1U) return false;
    for (const auto& entry : optionalCounts) if (entry.second > 1U) return false;
    if (duplicateIndexedField) return false;
    return true;
}

} // namespace epi_sculpt
