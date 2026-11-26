#include "SculptureRaymarcher.h"
#include "SculptureGBufferDiagnostics.h"
#include "SculptureVolumetricMemory.h"
#include "Sculpture4DFoliation.h"
#include "SculptureAtmosphereEngine.h"
#include "SculptureThreadPool.h"
#include <algorithm>
#include <array>
#include <exception>
#include <future>
#include <optional>
#include <stdexcept>
#include <thread>
#include <cmath>
#include <utility>

namespace epi_sculpt {

namespace {

struct RaymarchRowRange {
    std::uint32_t begin;
    std::uint32_t end;
};

RaymarchRowRange raymarchRows(std::uint32_t height, std::uint32_t workers,
                             std::uint32_t index) noexcept {
    // Quotient/remainder partition: every intermediate is bounded by height,
    // even at UINT32_MAX. Callers supply 1..16 workers and index < workers.
    const auto rows = height / workers;
    const auto extra = height % workers;
    const auto begin = index * rows + std::min(index, extra);
    return {begin, begin + rows + (index < extra ? 1U : 0U)};
}

struct RaymarchRayRange {
    std::size_t begin;
    std::size_t end;
};

RaymarchRayRange raymarchRays(std::size_t count, std::uint32_t workers,
                            std::uint32_t index) noexcept {
    const auto rays = count / workers;
    const auto extra = count % workers;
    const auto begin = index * rays + std::min<std::size_t>(index, extra);
    return {begin, begin + rays + (index < extra ? 1U : 0U)};
}

// Construct after the vector reservation. On a later launch failure this guard
// joins all earlier workers while their borrowed inputs/error slots still live.
class RaymarchBatchJoin final {
public:
    explicit RaymarchBatchJoin(std::vector<std::thread>& workers) noexcept : workers_(workers) {}
    RaymarchBatchJoin(const RaymarchBatchJoin&) = delete;
    RaymarchBatchJoin& operator=(const RaymarchBatchJoin&) = delete;
    ~RaymarchBatchJoin() noexcept { join(); }

    void join() noexcept {
        for (auto& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
    }

private:
    std::vector<std::thread>& workers_;
};

// Every accepted task borrows the synchronous render call's inputs/output.
// Drain before those borrows can expire, including when a later enqueue fails.
class RaymarchWorkGroup final {
public:
    static constexpr std::uint32_t kMaxWorkers = 16U;

    RaymarchWorkGroup() = default;
    RaymarchWorkGroup(const RaymarchWorkGroup&) = delete;
    RaymarchWorkGroup& operator=(const RaymarchWorkGroup&) = delete;
    ~RaymarchWorkGroup() noexcept { (void)drain(); }

    template<class Pool, class Task>
    void submit(Pool& pool, Task&& task) {
        if (submitted_ == tasks_.size()) {
            throw std::logic_error("raymarch work group capacity exceeded");
        }
        // Storage is already present; moving a future cannot throw after the
        // pool accepts work. Never lose its completion handle to allocation.
        tasks_[submitted_] = pool.enqueue(std::forward<Task>(task));
        ++submitted_;
    }

    void finish() {
        if (const auto error = drain()) std::rethrow_exception(error);
    }

private:
    std::exception_ptr drain() noexcept {
        std::exception_ptr firstError;
        for (std::size_t i = 0; i < submitted_; ++i) {
            try {
                tasks_[i].get();
            } catch (...) {
                if (!firstError) firstError = std::current_exception();
            }
        }
        submitted_ = 0;
        return firstError;
    }

    std::array<std::future<void>, kMaxWorkers> tasks_;
    std::size_t submitted_ = 0;
};

inline SculptureVec3 normalize(const SculptureVec3& v) noexcept {
    const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-6f) {
        return SculptureVec3{v.x / len, v.y / len, v.z / len};
    }
    return SculptureVec3{0.0f, 1.0f, 0.0f};
}

inline SculptureVec3 cross(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return SculptureVec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline float dot(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// A pinhole camera's basis and projection scales are constant for a render.
// Workers copy this value; only the NDC coordinates and final normalization
// vary per pixel. Both renderers and the public single-ray helper use this path.
struct PreparedMonoCamera {
    SculptureVec3 origin;
    SculptureVec3 forward;
    SculptureVec3 right;
    SculptureVec3 trueUp;
    float halfHeight;
    float halfWidth;

    [[nodiscard]] SculptureRay ray(float ndcX, float ndcY) const {
        const SculptureVec3 direction{
            forward.x + right.x * (ndcX * halfWidth) + trueUp.x * (ndcY * halfHeight),
            forward.y + right.y * (ndcX * halfWidth) + trueUp.y * (ndcY * halfHeight),
            forward.z + right.z * (ndcX * halfWidth) + trueUp.z * (ndcY * halfHeight)
        };
        const float length = std::sqrt(direction.x * direction.x
            + direction.y * direction.y + direction.z * direction.z);
        if (length > 1e-6f && std::isfinite(length)) {
            return SculptureRay{
                .origin = origin,
                .direction = SculptureVec3{
                    direction.x / length, direction.y / length, direction.z / length
                }
            };
        }
        return rayWithWideIntermediates(ndcX, ndcY);
    }

private:
    [[nodiscard]] SculptureRay rayWithWideIntermediates(float ndcX, float ndcY) const {
        // Rebuild from the original values: widening a float product after it
        // overflowed cannot recover its direction. Double holds products of
        // finite float coordinates/scales; hypot avoids squaring a large norm.
        const double horizontal = static_cast<double>(ndcX) * halfWidth;
        const double vertical = static_cast<double>(ndcY) * halfHeight;
        const double x = forward.x + right.x * horizontal + trueUp.x * vertical;
        const double y = forward.y + right.y * horizontal + trueUp.y * vertical;
        const double z = forward.z + right.z * horizontal + trueUp.z * vertical;
        const double length = std::hypot(x, y, z);
        if (!std::isfinite(length) || length <= 1e-6) {
            throw std::invalid_argument("unrepresentable mono camera ray direction");
        }
        return SculptureRay{
            .origin = origin,
            .direction = SculptureVec3{
                static_cast<float>(x / length),
                static_cast<float>(y / length),
                static_cast<float>(z / length)
            }
        };
    }
};

// Shading may use a fallback normal; a camera must retain its authored direction.
// Match the existing float normalization threshold and division for valid inputs.
[[nodiscard]] SculptureVec3 checkedCameraDirection(const SculptureVec3& v) {
    const float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (!std::isfinite(length) || length <= 1e-6f) {
        throw std::invalid_argument("degenerate or unrepresentable mono camera basis");
    }
    return SculptureVec3{v.x / length, v.y / length, v.z / length};
}

[[nodiscard]] PreparedMonoCamera prepareMonoCamera(
    const SculptureVec3& cameraPos, const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp, float fovRadians, float aspectRatio) {
    const auto finiteVector = [](const SculptureVec3& v) noexcept {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    };
    if (!finiteVector(cameraPos) || !finiteVector(cameraTarget) || !finiteVector(cameraUp)) {
        throw std::invalid_argument("nonfinite mono camera geometry");
    }
    if (!std::isfinite(fovRadians) || fovRadians <= 0.0f
        || fovRadians >= 3.14159265359f
        || !std::isfinite(aspectRatio) || aspectRatio <= 0.0f) {
        throw std::invalid_argument("invalid mono camera projection");
    }
    const SculptureVec3 forward = checkedCameraDirection(SculptureVec3{
        cameraTarget.x - cameraPos.x,
        cameraTarget.y - cameraPos.y,
        cameraTarget.z - cameraPos.z
    });
    const SculptureVec3 right = checkedCameraDirection(cross(forward, cameraUp));
    const SculptureVec3 trueUp = cross(right, forward);
    const float halfHeight = std::tan(fovRadians * 0.5f);
    const float halfWidth = halfHeight * aspectRatio;
    if (!std::isfinite(halfHeight) || halfHeight <= 0.0f
        || !std::isfinite(halfWidth) || halfWidth <= 0.0f) {
        throw std::invalid_argument("unrepresentable mono camera projection");
    }
    return {cameraPos, forward, right, trueUp, halfHeight, halfWidth};
}

} // namespace

struct SculptureRaymarcher::VolumeObservation {
    std::optional<SculptureVolumetricMemory::HostFieldView> field;
    std::optional<StageResolutionScale> resolution;
};

SculptureRaymarcher::VolumeObservation SculptureRaymarcher::acquireVolumeObservation(
    const SculptureVolumetricMemory* memory) const {
    VolumeObservation observation;
    if (!config_.enableVolumetricFog || memory == nullptr) return observation;

    // Resolution still governs procedural fog when feedback contribution is off.
    // Freeze it here so workers never query the live volume owner.
    observation.resolution = memory->currentResolutionScale();
    if (memory->config().enableFeedback) {
        observation.field = memory->tryAcquireHostField();
        if (!observation.field) {
            throw std::runtime_error("raymarch observation requires current volume host history");
        }
    }
    return observation;
}

SculptureRaymarcher::SculptureRaymarcher(SculptureRaymarchConfig config)
    : config_{} {
    if (!trySetConfig(config)) {
        throw std::invalid_argument("invalid sculpture raymarch configuration");
    }
}

float SculptureRaymarcher::estimateDistance(
    const SculptureVec3& p,
    const SculptureProgram& program,
    float time,
    float morph,
    const std::vector<float>& spatialBuffer
) const {
    const float density = program.sampleDensityField(p, time, morph, spatialBuffer);
    
    // Convert normalized density [0.0, 1.0] into a conservative signed distance field
    // Target isosurface boundary is set at density = 0.50
    const float implicitDist = (0.50f - density) * 0.35f;

    // Outer bounding sphere boundary to avoid marching in empty space
    const float radius = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    const float boundingDist = radius - config_.boundingRadius;

    return std::max(implicitDist, boundingDist);
}

float SculptureRaymarcher::estimateChamberDistance(
    const SculptureVec3& p,
    const SculptureProgram& program,
    float time,
    float morph
) const {
    const auto& params = program.params();
    const float worldScale = std::max(params.worldScale, 0.1f);
    const SculptureVec3 localP{p.x / worldScale, p.y / worldScale, p.z / worldScale};


    if (config_.chamberMode == SdfChamberMode::Foliation4DChamber || params.enable4D > 0U || params.enable4DB > 0U) {
        // Project 3D coordinate + latentW to 4D vector for Volumetric Raymarching of Hyper-Manifolds!
        SculptureVec4 p4{localP.x, localP.y, localP.z, params.latentW};
        
        // Spin the 4D polytope over time via Left-Right Isoclinic Rotations + dynamic 4D rotors
        const float thetaL = params.rotorAlpha + time * 0.35f;
        const float thetaR = params.rotorBeta + time * 0.25f;
        p4 = Sculpture4DFoliation::rotateSO4(p4, thetaL, thetaR);
        if (std::abs(params.rotorTheta4D) > 1e-5f) {
            const float cosT = std::cos(params.rotorTheta4D);
            const float sinT = std::sin(params.rotorTheta4D);
            const float zw_z = p4.z * cosT - p4.w * sinT;
            const float zw_w = p4.z * sinT + p4.w * cosT;
            p4.z = zw_z;
            p4.w = zw_w;
        }

        // Evaluate primary 4D manifold distance
        const auto manifoldTypeA = static_cast<Manifold4DType>(
            (params.enable4D > 0U ? params.enable4D - 1U : 0U) % kManifold4DTypeCount);
        float distA = Sculpture4DFoliation::sample4DManifoldSdf(p4, manifoldTypeA, time);

        // Smooth metric homotopy cross-fading to secondary 4D manifold during morph transitions
        if (params.enable4DB > 0U && params.enable4DB != params.enable4D && params.topologyBlend > 1e-4f) {
            const auto manifoldTypeB = static_cast<Manifold4DType>(
                (params.enable4DB - 1U) % kManifold4DTypeCount);
            float distB = Sculpture4DFoliation::sample4DManifoldSdf(p4, manifoldTypeB, time);
            const float tb = std::clamp(params.topologyBlend, 0.0f, 1.0f);
            return distA * (1.0f - tb) + distB * tb;
        }

        return distA;
    }

    // Hollow chamber box
    auto boxSdf = [](const SculptureVec3& q, const SculptureVec3& b) -> float {
        float dx = std::abs(q.x) - b.x;
        float dy = std::abs(q.y) - b.y;
        float dz = std::abs(q.z) - b.z;
        float mx = std::max({dx, dy, dz});
        float lenOutside = std::sqrt(std::max(dx, 0.0f)*std::max(dx, 0.0f) + std::max(dy, 0.0f)*std::max(dy, 0.0f) + std::max(dz, 0.0f)*std::max(dz, 0.0f));
        return lenOutside + std::min(mx, 0.0f);
    };

    float roomOuter = boxSdf(localP, SculptureVec3{3.5f, 3.5f, 3.5f});
    float roomInner = boxSdf(localP, SculptureVec3{3.2f, 3.2f, 3.2f});
    float chamberWalls = std::max(roomOuter, -roomInner);

    // Density field based sculpture core
    float density = program.sampleDensityField(p, time, morph, {});
    float sculptureCore = (0.50f - density) * 0.35f;

    // Colonnade pillars
    auto positiveFmod = [](float val, float m) -> float {
        float res = std::fmod(val, m);
        return (res < 0.0f) ? res + m : res;
    };
    SculptureVec3 repP{
        positiveFmod(localP.x + 1.5f, 3.0f) - 1.5f,
        localP.y,
        positiveFmod(localP.z + 1.5f, 3.0f) - 1.5f
    };
    auto cylinderSdf = [](const SculptureVec3& q, float r, float h) -> float {
        float dx = std::sqrt(q.x * q.x + q.z * q.z) - r;
        float dy = std::abs(q.y) - h;
        return std::sqrt(std::max(dx, 0.0f)*std::max(dx, 0.0f) + std::max(dy, 0.0f)*std::max(dy, 0.0f)) + std::min(std::max(dx, dy), 0.0f);
    };
    float pillars = cylinderSdf(repP, 0.18f, 3.2f);
    auto smoothUnion = [](float d1, float d2, float k) -> float {
        float h = std::clamp(0.5f + 0.5f * (d2 - d1) / k, 0.0f, 1.0f);
        return std::lerp(d2, d1, h) - k * h * (1.0f - h);
    };
    float architecture = smoothUnion(sculptureCore, pillars, 0.30f);
    float sceneSdf = smoothUnion(architecture, chamberWalls, 0.40f);

    return sceneSdf * worldScale;
}

SculptureVec3 SculptureRaymarcher::computeTetrahedralNormal(
    const SculptureVec3& p,
    const SculptureProgram& program,
    float time,
    float morph,
    float eps
) const {
    const float k1 = 1.0f;
    const float k2 = -1.0f;
    const SculptureVec3 p1{p.x + k1 * eps, p.y + k2 * eps, p.z + k2 * eps};
    const SculptureVec3 p2{p.x + k2 * eps, p.y + k2 * eps, p.z + k1 * eps};
    const SculptureVec3 p3{p.x + k2 * eps, p.y + k1 * eps, p.z + k2 * eps};
    const SculptureVec3 p4{p.x + k1 * eps, p.y + k1 * eps, p.z + k1 * eps};

    float d1 = (config_.chamberMode != SdfChamberMode::DensityField)
                   ? estimateChamberDistance(p1, program, time, morph)
                   : estimateDistance(p1, program, time, morph, {});
    float d2 = (config_.chamberMode != SdfChamberMode::DensityField)
                   ? estimateChamberDistance(p2, program, time, morph)
                   : estimateDistance(p2, program, time, morph, {});
    float d3 = (config_.chamberMode != SdfChamberMode::DensityField)
                   ? estimateChamberDistance(p3, program, time, morph)
                   : estimateDistance(p3, program, time, morph, {});
    float d4 = (config_.chamberMode != SdfChamberMode::DensityField)
                   ? estimateChamberDistance(p4, program, time, morph)
                   : estimateDistance(p4, program, time, morph, {});

    SculptureVec3 n{
        k1 * d1 + k2 * d2 + k2 * d3 + k1 * d4,
        k2 * d1 + k2 * d2 + k1 * d3 + k1 * d4,
        k2 * d1 + k1 * d2 + k2 * d3 + k1 * d4
    };
    return normalize(n);
}

float SculptureRaymarcher::evaluateHenyeyGreenstein(float cosTheta, float g) noexcept {
    return SculptureAtmosphereEngine::evaluateHenyeyGreenstein(cosTheta, g);
}

float SculptureRaymarcher::evaluateRayleigh(float cosTheta) noexcept {
    return SculptureAtmosphereEngine::evaluateRayleigh(cosTheta);
}

float SculptureRaymarcher::evaluateDualPhase(float cosTheta, float g, float mieBlend) noexcept {
    return SculptureAtmosphereEngine::evaluateDualPhase(cosTheta, g, mieBlend);
}

SculptureVec3 SculptureRaymarcher::acesToneMap(const SculptureVec3& hdr) noexcept {
    auto toneCurve = [](float x) noexcept -> float {
        x = std::max(0.0f, x);
        const float a = 2.51f;
        const float b = 0.03f;
        const float c = 2.43f;
        const float d = 0.59f;
        const float e = 0.14f;
        return std::clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
    };

    return SculptureVec3{
        toneCurve(hdr.x),
        toneCurve(hdr.y),
        toneCurve(hdr.z)
    };
}

SculptureRayHit SculptureRaymarcher::marchVolumetricRay(
    const SculptureRay& ray,
    const SculptureProgram& program,
    float time,
    float morph,
    const std::vector<float>& spatialBuffer,
    const SculptureVolumetricMemory* volumetricMemory
) const {
    const auto observation = acquireVolumeObservation(volumetricMemory);
    return marchVolumetricRayObserved(ray, program, time, morph, spatialBuffer, observation);
}

SculptureRayHit SculptureRaymarcher::marchVolumetricRayObserved(
    const SculptureRay& ray,
    const SculptureProgram& program,
    float time,
    float morph,
    const std::vector<float>& spatialBuffer,
    const VolumeObservation& observation
) const {
    SculptureRayHit hit = marchRay(ray, program, time, morph, spatialBuffer);

    if (!config_.enableVolumetricFog) {
        return hit;
    }

    const SculptureVec3 dir = normalize(ray.direction);
    const SculptureVec3 lightDir = normalize(config_.lightDirection);
    const float cosTheta = dot(dir, lightDir);
    const float phase = evaluateDualPhase(cosTheta, config_.fogAnisotropy, config_.mieRayleighBlend);

    const float maxMarchDist = hit.hit ? hit.distance : std::min(config_.maxDistance, config_.boundingRadius * 2.0f);
    if (maxMarchDist <= config_.minDistance) {
        return hit;
    }

    // Adapt step count based on volumetric memory resolution scale if available
    std::uint32_t baseSteps = config_.volumetricSteps;
    if (observation.resolution) {
        switch (*observation.resolution) {
            case StageResolutionScale::Quarter:
                baseSteps = std::max(4U, config_.volumetricSteps / 4U);
                break;
            case StageResolutionScale::Half:
                baseSteps = std::max(4U, config_.volumetricSteps / 2U);
                break;
            case StageResolutionScale::Full:
            default:
                baseSteps = std::max(4U, config_.volumetricSteps);
                break;
        }
    }
    const std::uint32_t numSteps = std::max(4U, baseSteps);
    const float stepSize = (maxMarchDist - config_.minDistance) / static_cast<float>(numSteps);

    float transR = 1.0f;
    float transG = 1.0f;
    float transB = 1.0f;
    float inscatterR = 0.0f;
    float inscatterG = 0.0f;
    float inscatterB = 0.0f;

    for (std::uint32_t i = 0; i < numSteps; ++i) {
        const float t = config_.minDistance + (static_cast<float>(i) + 0.5f) * stepSize;
        const SculptureVec3 pos{
            ray.origin.x + t * dir.x,
            ray.origin.y + t * dir.y,
            ray.origin.z + t * dir.z
        };

        const float radius = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
        if (radius > config_.boundingRadius) continue;

        const float rawDensity = program.sampleDensityField(pos, time, morph, spatialBuffer);
        float density = rawDensity * config_.fogDensityScale * program.params().volumetricDensityScale;

        // Dynamic coupling with camera-independent 3D volumetric recursive memory
        // [EDUCATIONAL ANNOTATION: Coarse Macro-Block Vacancy Leaping]
        // Before evaluating expensive 8-corner trilinear voxel lookups and optical extinction,
        // we query the 8x8x8 coarse occupancy bitmask. If this spatial macro-block contains
        // zero volumetric density in memory, we skip continuous sampling in O(1) CPU cycles.
        SculptureVec3 voxelGain{1.0f, 1.0f, 1.0f};
        float orbitEnergy = 0.0f;
        if (observation.field) {
            if (observation.field->isWorldPositionOccupied(pos.x, pos.y, pos.z)) {
                const auto voxel = observation.field->sampleWorld(pos.x, pos.y, pos.z);
                const float vDensity = voxel.colorDensity[3];
                if (vDensity > 0.0f) {
                    density += vDensity * config_.fogDensityScale;
                    voxelGain.x = std::clamp(voxel.colorDensity[0], 0.2f, 2.0f);
                    voxelGain.y = std::clamp(voxel.colorDensity[1], 0.2f, 2.0f);
                    voxelGain.z = std::clamp(voxel.colorDensity[2], 0.2f, 2.0f);
                    orbitEnergy = voxel.velocityEnergy[3];
                }
            }
        }

        if (density <= 1e-4f && orbitEnergy <= 1e-4f) continue;

        // Shadow attenuation along light ray
        const float shadow = evaluateSoftShadow(
            pos, lightDir, 0.03f, 2.0f, config_.shadowSoftness, program, time, morph, spatialBuffer
        );

        // Multi-spectral light radiance = direct sun radiance * phase * shadow + ambient, modulated by voxel color
        const float lightR = (config_.lightRadiance.x * shadow * phase + config_.ambientLight * 0.4f) * voxelGain.x;
        const float lightG = (config_.lightRadiance.y * shadow * phase + config_.ambientLight * 0.4f) * voxelGain.y;
        const float lightB = (config_.lightRadiance.z * shadow * phase + config_.ambientLight * 0.4f) * voxelGain.z;

        // Optical extinction sigma_t = sigma_a + sigma_s
        const float extR = (config_.absorptionCoeff.x + config_.scatteringCoeff.x) * density;
        const float extG = (config_.absorptionCoeff.y + config_.scatteringCoeff.y) * density;
        const float extB = (config_.absorptionCoeff.z + config_.scatteringCoeff.z) * density;

        const float stepTransR = std::exp(-extR * stepSize);
        const float stepTransG = std::exp(-extG * stepSize);
        const float stepTransB = std::exp(-extB * stepSize);

        // In-scattering accumulation with orbit energy coupling
        inscatterR += transR * (config_.scatteringCoeff.x * density * lightR + orbitEnergy * 0.04f) * stepSize;
        inscatterG += transG * (config_.scatteringCoeff.y * density * lightG + orbitEnergy * 0.04f) * stepSize;
        inscatterB += transB * (config_.scatteringCoeff.z * density * lightB + orbitEnergy * 0.04f) * stepSize;

        transR *= stepTransR;
        transG *= stepTransG;
        transB *= stepTransB;

        if (transR < 0.01f && transG < 0.01f && transB < 0.01f) break;
    }

    hit.transmittance = (transR + transG + transB) / 3.0f;
    hit.volumetricR = inscatterR;
    hit.volumetricG = inscatterG;
    hit.volumetricB = inscatterB;

    // Linear HDR composite
    const SculptureVec3 hdrComposite{
        hit.hit ? (hit.r * transR + inscatterR) : inscatterR,
        hit.hit ? (hit.g * transG + inscatterG) : inscatterG,
        hit.hit ? (hit.b * transB + inscatterB) : inscatterB
    };

    hit.hdrRadiance = hdrComposite;

    if (config_.enableAcesToneMapping) {
        const SculptureVec3 ldr = acesToneMap(hdrComposite);
        hit.r = ldr.x;
        hit.g = ldr.y;
        hit.b = ldr.z;
    } else {
        hit.r = std::clamp(hdrComposite.x, 0.0f, 1.0f);
        hit.g = std::clamp(hdrComposite.y, 0.0f, 1.0f);
        hit.b = std::clamp(hdrComposite.z, 0.0f, 1.0f);
    }

    return hit;
}

SculptureRayHit SculptureRaymarcher::marchRay(
    const SculptureRay& ray,
    const SculptureProgram& program,
    float time,
    float morph,
    const std::vector<float>& spatialBuffer
) const {
    SculptureRayHit hit{};
    float t = config_.minDistance;

    const SculptureVec3 dir = normalize(ray.direction);

    for (std::uint32_t step = 0; step < config_.maxSteps; ++step) {
        const SculptureVec3 pos{
            ray.origin.x + t * dir.x,
            ray.origin.y + t * dir.y,
            ray.origin.z + t * dir.z
        };

        const float d = (config_.chamberMode != SdfChamberMode::DensityField)
                            ? estimateChamberDistance(pos, program, time, morph)
                            : estimateDistance(pos, program, time, morph, spatialBuffer);

        // Count completed primary distance evaluations, including misses and
        // the first hit. Diagnostic effort is independent of shading/AO policy.
        hit.stepCount = step + 1U;

        if (d < config_.hitEpsilon) {
            hit.hit = true;
            hit.distance = t;
            hit.position = pos;
            hit.normal = (config_.chamberMode != SdfChamberMode::DensityField)
                             ? computeTetrahedralNormal(pos, program, time, morph, config_.hitEpsilon * 1.5f)
                             : program.evaluateFieldGradient(pos, time, morph, 0.005f, spatialBuffer);
            hit.density = program.sampleDensityField(pos, time, morph, spatialBuffer);
            hit.ambientOcclusion = std::clamp(1.0f - static_cast<float>(step) / static_cast<float>(config_.maxSteps), 0.0f, 1.0f);

            // Shading & Lighting Model
            const SculptureVec3 lightDir = normalize(config_.lightDirection);
            const float nDotL = std::max(dot(hit.normal, lightDir), 0.0f);
            const float shadow = evaluateSoftShadow(
                pos, lightDir, 0.02f, 2.5f, config_.shadowSoftness, program, time, morph, spatialBuffer
            );

            const float diffuse = nDotL * shadow;
            const float lighting = config_.ambientLight * hit.ambientOcclusion + diffuse * 0.82f;

            // Procedural OKLCH Hue palette mapping
            const float angle = std::atan2(pos.z, pos.x) / (2.0f * 3.14159265f);
            const float hue = angle + program.params().hueRotation;

            hit.r = std::clamp((0.5f + 0.5f * std::cos(6.28318f * (hue + 0.00f))) * lighting, 0.0f, 1.0f);
            hit.g = std::clamp((0.5f + 0.5f * std::cos(6.28318f * (hue + 0.33f))) * lighting, 0.0f, 1.0f);
            hit.b = std::clamp((0.5f + 0.5f * std::cos(6.28318f * (hue + 0.67f))) * lighting, 0.0f, 1.0f);

            return hit;
        }

        t += std::max(d * config_.stepRelaxation, 0.002f);
        if (t > config_.maxDistance) break;
    }

    return hit;
}

void SculptureRaymarcher::marchRayBatch(
    const SculptureRay* rays,
    SculptureRayHit* outHits,
    std::size_t count,
    const SculptureProgram& program,
    float time,
    float morph,
    const std::vector<float>& spatialBuffer,
    std::uint32_t threadCount,
    const SculptureVolumetricMemory* volumetricMemory
) const {
    if (!rays || !outHits || count == 0) return;
    const auto observation = acquireVolumeObservation(volumetricMemory);

    // Each worker writes only its own slot; joining synchronizes caller reads.
    std::array<std::exception_ptr, 16> errors{};
    const std::uint32_t numThreads = std::max(1U,
        std::min(threadCount, static_cast<std::uint32_t>(errors.size())));
    std::vector<std::thread> workers;
    workers.reserve(numThreads);
    RaymarchBatchJoin settlement(workers);

    for (std::uint32_t threadId = 0; threadId < numThreads; ++threadId) {
        const auto [start, end] = raymarchRays(count, numThreads, threadId);

        if (start >= end) continue;

        workers.emplace_back([this, rays, outHits, start, end, &program, time, morph, &spatialBuffer,
                              observation, &errors, threadId]() {
            try {
                for (std::size_t i = start; i < end; ++i) {
                    if (config_.enableVolumetricFog) {
                        outHits[i] = marchVolumetricRayObserved(rays[i], program, time, morph, spatialBuffer, observation);
                    } else {
                        outHits[i] = marchRay(rays[i], program, time, morph, spatialBuffer);
                    }
                }
            } catch (...) {
                errors[threadId] = std::current_exception();
            }
        });
    }

    settlement.join();
    for (const auto& error : errors) {
        if (error) std::rethrow_exception(error);
    }
}

// -----------------------------------------------------------------------------
// Educational Annotation: Penumbra Soft Shadows via SDF Raymarching
// -----------------------------------------------------------------------------
// Because the scene is defined by a Signed Distance Field (SDF), we don't need
// expensive shadow maps or volumetric tracing to get physically plausible soft 
// shadows. We can estimate the shadow penumbra by tracking the minimum ratio
// of distance-to-surface `h` over distance-traveled `t`.
// `k` controls the softness/hardness of the shadow.
// -----------------------------------------------------------------------------
float SculptureRaymarcher::evaluateSoftShadow(
    const SculptureVec3& surfacePos,
    const SculptureVec3& lightDir,
    float minT,
    float maxT,
    float k,
    const SculptureProgram& program,
    float time,
    float morph,
    const std::vector<float>& spatialBuffer
) const {
    float res = 1.0f;
    float t = minT;
    const SculptureVec3 lDir = normalize(lightDir);

    for (std::uint32_t step = 0; step < 32 && t < maxT; ++step) {
        const SculptureVec3 pos{
            surfacePos.x + t * lDir.x,
            surfacePos.y + t * lDir.y,
            surfacePos.z + t * lDir.z
        };

        const float h = estimateDistance(pos, program, time, morph, spatialBuffer);
        if (h < 0.001f) return 0.0f; // Fully occluded

        res = std::min(res, k * h / t);
        t += std::max(h * 0.85f, 0.015f);
    }

    return std::clamp(res, 0.0f, 1.0f);
}

SculptureRay SculptureRaymarcher::generateCameraRay(
    float ndcX,
    float ndcY,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    float aspectRatio
) {
    if (!std::isfinite(ndcX) || !std::isfinite(ndcY)) {
        throw std::invalid_argument("nonfinite mono camera coordinates");
    }
    return prepareMonoCamera(cameraPos, cameraTarget, cameraUp, fovRadians, aspectRatio)
        .ray(ndcX, ndcY);
}

void SculptureRaymarcher::renderImage(
    std::uint32_t width,
    std::uint32_t height,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    const SculptureProgram& program,
    float time,
    float morph,
    std::vector<float>& outRgbBuffer,
    const std::vector<float>& spatialBuffer,
    std::uint32_t threadCount,
    const SculptureVolumetricMemory* volumetricMemory
) const {
    if (width == 0 || height == 0) return;
    const auto elementCount = detail::checkedRaymarchElementCount<3>(width, height);
    const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    const auto camera = prepareMonoCamera(cameraPos, cameraTarget, cameraUp, fovRadians, aspectRatio);
    const auto observation = acquireVolumeObservation(volumetricMemory);
    outRgbBuffer.resize(elementCount);

    const std::uint32_t numThreads = std::max(1U, std::min(threadCount, RaymarchWorkGroup::kMaxWorkers));
    
    static SculptureThreadPool sharedWorkerPool(std::thread::hardware_concurrency());
    RaymarchWorkGroup work;

    for (std::uint32_t threadId = 0; threadId < numThreads; ++threadId) {
        const auto [startY, endY] = raymarchRows(height, numThreads, threadId);

        if (startY >= endY) continue;

        work.submit(sharedWorkerPool, [this, width, height, startY, endY, camera, &program, time, morph, &spatialBuffer, observation, &outRgbBuffer]() {
            for (std::uint32_t y = startY; y < endY; ++y) {
                const float ndcY = 1.0f - 2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
                for (std::uint32_t x = 0; x < width; ++x) {
                    const float ndcX = 2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(width) - 1.0f;
                    const SculptureRay ray = camera.ray(ndcX, ndcY);
                    const SculptureRayHit hit = marchVolumetricRayObserved(ray, program, time, morph, spatialBuffer, observation);

                    const std::size_t pixelIdx = (static_cast<std::size_t>(y) * width + x) * 3;
                    outRgbBuffer[pixelIdx + 0] = hit.r;
                    outRgbBuffer[pixelIdx + 1] = hit.g;
                    outRgbBuffer[pixelIdx + 2] = hit.b;
                }
            }
        });
    }

    work.finish();
}

SculptureDiagnosticImage SculptureRaymarcher::renderDiagnosticImage(
    std::uint32_t width,
    std::uint32_t height,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    const SculptureProgram& program,
    float time,
    float morph,
    const SculptureDiagnosticSettings& settings,
    const std::vector<float>& spatialBuffer,
    std::uint32_t threadCount,
    const SculptureVolumetricMemory* volumetricMemory
) const {
    if (!settings.valid()) throw std::invalid_argument("invalid raymarch diagnostic settings");
    SculptureGBuffer observation;
    renderGBuffer(width, height, cameraPos, cameraTarget, cameraUp, fovRadians,
        program, time, morph, observation, spatialBuffer, threadCount, volumetricMemory);
    return makeSculptureDiagnosticImage(observation, settings);
}

SculptureRay SculptureRaymarcher::generateStereoCameraRay(
    float ndcX,
    float ndcY,
    StereoEye eye,
    const SculptureStereoRig& rig,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    float aspectRatio
) {
    if (eye == StereoEye::Mono) {
        return generateCameraRay(ndcX, ndcY, cameraPos, cameraTarget, cameraUp, fovRadians, aspectRatio);
    }

    const SculptureVec3 forward = normalize(SculptureVec3{
        cameraTarget.x - cameraPos.x,
        cameraTarget.y - cameraPos.y,
        cameraTarget.z - cameraPos.z
    });
    const SculptureVec3 right = normalize(cross(forward, cameraUp));
    const SculptureVec3 trueUp = cross(right, forward);

    // Eye offset along the camera's right vector
    const float eyeSign = (eye == StereoEye::Left) ? -1.0f : 1.0f;
    const float eyeOffset = 0.5f * eyeSign * rig.interpupillaryDistance;
    const SculptureVec3 eyePos{
        cameraPos.x + right.x * eyeOffset,
        cameraPos.y + right.y * eyeOffset,
        cameraPos.z + right.z * eyeOffset
    };

    const float halfHeight = std::tan(fovRadians * 0.5f);
    const float halfWidth = halfHeight * aspectRatio;

    if (rig.toeInConvergence && rig.focalConvergenceDistance > 0.01f) {
        // Off-axis focal convergence: rays from each eye converge at the focal plane
        const float dist = rig.focalConvergenceDistance;
        const float focalX = cameraPos.x + forward.x * dist + right.x * (ndcX * halfWidth * dist) + trueUp.x * (ndcY * halfHeight * dist);
        const float focalY = cameraPos.y + forward.y * dist + right.y * (ndcX * halfWidth * dist) + trueUp.y * (ndcY * halfHeight * dist);
        const float focalZ = cameraPos.z + forward.z * dist + right.z * (ndcX * halfWidth * dist) + trueUp.z * (ndcY * halfHeight * dist);

        return SculptureRay{
            .origin = eyePos,
            .direction = normalize(SculptureVec3{
                focalX - eyePos.x,
                focalY - eyePos.y,
                focalZ - eyePos.z
            })
        };
    } else {
        // Parallel asymmetric frustum
        const SculptureVec3 dir = normalize(SculptureVec3{
            forward.x + right.x * (ndcX * halfWidth) + trueUp.x * (ndcY * halfHeight),
            forward.y + right.y * (ndcX * halfWidth) + trueUp.y * (ndcY * halfHeight),
            forward.z + right.z * (ndcX * halfWidth) + trueUp.z * (ndcY * halfHeight)
        });
        return SculptureRay{
            .origin = eyePos,
            .direction = dir
        };
    }
}

SculptureRay SculptureRaymarcher::generatePanoramicRay(
    float u,
    float v,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraForward,
    const SculptureVec3& cameraUp
) {
    const SculptureVec3 fwd = normalize(cameraForward);
    const SculptureVec3 right = normalize(cross(fwd, cameraUp));
    const SculptureVec3 trueUp = cross(right, fwd);

    // u in [0, 1] -> phi in [-pi, pi] (longitude)
    // v in [0, 1] -> theta in [pi/2, -pi/2] (latitude)
    const float phi = (u - 0.5f) * (2.0f * 3.14159265359f);
    const float theta = (0.5f - v) * 3.14159265359f;

    const float cosTheta = std::cos(theta);
    const float sinTheta = std::sin(theta);
    const float cosPhi = std::cos(phi);
    const float sinPhi = std::sin(phi);

    const SculptureVec3 dir = normalize(SculptureVec3{
        fwd.x * (cosPhi * cosTheta) + right.x * (sinPhi * cosTheta) + trueUp.x * sinTheta,
        fwd.y * (cosPhi * cosTheta) + right.y * (sinPhi * cosTheta) + trueUp.y * sinTheta,
        fwd.z * (cosPhi * cosTheta) + right.z * (sinPhi * cosTheta) + trueUp.z * sinTheta
    });

    return SculptureRay{
        .origin = cameraPos,
        .direction = dir
    };
}

void SculptureRaymarcher::renderStereoImagePair(
    std::uint32_t eyeWidth,
    std::uint32_t eyeHeight,
    const SculptureStereoRig& rig,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    const SculptureProgram& program,
    float time,
    float morph,
    std::vector<float>& outLeftRgbBuffer,
    std::vector<float>& outRightRgbBuffer,
    const std::vector<float>& spatialBuffer,
    std::uint32_t threadCount,
    const SculptureVolumetricMemory* volumetricMemory
) const {
    if (eyeWidth == 0 || eyeHeight == 0) return;
    const auto elementCount = detail::checkedRaymarchElementCount<3>(eyeWidth, eyeHeight);
    const auto observation = acquireVolumeObservation(volumetricMemory);
    outLeftRgbBuffer.reserve(elementCount);
    outRightRgbBuffer.reserve(elementCount);
    outLeftRgbBuffer.resize(elementCount);
    outRightRgbBuffer.resize(elementCount);

    const float aspectRatio = static_cast<float>(eyeWidth) / static_cast<float>(eyeHeight);
    const std::uint32_t numThreads = std::max(1U, std::min(threadCount, RaymarchWorkGroup::kMaxWorkers));
    
    static SculptureThreadPool sharedWorkerPool(std::thread::hardware_concurrency());
    RaymarchWorkGroup work;

    for (std::uint32_t threadId = 0; threadId < numThreads; ++threadId) {
        const auto [startY, endY] = raymarchRows(eyeHeight, numThreads, threadId);
        if (startY >= endY) continue;

        work.submit(sharedWorkerPool, [this, eyeWidth, eyeHeight, startY, endY, aspectRatio, rig, cameraPos, cameraTarget, cameraUp, fovRadians, &program, time, morph, &spatialBuffer, observation, &outLeftRgbBuffer, &outRightRgbBuffer]() {
            for (std::uint32_t y = startY; y < endY; ++y) {
                const float ndcY = 1.0f - 2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(eyeHeight);
                for (std::uint32_t x = 0; x < eyeWidth; ++x) {
                    const float ndcX = 2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(eyeWidth) - 1.0f;
                    const std::size_t pixelIdx = (static_cast<std::size_t>(y) * eyeWidth + x) * 3;

                    // Left eye ray & hit
                    const SculptureRay rayL = generateStereoCameraRay(ndcX, ndcY, StereoEye::Left, rig, cameraPos, cameraTarget, cameraUp, fovRadians, aspectRatio);
                    const SculptureRayHit hitL = marchVolumetricRayObserved(rayL, program, time, morph, spatialBuffer, observation);
                    outLeftRgbBuffer[pixelIdx + 0] = hitL.r;
                    outLeftRgbBuffer[pixelIdx + 1] = hitL.g;
                    outLeftRgbBuffer[pixelIdx + 2] = hitL.b;

                    // Right eye ray & hit
                    const SculptureRay rayR = generateStereoCameraRay(ndcX, ndcY, StereoEye::Right, rig, cameraPos, cameraTarget, cameraUp, fovRadians, aspectRatio);
                    const SculptureRayHit hitR = marchVolumetricRayObserved(rayR, program, time, morph, spatialBuffer, observation);
                    outRightRgbBuffer[pixelIdx + 0] = hitR.r;
                    outRightRgbBuffer[pixelIdx + 1] = hitR.g;
                    outRightRgbBuffer[pixelIdx + 2] = hitR.b;
                }
            }
        });
    }

    work.finish();
}

void SculptureRaymarcher::renderGBuffer(
    std::uint32_t width,
    std::uint32_t height,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    const SculptureProgram& program,
    float time,
    float morph,
    SculptureGBuffer& outGBuffer,
    const std::vector<float>& spatialBuffer,
    std::uint32_t threadCount,
    const SculptureVolumetricMemory* volumetricMemory,
    const SculptureVec3* prevCameraPos,
    const SculptureVec3* prevCameraTarget
) const {
    if (width == 0 || height == 0) return;
    (void)detail::checkedRaymarchElementCount<4>(width, height);
    const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    // One current camera supplies rays, view depth and projection scales.
    const auto camera = prepareMonoCamera(cameraPos, cameraTarget, cameraUp, fovRadians, aspectRatio);

    // Admit and copy the optional previous view before observation effects.
    // Motion projection must use the same origin that produced its basis.
    std::optional<PreparedMonoCamera> previousCamera;
    if (prevCameraPos != nullptr && prevCameraTarget != nullptr) {
        previousCamera = prepareMonoCamera(
            *prevCameraPos, *prevCameraTarget, cameraUp, fovRadians, aspectRatio);
    }

    const auto observation = acquireVolumeObservation(volumetricMemory);
    outGBuffer.resize(width, height);
    const std::uint32_t numThreads = std::max(1U, std::min(threadCount, RaymarchWorkGroup::kMaxWorkers));

    static SculptureThreadPool sharedWorkerPool(std::thread::hardware_concurrency());
    RaymarchWorkGroup work;

    for (std::uint32_t threadId = 0; threadId < numThreads; ++threadId) {
        const auto [startY, endY] = raymarchRows(height, numThreads, threadId);

        if (startY >= endY) continue;

        work.submit(sharedWorkerPool, [this, width, height, startY, endY, camera, &program, time, morph, &spatialBuffer, observation, &outGBuffer, previousCamera]() {
            for (std::uint32_t y = startY; y < endY; ++y) {
                const float ndcY = 1.0f - 2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
                for (std::uint32_t x = 0; x < width; ++x) {
                    const float ndcX = 2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(width) - 1.0f;
                    const SculptureRay ray = camera.ray(ndcX, ndcY);
                    const SculptureRayHit hit = marchVolumetricRayObserved(ray, program, time, morph, spatialBuffer, observation);

                    const std::size_t pixelIndex = static_cast<std::size_t>(y) * width + x;
                    const std::size_t c4Idx = pixelIndex * 4U;
                    const std::size_t c2Idx = pixelIndex * 2U;

                    // 1. Radiance & Transmittance (RGBA32F)
                    outGBuffer.radiance[c4Idx + 0] = hit.r;
                    outGBuffer.radiance[c4Idx + 1] = hit.g;
                    outGBuffer.radiance[c4Idx + 2] = hit.b;
                    outGBuffer.radiance[c4Idx + 3] = hit.transmittance;

                    // 2. Linear Camera View Depth (R32F)
                    if (hit.hit) {
                        const SculptureVec3 hitOffset{
                            hit.position.x - camera.origin.x,
                            hit.position.y - camera.origin.y,
                            hit.position.z - camera.origin.z
                        };
                        const float zDepth = dot(hitOffset, camera.forward);
                        outGBuffer.depth[pixelIndex] = (zDepth > 1e-4f) ? zDepth : hit.distance;
                    } else {
                        outGBuffer.depth[pixelIndex] = config_.maxDistance;
                    }

                    // 3. World Surface Normal & Trap Density (RGBA32F)
                    if (hit.hit) {
                        const SculptureVec3 nrm = normalize(hit.normal);
                        outGBuffer.normal[c4Idx + 0] = nrm.x;
                        outGBuffer.normal[c4Idx + 1] = nrm.y;
                        outGBuffer.normal[c4Idx + 2] = nrm.z;
                        outGBuffer.normal[c4Idx + 3] = hit.trapDensity;
                    } else {
                        outGBuffer.normal[c4Idx + 0] = 0.0f;
                        outGBuffer.normal[c4Idx + 1] = 0.0f;
                        outGBuffer.normal[c4Idx + 2] = 1.0f;
                        outGBuffer.normal[c4Idx + 3] = 0.0f;
                    }

                    // 4. Motion Velocity Vector (RG32F)
                    float vx = 0.0f;
                    float vy = 0.0f;
                    if (hit.hit && previousCamera) {
                        const SculptureVec3 prevRel{
                            hit.position.x - previousCamera->origin.x,
                            hit.position.y - previousCamera->origin.y,
                            hit.position.z - previousCamera->origin.z
                        };
                        const float prevZ = dot(prevRel, previousCamera->forward);
                        if (prevZ > 1e-4f) {
                            const float prevX = dot(prevRel, previousCamera->right);
                            const float prevY = dot(prevRel, previousCamera->trueUp);
                            const float prevNdcX = prevX / (prevZ * previousCamera->halfWidth);
                            const float prevNdcY = prevY / (prevZ * previousCamera->halfHeight);
                            vx = ndcX - prevNdcX;
                            vy = ndcY - prevNdcY;
                        }
                    }
                    outGBuffer.velocity[c2Idx + 0] = vx;
                    outGBuffer.velocity[c2Idx + 1] = vy;

                    // 5. Surface Attributes (RGBA32F: AO, Orbit Energy, Normalized Step Count, Hit Flag)
                    const float normSteps = static_cast<float>(hit.stepCount) / static_cast<float>(std::max(1U, config_.maxSteps));
                    outGBuffer.attributes[c4Idx + 0] = hit.ambientOcclusion;
                    outGBuffer.attributes[c4Idx + 1] = hit.orbitEnergy;
                    outGBuffer.attributes[c4Idx + 2] = normSteps;
                    outGBuffer.attributes[c4Idx + 3] = hit.hit ? 1.0f : 0.0f;
                }
            }
        });
    }

    work.finish();
}

void SculptureRaymarcher::renderStereoGBufferPair(
    std::uint32_t eyeWidth,
    std::uint32_t eyeHeight,
    const SculptureStereoRig& rig,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    const SculptureProgram& program,
    float time,
    float morph,
    SculptureStereoGBufferPair& outStereoGBuffer,
    const std::vector<float>& spatialBuffer,
    std::uint32_t threadCount,
    const SculptureVolumetricMemory* volumetricMemory,
    const SculptureVec3* prevCameraPos,
    const SculptureVec3* prevCameraTarget
) const {
    if (eyeWidth == 0 || eyeHeight == 0) return;
    (void)detail::checkedRaymarchElementCount<4>(eyeWidth, eyeHeight);
    const auto observation = acquireVolumeObservation(volumetricMemory);
    outStereoGBuffer.resize(eyeWidth, eyeHeight);

    const float aspectRatio = static_cast<float>(eyeWidth) / static_cast<float>(eyeHeight);
    const std::uint32_t numThreads = std::max(1U, std::min(threadCount, RaymarchWorkGroup::kMaxWorkers));

    const SculptureVec3 forward = normalize(SculptureVec3{
        cameraTarget.x - cameraPos.x,
        cameraTarget.y - cameraPos.y,
        cameraTarget.z - cameraPos.z
    });
    const SculptureVec3 right = normalize(cross(forward, cameraUp));
    const SculptureVec3 trueUp = cross(right, forward);

    const bool hasPrevCamera = (prevCameraPos != nullptr && prevCameraTarget != nullptr);
    SculptureVec3 prevForward{0.0f, 0.0f, -1.0f};
    SculptureVec3 prevRight{1.0f, 0.0f, 0.0f};
    SculptureVec3 prevTrueUp{0.0f, 1.0f, 0.0f};
    const float halfHeight = std::tan(fovRadians * 0.5f);
    const float halfWidth = halfHeight * aspectRatio;

    if (hasPrevCamera) {
        prevForward = normalize(SculptureVec3{
            prevCameraTarget->x - prevCameraPos->x,
            prevCameraTarget->y - prevCameraPos->y,
            prevCameraTarget->z - prevCameraPos->z
        });
        prevRight = normalize(cross(prevForward, cameraUp));
        prevTrueUp = cross(prevRight, prevForward);
    }

    const float halfIpd = 0.5f * rig.interpupillaryDistance;
    const SculptureVec3 leftEyePos{
        cameraPos.x - right.x * halfIpd,
        cameraPos.y - right.y * halfIpd,
        cameraPos.z - right.z * halfIpd
    };
    const SculptureVec3 rightEyePos{
        cameraPos.x + right.x * halfIpd,
        cameraPos.y + right.y * halfIpd,
        cameraPos.z + right.z * halfIpd
    };

    SculptureVec3 prevLeftEyePos = leftEyePos;
    SculptureVec3 prevRightEyePos = rightEyePos;
    if (hasPrevCamera && prevCameraPos != nullptr) {
        prevLeftEyePos = SculptureVec3{
            prevCameraPos->x - prevRight.x * halfIpd,
            prevCameraPos->y - prevRight.y * halfIpd,
            prevCameraPos->z - prevRight.z * halfIpd
        };
        prevRightEyePos = SculptureVec3{
            prevCameraPos->x + prevRight.x * halfIpd,
            prevCameraPos->y + prevRight.y * halfIpd,
            prevCameraPos->z + prevRight.z * halfIpd
        };
    }

    static SculptureThreadPool sharedWorkerPool(std::thread::hardware_concurrency());
    RaymarchWorkGroup work;

    for (std::uint32_t threadId = 0; threadId < numThreads; ++threadId) {
        const auto [startY, endY] = raymarchRows(eyeHeight, numThreads, threadId);
        if (startY >= endY) continue;

        work.submit(sharedWorkerPool, [this, eyeWidth, eyeHeight, startY, endY, aspectRatio, rig, cameraPos, cameraTarget, cameraUp, fovRadians, &program, time, morph, &spatialBuffer, observation, &outStereoGBuffer, forward, leftEyePos, rightEyePos, hasPrevCamera, prevForward, prevRight, prevTrueUp, prevLeftEyePos, prevRightEyePos, halfWidth, halfHeight, halfIpd]() {
            for (std::uint32_t y = startY; y < endY; ++y) {
                const float ndcY = 1.0f - 2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(eyeHeight);
                for (std::uint32_t x = 0; x < eyeWidth; ++x) {
                    const float ndcX = 2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(eyeWidth) - 1.0f;
                    const std::size_t pixelIndex = static_cast<std::size_t>(y) * eyeWidth + x;
                    const std::size_t c4Idx = pixelIndex * 4U;
                    const std::size_t c2Idx = pixelIndex * 2U;

                    // Left Eye Ray & Hit
                    const SculptureRay rayL = generateStereoCameraRay(ndcX, ndcY, StereoEye::Left, rig, cameraPos, cameraTarget, cameraUp, fovRadians, aspectRatio);
                    const SculptureRayHit hitL = marchVolumetricRayObserved(rayL, program, time, morph, spatialBuffer, observation);

                    // Left Eye Radiance
                    outStereoGBuffer.left.radiance[c4Idx + 0] = hitL.r;
                    outStereoGBuffer.left.radiance[c4Idx + 1] = hitL.g;
                    outStereoGBuffer.left.radiance[c4Idx + 2] = hitL.b;
                    outStereoGBuffer.left.radiance[c4Idx + 3] = hitL.transmittance;

                    // Left Eye Depth
                    if (hitL.hit) {
                        const SculptureVec3 offsetL{
                            hitL.position.x - leftEyePos.x,
                            hitL.position.y - leftEyePos.y,
                            hitL.position.z - leftEyePos.z
                        };
                        const float zDepthL = dot(offsetL, forward);
                        outStereoGBuffer.left.depth[pixelIndex] = (zDepthL > 1e-4f) ? zDepthL : hitL.distance;
                    } else {
                        outStereoGBuffer.left.depth[pixelIndex] = config_.maxDistance;
                    }

                    // Left Eye Normal
                    if (hitL.hit) {
                        const SculptureVec3 nrmL = normalize(hitL.normal);
                        outStereoGBuffer.left.normal[c4Idx + 0] = nrmL.x;
                        outStereoGBuffer.left.normal[c4Idx + 1] = nrmL.y;
                        outStereoGBuffer.left.normal[c4Idx + 2] = nrmL.z;
                        outStereoGBuffer.left.normal[c4Idx + 3] = hitL.trapDensity;
                    } else {
                        outStereoGBuffer.left.normal[c4Idx + 0] = 0.0f;
                        outStereoGBuffer.left.normal[c4Idx + 1] = 0.0f;
                        outStereoGBuffer.left.normal[c4Idx + 2] = 1.0f;
                        outStereoGBuffer.left.normal[c4Idx + 3] = 0.0f;
                    }

                    // Left Eye Velocity
                    float vxL = 0.0f, vyL = 0.0f;
                    if (hitL.hit && hasPrevCamera) {
                        const SculptureVec3 prevRelL{
                            hitL.position.x - prevLeftEyePos.x,
                            hitL.position.y - prevLeftEyePos.y,
                            hitL.position.z - prevLeftEyePos.z
                        };
                        const float prevZL = dot(prevRelL, prevForward);
                        if (prevZL > 1e-4f) {
                            const float prevXL = dot(prevRelL, prevRight);
                            const float prevYL = dot(prevRelL, prevTrueUp);
                            const float toeOffsetL = (rig.toeInConvergence && rig.focalConvergenceDistance > 0.01f)
                                ? prevZL * (halfIpd / rig.focalConvergenceDistance) : 0.0f;
                            vxL = ndcX - (prevXL - toeOffsetL) / (prevZL * halfWidth);
                            vyL = ndcY - prevYL / (prevZL * halfHeight);
                        }
                    }
                    outStereoGBuffer.left.velocity[c2Idx + 0] = vxL;
                    outStereoGBuffer.left.velocity[c2Idx + 1] = vyL;

                    // Left Eye Attributes
                    const float normStepsL = static_cast<float>(hitL.stepCount) / static_cast<float>(std::max(1U, config_.maxSteps));
                    outStereoGBuffer.left.attributes[c4Idx + 0] = hitL.ambientOcclusion;
                    outStereoGBuffer.left.attributes[c4Idx + 1] = hitL.orbitEnergy;
                    outStereoGBuffer.left.attributes[c4Idx + 2] = normStepsL;
                    outStereoGBuffer.left.attributes[c4Idx + 3] = hitL.hit ? 1.0f : 0.0f;

                    // Right Eye Ray & Hit
                    const SculptureRay rayR = generateStereoCameraRay(ndcX, ndcY, StereoEye::Right, rig, cameraPos, cameraTarget, cameraUp, fovRadians, aspectRatio);
                    const SculptureRayHit hitR = marchVolumetricRayObserved(rayR, program, time, morph, spatialBuffer, observation);

                    // Right Eye Radiance
                    outStereoGBuffer.right.radiance[c4Idx + 0] = hitR.r;
                    outStereoGBuffer.right.radiance[c4Idx + 1] = hitR.g;
                    outStereoGBuffer.right.radiance[c4Idx + 2] = hitR.b;
                    outStereoGBuffer.right.radiance[c4Idx + 3] = hitR.transmittance;

                    // Right Eye Depth
                    if (hitR.hit) {
                        const SculptureVec3 offsetR{
                            hitR.position.x - rightEyePos.x,
                            hitR.position.y - rightEyePos.y,
                            hitR.position.z - rightEyePos.z
                        };
                        const float zDepthR = dot(offsetR, forward);
                        outStereoGBuffer.right.depth[pixelIndex] = (zDepthR > 1e-4f) ? zDepthR : hitR.distance;
                    } else {
                        outStereoGBuffer.right.depth[pixelIndex] = config_.maxDistance;
                    }

                    // Right Eye Normal
                    if (hitR.hit) {
                        const SculptureVec3 nrmR = normalize(hitR.normal);
                        outStereoGBuffer.right.normal[c4Idx + 0] = nrmR.x;
                        outStereoGBuffer.right.normal[c4Idx + 1] = nrmR.y;
                        outStereoGBuffer.right.normal[c4Idx + 2] = nrmR.z;
                        outStereoGBuffer.right.normal[c4Idx + 3] = hitR.trapDensity;
                    } else {
                        outStereoGBuffer.right.normal[c4Idx + 0] = 0.0f;
                        outStereoGBuffer.right.normal[c4Idx + 1] = 0.0f;
                        outStereoGBuffer.right.normal[c4Idx + 2] = 1.0f;
                        outStereoGBuffer.right.normal[c4Idx + 3] = 0.0f;
                    }

                    // Right Eye Velocity
                    float vxR = 0.0f, vyR = 0.0f;
                    if (hitR.hit && hasPrevCamera) {
                        const SculptureVec3 prevRelR{
                            hitR.position.x - prevRightEyePos.x,
                            hitR.position.y - prevRightEyePos.y,
                            hitR.position.z - prevRightEyePos.z
                        };
                        const float prevZR = dot(prevRelR, prevForward);
                        if (prevZR > 1e-4f) {
                            const float prevXR = dot(prevRelR, prevRight);
                            const float prevYR = dot(prevRelR, prevTrueUp);
                            const float toeOffsetR = (rig.toeInConvergence && rig.focalConvergenceDistance > 0.01f)
                                ? prevZR * (halfIpd / rig.focalConvergenceDistance) : 0.0f;
                            vxR = ndcX - (prevXR + toeOffsetR) / (prevZR * halfWidth);
                            vyR = ndcY - prevYR / (prevZR * halfHeight);
                        }
                    }
                    outStereoGBuffer.right.velocity[c2Idx + 0] = vxR;
                    outStereoGBuffer.right.velocity[c2Idx + 1] = vyR;

                    // Right Eye Attributes
                    const float normStepsR = static_cast<float>(hitR.stepCount) / static_cast<float>(std::max(1U, config_.maxSteps));
                    outStereoGBuffer.right.attributes[c4Idx + 0] = hitR.ambientOcclusion;
                    outStereoGBuffer.right.attributes[c4Idx + 1] = hitR.orbitEnergy;
                    outStereoGBuffer.right.attributes[c4Idx + 2] = normStepsR;
                    outStereoGBuffer.right.attributes[c4Idx + 3] = hitR.hit ? 1.0f : 0.0f;
                }
            }
        });
    }

    work.finish();
}

} // namespace epi_sculpt
