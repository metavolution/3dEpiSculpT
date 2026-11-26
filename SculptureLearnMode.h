// Canonical Learn Mode vocabulary shared by input, the CPU oracle, and HUDs.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <sstream>

namespace epi_sculpt::learn_mode {

enum class Stage : std::uint8_t {
    Transform = 0U,
    Reduce = 1U,
    FeedbackSample = 2U,
    Color = 3U,
    Seed = 4U,
    Blend = 5U,
};

inline constexpr std::size_t kStageCount = 6U;

enum class DiagnosticStage : std::uint8_t {
    FullPipeline = 0U,
    Transform = 1U,
    Reduce = 2U,
    SeedWindow = 3U,
    MaterialCoordinate = 4U,
    OrbitEnergy = 5U,
    OrbitTrap = 6U,
};

inline constexpr std::uint32_t kAllStages = 0x3FU;
inline constexpr std::uint32_t kDiagnosticShift = 8U;
inline constexpr std::uint32_t kDiagnosticBits = 0xFU << kDiagnosticShift;
inline constexpr std::uint32_t kResetStagesCommand = 0x80000000U | kAllStages;
inline constexpr std::uint32_t kCycleDiagnosticCommand = 0xFFFFFFFFU;
inline constexpr std::uint32_t kSoloStageCommandMask = 0x40000000U;
inline constexpr std::uint32_t kAccumulativeStageCommandMask = 0x20000000U;

[[nodiscard]] constexpr std::uint32_t bit(Stage stage) noexcept {
    return 1U << static_cast<std::uint32_t>(stage);
}

[[nodiscard]] constexpr std::uint32_t commandSoloStage(Stage stage) noexcept {
    return kSoloStageCommandMask | bit(stage);
}

[[nodiscard]] constexpr std::uint32_t commandAccumulativeStage(Stage maxStage) noexcept {
    return kAccumulativeStageCommandMask | static_cast<std::uint32_t>(maxStage);
}

[[nodiscard]] constexpr bool enabled(std::uint32_t stageMask, Stage stage) noexcept {
    return (stageMask & bit(stage)) != 0U;
}

[[nodiscard]] constexpr std::uint32_t maskForSoloStage(Stage stage) noexcept {
    return bit(stage);
}

[[nodiscard]] constexpr std::uint32_t maskForAccumulativeStage(Stage maxStage) noexcept {
    const auto maxIdx = static_cast<std::uint32_t>(maxStage);
    return (maxIdx >= 5U) ? kAllStages : ((1U << (maxIdx + 1U)) - 1U);
}

[[nodiscard]] constexpr DiagnosticStage diagnostic(std::uint32_t stageMask) noexcept {
    const auto raw = (stageMask & kDiagnosticBits) >> kDiagnosticShift;
    return raw <= static_cast<std::uint32_t>(DiagnosticStage::OrbitTrap)
        ? static_cast<DiagnosticStage>(raw)
        : DiagnosticStage::FullPipeline;
}

[[nodiscard]] constexpr std::uint32_t withDiagnostic(
    std::uint32_t stageMask, DiagnosticStage stage) noexcept {
    return (stageMask & ~kDiagnosticBits)
        | (static_cast<std::uint32_t>(stage) << kDiagnosticShift);
}

[[nodiscard]] constexpr std::uint32_t cycleDiagnostic(std::uint32_t stageMask) noexcept {
    constexpr std::uint32_t kCount = static_cast<std::uint32_t>(DiagnosticStage::OrbitTrap) + 1U;
    const auto next = (static_cast<std::uint32_t>(diagnostic(stageMask)) + 1U) % kCount;
    return withDiagnostic(stageMask, static_cast<DiagnosticStage>(next));
}

[[nodiscard]] constexpr std::uint32_t withAllStages(std::uint32_t stageMask) noexcept {
    return (stageMask & ~kAllStages) | kAllStages;
}

// Preserve unknown bits for forward-compatible state files, but canonicalize
// the diagnostic field before it crosses a CPU/GPU or persistence boundary.
[[nodiscard]] constexpr std::uint32_t normalize(std::uint32_t stageMask) noexcept {
    return withDiagnostic(stageMask, diagnostic(stageMask));
}

[[nodiscard]] constexpr std::string_view stageName(Stage stage) noexcept {
    switch (stage) {
    case Stage::Transform: return "Stage 1: Transform (SO(4) / Affine Plane Lift)";
    case Stage::Reduce: return "Stage 2: Reduce (Non-Euclidean Domain Folding)";
    case Stage::FeedbackSample: return "Stage 3: Feedback Sample (Temporal Recurrence)";
    case Stage::Color: return "Stage 4: Color (Analytical OKLCH / Palette Map)";
    case Stage::Seed: return "Stage 5: Seed (Gaussian Spatial Kernel Window)";
    case Stage::Blend: return "Stage 6: Blend (Convex Morphological Coupling)";
    }
    return "Unknown Stage";
}

[[nodiscard]] constexpr std::string_view stageMathematicalFormula(Stage stage) noexcept {
    switch (stage) {
    case Stage::Transform:
        return "x_{k+1} = R_{SO(4)}(\\theta_1, \\theta_2) [x_k, y_k, z_k, w]^T + b_k";
    case Stage::Reduce:
        return "x_{red} = f_{reduction}(x) in { Torus, Clifford, Ammann-Beenker, PareidoliaCathedral, ... }";
    case Stage::FeedbackSample:
        return "x_{fb} = (1 - \\beta) x_{seed} + \\beta * \\gamma * x_{prev}";
    case Stage::Color:
        return "\\mu = fract( atan2(z, x)/(2\\pi) + \\phi_0 + \\omega t + 0.12 ln(1 + ||x||) + 0.08 E_{orbit} )";
    case Stage::Seed:
        return "W(x, x_{seed}) = exp( -||x - x_{seed}||^2 / (2\\sigma^2) )";
    case Stage::Blend:
        return "x_{out} = (1 - \\alpha) x_{reduced} + \\alpha x_{seed},  \\alpha = clamp(\\alpha_0 + \\alpha_{seed} W * P(t), 0, 0.95)";
    }
    return "";
}

[[nodiscard]] constexpr std::string_view stageEducationalExplanation(Stage stage) noexcept {
    switch (stage) {
    case Stage::Transform:
        return "Lifts the current 3D spatial coordinate into 4D space via latent W coordinate, applying dual-isoclinic SO(4) rotations and inner conformal maps.";
    case Stage::Reduce:
        return "Folds unbounded Euclidean space into a compact non-Euclidean manifold (e.g. Clifford torus, Poincaré disc, or hyperbolic honeycomb).";
    case Stage::FeedbackSample:
        return "Recursively injects historical orbital trajectory state with exponential temporal decay (gamma) and feedback blend (beta).";
    case Stage::Color:
        return "Calculates an invariant material coordinate mu from the toroidal angle, orbital kinetic energy, and logarithmic domain magnitude.";
    case Stage::Seed:
        return "Evaluates a localized spatial injection kernel (Airy, Attractor, Hopf, Phyllotaxis) to anchor boundary conditions.";
    case Stage::Blend:
        return "Couples the folded iterative domain back to the seed geometry via a time-pulsed alpha blend, locking in attractor stability.";
    }
    return "";
}

[[nodiscard]] constexpr const char* diagnosticDescription(
    DiagnosticStage stage) noexcept {
    switch (stage) {
    case DiagnosticStage::FullPipeline: return "0 (Full Pipeline)";
    case DiagnosticStage::Transform: return "1 (Raw Transform Orbit)";
    case DiagnosticStage::Reduce: return "2 (Domain Reduction Space)";
    case DiagnosticStage::SeedWindow: return "3 (Seed Alpha Density Mask)";
    case DiagnosticStage::MaterialCoordinate: return "4 (Material Coordinate Isolator)";
    case DiagnosticStage::OrbitEnergy: return "5 (Orbital Energy Field)";
    case DiagnosticStage::OrbitTrap: return "6 (Hyper-Complex Orbit Trap)";
    }
    return "0 (Full Pipeline)";
}

[[nodiscard]] constexpr std::string_view diagnosticMathematicalFormula(
    DiagnosticStage stage) noexcept {
    switch (stage) {
    case DiagnosticStage::FullPipeline:
        return "x_{out} = (1 - \\alpha) x_{reduced} + \\alpha x_{seed},  \\mu = f_{color}(x)";
    case DiagnosticStage::Transform:
        return "x_{k+1} = R_{SO(4)}(\\theta_1, \\theta_2) [x_k, y_k, z_k, w]^T + b_k";
    case DiagnosticStage::Reduce:
        return "x_{red} = f_{reduction}(x) in { Torus, Clifford, Poincare, Ammann-Beenker, ... }";
    case DiagnosticStage::SeedWindow:
        return "W(x, x_{seed}) = exp( -||x - x_{seed}||^2 / (2\\sigma^2) )";
    case DiagnosticStage::MaterialCoordinate:
        return "\\mu = fract( atan2(z, x)/(2\\pi) + \\phi_0 + \\omega t + 0.12 ln(1 + ||x||) + 0.08 E_{orbit} )";
    case DiagnosticStage::OrbitEnergy:
        return "E_{orbit} = \\sum_{i=1}^N ||x_i - x_{i-1}||^2 / N";
    case DiagnosticStage::OrbitTrap:
        return "D_{trap} = \\min_{i} dist(x_i, M_{trap})";
    }
    return "";
}

[[nodiscard]] constexpr std::string_view diagnosticExplanation(
    DiagnosticStage stage) noexcept {
    switch (stage) {
    case DiagnosticStage::FullPipeline:
        return "Complete 6-stage execution: Transform -> Reduce -> Feedback -> Color -> Seed -> Blend.";
    case DiagnosticStage::Transform:
        return "Visualizes raw 4D-projected spatial orbits prior to non-Euclidean reduction folding.";
    case DiagnosticStage::Reduce:
        return "Visualizes the compact manifold geometry after domain reduction folding.";
    case DiagnosticStage::SeedWindow:
        return "Isolates the localized Gaussian injection boundary condition density mask.";
    case DiagnosticStage::MaterialCoordinate:
        return "Displays the invariant 1D scalar material coordinate mu mapped directly to luminosity.";
    case DiagnosticStage::OrbitEnergy:
        return "Maps the kinetic acceleration and velocity divergence of the dynamical attractor.";
    case DiagnosticStage::OrbitTrap:
        return "Visualizes proximity to geometric cross-sections, planes, or point attractors.";
    }
    return "";
}

// Learn Mode must explain not only the mathematical recurrence, but also why
// it remains stable when observed through several cameras.  This is a runtime
// invariant rather than a seventh math stage: OpenCL evolves one field, while
// Vulkan views only observe the same completed generation.
[[nodiscard]] constexpr std::string_view canonicalFeedbackDeliveryExplanation() noexcept {
    return "OpenCL evolves one camera-independent feedback field per generation. "
           "The completed field carries an immutable generation/reset identity and, "
           "for external memory, a ready lease. Vulkan waits for that exact lease and "
           "all desktop, stereo, or OpenXR views sample the same field; view cameras "
           "never feed back into simulation. A failed lease is quarantined instead of "
           "being replaced with CPU-generated visual data.";
}

[[nodiscard]] inline std::string formatFullPedagogicalReport(std::uint32_t stageMask) {
    std::ostringstream ss;
    ss << "=== EPIMORPHIC 3D MATHEMATICAL PIPELINE: LEARN MODE DECONSTRUCTION ===\n";
    ss << "Active Stage Mask: 0x" << std::hex << (stageMask & kAllStages) << std::dec << "\n";
    ss << "Active Diagnostic: " << diagnosticDescription(diagnostic(stageMask)) << "\n\n";

    constexpr Stage kAllOrderedStages[] = {
        Stage::Transform, Stage::Reduce, Stage::FeedbackSample,
        Stage::Color, Stage::Seed, Stage::Blend
    };

    for (const auto s : kAllOrderedStages) {
        const bool isEnabled = enabled(stageMask, s);
        ss << "[" << (isEnabled ? "ACTIVE" : "BYPASS") << "] " << stageName(s) << "\n";
        ss << "  Formula:     " << stageMathematicalFormula(s) << "\n";
        ss << "  Explanation: " << stageEducationalExplanation(s) << "\n\n";
    }
    ss << "=== CANONICAL FEEDBACK DELIVERY CONTRACT ===\n";
    ss << canonicalFeedbackDeliveryExplanation() << "\n";
    return ss.str();
}

} // namespace epi_sculpt::learn_mode

namespace epi_sculpt {
struct SculptureProgramParams;

namespace learn_mode {
[[nodiscard]] std::string formatParametricFormula(
    Stage stage, const SculptureProgramParams& params);

[[nodiscard]] constexpr float calculateStageEnergyRatio(
    std::uint32_t stageMask, Stage stage) noexcept {
    if (!enabled(stageMask, stage)) return 0.0f;
    std::uint32_t activeCount = 0;
    for (std::uint8_t s = 0; s < 6; ++s) {
        if (enabled(stageMask, static_cast<Stage>(s))) ++activeCount;
    }
    return activeCount > 0 ? (1.0f / static_cast<float>(activeCount)) : 0.0f;
}

} // namespace learn_mode
} // namespace epi_sculpt
