#pragma once

#include "SculptureProgramSchema.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <limits>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: MULTIDIMENSIONAL BARYCENTRIC MORPH SPACE & VECTOR PAD
// -----------------------------------------------------------------------------
// In generative visual computing and electronic sound synthesis (vector synthesis),
// a "Morph Space" represents a continuous 2D manifold populated by discrete
// mathematical attractors or preset states (nodes).
//
// Traditional morphing transitions along a 1D linear trajectory (Deck A <-> Deck B).
// A 2D Morph Space generalizes this to a planar vector manifold:
//   P(x, y) = Sum_i ( w_i(x, y) * P_i )
//
// For 4 corners (SW, SE, NW, NE), bilinear blending guarantees partition of unity:
//   Sum(w_i) == 1.0  for all (x, y) in [0, 1]^2
//
// For arbitrary N-node constellations, modified Shepard Inverse Distance Weighting
// (IDW) is evaluated:
//   w_tilde_i = 1.0 / (d_i^2 + epsilon)
//   w_i = (w_tilde_i * weight_i) / Sum_j(w_tilde_j * weight_j)
//
// Crucially, circular parameters (such as SO(4) Clifford rotor angles and hue)
// cannot be averaged linearly because of the 2*pi branch cut.
// SculptureMorphSpace computes the directional circular mean on S^1:
//   theta_bar = atan2( Sum(w_i * sin(theta_i)), Sum(w_i * cos(theta_i)) )
// -----------------------------------------------------------------------------

enum class MorphSpaceMotion : std::uint8_t {
    Static = 0,     // Manual XY cursor control
    Orbital,        // Smooth circular/elliptical orbit around center
    Lissajous,      // Harmonic Lissajous curve with frequency ratios
    Brownian,       // Continuous bounded random walk
    Bounce,         // 2D particle bouncing off boundaries with elasticity
    Path            // Deterministic spline trajectory across timed waypoints
};

[[nodiscard]] constexpr std::string_view motionName(MorphSpaceMotion motion) noexcept {
    switch (motion) {
    case MorphSpaceMotion::Static: return "Static";
    case MorphSpaceMotion::Orbital: return "Orbital";
    case MorphSpaceMotion::Lissajous: return "Lissajous";
    case MorphSpaceMotion::Brownian: return "Brownian";
    case MorphSpaceMotion::Bounce: return "Bounce";
    case MorphSpaceMotion::Path: return "Path";
    default: return "Unknown";
    }
}

enum class MorphSpaceCorner : std::uint8_t {
    SouthWest = 0, // (0.0, 0.0)
    SouthEast = 1, // (1.0, 0.0)
    NorthWest = 2, // (0.0, 1.0)
    NorthEast = 3  // (1.0, 1.0)
};

[[nodiscard]] constexpr std::string_view cornerName(MorphSpaceCorner corner) noexcept {
    switch (corner) {
    case MorphSpaceCorner::SouthWest: return "SouthWest (0,0)";
    case MorphSpaceCorner::SouthEast: return "SouthEast (1,0)";
    case MorphSpaceCorner::NorthWest: return "NorthWest (0,1)";
    case MorphSpaceCorner::NorthEast: return "NorthEast (1,1)";
    default: return "Unknown";
    }
}

struct MorphSpaceNode {
    std::string id;
    std::string title;
    float x = 0.0f;
    float y = 0.0f;
    SculptureProgramParams params{};
    float weight = 1.0f;
    bool active = true;
};

struct MorphSpaceWaypoint {
    float x = 0.5f;
    float y = 0.5f;
    float dwellSeconds = 2.0f;
    float travelSeconds = 2.0f;
    std::string tag{};
};

struct MorphSpaceQuadrantBank {
    std::string id;
    std::string title;
    std::array<std::string, 4> cornerPresetNames; // SW, SE, NW, NE
    std::array<SculptureProgramParams, 4> cornerParams;
};

enum class MorphSpaceBankIdStatus { Missing, Found, Ambiguous };

struct MorphSpaceBankIdResolution {
    MorphSpaceBankIdStatus status = MorphSpaceBankIdStatus::Missing;
    // Valid only on Found; consume before the next bank-registry mutation.
    std::optional<std::size_t> index;
};

struct SculptureMorphSpaceState {
    bool active = false;
    float cursorX = 0.5f;
    float cursorY = 0.5f;
    MorphSpaceMotion motionMode = MorphSpaceMotion::Static;
    float motionSpeed = 0.5f;
    float shepardPower = 2.0f;
    std::string activeBankName{};
    bool pathLooping = true;

    struct WaypointState {
        float x = 0.5f;
        float y = 0.5f;
        float dwellSeconds = 2.0f;
        float travelSeconds = 2.0f;
        std::string tag{};
    };
    std::vector<WaypointState> waypoints{};
};

class SculptureMorphSpace {
public:
    SculptureMorphSpace() noexcept;

    // -------------------------------------------------------------------------
    // Node Management
    // -------------------------------------------------------------------------
    void setCorner(MorphSpaceCorner corner, const SculptureProgramParams& params,
                   std::string_view id = "", std::string_view title = "");
    void setNode(std::size_t index, float x, float y, const SculptureProgramParams& params,
                 std::string_view id = "", std::string_view title = "");
    void addNode(float x, float y, const SculptureProgramParams& params,
                 std::string_view id = "", std::string_view title = "");
    void clearNodes() noexcept;
    [[nodiscard]] std::size_t nodeCount() const noexcept { return nodes_.size(); }
    [[nodiscard]] const MorphSpaceNode* node(std::size_t index) const noexcept;
    [[nodiscard]] MorphSpaceNode* node(std::size_t index) noexcept;

    // -------------------------------------------------------------------------
    // Thematic Quadrant Banks
    // -------------------------------------------------------------------------
    void addQuadrantBank(MorphSpaceQuadrantBank bank);
    void loadBank(std::size_t index);
    void loadBank(std::string_view nameOrId);
    void cycleBank();
    [[nodiscard]] std::size_t bankCount() const noexcept { return quadrantBanks_.size(); }
    [[nodiscard]] std::size_t activeBankIndex() const noexcept { return activeBankIndex_; }
    [[nodiscard]] std::string_view activeBankName() const noexcept;
    [[nodiscard]] std::string_view activeBankTitle() const noexcept;
    [[nodiscard]] std::vector<std::string_view> availableBankNames() const;
    [[nodiscard]] const MorphSpaceQuadrantBank* activeBank() const noexcept;
    void setBankCornerParams(std::size_t bankIndex, MorphSpaceCorner corner, const SculptureProgramParams& params);
    // Resolver: stable ID -> borrowed const params, or null to retain the corner.
    // It must not mutate this controller; no callback or pointer escapes.
    template<class ResolveParams>
    void refreshBankCornerParams(ResolveParams&& resolve) {
        for (auto& bank : quadrantBanks_) {
            for (std::size_t corner = 0; corner < bank.cornerPresetNames.size(); ++corner) {
                const auto& id = bank.cornerPresetNames[corner];
                if (id.empty()) continue;
                if (const SculptureProgramParams* params = resolve(std::string_view(id))) {
                    bank.cornerParams[corner] = *params;
                }
            }
        }
        // Keep selection fixed while resolving stored definitions, then
        // materialize the selected bank's four nodes just once.
        loadBank(activeBankIndex_);
    }
    [[nodiscard]] std::optional<std::size_t> findBankIndex(std::string_view nameOrId) const noexcept;
    [[nodiscard]] bool hasBank(std::string_view nameOrId) const noexcept;
    [[nodiscard]] MorphSpaceBankIdResolution resolveExactBankId(std::string_view id) const noexcept;
    // Empty/ambiguous IDs reject without mutation. Missing exact IDs append;
    // unique exact IDs replace a prepared bank. Materialization may still throw.
    [[nodiscard]] std::optional<std::size_t> tryUpsertCategoryBank(
        std::string_view categoryId,
        std::string_view categoryTitle,
        const std::array<std::string, 4>& cornerPresetNames,
        const std::array<SculptureProgramParams, 4>& cornerParams,
        bool autoLoad = true
    );
    // Compatibility entry point: successful indices are unchanged; rejection
    // returns kInvalidBankIndex. New workflow callers should use the optional API.
    static constexpr std::size_t kInvalidBankIndex = std::numeric_limits<std::size_t>::max();
    std::size_t upsertCategoryBank(
        std::string_view categoryId,
        std::string_view categoryTitle,
        const std::array<std::string, 4>& cornerPresetNames,
        const std::array<SculptureProgramParams, 4>& cornerParams,
        bool autoLoad = true
    );

    // -------------------------------------------------------------------------
    // Waypoint Trajectory Path Sequencer
    // -------------------------------------------------------------------------
    void addWaypoint(float x, float y, float dwellSec = 2.0f, float travelSec = 2.0f, std::string_view tag = "");
    void clearPath() noexcept;
    [[nodiscard]] std::size_t waypointCount() const noexcept { return waypoints_.size(); }
    void setPathLooping(bool loop) noexcept { loopPath_ = loop; }
    [[nodiscard]] bool isPathLooping() const noexcept { return loopPath_; }
    [[nodiscard]] std::size_t activeWaypointIndex() const noexcept { return activeWaypointIndex_; }
    [[nodiscard]] float pathProgress() const noexcept;
    void togglePathPlayback() noexcept;
    [[nodiscard]] const std::vector<MorphSpaceWaypoint>& waypoints() const noexcept { return waypoints_; }

    // -------------------------------------------------------------------------
    // Active State & Cursor Positioning
    // -------------------------------------------------------------------------
    void setActive(bool active) noexcept { active_ = active; }
    [[nodiscard]] bool isActive() const noexcept { return active_; }
    void toggleActive() noexcept { active_ = !active_; }

    void setCursor(float x, float y) noexcept;
    void nudgeCursor(float dx, float dy) noexcept;
    [[nodiscard]] std::pair<float, float> cursor() const noexcept { return {cursorX_, cursorY_}; }
    [[nodiscard]] float cursorX() const noexcept { return cursorX_; }
    [[nodiscard]] float cursorY() const noexcept { return cursorY_; }

    // -------------------------------------------------------------------------
    // Autonomous Motion Generation
    // -------------------------------------------------------------------------
    void setMotionMode(MorphSpaceMotion mode) noexcept { motionMode_ = mode; }
    void cycleMotionMode() noexcept;
    [[nodiscard]] MorphSpaceMotion motionMode() const noexcept { return motionMode_; }
    [[nodiscard]] std::string_view motionModeName() const noexcept { return motionName(motionMode_); }

    void setMotionSpeed(float speed) noexcept { motionSpeed_ = std::clamp(speed, 0.01f, 10.0f); }
    [[nodiscard]] float motionSpeed() const noexcept { return motionSpeed_; }

    void setShepardPower(float power) noexcept { shepardPower_ = std::clamp(power, 1.0f, 8.0f); }
    [[nodiscard]] float shepardPower() const noexcept { return shepardPower_; }

    // Advance continuous motion and update cursor
    void update(float dtSeconds) noexcept;

    // -------------------------------------------------------------------------
    // Mathematical Weight Evaluation & Parameter Synthesis
    // -------------------------------------------------------------------------
    // Evaluates partition-of-unity blending weights [0, 1] summing to 1.0
    [[nodiscard]] std::vector<float> computeWeights() const noexcept;

    // Synthesizes continuous manifold state from active nodes and current weights
    [[nodiscard]] SculptureProgramParams synthesizeParams() const noexcept;

    // Formats compact HUD telemetry string
    [[nodiscard]] std::string formatHudStatus() const;

    // -------------------------------------------------------------------------
    // State Persistence & Snapshot Serialization
    // -------------------------------------------------------------------------
    [[nodiscard]] SculptureMorphSpaceState exportState() const noexcept;
    void importState(const SculptureMorphSpaceState& state);
    // Persisted references are IDs, never titles. False means identity rejection
    // without mutation. A valid bank replaces all nodes; bank-free imports must
    // be inactive. Allocation failures after admission may still throw.
    [[nodiscard]] bool importCapturedState(const SculptureMorphSpaceState& state);

private:
    void importControllerState(const SculptureMorphSpaceState& state);

    std::vector<MorphSpaceNode> nodes_{};
    bool active_ = false;
    float cursorX_ = 0.5f;
    float cursorY_ = 0.5f;

    MorphSpaceMotion motionMode_ = MorphSpaceMotion::Static;
    float motionSpeed_ = 0.5f;
    float motionPhase_ = 0.0f;
    float shepardPower_ = 2.0f;

    // Thematic Quadrant Banks
    std::vector<MorphSpaceQuadrantBank> quadrantBanks_{};
    std::size_t activeBankIndex_ = 0;

    // Waypoint Trajectory Sequencer
    std::vector<MorphSpaceWaypoint> waypoints_{};
    std::size_t activeWaypointIndex_ = 0;
    float waypointTimer_ = 0.0f;
    bool isTravelling_ = false;
    bool loopPath_ = true;

    // Bounce physics state
    float velX_ = 0.25f;
    float velY_ = 0.35f;

    // Brownian seed state
    float brownianNoise_ = 0.0f;
};

} // namespace epi_sculpt
