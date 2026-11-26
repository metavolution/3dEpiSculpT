#include "SculptureSpatialMemoryField.h"

#include <cmath>
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace epi_sculpt {

namespace {

inline float clampf(float val, float minVal, float maxVal) noexcept {
    return std::max(minVal, std::min(val, maxVal));
}

// Simple deterministic harmonic potential for 3D curl field
inline float harmonicPotential(float x, float y, float z, float seed) noexcept {
    return std::sin(x * 1.7f + seed) * std::cos(y * 1.3f - seed * 0.7f) * std::sin(z * 1.9f + seed * 1.3f);
}

inline float evaluateSeedSpatialPattern(const SculptureProgramParams& params, float wx, float wy, float wz, float simTime) noexcept {
    const float dist = std::sqrt(wx * wx + wy * wy + wz * wz);
    const float w = std::max(params.seedWidth, 0.001f);
    constexpr float pi = 3.14159265358979323846f;

    switch (params.seedType) {
    case SeedType::Border: {
        const float s1 = std::clamp((dist - w) / w, 0.0f, 1.0f);
        const float s = 1.0f - (s1 * s1 * (3.0f - 2.0f * s1));
        return s * s;
    }
    case SeedType::WaveRing: {
        const float cosVal = std::cos(25.0f * dist - simTime * 5.0f);
        const float ring = std::clamp((cosVal - 0.7f) / 0.3f, 0.0f, 1.0f);
        const float env = 1.0f - std::clamp(dist / (3.0f * w), 0.0f, 1.0f);
        return ring * env;
    }
    case SeedType::Cross: {
        const float minAxis = std::min({std::abs(wx), std::abs(wy), std::abs(wz)});
        return 1.0f - std::clamp((minAxis - w * 0.1f) / (w * 0.4f), 0.0f, 1.0f);
    }
    case SeedType::Dipole: {
        const float r = std::max(dist, 0.001f);
        const float cosTh = wy / r;
        const float sin2Th = 1.0f - cosTh * cosTh;
        return std::clamp((sin2Th * std::cos(15.0f * r - simTime * 6.0f)) / (1.0f + r * 2.0f), 0.0f, 1.0f);
    }
    case SeedType::Airy: {
        const float r = std::max(dist, 0.0001f);
        const float x = 15.0f * r;
        const float airy = 2.0f * (std::sin(x) / (x * x) - std::cos(x) / x) / x;
        return std::clamp(airy * airy, 0.0f, 1.0f);
    }
    case SeedType::Lissajous: {
        const float lx = 0.5f * std::sin(3.0f * simTime + wx * 5.0f);
        const float ly = 0.5f * std::sin(2.0f * simTime + wy * 5.0f + 1.57079632f);
        const float lz = 0.5f * std::sin(4.0f * simTime + wz * 5.0f);
        const float d = std::sqrt((wx - lx)*(wx - lx) + (wy - ly)*(wy - ly) + (wz - lz)*(wz - lz));
        return 1.0f - std::clamp(d / w, 0.0f, 1.0f);
    }
    case SeedType::Attractor: {
        const float dx = 10.0f * (wy - wx);
        const float dy = wx * (28.0f - wz) - wy;
        const float dz = wx * wy - 2.666f * wz;
        const float speed = std::sqrt(dx * dx + dy * dy + dz * dz) * 0.05f;
        return 1.0f - std::clamp(std::abs(speed - 1.0f) / (w * 2.0f), 0.0f, 1.0f);
    }
    case SeedType::Hopf: {
        const float r_xy = std::sqrt(wx * wx + wy * wy);
        const float d_core = std::abs(r_xy - 1.0f);
        const float d_hopf = std::sqrt(d_core * d_core + wz * wz);
        return 1.0f - std::clamp(std::abs(d_hopf - 0.35f) / (w * 2.5f), 0.0f, 1.0f);
    }
    case SeedType::HyperSphere4D: {
        const float r4 = std::sqrt(dist * dist + params.latentW * params.latentW);
        return 1.0f - std::clamp(std::abs(r4 - 1.0f) / (w * 2.0f), 0.0f, 1.0f);
    }
    case SeedType::CrossPolytope4D: {
        const float m4 = std::abs(wx) + std::abs(wy) + std::abs(wz) + std::abs(params.latentW);
        return 1.0f - std::clamp(std::abs(m4 - 1.2f) / (w * 2.0f), 0.0f, 1.0f);
    }
    case SeedType::TorusKnotPQ: {
        const float phi = std::atan2(wy, wx);
        const float r_xy = std::sqrt(wx * wx + wy * wy);
        const float knot_r = 1.0f + 0.35f * std::cos(3.0f * phi + simTime * 0.5f);
        const float knot_z = 0.35f * std::sin(2.0f * phi + simTime * 0.5f);
        const float d = std::sqrt((r_xy - knot_r) * (r_xy - knot_r) + (wz - knot_z) * (wz - knot_z));
        return 1.0f - std::clamp(d / (w * 2.2f), 0.0f, 1.0f);
    }
    case SeedType::ChladniCymatics: {
        const float drift = simTime * 0.25f;
        const float psi = std::cos(3.0f * pi * wx + drift) * std::cos(2.0f * pi * wy - drift * 0.5f) * std::cos(4.0f * pi * wz)
                        - std::sin(2.0f * pi * wx - drift) * std::sin(3.0f * pi * wy + drift * 0.5f) * std::sin(4.0f * pi * wz);
        return 1.0f - std::clamp(std::abs(psi) / (w * 2.0f), 0.0f, 1.0f);
    }
    case SeedType::TuringPerturbation: {
        const float spotWave = std::cos(8.0f * wx) * std::cos(8.0f * wy) * std::cos(8.0f * wz);
        const float vConc = std::clamp(0.5f + 0.5f * spotWave - dist * 0.5f, 0.0f, 1.0f);
        return 1.0f - std::clamp(std::abs(vConc - 0.45f) / 0.5f, 0.0f, 1.0f);
    }
    case SeedType::JuliaQuaternionicSlice: {
        const float qx = wx, qy = wy, qz = wz, qw = params.latentW;
        const float qlen = std::sqrt(qx * qx + qy * qy + qz * qz + qw * qw);
        return 1.0f - std::clamp(std::abs(qlen - 1.0f) / (w * 2.0f), 0.0f, 1.0f);
    }
    case SeedType::OctonionFibrationS7: {
        const float r_xy = std::sqrt(wx * wx + wy * wy);
        const float d8 = std::sqrt(r_xy * r_xy + wz * wz + params.latentW * params.latentW);
        return 1.0f - std::clamp(std::abs(d8 - 0.85f) / (w * 2.2f), 0.0f, 1.0f);
    }
    case SeedType::FibonacciPhyllotaxis: {
        const float r = std::max(dist, 1e-4f);
        const float theta = std::atan2(wy, wx);
        const float phi = std::acos(std::clamp(wz / r, -1.0f, 1.0f));
        return 1.0f - std::clamp(std::abs(std::sin(13.0f * theta + 21.0f * phi) - 0.5f) / (w * 1.5f), 0.0f, 1.0f);
    }
    case SeedType::KuramotoSynchronization: {
        const float r = std::max(dist, 1e-4f);
        const float theta = std::atan2(wy, wx);
        const float phi = std::acos(std::clamp(wz / r, -1.0f, 1.0f));
        const float globalMeanPhase = simTime * 2.5f;
        const float sync = std::sin(3.0f * theta - globalMeanPhase) + std::sin(5.0f * phi - globalMeanPhase * 1.618f);
        return 1.0f - std::clamp(std::abs(sync - 0.5f) / (w * 2.0f), 0.0f, 1.0f);
    }
    }
    return 0.5f;
}

} // namespace

SculptureSpatialMemoryField::SculptureSpatialMemoryField(std::size_t resolution, float worldExtent)
{
    resize(resolution, worldExtent);
}

SculptureSpatialMemoryField::~SculptureSpatialMemoryField() = default;
SculptureSpatialMemoryField::SculptureSpatialMemoryField(SculptureSpatialMemoryField&&) noexcept = default;
SculptureSpatialMemoryField& SculptureSpatialMemoryField::operator=(SculptureSpatialMemoryField&&) noexcept = default;

void SculptureSpatialMemoryField::resize(std::size_t resolution, float worldExtent) {
    const auto nextResolution = std::max<std::size_t>(8U, resolution);
    // N^3 must fit the vector's element/byte capacity. Divide before multiplying
    // so even an extreme request can be rejected without integer overflow.
    const auto capacity = std::min(fieldA_.max_size(), fieldB_.max_size());
    if (nextResolution > capacity / nextResolution / nextResolution) {
        throw std::length_error("spatial field dimensions exceed buffer capacity");
    }
    const auto nextCount = nextResolution * nextResolution * nextResolution;
    if (!std::isfinite(worldExtent)) {
        throw std::invalid_argument("spatial field extent must be finite");
    }
    const float nextExtent = std::max(0.1f, worldExtent);
    const float nextSpan = nextExtent * 2.0f;
    const float nextSpacing = nextSpan / static_cast<float>(nextResolution);
    if (!std::isfinite(nextSpan) || !std::isfinite(nextSpacing) || nextSpacing <= 0.0f) {
        throw std::invalid_argument("spatial field geometry is not representable");
    }

    // Keep both old histories and their selected pointers valid until BOTH
    // candidates exist. SpatialVoxel defaults already encode the reset state.
    std::vector<SpatialVoxel> nextA(nextCount);
    std::vector<SpatialVoxel> nextB(nextCount);

    // Standard-allocator vector swaps and the remaining assignments cannot throw.
    fieldA_.swap(nextA);
    fieldB_.swap(nextB);
    resolution_ = nextResolution;
    totalVoxels_ = nextCount;
    worldExtent_ = nextExtent;
    voxelSize_ = nextSpacing;
    readBuffer_ = fieldA_.data();
    writeBuffer_ = fieldB_.data();
    totalEnergy_ = 0.0f;
    meanDensity_ = 0.0f;
}

void SculptureSpatialMemoryField::reset() noexcept {
    for (std::size_t i = 0; i < totalVoxels_; ++i) {
        if (i < fieldA_.size()) {
            fieldA_[i] = SpatialVoxel{};
            fieldA_[i].chemicalU = 1.0f;
        }
        if (i < fieldB_.size()) {
            fieldB_[i] = SpatialVoxel{};
            fieldB_[i].chemicalU = 1.0f;
        }
    }
    totalEnergy_ = 0.0f;
    meanDensity_ = 0.0f;
    
}

std::array<float, 3> SculptureSpatialMemoryField::evaluateCurlNoise(
    float x, float y, float z, float freq, float strength) noexcept {
    if (strength <= 0.0001f) return {0.0f, 0.0f, 0.0f};

    const float eps = 0.01f;
    const float inv2eps = 0.5f / eps;
    const float px = x * freq;
    const float py = y * freq;
    const float pz = z * freq;

    // Vector potential Psi = (Psi_x, Psi_y, Psi_z)
    // Curl = (dPsi_z/dy - dPsi_y/dz, dPsi_x/dz - dPsi_z/dx, dPsi_y/dx - dPsi_x/dy)
    const float psi_z_py = harmonicPotential(px, py + eps, pz, 1.0f);
    const float psi_z_my = harmonicPotential(px, py - eps, pz, 1.0f);
    const float psi_y_pz = harmonicPotential(px, py, pz + eps, 2.0f);
    const float psi_y_mz = harmonicPotential(px, py, pz - eps, 2.0f);

    const float psi_x_pz = harmonicPotential(px, py, pz + eps, 3.0f);
    const float psi_x_mz = harmonicPotential(px, py, pz - eps, 3.0f);
    const float psi_z_px = harmonicPotential(px + eps, py, pz, 1.0f);
    const float psi_z_mx = harmonicPotential(px - eps, py, pz, 1.0f);

    const float psi_y_px = harmonicPotential(px + eps, py, pz, 2.0f);
    const float psi_y_mx = harmonicPotential(px - eps, py, pz, 2.0f);
    const float psi_x_py = harmonicPotential(px, py + eps, pz, 3.0f);
    const float psi_x_my = harmonicPotential(px, py - eps, pz, 3.0f);

    const float curlX = (psi_z_py - psi_z_my - (psi_y_pz - psi_y_mz)) * inv2eps * strength;
    const float curlY = (psi_x_pz - psi_x_mz - (psi_z_px - psi_z_mx)) * inv2eps * strength;
    const float curlZ = (psi_y_px - psi_y_mx - (psi_x_py - psi_x_my)) * inv2eps * strength;

    return {curlX, curlY, curlZ};
}

SpatialFieldSample SculptureSpatialMemoryField::sampleTrilinear(float x, float y, float z) const noexcept {
    // Map [-worldExtent, +worldExtent] -> [0, resolution - 1]
    const float normX = (x + worldExtent_) / (2.0f * worldExtent_);
    const float normY = (y + worldExtent_) / (2.0f * worldExtent_);
    const float normZ = (z + worldExtent_) / (2.0f * worldExtent_);

    if (normX < 0.0f || normX > 1.0f || normY < 0.0f || normY > 1.0f || normZ < 0.0f || normZ > 1.0f) {
        return {};
    }

    const float gridCoordX = normX * static_cast<float>(resolution_ - 1U);
    const float gridCoordY = normY * static_cast<float>(resolution_ - 1U);
    const float gridCoordZ = normZ * static_cast<float>(resolution_ - 1U);

    const std::size_t x0 = static_cast<std::size_t>(gridCoordX);
    const std::size_t y0 = static_cast<std::size_t>(gridCoordY);
    const std::size_t z0 = static_cast<std::size_t>(gridCoordZ);

    const std::size_t x1 = std::min(x0 + 1U, resolution_ - 1U);
    const std::size_t y1 = std::min(y0 + 1U, resolution_ - 1U);
    const std::size_t z1 = std::min(z0 + 1U, resolution_ - 1U);

    const float fx = gridCoordX - static_cast<float>(x0);
    const float fy = gridCoordY - static_cast<float>(y0);
    const float fz = gridCoordZ - static_cast<float>(z0);

    const auto& c000 = readVoxel(x0, y0, z0);
    const auto& c100 = readVoxel(x1, y0, z0);
    const auto& c010 = readVoxel(x0, y1, z0);
    const auto& c110 = readVoxel(x1, y1, z0);
    const auto& c001 = readVoxel(x0, y0, z1);
    const auto& c101 = readVoxel(x1, y0, z1);
    const auto& c011 = readVoxel(x0, y1, z1);
    const auto& c111 = readVoxel(x1, y1, z1);

    const float w000 = (1.0f - fx) * (1.0f - fy) * (1.0f - fz);
    const float w100 = fx * (1.0f - fy) * (1.0f - fz);
    const float w010 = (1.0f - fx) * fy * (1.0f - fz);
    const float w110 = fx * fy * (1.0f - fz);
    const float w001 = (1.0f - fx) * (1.0f - fy) * fz;
    const float w101 = fx * (1.0f - fy) * fz;
    const float w011 = (1.0f - fx) * fy * fz;
    const float w111 = fx * fy * fz;

    SpatialFieldSample s{};
    s.density = c000.density * w000 + c100.density * w100 + c010.density * w010 + c110.density * w110
              + c001.density * w001 + c101.density * w101 + c011.density * w011 + c111.density * w111;
    s.vx      = c000.vx * w000 + c100.vx * w100 + c010.vx * w010 + c110.vx * w110
              + c001.vx * w001 + c101.vx * w101 + c011.vx * w011 + c111.vx * w111;
    s.vy      = c000.vy * w000 + c100.vy * w100 + c010.vy * w010 + c110.vy * w110
              + c001.vy * w001 + c101.vy * w101 + c011.vy * w011 + c111.vy * w111;
    s.vz      = c000.vz * w000 + c100.vz * w100 + c010.vz * w010 + c110.vz * w110
              + c001.vz * w001 + c101.vz * w101 + c011.vz * w011 + c111.vz * w111;
    s.hue     = c000.hue * w000 + c100.hue * w100 + c010.hue * w010 + c110.hue * w110
              + c001.hue * w001 + c101.hue * w101 + c011.hue * w011 + c111.hue * w111;
    s.hue = std::fmod(s.hue, 1.0f);
    if (s.hue < 0.0f) s.hue += 1.0f;

    s.energy =
        w000 * c000.energy + w100 * c100.energy +
        w010 * c010.energy + w110 * c110.energy +
        w001 * c001.energy + w101 * c101.energy +
        w011 * c011.energy + w111 * c111.energy;
        
    s.chemicalU =
        w000 * c000.chemicalU + w100 * c100.chemicalU +
        w010 * c010.chemicalU + w110 * c110.chemicalU +
        w001 * c001.chemicalU + w101 * c101.chemicalU +
        w011 * c011.chemicalU + w111 * c111.chemicalU;
        
    s.chemicalV =
        w000 * c000.chemicalV + w100 * c100.chemicalV +
        w010 * c010.chemicalV + w110 * c110.chemicalV +
        w001 * c001.chemicalV + w101 * c101.chemicalV +
        w011 * c011.chemicalV + w111 * c111.chemicalV;

    return s;
}

void SculptureSpatialMemoryField::evolve(float deltaSeconds, const SculptureProgramParams& params, float simTime) {
    // A paused or invalid offer must not inject seed, change summaries or swap
    // history buffers. Clamping alone would turn a pause into an accepted step.
    if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f || !std::isfinite(simTime)) {
        return;
    }

    const float dt = std::clamp(deltaSeconds, 0.0001f, 0.1f);
    const float fade = std::clamp(params.temporalFade > 0.0f ? params.temporalFade : 0.96f, 0.0f, 0.999f);
    const float blend = std::clamp(params.temporalFeedbackBlend, 0.0f, 1.0f);
    const float viscosity = std::max(0.0f, params.kineticViscosity);
    const float advection = std::max(0.0f, params.advectionSpeed);
    const float curlStr = std::max(0.0f, params.curlStrength);
    const float seedPulseFreq = params.seedPulseFrequency > 0.0f ? params.seedPulseFrequency : 1.0f;
    const float seedWidth = std::max(0.05f, params.seedWidth);
    const float seedAlpha = std::clamp(params.seedAlpha, 0.0f, 1.0f);

    float accumulatedEnergy = 0.0f;
    float accumulatedDensity = 0.0f;

    const float halfRes = static_cast<float>(resolution_) * 0.5f;

    for (std::size_t z = 0; z < resolution_; ++z) {
        const float wz = (static_cast<float>(z) - halfRes) * voxelSize_;
        for (std::size_t y = 0; y < resolution_; ++y) {
            const float wy = (static_cast<float>(y) - halfRes) * voxelSize_;
            for (std::size_t x = 0; x < resolution_; ++x) {
                const float wx = (static_cast<float>(x) - halfRes) * voxelSize_;
                const std::size_t idx = index(x, y, z);
                const auto& current = readBuffer_[idx];

                // 1. Semi-Lagrangian Backtrace along velocity
                const float backX = wx - current.vx * advection * dt;
                const float backY = wy - current.vy * advection * dt;
                const float backZ = wz - current.vz * advection * dt;

                const SpatialFieldSample advected = sampleTrilinear(backX, backY, backZ);

                // 2. Curl Turbulence & Kinetic Viscosity
                const auto curl = evaluateCurlNoise(wx, wy, wz, 1.5f, curlStr);
                const float damp = std::max(0.0f, 1.0f - viscosity * dt);

                const float nextVx = advected.vx * damp + curl[0] * dt;
                const float nextVy = advected.vy * damp + curl[1] * dt;
                const float nextVz = advected.vz * damp + curl[2] * dt;

                // 3. 3D Seed Window Wavepacket Injection (WCA Formulation)
                const float seedSpatialPattern = evaluateSeedSpatialPattern(params, wx, wy, wz, simTime);
                const auto subspace = proceduralWorld_.evaluate(wx, wy, wz);
                const float pulse = std::cos(seedPulseFreq * simTime);
                const float baseDensity = seedAlpha * seedSpatialPattern * (0.5f + 0.5f * pulse);
                const float proceduralDensity = std::max(0.0f, subspace.materialDensity) * (0.5f + 0.5f * pulse) * seedAlpha;
                const float injectedDensity = std::max(baseDensity, proceduralDensity);

                // 4. Gray-Scott Reaction-Diffusion
                // Calculate 6-neighbor Laplacian (periodic boundaries)
                const std::size_t res = resolution_;
                const std::size_t xm1 = (x + res - 1) % res;
                const std::size_t xp1 = (x + 1) % res;
                const std::size_t ym1 = (y + res - 1) % res;
                const std::size_t yp1 = (y + 1) % res;
                const std::size_t zm1 = (z + res - 1) % res;
                const std::size_t zp1 = (z + 1) % res;

                const auto& cxm = readBuffer_[index(xm1, y, z)];
                const auto& cxp = readBuffer_[index(xp1, y, z)];
                const auto& cym = readBuffer_[index(x, ym1, z)];
                const auto& cyp = readBuffer_[index(x, yp1, z)];
                const auto& czm = readBuffer_[index(x, y, zm1)];
                const auto& czp = readBuffer_[index(x, y, zp1)];

                const float lapU = cxm.chemicalU + cxp.chemicalU + cym.chemicalU + cyp.chemicalU + czm.chemicalU + czp.chemicalU - 6.0f * current.chemicalU;
                const float lapV = cxm.chemicalV + cxp.chemicalV + cym.chemicalV + cyp.chemicalV + czm.chemicalV + czp.chemicalV - 6.0f * current.chemicalV;

                // Typical GS parameters (e.g. Mitosis or Coral pattern) driven by preset parameters
                const float Du = 0.16f;
                const float Dv = 0.08f;
                const float F = 0.025f + clampf(params.seedPulseFrequency, 0.0f, 0.04f); // Feed rate
                const float K = 0.055f + clampf(params.seedPulseAmplitude, 0.0f, 0.02f); // Kill rate
                const float reactionSpeed = 80.0f; // Scale dt for visibility
                
                const float uv2 = current.chemicalU * current.chemicalV * current.chemicalV;
                float nextU = current.chemicalU + (Du * lapU - uv2 + F * (1.0f - current.chemicalU)) * dt * reactionSpeed;
                float nextV = current.chemicalV + (Dv * lapV + uv2 - (F + K) * current.chemicalV) * dt * reactionSpeed;
                
                // Couple Wavepacket into V
                nextV = clampf(nextV + injectedDensity * 0.5f, 0.0f, 1.0f);
                nextU = clampf(nextU, 0.0f, 1.0f);

                // 5. Temporal Blend & Dissipation
                // Matter density is heavily driven by the Turing V concentration, combined with advection
                const float rdDensity = nextV; 
                const float nextDensity = clampf(advected.density * fade + rdDensity * blend, 0.0f, 1.0f);
                const float nextHue = std::fmod(advected.hue + params.hueSpeed * dt + 0.001f + nextV * 0.05f + std::max(0.0f, subspace.hueOffset) * dt * 0.1f, 1.0f);
                const float kineticEnergy = 0.5f * (nextVx*nextVx + nextVy*nextVy + nextVz*nextVz);

                auto& out = writeBuffer_[idx];
                out.density = nextDensity;
                out.vx = nextVx;
                out.vy = nextVy;
                out.vz = nextVz;
                out.hue = nextHue;
                out.energy = kineticEnergy + nextDensity;
                out.chemicalU = nextU;
                out.chemicalV = nextV;

                accumulatedEnergy += out.energy;
                accumulatedDensity += out.density;
            }
        }
    }

    totalEnergy_ = accumulatedEnergy;
    meanDensity_ = totalVoxels_ > 0 ? (accumulatedDensity / static_cast<float>(totalVoxels_)) : 0.0f;

    // Swap read and write buffers
    std::swap(readBuffer_, writeBuffer_);
}

void SculptureSpatialMemoryField::injectTextureSlice(
    const std::vector<float>& inTextureRgba,
    std::uint32_t width,
    std::uint32_t height,
    float zPlaneOffset,
    float weight
) {
    if (width == 0 || height == 0 || inTextureRgba.empty() || resolution_ < 2) return;
    const float w = std::clamp(weight, 0.0f, 1.0f);
    if (w <= 1e-5f) return;

    const bool isRgba = (inTextureRgba.size() >= static_cast<std::size_t>(width) * height * 4U);
    const float halfRes = static_cast<float>(resolution_) * 0.5f;
    const float sliceThickness = voxelSize_ * 1.5f;

    for (std::size_t z = 0; z < resolution_; ++z) {
        const float wz = (static_cast<float>(z) - halfRes) * voxelSize_;
        const float distZ = std::abs(wz - zPlaneOffset);
        if (distZ > sliceThickness * 2.0f) continue;
        const float zWeight = std::exp(-(distZ * distZ) / (2.0f * sliceThickness * sliceThickness));

        for (std::size_t y = 0; y < resolution_; ++y) {
            const float wy = (static_cast<float>(y) - halfRes) * voxelSize_;
            const float normY = (wy + worldExtent_) / (2.0f * worldExtent_);
            if (normY < 0.0f || normY > 1.0f) continue;
            const std::uint32_t ty = std::min(static_cast<std::uint32_t>(normY * (height - 1U)), height - 1U);

            for (std::size_t x = 0; x < resolution_; ++x) {
                const float wx = (static_cast<float>(x) - halfRes) * voxelSize_;
                const float normX = (wx + worldExtent_) / (2.0f * worldExtent_);
                if (normX < 0.0f || normX > 1.0f) continue;
                const std::uint32_t tx = std::min(static_cast<std::uint32_t>(normX * (width - 1U)), width - 1U);

                const std::size_t texIdx = static_cast<std::size_t>(tx + ty * width);
                float intensity = 0.0f;
                float hueVal = 0.0f;
                if (isRgba) {
                    const std::size_t rgbaIdx = texIdx * 4U;
                    const float r = inTextureRgba[rgbaIdx + 0];
                    const float g = inTextureRgba[rgbaIdx + 1];
                    const float b = inTextureRgba[rgbaIdx + 2];
                    intensity = 0.299f * r + 0.587f * g + 0.114f * b;
                    hueVal = std::fmod(std::atan2(g - b, r - 0.5f * (g + b)) / (2.0f * 3.14159265f) + 1.0f, 1.0f);
                } else if (texIdx < inTextureRgba.size()) {
                    intensity = inTextureRgba[texIdx];
                    hueVal = intensity;
                }

                const std::size_t idx = index(x, y, z);
                auto& voxel = readBuffer_[idx];
                const float blendFactor = w * zWeight;
                voxel.density = std::clamp(voxel.density * (1.0f - blendFactor) + intensity * blendFactor, 0.0f, 1.0f);
                voxel.hue = std::fmod(voxel.hue * (1.0f - blendFactor) + hueVal * blendFactor, 1.0f);
                voxel.energy += intensity * blendFactor;
            }
        }
    }

    float accumulatedEnergy = 0.0f;
    float accumulatedDensity = 0.0f;
    for (std::size_t i = 0; i < totalVoxels_; ++i) {
        accumulatedEnergy += readBuffer_[i].energy;
        accumulatedDensity += readBuffer_[i].density;
    }
    totalEnergy_ = accumulatedEnergy;
    meanDensity_ = totalVoxels_ > 0 ? (accumulatedDensity / static_cast<float>(totalVoxels_)) : 0.0f;
}

void SculptureSpatialMemoryField::extractTextureSlice(
    std::vector<float>& outTextureRgba,
    std::uint32_t width,
    std::uint32_t height,
    float zPlaneOffset
) const {
    if (width == 0 || height == 0) return;
    const std::size_t totalPixels = static_cast<std::size_t>(width) * height;
    outTextureRgba.assign(totalPixels * 4U, 0.0f);

    const float invW = (width > 1U) ? (1.0f / static_cast<float>(width - 1U)) : 1.0f;
    const float invH = (height > 1U) ? (1.0f / static_cast<float>(height - 1U)) : 1.0f;

    for (std::uint32_t y = 0; y < height; ++y) {
        const float wy = (static_cast<float>(y) * invH * 2.0f - 1.0f) * worldExtent_;
        for (std::uint32_t x = 0; x < width; ++x) {
            const float wx = (static_cast<float>(x) * invW * 2.0f - 1.0f) * worldExtent_;
            const SpatialFieldSample sample = sampleTrilinear(wx, wy, zPlaneOffset);

            const std::size_t pixelIdx = static_cast<std::size_t>(x + y * width) * 4U;
            outTextureRgba[pixelIdx + 0] = std::clamp(sample.density + 0.5f * std::abs(sample.vx), 0.0f, 1.0f);
            outTextureRgba[pixelIdx + 1] = std::clamp(sample.density * 0.8f + 0.5f * std::abs(sample.vy), 0.0f, 1.0f);
            outTextureRgba[pixelIdx + 2] = std::clamp(sample.hue, 0.0f, 1.0f);
            outTextureRgba[pixelIdx + 3] = std::clamp(sample.density, 0.0f, 1.0f);
        }
    }
}

void SculptureSpatialMemoryField::modulatePotentialField(
    const std::vector<float>& scalarGrid,
    float strength
) {
    if (scalarGrid.empty() || resolution_ < 2) return;
    const float s = clampf(strength, -5.0f, 5.0f);
    const std::size_t count = std::min(totalVoxels_, scalarGrid.size());

    for (std::size_t i = 0; i < count; ++i) {
        auto& voxel = readBuffer_[i];
        const float potential = scalarGrid[i];
        voxel.density = clampf(voxel.density + potential * s * 0.1f, 0.0f, 1.0f);
        voxel.energy = std::max(0.0f, voxel.energy + std::abs(potential * s));
    }

    float accumulatedEnergy = 0.0f;
    float accumulatedDensity = 0.0f;
    for (std::size_t i = 0; i < totalVoxels_; ++i) {
        accumulatedEnergy += readBuffer_[i].energy;
        accumulatedDensity += readBuffer_[i].density;
    }
    totalEnergy_ = accumulatedEnergy;
    meanDensity_ = totalVoxels_ > 0 ? (accumulatedDensity / static_cast<float>(totalVoxels_)) : 0.0f;
}

} // namespace epi_sculpt

