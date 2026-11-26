// Renderer-neutral starting frames for bounded dedicated-world recipes.
// These are live-orbit defaults, never captured-camera replacements.

#pragma once

#include "SculptureWorldRecipe.h"
#include <cmath>

namespace epi_sculpt {

struct SculptureWorldFrameHint {
    float eyeX = 0.0f;
    float eyeY = 4.0f;
    float eyeZ = 12.0f;
    float targetX = 0.0f;
    float targetY = 1.0f;
    float targetZ = 0.0f;
};

[[nodiscard]] constexpr SculptureWorldFrameHint makeWorldFrameHint(
    const WorldRecipe& recipe) noexcept {
    switch (recipe.generator) {
    case WorldGeneratorKind::KuramotoLattice: return {0.0f, 2.4f, 13.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::HopfFibers: return {0.0f, 3.0f, 12.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::GoldenPhyllotaxis: return {0.0f, 7.0f, 18.0f, 0.0f, 1.2f, 0.0f};
    case WorldGeneratorKind::StrangeAttractorLanterns: return {0.0f, 3.2f, 12.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::Cell120Hologram: return {0.0f, 3.8f, 15.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::AmmannBeenkerField: return {0.0f, 8.5f, 15.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::ChladniMembrane: return {0.0f, 10.5f, 13.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::LichtenbergForest: return {0.0f, 5.5f, 17.0f, 0.0f, 1.0f, 0.0f};
    case WorldGeneratorKind::EpimorphicGyroidMaze: return {0.0f, 1.0f, 15.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::OrbitSamples: return {0.0f, 2.5f, 13.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::HypercubeTesseract: return {0.0f, 3.5f, 14.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::ProceduralWoodland: return {0.0f, 4.0f, 15.0f, 0.0f, 1.0f, 0.0f};
    case WorldGeneratorKind::BioluminescentSporeSwarm: return {0.0f, 3.0f, 13.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::DeepSeaSiphonophores: return {0.0f, 3.5f, 14.5f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::NeuralSynapseLattice: return {0.0f, 3.0f, 13.5f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::KaliSetOrbits: return {0.0f, 2.0f, 12.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::MandelbulbFractal: return {0.0f, 2.5f, 14.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::CalabiYauManifold: return {0.0f, 3.5f, 14.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::LSystemFractal: return {0.0f, 6.0f, 20.0f, 0.0f, 3.0f, 0.0f};
    case WorldGeneratorKind::HydrogenAtomOrbitals: return {0.0f, 4.0f, 16.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::CliffordHopfSolitonLattice: return {0.0f, 4.2f, 16.5f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::PenroseTwistorFoliation: return {0.0f, 3.8f, 15.5f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::LieE8GossetLattice: return {0.0f, 4.0f, 17.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::ConformalRelief25D: return {0.0f, 6.0f, 14.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::ToroidalFoliation35D: return {0.0f, 4.0f, 15.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::DedekindEtaQuarticLattice4D: return {0.0f, 4.5f, 16.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::KuramotoHyperbolicCanopy35D: return {0.0f, 5.0f, 15.0f, 0.0f, 0.5f, 0.0f};
    case WorldGeneratorKind::HyperbolicHoneycombHorosphere4D: return {0.0f, 4.2f, 16.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::Hecatonicosachoron120Cell4D: return {0.0f, 3.8f, 15.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::DiracMonopoleQuantumTunnel: return {0.0f, 3.2f, 15.0f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::Su2YangMillsInstanton: return {0.0f, 3.5f, 15.5f, 0.0f, 0.0f, 0.0f};
    case WorldGeneratorKind::NeonBubblesphere:
        // These are authored viewing defaults, not alternate scene geometry.
        // Sparse triangles favour a close axial kaleidoscope read; dense
        // hexagons step back and lift the eye so nested truss depth separates.
        switch (recipe.mirrorTopology) {
        case MirrorCellTopology::Triangular:    return {0.0f, 2.8f, 12.8f, 0.0f, 0.25f, 0.0f};
        case MirrorCellTopology::Quadrilateral: return {1.0f, 3.3f, 14.4f, 0.0f, 0.45f, 0.0f};
        case MirrorCellTopology::Pentagonal:    return {1.8f, 3.8f, 16.0f, 0.0f, 0.65f, 0.0f};
        case MirrorCellTopology::Hexagonal:     return {2.6f, 4.6f, 18.2f, 0.0f, 0.85f, 0.0f};
        }
        break;
    }
    return {0.0f, 4.0f, 12.0f, 0.0f, 1.0f, 0.0f};
}

[[nodiscard]] constexpr float frameSquaredDistance(const SculptureWorldFrameHint& hint) noexcept {
    const float dx = hint.eyeX - hint.targetX;
    const float dy = hint.eyeY - hint.targetY;
    const float dz = hint.eyeZ - hint.targetZ;
    return dx * dx + dy * dy + dz * dz;
}

[[nodiscard]] inline float frameOrbitRadius(const SculptureWorldFrameHint& hint) noexcept {
    const float dx = hint.eyeX - hint.targetX;
    const float dy = hint.eyeY - hint.targetY;
    const float dz = hint.eyeZ - hint.targetZ;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace epi_sculpt
