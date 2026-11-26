#pragma once

#include "SculptureProgram.h"
#include "SculptureDiagnosticView.h"
#include "SculptureGBuffer.h"
#include "SculptureStereoRig.h"
#include <algorithm>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <stdexcept>

namespace epi_sculpt {

class SculptureVolumetricMemory;

// -----------------------------------------------------------------------------
// Educational Annotation: 3D Epimorphism Raymarching & Implicit Manifolds
// In addition to discrete 16,384 bead particle point-clouds, the epimorphism
// mathematical engine can be observed as a continuous implicit 3D manifold.
// This module implements analytic sphere-tracing, calculating ray-surface
// intersections, analytical surface normal gradients, ambient occlusion,
// and soft shadow penumbras directly from the dynamic state field.
// -----------------------------------------------------------------------------

struct SculptureRay {
    SculptureVec3 origin{};
    SculptureVec3 direction{}; // Must be normalized
};

struct SculptureRayHit {
    bool hit = false;
    float distance = 0.0f;
    SculptureVec3 position{};
    SculptureVec3 normal{};
    float density = 0.0f;
    float orbitEnergy = 0.0f;
    float trapDensity = 0.0f;
    std::uint32_t stepCount = 0; // Primary distance evaluations, including misses/first hit.
    float ambientOcclusion = 1.0f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    
    // Participating media & volumetric in-scattering
    float transmittance = 1.0f;
    float volumetricR = 0.0f;
    float volumetricG = 0.0f;
    float volumetricB = 0.0f;

    // HDR linear radiance before tone mapping
    SculptureVec3 hdrRadiance{};
};

enum class StereoEye : std::uint8_t {
    Mono = 0,
    Left = 1,
    Right = 2
};

enum class SdfChamberMode : std::uint8_t {
    DensityField = 0,
    EpimorphicChamber = 1,
    Foliation4DChamber = 2
};

struct SculptureRaymarchConfig {
    std::uint32_t maxSteps = 128;
    float minDistance = 0.02f;
    float maxDistance = 20.0f;
    float hitEpsilon = 0.002f;
    float stepRelaxation = 0.85f;
    float boundingRadius = 4.5f;
    SculptureVec3 lightDirection{0.577f, 0.577f, 0.577f}; // Normalized key light
    SculptureVec3 lightRadiance{2.2f, 1.9f, 1.6f};        // Direct sun/light radiance
    float ambientLight = 0.18f;
    float shadowSoftness = 16.0f;
    
    // VR / Stereoscopic Parameters
    bool enableStereoVr = false;
    float eyeSeparation = 0.065f; // IPD (Inter-Pupillary Distance) in meters
    SdfChamberMode chamberMode = SdfChamberMode::EpimorphicChamber;

    // Volumetric Atmospheric Participating Media
    bool enableVolumetricFog = true;
    float fogDensityScale = 0.65f;
    float fogAnisotropy = 0.45f; // Henyey-Greenstein asymmetry factor g in (-1, 1)
    float mieRayleighBlend = 0.70f; // 0.0 = pure Rayleigh, 1.0 = pure Mie
    SculptureVec3 absorptionCoeff{0.10f, 0.15f, 0.25f}; // Wavelength-dependent absorption
    SculptureVec3 scatteringCoeff{0.85f, 0.75f, 0.65f}; // Wavelength-dependent scattering
    std::uint32_t volumetricSteps = 32;
    bool enableAcesToneMapping = true;

    // Admit the complete value, including disabled features. Finite artistic
    // ranges and step budgets retain their existing interpretation.
    [[nodiscard]] bool valid() const noexcept {
        switch (chamberMode) {
        case SdfChamberMode::DensityField:
        case SdfChamberMode::EpimorphicChamber:
        case SdfChamberMode::Foliation4DChamber:
            break;
        default:
            return false;
        }
        const auto finiteVector = [](const SculptureVec3& v) noexcept {
            return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
        };
        return std::isfinite(minDistance) && minDistance >= 0.0f
            && std::isfinite(maxDistance) && maxDistance > minDistance
            && std::isfinite(hitEpsilon) && std::isfinite(stepRelaxation)
            && std::isfinite(boundingRadius)
            && finiteVector(lightDirection) && finiteVector(lightRadiance)
            && std::isfinite(ambientLight) && std::isfinite(shadowSoftness)
            && std::isfinite(eyeSeparation) && std::isfinite(fogDensityScale)
            && std::isfinite(fogAnisotropy) && std::isfinite(mieRayleighBlend)
            && finiteVector(absorptionCoeff) && finiteVector(scatteringCoeff);
    }
};

class SculptureRaymarcher {
public:
    // Throws invalid_argument if the supplied configuration fails admission.
    explicit SculptureRaymarcher(SculptureRaymarchConfig config = {});

    [[nodiscard]] const SculptureRaymarchConfig& config() const noexcept { return config_; }
    // Whole-value replacement: rejection preserves the last valid config.
    // As with rendering inputs, callers must exclude concurrent mutation.
    [[nodiscard]] bool trySetConfig(const SculptureRaymarchConfig& config) noexcept {
        if (!config.valid()) return false;
        config_ = config;
        return true;
    }
    // Compatibility setter; use trySetConfig when rejection must be observed.
    void setConfig(const SculptureRaymarchConfig& config) noexcept {
        (void)trySetConfig(config);
    }

    // Evaluates a single ray against the mathematical epimorphism manifold
    [[nodiscard]] SculptureRayHit marchRay(
        const SculptureRay& ray,
        const SculptureProgram& program,
        float time,
        float morph,
        const std::vector<float>& spatialBuffer = {}
    ) const;

    // Volume-enabled observations acquire current host history on the caller
    // before evaluation/output mutation. Unavailable history throws; it is not
    // vacuum. Keep the volume owner alive and unchanged until the call returns
    // or unwinds; workers share a borrowed view, not a retained snapshot/lock.
    // Evaluates volumetric participating media in-scattering along a ray segment
    [[nodiscard]] SculptureRayHit marchVolumetricRay(
        const SculptureRay& ray,
        const SculptureProgram& program,
        float time,
        float morph,
        const std::vector<float>& spatialBuffer = {},
        const SculptureVolumetricMemory* volumetricMemory = nullptr
    ) const;

    // Evaluates the Henyey-Greenstein phase function for aerosol / Mie angular scattering
    [[nodiscard]] static float evaluateHenyeyGreenstein(float cosTheta, float g) noexcept;

    // Evaluates the Rayleigh phase function for micro-particle molecular scattering
    [[nodiscard]] static float evaluateRayleigh(float cosTheta) noexcept;

    // Evaluates composite dual-phase (Rayleigh + Mie/HG) angular scattering
    [[nodiscard]] static float evaluateDualPhase(float cosTheta, float g, float mieBlend) noexcept;

    // Cinematic ACES Narkowicz / Hill High-Dynamic-Range Tone Mapper
    [[nodiscard]] static SculptureVec3 acesToneMap(const SculptureVec3& hdr) noexcept;

    // Evaluates a batch across 1..16 CPU workers. Every started worker joins
    // before return/unwind. Launch errors take precedence; otherwise the first
    // worker error in range order is rethrown after all joins. Output may be
    // partial on failure and must be discarded. Input arrays contain count items.
    void marchRayBatch(
        const SculptureRay* rays,
        SculptureRayHit* outHits,
        std::size_t count,
        const SculptureProgram& program,
        float time,
        float morph,
        const std::vector<float>& spatialBuffer = {},
        std::uint32_t threadCount = 4,
        const SculptureVolumetricMemory* volumetricMemory = nullptr
    ) const;

    // Pooled image/G-buffer calls settle every submitted task before return or
    // unwind. Submission errors take precedence; otherwise the first task error
    // in submission order is rethrown after settlement. Output may be partial on
    // failure and must not be treated as a completed observation.
    // Mono RGB/G-buffer renders prepare one camera basis/projection per call;
    // workers hold value copies, with no retained camera cache between renders.
    // Invalid mono cameras throw invalid_argument before volume acquisition,
    // output mutation or dispatch. Empty extents remain no-ops. For motion, a
    // complete previous position/target pair is admitted and copied likewise;
    // a lone previous-camera pointer continues to mean no previous view.
    // High-resolution multi-threaded viewport image synthesis into RGB float buffer
    void renderImage(
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
        const std::vector<float>& spatialBuffer = {},
        std::uint32_t threadCount = 4,
        const SculptureVolumetricMemory* volumetricMemory = nullptr
    ) const;

    // Explicit learning/inspection request. Render one G-buffer, then derive a
    // display-only diagnostic; no extra simulation or automatic auxiliary work.
    [[nodiscard]] SculptureDiagnosticImage renderDiagnosticImage(
        std::uint32_t width,
        std::uint32_t height,
        const SculptureVec3& cameraPos,
        const SculptureVec3& cameraTarget,
        const SculptureVec3& cameraUp,
        float fovRadians,
        const SculptureProgram& program,
        float time,
        float morph,
        const SculptureDiagnosticSettings& settings = {},
        const std::vector<float>& spatialBuffer = {},
        std::uint32_t threadCount = 4,
        const SculptureVolumetricMemory* volumetricMemory = nullptr
    ) const;

    // High-resolution multi-threaded viewport G-Buffer synthesis into multi-target buffers
    void renderGBuffer(
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
        const std::vector<float>& spatialBuffer = {},
        std::uint32_t threadCount = 4,
        const SculptureVolumetricMemory* volumetricMemory = nullptr,
        const SculptureVec3* prevCameraPos = nullptr,
        const SculptureVec3* prevCameraTarget = nullptr
    ) const;

    // Estimates soft shadow attenuation along light vector
    [[nodiscard]] float evaluateSoftShadow(
        const SculptureVec3& surfacePos,
        const SculptureVec3& lightDir,
        float minT,
        float maxT,
        float k,
        const SculptureProgram& program,
        float time,
        float morph,
        const std::vector<float>& spatialBuffer = {}
    ) const;

    // Camera ray synthesis from normalized device coordinates [-1.0, 1.0]^2
    // Throws invalid_argument for nonfinite coordinates/geometry, degenerate
    // or unrepresentable float basis, FOV outside (0, pi), nonpositive/nonfinite
    // aspect, or nonpositive/nonfinite projection scales. Basis normalization
    // requires lengths > 1e-6. Finite coordinates outside the viewport are allowed.
    // Direction synthesis retains ordinary float arithmetic when its length is
    // usable; otherwise it recomputes with wide intermediates. An unusable wide
    // result throws invalid_argument instead of substituting a fallback direction.
    [[nodiscard]] static SculptureRay generateCameraRay(
        float ndcX,
        float ndcY,
        const SculptureVec3& cameraPos,
        const SculptureVec3& cameraTarget,
        const SculptureVec3& cameraUp,
        float fovRadians,
        float aspectRatio
    );

    // Stereoscopic camera ray synthesis with accurate ocular parallax & convergence
    [[nodiscard]] static SculptureRay generateStereoCameraRay(
        float ndcX,
        float ndcY,
        StereoEye eye,
        const SculptureStereoRig& rig,
        const SculptureVec3& cameraPos,
        const SculptureVec3& cameraTarget,
        const SculptureVec3& cameraUp,
        float fovRadians,
        float aspectRatio
    );

    // Full 360-degree Equirectangular Panoramic Spherical VR Ray Synthesis
    // u in [0, 1] maps to longitude [-pi, pi], v in [0, 1] maps to latitude [pi/2, -pi/2]
    [[nodiscard]] static SculptureRay generatePanoramicRay(
        float u,
        float v,
        const SculptureVec3& cameraPos,
        const SculptureVec3& cameraForward,
        const SculptureVec3& cameraUp
    );

    // Renders synchronized stereoscopic Left & Right eye RGB image buffers for VR headsets
    void renderStereoImagePair(
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
        const std::vector<float>& spatialBuffer = {},
        std::uint32_t threadCount = 4,
        const SculptureVolumetricMemory* volumetricMemory = nullptr
    ) const;

    // Renders synchronized stereoscopic Left & Right eye G-Buffer pairs for VR headsets & XR composition
    void renderStereoGBufferPair(
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
        const std::vector<float>& spatialBuffer = {},
        std::uint32_t threadCount = 4,
        const SculptureVolumetricMemory* volumetricMemory = nullptr,
        const SculptureVec3* prevCameraPos = nullptr,
        const SculptureVec3* prevCameraTarget = nullptr
    ) const;

    [[nodiscard]] float estimateDistance(
        const SculptureVec3& p,
        const SculptureProgram& program,
        float time,
        float morph,
        const std::vector<float>& spatialBuffer = {}
    ) const;

    [[nodiscard]] float estimateChamberDistance(
        const SculptureVec3& p,
        const SculptureProgram& program,
        float time,
        float morph
    ) const;

    [[nodiscard]] SculptureVec3 computeTetrahedralNormal(
        const SculptureVec3& p,
        const SculptureProgram& program,
        float time,
        float morph,
        float eps = 0.002f
    ) const;

private:
    struct VolumeObservation;
    [[nodiscard]] VolumeObservation acquireVolumeObservation(
        const SculptureVolumetricMemory* memory) const;
    [[nodiscard]] SculptureRayHit marchVolumetricRayObserved(
        const SculptureRay& ray,
        const SculptureProgram& program,
        float time,
        float morph,
        const std::vector<float>& spatialBuffer,
        const VolumeObservation& observation
    ) const;

    SculptureRaymarchConfig config_;
};

} // namespace epi_sculpt
