#pragma once

#include "SculptureProgramSchema.h"

#include <cstdint>
#include <random>
#include <string>
#include <array>
#include <cmath>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: LIVING QUASI-PERIODIC PALETTE AGENT & OKLAB ENGINE
// -----------------------------------------------------------------------------
// Realizes the project vision requirement for:
// - "Living palette agent" (docu/project-vision.txt:49-51, include/epi/presets/pattern/living_palette_agent.hpp)
// - "Perceptual-space color modulation: animate hue while preserving luminance stability"
// - Quasi-periodic irrational harmonic drift (Golden Ratio phi, Euler e, sqrt(2))
// - Dynamical resonance coupling with analytical metrics (Lyapunov index, kinetic energy)
//
// In contrast to static linear hue rotation, this engine:
// 1. Navigates perceptual OKLCH color space without gamut clipping or luminance dips.
// 2. Produces multi-frequency wave breathing where chromatic dispersion expands
//    during chaotic/turbulent mathematical states and relaxes during laminar orbits.
// 3. Implements strict OKLab <-> Linear sRGB analytical conversions with chromatic
//    gamut compression.
// -----------------------------------------------------------------------------

struct LivingPaletteDynamicsMetrics {
    float kineticEnergy = 0.0f;       // Rate of change in mathematical manifold
    float lyapunovIndex = 0.0f;       // Local divergence rate / chaotic complexity
    float topologicalEntropy = 0.0f;  // Phase space complexity
    float transientIntensity = 0.0f;  // Sharp transient or acoustic stimulation
};

enum class PaletteAgentMode : std::uint8_t {
    Autonomous = 0,       // Self-guided psychological drift (curiosity vs discipline)
    DynamicsCoupled = 1,  // Driven directly by Lyapunov index and kinetic energy
    HarmonizedBreathing = 2, // Steady quasi-periodic multi-frequency breathing
    Experimental = 3      // High-dispersion chromatic expansion
};

struct OklabColor {
    float L = 0.7f;
    float a = 0.0f;
    float b = 0.0f;
};

struct OklchColor {
    float L = 0.7f;
    float C = 0.15f;
    float h = 0.0f; // in radians [0, 2*pi)
};

struct PaletteAgentState {
    float targetHue = 0.0f;
    float targetChroma = 0.15f;
    float currentHue = 0.0f;
    float currentChroma = 0.15f;
    float baseLuminance = 0.72f;
    float curiosity = 0.5f;     // Determines how far/fast it wanders
    float discipline = 0.8f;    // Determines how strictly it returns to safe zones
    float timeSinceShift = 0.0f;
    float cumulativeTime = 0.0f;
    float breathingPhase = 0.0f;
    float effectiveBreathFreq = 0.06f;
    PaletteAgentMode mode = PaletteAgentMode::DynamicsCoupled;
};

class SculpturePaletteAgent {
public:
    explicit SculpturePaletteAgent(std::uint32_t seed = 0xE91C0DEu);

    // Primary update: advances autonomous state and injects mapped color into params
    void update(SculptureProgramParams& params, float deltaSeconds);

    // Advanced update coupled with real-time dynamical feedback metrics
    void updateWithDynamics(
        SculptureProgramParams& params,
        float deltaSeconds,
        const LivingPaletteDynamicsMetrics& metrics
    );

    // Forces a sudden harmonic shift (e.g. on manual trigger, beat drop, or cue)
    void stimulate(float intensity);

    // Reseeds stochastic drift generator
    void reseed(std::uint32_t seed);

    // Mode selection
    void setMode(PaletteAgentMode mode) noexcept { state_.mode = mode; }
    [[nodiscard]] PaletteAgentMode mode() const noexcept { return state_.mode; }

    [[nodiscard]] const PaletteAgentState& state() const noexcept { return state_; }
    [[nodiscard]] std::string formatHudStatus() const;

    // -------------------------------------------------------------------------
    // Analytical Color Space Mathematical Primitives
    // -------------------------------------------------------------------------
    [[nodiscard]] static OklchColor oklabToOklch(const OklabColor& lab) noexcept;
    [[nodiscard]] static OklabColor oklchToOklab(const OklchColor& lch) noexcept;
    [[nodiscard]] static std::array<float, 3> oklabToLinearSrgb(const OklabColor& lab) noexcept;
    [[nodiscard]] static OklabColor linearSrgbToOklab(float r, float g, float b) noexcept;
    [[nodiscard]] static OklchColor clampGamutSafe(const OklchColor& lch) noexcept;

private:
    void evaluateNextTarget();

    PaletteAgentState state_{};
    std::mt19937 rng_;
};

} // namespace epi_sculpt
