#pragma once

#include "SculptureProgramSchema.h"
#include "Sculpture4DFoliation.h"

#include <cmath>

namespace epi_sculpt {

class SculptureMorphChoreographer;
class SculptureMultiTrackChoreographer;
class SculpturePerformanceDeck;

// -----------------------------------------------------------------------------
// [EDUCATIONAL NOTE] Canonical Sculpture Parameter & SO(4) Rotor Composer
// -----------------------------------------------------------------------------
// Consolidates parameter composition, downbeat kick envelope injection,
// 4D SO(4) isoclinic rotor kinematics, W-hyperplane slicing bounds, and
// fault-tolerant NaN/Inf sanitization into one stateless, deterministic module.
//
// Shared by:
//   - SculpturePipelineCoordinator (headless & runtime frame coordination)
//   - SculptureApplication (interactive desktop/VR simulation loop)
//   - SculptureWorldNavigator (live 4D rotor & W-slice scrubbing deck)
// -----------------------------------------------------------------------------
class SculptureParameterComposer {
public:
    static inline constexpr float kTwoPi = 6.2831853f;

    // Normalizes an SO(4) isoclinic rotor angle into [0, 2*pi).
    [[nodiscard]] static float normalizeRotorAngle(float radians) noexcept;

    // Clamps the 4D hyperplane slicing coordinate W into [minW, maxW].
    [[nodiscard]] static float clampLatentW(
        float w,
        float minW = -1.0f,
        float maxW = 1.0f) noexcept;

    // Resets 4D hyperplane and SO(4) isoclinic rotor angles to canonical baseline.
    static void reset4DRotors(SculptureProgramParams& params) noexcept;

    // Applies the canonical performance-deck downbeat kick envelope to seed pulse
    // amplitude and world scale.
    static void applyDownbeatKickModulation(
        SculptureProgramParams& params,
        float downbeatKick) noexcept {
        if (std::isfinite(downbeatKick) && downbeatKick > 0.001f) {
            params.seedPulseAmplitude += downbeatKick * 0.35f;
            params.worldScale *= (1.0f + downbeatKick * 0.04f);
        }
    }

    // Composes master program parameters from the choreographer, optional
    // multi-track timeline, and optional A/B performance deck.
    [[nodiscard]] static SculptureProgramParams composeMasterProgramParams(
        const SculptureMorphChoreographer& choreographer,
        const SculptureMultiTrackChoreographer* multiTrack,
        bool multiTrackEnabled,
        const SculpturePerformanceDeck* performanceDeck,
        bool performanceDeckEnabled,
        std::uint32_t stageMask) noexcept;

    // Evaluates Kuramoto-coupled 4D foliation wave field and continuous SO(4)
    // rotor kinematics for the pipeline coordinator.
    static void stepKuramotoFoliationWave4D(
        SculptureProgramParams& params,
        const Foliation4DConfig& foliationConfig,
        float simulatedTime,
        float deltaSeconds,
        float couplingK,
        float audioDrive) noexcept;

    // Fault-tolerant NaN/Inf and mathematical range sanitizer executed prior to
    // OpenCL / Vulkan GPU submission.
    static void sanitizeProgramParams(SculptureProgramParams& params) noexcept;
};

} // namespace epi_sculpt
