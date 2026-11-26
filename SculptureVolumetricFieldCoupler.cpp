// -----------------------------------------------------------------------------
// Educational Annotation: 3D VOLUMETRIC FIELD COUPLER & PHYSICAL MEMORY ADVECTION
// -----------------------------------------------------------------------------
// Implementation of bidirectional dynamical coupling between persistent 3D
// volumetric recursive memory and procedural world instances & relief surfaces.
// -----------------------------------------------------------------------------

#include "SculptureVolumetricFieldCoupler.h"

#include <chrono>
#include <cmath>
#include <algorithm>

namespace epi_sculpt {

namespace {

inline std::array<float, 4> quatMultiply(const std::array<float, 4>& q1, const std::array<float, 4>& q2) noexcept {
    // Hamilton product: q1 = (x1, y1, z1, w1), q2 = (x2, y2, z2, w2)
    const float x1 = q1[0], y1 = q1[1], z1 = q1[2], w1 = q1[3];
    const float x2 = q2[0], y2 = q2[1], z2 = q2[2], w2 = q2[3];

    return {
        w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2,
        w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2,
        w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2,
        w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2
    };
}

inline void normalizeQuat(std::array<float, 4>& q) noexcept {
    const float lenSq = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
    if (lenSq > 1e-8f) {
        const float invLen = 1.0f / std::sqrt(lenSq);
        q[0] *= invLen;
        q[1] *= invLen;
        q[2] *= invLen;
        q[3] *= invLen;
    } else {
        q = {0.0f, 0.0f, 0.0f, 1.0f}; // Identity quaternion
    }
}

inline float wrapCoord(float val, float radius) noexcept {
    const float span = 2.0f * radius;
    if (span <= 1e-4f) return 0.0f;
    float offset = std::fmod(val + radius, span);
    if (offset < 0.0f) offset += span;
    return offset - radius;
}

} // namespace

SculptureVolumetricFieldCoupler::SculptureVolumetricFieldCoupler(const VolumetricCouplerConfig& config)
    : config_(config) {}

SculptureVec3 SculptureVolumetricFieldCoupler::computeDensityGradient(
    const SculptureVolumetricMemory& memory,
    float x, float y, float z,
    float epsilon
) const noexcept {
    const auto field = memory.tryAcquireHostField();
    return field ? computeDensityGradient(*field, x, y, z, epsilon) : SculptureVec3{};
}

SculptureVec3 SculptureVolumetricFieldCoupler::computeDensityGradient(
    const SculptureVolumetricMemory::HostFieldView& field,
    float x, float y, float z,
    float epsilon
) const noexcept {
    const float eps = (epsilon > 1e-5f) ? epsilon : 0.035f;
    const float inv2eps = 0.5f / eps;

    const float dX = (field.sampleDensity(x + eps, y, z) - field.sampleDensity(x - eps, y, z)) * inv2eps;
    const float dY = (field.sampleDensity(x, y + eps, z) - field.sampleDensity(x, y - eps, z)) * inv2eps;
    const float dZ = (field.sampleDensity(x, y, z + eps) - field.sampleDensity(x, y, z - eps)) * inv2eps;

    return SculptureVec3{dX, dY, dZ};
}

SculptureVec3 SculptureVolumetricFieldCoupler::computeVelocityVorticity(
    const SculptureVolumetricMemory& memory,
    float x, float y, float z,
    float epsilon
) const noexcept {
    const auto field = memory.tryAcquireHostField();
    return field ? computeVelocityVorticity(*field, x, y, z, epsilon) : SculptureVec3{};
}

SculptureVec3 SculptureVolumetricFieldCoupler::computeVelocityVorticity(
    const SculptureVolumetricMemory::HostFieldView& field,
    float x, float y, float z,
    float epsilon
) const noexcept {
    const float eps = (epsilon > 1e-5f) ? epsilon : 0.035f;
    const float inv2eps = 0.5f / eps;

    // Sample velocity vector at +/- eps on all 3 coordinate axes
    const auto uXp = field.sampleWorld(x + eps, y, z).velocityEnergy;
    const auto uXm = field.sampleWorld(x - eps, y, z).velocityEnergy;
    const auto uYp = field.sampleWorld(x, y + eps, z).velocityEnergy;
    const auto uYm = field.sampleWorld(x, y - eps, z).velocityEnergy;
    const auto uZp = field.sampleWorld(x, y, z + eps).velocityEnergy;
    const auto uZm = field.sampleWorld(x, y, z - eps).velocityEnergy;

    // omega = curl(u) = (du_z/dy - du_y/dz, du_x/dz - du_z/dx, du_y/dx - du_x/dy)
    const float duz_dy = (uYp[2] - uYm[2]) * inv2eps;
    const float duy_dz = (uZp[1] - uZm[1]) * inv2eps;
    const float dux_dz = (uZp[0] - uZm[0]) * inv2eps;
    const float duz_dx = (uXp[2] - uXm[2]) * inv2eps;
    const float duy_dx = (uXp[1] - uXm[1]) * inv2eps;
    const float dux_dy = (uYp[0] - uYm[0]) * inv2eps;

    return SculptureVec3{
        duz_dy - duy_dz,
        dux_dz - duz_dx,
        duy_dx - dux_dy
    };
}

void SculptureVolumetricFieldCoupler::coupleInstances(
    const SculptureVolumetricMemory& memory,
    std::span<world_gpu_abi::WorldInstanceGpu> instances,
    float deltaSeconds,
    float /*simTime*/
) {
    telemetry_.hostFieldUnavailable = false;
    if (!config_.enableCoupling || instances.empty()) {
        telemetry_.active = false;
        return;
    }

    const auto startTime = std::chrono::high_resolution_clock::now();

    // Acquire before occupancy can cull an instance. A failed transfer is not
    // evidence of vacuum and cannot authorize partial coupling from stale data.
    const auto field = memory.tryAcquireHostField();
    if (!field) {
        resetTelemetry();
        telemetry_.hostFieldUnavailable = true;
        return;
    }

    const float dt = std::clamp(deltaSeconds, 0.0001f, 0.1f);
    const float R = (config_.worldBoundingRadius > 0.1f) ? config_.worldBoundingRadius : 2.5f;

    std::size_t processedCount = 0U;
    std::size_t culledCount = 0U;
    double densityAccum = 0.0;
    float peakVel = 0.0f;
    double vorticityAccum = 0.0;
    double energyModAccum = 0.0;

    for (auto& inst : instances) {
        float px = inst.positionScale[0];
        float py = inst.positionScale[1];
        float pz = inst.positionScale[2];

        // 1. Hierarchical Macro-Occupancy & Vacancy Check
        if (config_.enableVacancyCulling) {
            if (!field->isWorldPositionOccupied(px, py, pz)) {
                inst.positionScale[3] = 0.0f; // Zero uniform scale effectively culls instance
                ++culledCount;
                ++processedCount;
                continue;
            }
        }

        // 2. Continuous Trilinear Voxel Sampling
        const auto voxel = field->sampleWorld(px, py, pz);
        const float rho = std::clamp(voxel.colorDensity[3], 0.0f, 1.0f);
        const float vx = voxel.velocityEnergy[0];
        const float vy = voxel.velocityEnergy[1];
        const float vz = voxel.velocityEnergy[2];
        const float energy = std::max(0.0f, voxel.velocityEnergy[3]);

        // Vacancy density threshold check
        if (config_.enableVacancyCulling && rho < config_.vacancyDensityThreshold) {
            inst.positionScale[3] = 0.0f;
            ++culledCount;
            ++processedCount;
            continue;
        }

        const float velMag = std::sqrt(vx * vx + vy * vy + vz * vz);
        if (velMag > peakVel) peakVel = velMag;
        densityAccum += rho;

        // 3. Physical Flow Advection
        if (config_.enableAdvection) {
            px += vx * dt * config_.advectionRate;
            py += vy * dt * config_.advectionRate;
            pz += vz * dt * config_.advectionRate;
        }

        // 4. Density-Gradient Buoyancy & Repulsion Force
        if (config_.enableDensityBuoyancy) {
            const auto grad = computeDensityGradient(*field, px, py, pz, config_.gradientProbeEpsilon);
            px += grad.x * dt * config_.buoyancyStrength;
            py += grad.y * dt * config_.buoyancyStrength;
            pz += grad.z * dt * config_.buoyancyStrength;
        }

        // 5. Spatial Boundary Handling (Toroidal Wrapping vs Bounding Box Clamp)
        if (config_.enableToroidalWrap) {
            px = wrapCoord(px, R);
            py = wrapCoord(py, R);
            pz = wrapCoord(pz, R);
        } else if (config_.clampToWorldBounds) {
            px = std::clamp(px, -R, R);
            py = std::clamp(py, -R, R);
            pz = std::clamp(pz, -R, R);
        }

        inst.positionScale[0] = px;
        inst.positionScale[1] = py;
        inst.positionScale[2] = pz;

        // 6. Fluid Vorticity & Eddy Angular Momentum (Quaternion Spin)
        if (config_.enableVorticitySpin) {
            const auto omega = computeVelocityVorticity(*field, px, py, pz, config_.gradientProbeEpsilon);
            const float omegaMag = std::sqrt(omega.x * omega.x + omega.y * omega.y + omega.z * omega.z);
            vorticityAccum += omegaMag;

            if (omegaMag > 1e-4f) {
                const float spinAngle = omegaMag * dt * config_.vorticitySpinRate;
                const float halfAngle = spinAngle * 0.5f;
                const float sinHalf = std::sin(halfAngle);
                const float invMag = 1.0f / omegaMag;

                const std::array<float, 4> deltaQ{
                    omega.x * invMag * sinHalf,
                    omega.y * invMag * sinHalf,
                    omega.z * invMag * sinHalf,
                    std::cos(halfAngle)
                };

                inst.orientation = quatMultiply(deltaQ, inst.orientation);
                normalizeQuat(inst.orientation);
            }
        }

        // 7. Color & Orbit Energy Emission Coupling
        if (config_.enableColorCoupling) {
            const float blend = std::clamp(config_.colorCouplingWeight * (0.25f + 0.75f * rho), 0.0f, 1.0f);
            inst.tintEmission[0] = (1.0f - blend) * inst.tintEmission[0] + blend * voxel.colorDensity[0];
            inst.tintEmission[1] = (1.0f - blend) * inst.tintEmission[1] + blend * voxel.colorDensity[1];
            inst.tintEmission[2] = (1.0f - blend) * inst.tintEmission[2] + blend * voxel.colorDensity[2];
        }

        if (config_.enableEnergyModulation) {
            const float energyMod = 1.0f + config_.energyEmissionGain * energy;
            inst.tintEmission[3] *= energyMod;
            energyModAccum += energyMod;
        }

        // 8. PBR Roughness & Turbulence Coupling
        if (config_.enablePbrTurbulence) {
            const float kineticEnergy = 0.5f * (vx * vx + vy * vy + vz * vz);
            const float baseRoughness = (inst.pbrOverride[1] >= 0.0f) ? inst.pbrOverride[1] : 0.45f;
            inst.pbrOverride[1] = std::clamp(baseRoughness + config_.roughnessGain * kineticEnergy * (0.5f + 0.5f * rho), 0.04f, 0.96f);
            if (inst.pbrOverride[0] < 0.0f) {
                inst.pbrOverride[0] = 0.50f; // Canonical metallic baseline
            }
        }

        ++processedCount;
    }

    const auto endTime = std::chrono::high_resolution_clock::now();
    const float elapsedMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    // Update Telemetry
    telemetry_.totalInstancesProcessed = processedCount;
    telemetry_.culledVacantInstances = culledCount;
    telemetry_.meanSampledDensity = processedCount > 0 ? static_cast<float>(densityAccum / processedCount) : 0.0f;
    telemetry_.peakSampledVelocity = peakVel;
    telemetry_.meanVorticityMagnitude = processedCount > 0 ? static_cast<float>(vorticityAccum / processedCount) : 0.0f;
    telemetry_.meanEnergyModulation = processedCount > 0 ? static_cast<float>(energyModAccum / processedCount) : 1.0f;
    telemetry_.couplingExecutionTimeMs = elapsedMs;
    telemetry_.active = true;
}

void SculptureVolumetricFieldCoupler::coupleSnapshot(
    const SculptureVolumetricMemory& memory,
    world_gpu_abi::WorldSnapshotGpu& snapshot,
    float deltaSeconds,
    float simTime
) {
    const std::uint32_t activeCount = std::min(
        snapshot.frame.activeInstanceCount,
        static_cast<std::uint32_t>(snapshot.instances.size())
    );
    coupleInstances(
        memory,
        std::span<world_gpu_abi::WorldInstanceGpu>(snapshot.instances.data(), activeCount),
        deltaSeconds,
        simTime
    );
}

std::vector<float> SculptureVolumetricFieldCoupler::extractReliefSlice(
    const SculptureVolumetricMemory& memory,
    std::uint32_t width,
    std::uint32_t height,
    float sliceZ,
    float energyMix
) const {
    const auto observation = memory.tryAcquireHostField();
    if (!observation) return {};

    const std::uint32_t w = width > 0 ? width : 128U;
    const std::uint32_t h = height > 0 ? height : 128U;
    std::vector<float> field(static_cast<std::size_t>(w) * h, 0.0f);

    const float R = (config_.worldBoundingRadius > 0.1f) ? config_.worldBoundingRadius : 2.5f;
    const float clampedSliceZ = std::clamp(sliceZ, -R, R);
    const float mix = std::clamp(energyMix, 0.0f, 1.0f);

    for (std::uint32_t j = 0U; j < h; ++j) {
        const float v = (static_cast<float>(j) + 0.5f) / static_cast<float>(h);
        const float y = (2.0f * v - 1.0f) * R;

        for (std::uint32_t i = 0U; i < w; ++i) {
            const float u = (static_cast<float>(i) + 0.5f) / static_cast<float>(w);
            const float x = (2.0f * u - 1.0f) * R;

            const auto voxel = observation->sampleWorld(x, y, clampedSliceZ);
            const float rho = std::clamp(voxel.colorDensity[3], 0.0f, 1.0f);
            const float energy = std::clamp(voxel.velocityEnergy[3], 0.0f, 1.0f);

            const float combined = (1.0f - mix) * rho + mix * energy;
            field[static_cast<std::size_t>(j) * w + i] = std::clamp(combined, 0.0f, 1.0f);
        }
    }

    return field;
}

std::vector<float> SculptureVolumetricFieldCoupler::extractOrientedSlice(
    const SculptureVolumetricMemory& memory,
    std::uint32_t width,
    std::uint32_t height,
    const SculptureVec3& origin,
    const SculptureVec3& uAxis,
    const SculptureVec3& vAxis,
    float halfExtent,
    float energyMix
) const {
    const auto observation = memory.tryAcquireHostField();
    if (!observation) return {};

    const std::uint32_t w = width > 0 ? width : 128U;
    const std::uint32_t h = height > 0 ? height : 128U;
    std::vector<float> field(static_cast<std::size_t>(w) * h, 0.0f);

    const float extent = halfExtent > 0.1f ? halfExtent : 2.5f;
    const float mix = std::clamp(energyMix, 0.0f, 1.0f);

    for (std::uint32_t j = 0U; j < h; ++j) {
        const float v = (2.0f * (static_cast<float>(j) + 0.5f) / static_cast<float>(h)) - 1.0f;
        const float vScaled = v * extent;

        for (std::uint32_t i = 0U; i < w; ++i) {
            const float u = (2.0f * (static_cast<float>(i) + 0.5f) / static_cast<float>(w)) - 1.0f;
            const float uScaled = u * extent;

            const float px = origin.x + uScaled * uAxis.x + vScaled * vAxis.x;
            const float py = origin.y + uScaled * uAxis.y + vScaled * vAxis.y;
            const float pz = origin.z + uScaled * uAxis.z + vScaled * vAxis.z;

            const auto voxel = observation->sampleWorld(px, py, pz);
            const float rho = std::clamp(voxel.colorDensity[3], 0.0f, 1.0f);
            const float energy = std::clamp(voxel.velocityEnergy[3], 0.0f, 1.0f);

            const float combined = (1.0f - mix) * rho + mix * energy;
            field[static_cast<std::size_t>(j) * w + i] = std::clamp(combined, 0.0f, 1.0f);
        }
    }

    return field;
}

} // namespace epi_sculpt
