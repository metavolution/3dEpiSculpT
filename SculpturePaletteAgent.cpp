#include "SculpturePaletteAgent.h"

#include <cmath>
#include <algorithm>
#include <numbers>
#include <sstream>
#include <iomanip>

namespace epi_sculpt {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;
constexpr float kPhi = 1.61803398874989484820f; // Golden ratio
constexpr float kEuler = 2.71828182845904523536f; // Napier's constant

} // namespace

SculpturePaletteAgent::SculpturePaletteAgent(std::uint32_t seed) : rng_(seed) {
    evaluateNextTarget();
}

void SculpturePaletteAgent::reseed(std::uint32_t seed) {
    rng_.seed(seed);
    state_ = {};
    evaluateNextTarget();
}

void SculpturePaletteAgent::evaluateNextTarget() {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    // Curiosity drives hue shifts in perceptual space
    const float hueShift = (dist(rng_) - 0.5f) * state_.curiosity * 0.5f;
    state_.targetHue = std::fmod(state_.currentHue + hueShift + 1.0f, 1.0f);

    // Discipline pulls chroma towards a stable, harmonious baseline
    const float safeChroma = 0.14f;
    const float wildChroma = 0.06f + dist(rng_) * 0.22f;
    state_.targetChroma = std::lerp(wildChroma, safeChroma, state_.discipline);

    state_.timeSinceShift = 0.0f;
}

void SculpturePaletteAgent::stimulate(float intensity) {
    state_.curiosity = std::clamp(state_.curiosity + intensity * 0.5f, 0.0f, 1.0f);
    state_.discipline = std::clamp(state_.discipline - intensity * 0.3f, 0.0f, 1.0f);

    if (intensity > 0.5f) {
        evaluateNextTarget();
    }
}

void SculpturePaletteAgent::update(SculptureProgramParams& params, float deltaSeconds) {
    LivingPaletteDynamicsMetrics defaultMetrics{};
    updateWithDynamics(params, deltaSeconds, defaultMetrics);
}

void SculpturePaletteAgent::updateWithDynamics(
    SculptureProgramParams& params,
    float deltaSeconds,
    const LivingPaletteDynamicsMetrics& metrics
) {
    state_.timeSinceShift += deltaSeconds;
    state_.cumulativeTime += deltaSeconds;

    // React to dynamical feedback: high Lyapunov/chaos raises curiosity; stability raises discipline
    if (state_.mode == PaletteAgentMode::DynamicsCoupled) {
        const float targetCuriosity = std::clamp(0.25f + metrics.lyapunovIndex * 0.6f + metrics.kineticEnergy * 0.3f, 0.1f, 0.95f);
        state_.curiosity = std::lerp(state_.curiosity, targetCuriosity, deltaSeconds * 0.25f);
        state_.discipline = std::lerp(state_.discipline, 1.0f - state_.curiosity * 0.7f, deltaSeconds * 0.15f);

        // Modulate breathing frequency based on system kinetic energy
        state_.effectiveBreathFreq = std::clamp(0.04f + metrics.kineticEnergy * 0.08f + metrics.lyapunovIndex * 0.04f, 0.02f, 0.25f);
    } else if (state_.mode == PaletteAgentMode::HarmonizedBreathing) {
        state_.curiosity = std::lerp(state_.curiosity, 0.3f, deltaSeconds * 0.1f);
        state_.discipline = std::lerp(state_.discipline, 0.85f, deltaSeconds * 0.1f);
        state_.effectiveBreathFreq = 0.05f;
    } else if (state_.mode == PaletteAgentMode::Experimental) {
        state_.curiosity = 0.85f;
        state_.discipline = 0.3f;
        state_.effectiveBreathFreq = 0.12f;
    } else {
        // Autonomous relaxation back to baseline
        state_.curiosity = std::lerp(state_.curiosity, 0.35f, deltaSeconds * 0.1f);
        state_.discipline = std::lerp(state_.discipline, 0.75f, deltaSeconds * 0.05f);
        state_.effectiveBreathFreq = 0.06f;
    }

    if (metrics.transientIntensity > 0.0f) {
        stimulate(metrics.transientIntensity);
    }

    const float shiftInterval = 4.0f + (1.0f - state_.curiosity) * 5.0f;
    if (state_.timeSinceShift > shiftInterval) {
        evaluateNextTarget();
    }

    // Quasi-irrational drift modulation (Golden ratio and Napier harmonics)
    const float tSlow = state_.cumulativeTime * 0.04f;
    const float quasiOffset = 0.08f * (std::sin(tSlow)
        + 0.5f * std::sin(tSlow * kPhi)
        + 0.25f * std::sin(tSlow * kEuler));

    // Smooth shortest-path circular hue interpolation
    float hueDelta = state_.targetHue - state_.currentHue;
    if (hueDelta > 0.5f) hueDelta -= 1.0f;
    if (hueDelta < -0.5f) hueDelta += 1.0f;

    const float interpolationSpeed = 0.4f * (0.15f + state_.curiosity);
    state_.currentHue += hueDelta * deltaSeconds * interpolationSpeed;
    state_.currentHue = std::fmod(state_.currentHue + 1.0f, 1.0f);

    state_.currentChroma = std::lerp(state_.currentChroma, state_.targetChroma, deltaSeconds * interpolationSpeed);

    // Multi-band chromatic breathing
    state_.breathingPhase = std::fmod(state_.breathingPhase + state_.effectiveBreathFreq * kTwoPi * deltaSeconds, kTwoPi);
    const float breathFactor = 1.0f + 0.35f * std::sin(state_.breathingPhase)
                             + 0.15f * std::sin(state_.breathingPhase * kPhi);

    // Gamut-safe OKLCH color construction
    OklchColor lch;
    lch.L = std::clamp(state_.baseLuminance + 0.04f * std::sin(state_.breathingPhase * 0.5f), 0.2f, 0.95f);
    lch.C = state_.currentChroma * breathFactor;
    lch.h = std::fmod(state_.currentHue + quasiOffset + 1.0f, 1.0f) * kTwoPi;

    const OklchColor safeLch = clampGamutSafe(lch);

    // Set canonical parameter mappings
    params.colorEffect = ColorEffectType::LivingPaletteAgent;
    params.hueRotation = safeLch.h / kTwoPi;
    params.chromaBreath = safeLch.C;
    params.lumaDrift = safeLch.L;
    params.hueSpeed = state_.effectiveBreathFreq;
}

std::string SculpturePaletteAgent::formatHudStatus() const {
    char buf[256];
    const float hueDeg = state_.currentHue * 360.0f;
    const char* modeStr = "AUTO";
    switch (state_.mode) {
    case PaletteAgentMode::Autonomous: modeStr = "AUTO"; break;
    case PaletteAgentMode::DynamicsCoupled: modeStr = "DYN-SYNC"; break;
    case PaletteAgentMode::HarmonizedBreathing: modeStr = "HARMONIC"; break;
    case PaletteAgentMode::Experimental: modeStr = "EXP"; break;
    }

    std::snprintf(buf, sizeof(buf),
        "[PALETTE: OKLab Living Agent (%s) | Hue: %3.0f° | Chroma: %.2f | Breath: %.2fHz | Cur: %.2f]",
        modeStr, hueDeg, state_.currentChroma, state_.effectiveBreathFreq, state_.curiosity);
    return std::string(buf);
}

// -----------------------------------------------------------------------------
// Analytical Color Space Mathematical Primitives
// -----------------------------------------------------------------------------

OklchColor SculpturePaletteAgent::oklabToOklch(const OklabColor& lab) noexcept {
    OklchColor lch;
    lch.L = lab.L;
    lch.C = std::sqrt(lab.a * lab.a + lab.b * lab.b);
    lch.h = std::atan2(lab.b, lab.a);
    if (lch.h < 0.0f) {
        lch.h += kTwoPi;
    }
    return lch;
}

OklabColor SculpturePaletteAgent::oklchToOklab(const OklchColor& lch) noexcept {
    OklabColor lab;
    lab.L = lch.L;
    lab.a = lch.C * std::cos(lch.h);
    lab.b = lch.C * std::sin(lch.h);
    return lab;
}

std::array<float, 3> SculpturePaletteAgent::oklabToLinearSrgb(const OklabColor& lab) noexcept {
    const float l_ = lab.L + 0.3963377774f * lab.a + 0.2158037573f * lab.b;
    const float m_ = lab.L - 0.1055613458f * lab.a - 0.0638541728f * lab.b;
    const float s_ = lab.L - 0.0894841775f * lab.a - 1.2914855480f * lab.b;

    const float l = l_ * l_ * l_;
    const float m = m_ * m_ * m_;
    const float s = s_ * s_ * s_;

    const float r = +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
    const float g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
    const float b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;

    return {r, g, b};
}

OklabColor SculpturePaletteAgent::linearSrgbToOklab(float r, float g, float b) noexcept {
    const float l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
    const float m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
    const float s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

    const float l_ = std::cbrt(std::max(l, 0.0f));
    const float m_ = std::cbrt(std::max(m, 0.0f));
    const float s_ = std::cbrt(std::max(s, 0.0f));

    OklabColor lab;
    lab.L = 0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_;
    lab.a = 1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_;
    lab.b = 0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_;
    return lab;
}

OklchColor SculpturePaletteAgent::clampGamutSafe(const OklchColor& lch) noexcept {
    OklchColor safe = lch;
    safe.L = std::clamp(safe.L, 0.0f, 1.0f);

    // Fast bisection clamp to ensure linear sRGB lies within [0.0, 1.0] without clipping
    float lowC = 0.0f;
    float highC = safe.C;

    for (int iter = 0; iter < 8; ++iter) {
        safe.C = 0.5f * (lowC + highC);
        const auto rgb = oklabToLinearSrgb(oklchToOklab(safe));
        const bool inside = (rgb[0] >= 0.0f && rgb[0] <= 1.0f)
                         && (rgb[1] >= 0.0f && rgb[1] <= 1.0f)
                         && (rgb[2] >= 0.0f && rgb[2] <= 1.0f);
        if (inside) {
            lowC = safe.C; // Can afford more chroma
        } else {
            highC = safe.C; // Out of gamut, compress chroma
        }
    }

    safe.C = lowC;
    return safe;
}

} // namespace epi_sculpt
