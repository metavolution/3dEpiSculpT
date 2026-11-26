#pragma once

#include "SculptureProgramSchema.h"
#include <vector>
#include <string>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: 3D PROCEDURAL WORLD GENERATION (VR SUBSPACE)
// -----------------------------------------------------------------------------
// Extends the 2D epimorphic complex math into 3D volume generation.
// Uses recursive coordinate folding (Domain Reduction) and nonlinear 
// spatial inversion to generate high-level architectural Pareidolia 
// patterns (e.g. cathedrals, organic structures) specifically for VR.
// -----------------------------------------------------------------------------

struct SubspaceConfig {
    int recursionDepth = 6;
    float boxScale = 2.0f;
    float sphereRadius = 0.5f;
    float magicFold = 1.0f;
    bool enableOrganicDistortion = true;
};

class Sculpture3DProceduralWorld {
public:
    Sculpture3DProceduralWorld();

    void setConfig(const SubspaceConfig& config);
    const SubspaceConfig& config() const;

    // Evaluates a 3D point in the VR subspace, returning distance and procedural material
    struct EvalResult {
        float distance;
        float materialDensity;
        float hueOffset;
    };

    EvalResult evaluate(float x, float y, float z) const;

private:
    SubspaceConfig config_;
    
    // Core 3D Epimorphic Fold Operator
    void fold(float& x, float& y, float& z) const;
    
    // Nonlinear spherical inversion (replaces 2D complex exp/sinh)
    void invert(float& x, float& y, float& z, float& scale) const;
};

} // namespace epi_sculpt
