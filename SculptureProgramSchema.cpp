#include "SculptureProgramSchema.h"

#include "SculptureFractalCatalog.h"

namespace epi_sculpt {

std::string_view name(InnerMap inner) noexcept {
    return SculptureFractalCatalog::getInnerName(static_cast<std::uint32_t>(inner));
}

std::string_view name(ReductionMode reduction) noexcept {
    return SculptureFractalCatalog::getReductionName(static_cast<std::uint32_t>(reduction));
}

std::string_view name(StageResolutionScale scale) noexcept {
    switch (scale) {
    case StageResolutionScale::Full: return "full (50^3)";
    case StageResolutionScale::Half: return "half (25^3)";
    case StageResolutionScale::Quarter: return "quarter (12^3)";
    case StageResolutionScale::Eighth: return "eighth (6^3)";
    }
    return "unknown";
}

std::string_view name(SeedType seed) noexcept {
    return SculptureFractalCatalog::getSeedName(static_cast<std::uint32_t>(seed));
}

std::string_view name(ColorEffectType effect) noexcept {
    return SculptureFractalCatalog::getColorEffectName(static_cast<std::uint32_t>(effect));
}

std::string getPipelineDescription(const SculptureProgramParams& params) {
    auto getEquation = [](InnerMap inner) -> std::string {
        switch (inner) {
        case InnerMap::Identity: return "z";
        case InnerMap::Square: return "z^2";
        case InnerMap::Cube: return "z^3";
        case InnerMap::Sin: return "sin(z)";
        case InnerMap::Cos: return "cos(z)";
        case InnerMap::Sinh: return "sinh(z)";
        case InnerMap::Cosh: return "cosh(z)";
        case InnerMap::Tanh: return "tanh(z)";
        case InnerMap::Exp: return "e^z";
        case InnerMap::Invert: return "1/z";
        case InnerMap::Kaleidoscope: return "kal(z)";
        case InnerMap::Log: return "log(z)";
        case InnerMap::Henon: return "henon(z)";
        case InnerMap::Clifford: return "cliff(z)";
        case InnerMap::Lissajous: return "liss(z)";
        case InnerMap::Mandelbox: return "mbox(z)";
        case InnerMap::Gyroid: return "gyroid(z)";
        case InnerMap::Drop: return "drop(z)";
        case InnerMap::Mobius: return "mobius(z)";
        case InnerMap::QuaternionJulia: return "quat_julia(q)";
        case InnerMap::QuaternionMandelbrot: return "quat_mandel(q)";
        case InnerMap::OctonionFeedback: return "octonion(q)";
        case InnerMap::PoincareHyperbolic: return "poincare(z)";
        case InnerMap::KleinProjective: return "klein(z)";
        case InnerMap::CalabiYauQuintic: return "calabi_yau(z)";
        case InnerMap::OctonionCayley: return "octonion_cayley(z)";
        case InnerMap::CliffordHopf: return "cliff_hopf(z)";
        case InnerMap::LorenzAttractor: return "lorenz(z)";
        case InnerMap::RosslerAttractor: return "rossler(z)";
        case InnerMap::AizawaAttractor: return "aizawa(z)";
        case InnerMap::ChenAttractor: return "chen(z)";
        case InnerMap::ReactionDiffusionTuring: return "turing_rd(z)";
        case InnerMap::HyperbolicMandelboxFold: return "mandelbox_fold(z)";
        case InnerMap::HyperbolicExponentialHomotopy: return "homotopy_sinh_exp(z)";
        case InnerMap::OctonionAssociatorManifold: return "octonion_associator(z)";
        case InnerMap::HolomorphicPotential: return "holomorphic_pot(z)";
        case InnerMap::HyperbolicHoneycomb: return "hyp_honeycomb(z)";
        case InnerMap::KuramotoLattice: return "kuramoto_sync(z)";
        case InnerMap::QuantumWave: return "quantum_wave(z)";
        case InnerMap::JacobiEllipticSnCn: return "jacobi_sn_cn(z)";
        case InnerMap::ChebyshevPolynomial: return "chebyshev_poly(z)";
        case InnerMap::PenroseTiling4D: return "penrose_tiling(z)";
        case InnerMap::SiphonophoreAttractor: return "siphonophore_attractor(z)";
        case InnerMap::KaliSetOrbit: return "kali_set_orbit(z)";
        }
        return "f(z)";
    };
    
    std::string eqA = getEquation(params.inner);
    if (params.topologyBlend > 0.01f) {
        std::string eqB = getEquation(params.innerB);
        return "f(z) = Post * lerp(" + eqA + ", " + eqB + ", blend)";
    }
    
    std::string fullEq = eqA;
    // Simple textual replacement for inner map educational display
    size_t zPos = fullEq.find('z');
    if (zPos != std::string::npos) {
        fullEq.replace(zPos, 1, "(Pre * z)");
    } else {
        fullEq += "(Pre * z)";
    }
    return "f(z) = Post * " + fullEq;
}

} // namespace epi_sculpt
