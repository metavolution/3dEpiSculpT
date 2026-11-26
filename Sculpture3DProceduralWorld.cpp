#include "Sculpture3DProceduralWorld.h"
#include <cmath>
#include <algorithm>

namespace epi_sculpt {

Sculpture3DProceduralWorld::Sculpture3DProceduralWorld() = default;

void Sculpture3DProceduralWorld::setConfig(const SubspaceConfig& config) {
    config_ = config;
}

const SubspaceConfig& Sculpture3DProceduralWorld::config() const {
    return config_;
}

void Sculpture3DProceduralWorld::fold(float& x, float& y, float& z) const {
    // 3D "Torus/Grid" reduction equivalent to the 2D domain fold
    x = std::abs(x);
    y = std::abs(y);
    z = std::abs(z);
    
    if (x < y) std::swap(x, y);
    if (x < z) std::swap(x, z);
    if (y < z) std::swap(y, z);
    
    // Asymmetric translation for Pareidolia / Wabi-Sabi organic feeling
    if (config_.enableOrganicDistortion) {
        x -= 0.1f;
        y -= 0.05f;
    }
}

void Sculpture3DProceduralWorld::invert(float& x, float& y, float& z, float& scale) const {
    // 3D Spherical Inversion (Mandelbox-style inner nonlinear map)
    float r2 = x*x + y*y + z*z;
    float r2_clamped = std::max(r2, 0.0001f);
    
    if (r2 < config_.sphereRadius) {
        // Inner core inversion (creates the "cathedral" arches)
        float factor = 1.0f / config_.sphereRadius;
        x *= factor;
        y *= factor;
        z *= factor;
        scale *= factor;
    } else if (r2 < 1.0f) {
        // Outer shell inversion
        float factor = 1.0f / r2_clamped;
        x *= factor;
        y *= factor;
        z *= factor;
        scale *= factor;
    }
}

Sculpture3DProceduralWorld::EvalResult Sculpture3DProceduralWorld::evaluate(float x, float y, float z) const {
    float scale = 1.0f;
    float offset_x = x * config_.magicFold;
    float offset_y = y * config_.magicFold;
    float offset_z = z * config_.magicFold;
    
    for (int i = 0; i < config_.recursionDepth; ++i) {
        fold(x, y, z);
        invert(x, y, z, scale);
        
        x = x * config_.boxScale + offset_x;
        y = y * config_.boxScale + offset_y;
        z = z * config_.boxScale + offset_z;
        scale = scale * config_.boxScale + 1.0f; // Scale accumulation
    }
    
    float length = std::sqrt(x*x + y*y + z*z);
    float distance = length / std::abs(scale);
    
    // Pareidolia Material: extract semantic depth features
    float density = std::exp(-distance * 2.0f);
    float hue = std::fmod(std::abs(x * y * z) * 0.1f, 1.0f);
    
    return {distance, density, hue};
}

} // namespace epi_sculpt
