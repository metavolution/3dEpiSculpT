#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <memory>

#include "SculptureWorldFraming.h"
#include "SculptureWorldRecipe.h"
#include "mgfx/Camera.h"

namespace epi_sculpt {

// Forward declarations
class SculptureWorldNavigator;

// -----------------------------------------------------------------------------
// Educational Annotation: AUTONOMOUS WORLD TOUR & WAYPOINT CHOREOGRAPHY
// -----------------------------------------------------------------------------
// In live installations, digital galleries, and continuous performances,
// interactive mathematical worlds must be experienced both under active manual
// exploration and through autonomous, hands-free cinematic tours.
//
// SculptureWorldTourChoreographer provides:
//  1. Playlists & Curated Sequences: Traverses authored tours or the entire catalog.
//  2. Dwell & Morph Timers: Stages each mathematical world at rest before smoothly
//     morphing parameters into the next topology.
//  3. Geodesic Camera Flight: Interpolates camera eye and target along smooth
//     quintic polynomial splines with radial clearance domes, preventing clipping
//     through interior geometries when crossing dimensional scales.
//  4. Non-Intrusive Autopilot: Automatically yields to manual user intervention
//     and seamlessly resumes the tour after an inactivity timeout.
// -----------------------------------------------------------------------------

enum class WorldTourMode : std::uint8_t {
    SequentialCatalog,   // Steps sequentially through all implemented catalog entries
    CuratedPlaylist,     // Follows a specified ordered list of preset aliases or IDs
    ShuffleCatalog,      // Pseudo-random exploration avoiding immediate repeats
};

[[nodiscard]] constexpr const char* name(WorldTourMode mode) noexcept {
    switch (mode) {
    case WorldTourMode::SequentialCatalog: return "Sequential";
    case WorldTourMode::CuratedPlaylist: return "Curated";
    case WorldTourMode::ShuffleCatalog: return "Shuffle";
    default: return "Unknown";
    }
}

struct WorldTourConfig {
    bool enabled = false;
    WorldTourMode mode = WorldTourMode::SequentialCatalog;
    float dwellSeconds = 14.0f;           // Duration to showcase each world at rest
    float transitionSeconds = 3.5f;       // Duration of camera flight & parameter morph
    float inactivityResumeSeconds = 12.0f;// Inactivity timeout before autopilot resumes
    float orbitalArcHeight = 2.5f;        // Clearance dome elevation during inter-world flight
    bool enableParameterMorph = true;     // Geodesic parameter morphing during transitions
    bool enableAutoCameraGlide = true;    // Smooth spline camera trajectories
    std::string tourName = "Full Catalog Autopilot";
};

struct WorldTourStatus {
    bool active = false;
    bool inTransition = false;
    bool userIntervention = false;
    std::size_t waypointIndex = 0U;
    std::size_t totalWaypoints = 0U;
    float timeRemainingInDwell = 0.0f;
    float transitionProgress = 0.0f;      // Normalized [0.0, 1.0]
    std::string currentPresetId;
    std::string nextPresetId;
    std::string tourName;
};

class SculptureWorldTourChoreographer {
public:
    explicit SculptureWorldTourChoreographer(WorldTourConfig config = {});

    // Configuration & Playlist Management
    void setConfig(const WorldTourConfig& config) noexcept { config_ = config; }
    [[nodiscard]] const WorldTourConfig& config() const noexcept { return config_; }

    void setPlaylist(std::vector<std::string> presetIds, std::string_view tourName = "Custom Playlist");
    void reset() noexcept;

    void enable(bool active = true) noexcept;
    void toggle() noexcept { enable(!config_.enabled); }
    [[nodiscard]] bool isEnabled() const noexcept { return config_.enabled; }

    // Dynamic catalog tracking & tour mode cycling
    void setCatalogSize(std::size_t size) noexcept { catalogSize_ = size; }
    [[nodiscard]] std::size_t catalogSize() const noexcept { return catalogSize_; }

    void setTourMode(WorldTourMode mode) noexcept;
    void cycleTourMode() noexcept;
    [[nodiscard]] WorldTourMode tourMode() const noexcept { return config_.mode; }

    [[nodiscard]] const std::vector<std::string>& playlist() const noexcept { return playlist_; }
    [[nodiscard]] const std::string& tourName() const noexcept { return config_.tourName; }
    void setTourName(std::string_view name) { config_.tourName = name; }

    // User interaction notification (temporarily yields tour autopilot)
    void notifyUserInteraction() noexcept;

    // Simulation tick: advances dwell/transition timers and coordinates navigator transitions
    void tick(SculptureWorldNavigator& navigator, float deltaSeconds);

    // Active flight camera during inter-world transitions (returns nullopt when idle)
    [[nodiscard]] std::optional<mgfx::Camera> activeFlightCamera(float baseFovYRad = 0.959931f) const noexcept;

    // Real-time status query for HUD telemetry and diagnostics
    [[nodiscard]] WorldTourStatus status() const noexcept;

    // Manual navigation within playlist
    void advanceNext(SculptureWorldNavigator& navigator);
    void advancePrev(SculptureWorldNavigator& navigator);

    // Lookahead query for proactive OpenCL kernel pre-warming during tour dwell
    [[nodiscard]] std::optional<WorldGeneratorKind> peekNextGenerator(
        const SculptureWorldNavigator& navigator) const noexcept;

private:
    void startTransition(SculptureWorldNavigator& navigator, std::size_t targetIndex);
    void finishTransition(SculptureWorldNavigator& navigator);
    [[nodiscard]] std::size_t computeNextIndex(const SculptureWorldNavigator& navigator) const noexcept;

    WorldTourConfig config_{};
    std::vector<std::string> playlist_;
    std::size_t currentWaypointIndex_ = 0U;
    std::size_t catalogSize_ = 31U;

    float dwellTimer_ = 0.0f;
    float transitionTimer_ = 0.0f;
    float userInactivityTimer_ = 0.0f;
    bool isTransitioning_ = false;
    bool userIntervened_ = false;

    SculptureWorldFrameHint fromHint_{};
    SculptureWorldFrameHint toHint_{};
    mgfx::Camera fromCamera_{};
    mgfx::Camera toCamera_{};
    std::string currentPresetId_;
    std::string nextPresetId_;
};

} // namespace epi_sculpt
