#include "SculpturePresentationDirector.h"
#include "ImplicitExternalImageTransport.h"
#include "SculptureAtmosphereEngine.h"
#include "SculptureWorldGpuAbi.h"
#include "mgfx/vk/Buffer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <sstream>
#include <span>
#include <iomanip>
#include <memory>
#include <utility>
#include <stdexcept>

namespace epi_sculpt {

namespace {
    mgfx::pbr::Material presentationMaterial(
        const mgfx::math::vec4& color, float metallic, float roughness,
        const mgfx::math::vec3& emission) {
        mgfx::pbr::Material material{};
        material.baseColorFactor = color;
        material.metallic = metallic;
        material.roughness = roughness;
        material.clearcoatFactor = 0.75f;
        material.clearcoatRoughnessFactor = 0.12f;
        material.emissiveFactor = emission;
        material.emissiveStrength = emission == mgfx::math::vec3(0.0f) ? 0.0f : 1.0f;
        return material;
    }

    mgfx::math::vec3 hsvToRgb(float h, float s, float v) {
        float c = v * s;
        float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;
        mgfx::math::vec3 rgb(0.0f);
        if (h >= 0.0f && h < 60.0f) rgb = {c, x, 0.0f};
        else if (h >= 60.0f && h < 120.0f) rgb = {x, c, 0.0f};
        else if (h >= 120.0f && h < 180.0f) rgb = {0.0f, c, x};
        else if (h >= 180.0f && h < 240.0f) rgb = {0.0f, x, c};
        else if (h >= 240.0f && h < 300.0f) rgb = {x, 0.0f, c};
        else rgb = {c, 0.0f, x};
        return rgb + mgfx::math::vec3(m);
    }
} // namespace

mgfx::MaterialHandle SculpturePresentationDirector::createMaterial(mgfx::Engine& engine, const mgfx::math::vec4& color, float metallic, float roughness, const mgfx::math::vec3& emission) {
    return engine.materials()->createMaterial(
        presentationMaterial(color, metallic, roughness, emission));
}

SculpturePresentationDirector::SculpturePresentationDirector(
    mgfx::Engine& engine, const ImplicitFieldProgram& implicitProgram)
    : ownedImplicitSession_(std::make_unique<SculptureImplicitObservationSession>(
          engine, implicitProgram)),
      implicitSession_(*ownedImplicitSession_), engine_(engine) {
    initializeSceneResources();
}

SculpturePresentationDirector::SculpturePresentationDirector(
    mgfx::Engine& engine, SculptureImplicitObservationSession& implicitSession)
    : implicitSession_(implicitSession), engine_(engine) {
    if (!implicitSession_.usesEngine(engine)) {
        throw std::invalid_argument("implicit session belongs to a different Engine");
    }
    initializeSceneResources();
}

void SculpturePresentationDirector::initializeSceneResources() {
    beadMesh_ = mgfx::vk::Mesh::makeSphere(engine_.context(), 0.15f, 8, 6);
    torusKnotMesh_ = mgfx::vk::Mesh::makeTorusKnot(engine_.context(), 0.12f, 0.04f, 64, 16);
    cubeMesh_ = mgfx::vk::Mesh::makeCube(engine_.context());
    cylinderMesh_ = mgfx::vk::Mesh::makeCylinder(engine_.context(), 0.12f, 0.4f, 16);
    // This is a stable thin base profile. The version-3 world ABI supplies
    // per-instance length and cross-section for selected truss edges.
    trussStrutMesh_ = mgfx::vk::Mesh::makeCylinder(engine_.context(), 0.022f, 1.35f, 12);
    tileMesh_ = mgfx::vk::Mesh::makePlane(engine_.context(), 1.0f, 1);
    groundMesh_ = mgfx::vk::Mesh::makePlane(engine_.context(), 18.0f, 1);

    sculptureMaterials_ = {
        createMaterial(engine_, mgfx::math::vec4(0.10f, 0.82f, 1.00f, 1.0f), 0.72f, 0.18f, mgfx::math::vec3(0.03f, 0.22f, 0.35f)),
        createMaterial(engine_, mgfx::math::vec4(0.92f, 0.14f, 0.72f, 1.0f), 0.35f, 0.24f, mgfx::math::vec3(0.24f, 0.02f, 0.18f)),
        createMaterial(engine_, mgfx::math::vec4(1.00f, 0.63f, 0.13f, 1.0f), 0.92f, 0.20f, mgfx::math::vec3(0.18f, 0.055f, 0.008f)),
    };

    // One director presents one selected world per family. Native materials
    // belong to that active presentation, not to every catalog possibility.
    // MaterialLibrary freezes each frame's values before native playback.
    for (auto& material : worldMaterialPalette_)
        material = engine_.materials()->createUniqueMaterial(presentationMaterial(
            mgfx::math::vec4(1.0f), 0.0f, 1.0f, mgfx::math::vec3(0.0f)));
    groundMaterial_ = createMaterial(engine_, mgfx::math::vec4(0.025f, 0.035f, 0.060f, 1.0f), 0.15f, 0.86f, mgfx::math::vec3(0.0f));

    mgfx::pbr::Atmosphere atmosphere{};
    atmosphere.horizonTint = mgfx::math::vec3(0.0f);
    atmosphere.zenithTint = mgfx::math::vec3(0.0f);
    atmosphere.groundTint = mgfx::math::vec3(0.0f);
    atmosphere.worldRadiance = 0.0f;
    engine_.setAtmosphere(atmosphere);

    mgfx::pbr::LightSourceRegistry lights{};
    lights.slots[0].kind = mgfx::pbr::LightKind::Directional;
    lights.slots[0].positionOrDirection = glm::normalize(mgfx::math::vec3(-0.7f, -1.0f, -0.4f));
    lights.slots[0].color = mgfx::math::vec3(1.0f, 0.80f, 0.68f);
    lights.slots[0].radiance = 3.8f;
    lights.slots[0].castsShadow = true;
    lights.slots[1].kind = mgfx::pbr::LightKind::Directional;
    lights.slots[1].positionOrDirection = glm::normalize(mgfx::math::vec3(0.6f, -0.25f, 0.8f));
    lights.slots[1].color = mgfx::math::vec3(0.35f, 0.65f, 1.0f);
    lights.slots[1].radiance = 1.4f;
    engine_.setLights(lights);
}


void SculpturePresentationDirector::setInstanceMesh(mgfx::vk::Mesh&& mesh) {
    beadMesh_ = std::move(mesh);
}
void SculpturePresentationDirector::setInstanceMaterial(mgfx::MaterialHandle material) {
    if (material != mgfx::kInvalidMaterial) {
        sculptureMaterials_[0] = material;
        sculptureMaterials_[1] = material;
        sculptureMaterials_[2] = material;
    }
}

const SculpturePresentationDirector::MaterialBands&
SculpturePresentationDirector::selectWorldMaterialPalette(
    const WorldSnapshotDelivery& delivery) {
    // A delivery is immutable after OpenCL completion. Its recipe id and
    // topology bits are therefore the complete authoritative palette key.
    // Invalid or pending records retain the proven compact sculpture palette.
    if (!delivery) return sculptureMaterials_;
    const std::size_t generator = delivery.snapshot->recipe.generatorId;
    if (generator >= kWorldMaterialPaletteCount) return sculptureMaterials_;
    const auto topology = static_cast<std::size_t>(
        world_gpu_abi::mirrorTopologyFromFlags(delivery.snapshot->recipe.flags));
    if (topology >= kMirrorCellTopologyCount) return sculptureMaterials_;
    const std::size_t key = generator * kMirrorCellTopologyCount + topology;
    const auto attempt = engine_.activeFrameAttempt();
    const bool collecting = engine_.drawCollectionActive();
    if (collecting && worldMaterialPaletteAttempt_ == attempt
        && worldMaterialPaletteKey_ && *worldMaterialPaletteKey_ != key)
        throw std::logic_error("one director cannot select two world palettes in one frame family");
    if (!worldMaterialPaletteKey_ || *worldMaterialPaletteKey_ != key) {
        const auto preset = lookupWorldPresentationPreset(
            static_cast<WorldGeneratorKind>(generator),
            static_cast<MirrorCellTopology>(topology));
        // Invalidate before mutation: if admission throws, every later key
        // rebuilds all bands instead of mistaking a partial update for a hit.
        worldMaterialPaletteKey_.reset();
        for (std::size_t band = 0; band < worldMaterialPalette_.size(); ++band) {
            const auto& style = preset.materials[band];
            engine_.materials()->updateMaterial(worldMaterialPalette_[band],
                presentationMaterial(
                    {style.baseColor[0], style.baseColor[1], style.baseColor[2], style.baseColor[3]},
                    style.metallic, style.roughness,
                    {style.emission[0], style.emission[1], style.emission[2]}));
        }
        worldMaterialPaletteKey_ = key;
    }
    if (collecting) worldMaterialPaletteAttempt_ = attempt;
    return worldMaterialPalette_;
}

mgfx::PostProcessFrameIntent SculpturePresentationDirector::worldPresentationIntent(
    const WorldSnapshotDelivery& delivery) const noexcept {
    // No completed topology-aware recipe means identity observation. Returning
    // a value never mutates Engine config or leaves restoration work pending.
    if (!delivery) return {};
    const std::size_t generator = delivery.snapshot->recipe.generatorId;
    if (generator >= kWorldMaterialPaletteCount) return {};
    const auto topology = world_gpu_abi::mirrorTopologyFromFlags(
        delivery.snapshot->recipe.flags);
    const auto preset = lookupWorldPresentationPreset(
        static_cast<WorldGeneratorKind>(generator), topology);
    if (!preset.post.topologyAware) return {};

    const auto& baseline = engine_.config().postProcess;
    mgfx::PostProcessFrameIntent intent{};
    intent.volumetricStrength = baseline.volumetricStrength * preset.post.fogStrengthScale;
    intent.volumetricDistanceDensity = baseline.volumetricDistanceDensity
        * preset.post.fogDistanceDensityScale;
    intent.volumetricInScatterStrength = baseline.volumetricInScatterStrength
        * preset.post.fogInScatterScale;
    intent.vignetteStrength = baseline.vignetteStrength * preset.post.vignetteScale;
    intent.volumetricTint = mgfx::math::vec4(
        preset.post.fogTint[0], preset.post.fogTint[1], preset.post.fogTint[2],
        baseline.volumetricTint.w);
    // Explicit application tuning has final precedence over the recipe.
    if (activeTuning_.enabled) {
        intent.volumetricDistanceDensity = activeTuning_.fogDensity;
        intent.halationThreshold = activeTuning_.emissionThreshold;
        intent.halationStrength = activeTuning_.emissionMultiplier;
    }
    return intent;
}

void SculpturePresentationDirector::setPresentationTuning(bool enabled, float fogDensity, float emissionThreshold, float emissionMultiplier) noexcept {
    activeTuning_.enabled = enabled;
    activeTuning_.fogDensity = fogDensity;
    activeTuning_.emissionThreshold = emissionThreshold;
    activeTuning_.emissionMultiplier = emissionMultiplier;
}

bool SculpturePresentationDirector::configureSanctuaryEnvironment(
    const mgfx::procedural::DojoSanctuarySemanticConfig& config) {
    const bool adopted = sanctuaryHost_.configure(engine_, config);
    if (adopted && config.mergeSanctuaryLights) {
        mgfx::pbr::LightSourceRegistry lights{};
        lights.slots[0].kind = mgfx::pbr::LightKind::Directional;
        lights.slots[0].positionOrDirection = glm::normalize(mgfx::math::vec3(-0.7f, -1.0f, -0.4f));
        lights.slots[0].color = mgfx::math::vec3(1.0f, 0.80f, 0.68f);
        lights.slots[0].radiance = 3.8f;
        lights.slots[0].castsShadow = true;
        lights.slots[1].kind = mgfx::pbr::LightKind::Directional;
        lights.slots[1].positionOrDirection = glm::normalize(mgfx::math::vec3(0.6f, -0.25f, 0.8f));
        lights.slots[1].color = mgfx::math::vec3(0.35f, 0.65f, 1.0f);
        lights.slots[1].radiance = 1.4f;
        lights.activeCount = 2U;
        const auto alignM = sanctuaryHost_.sanctuaryAlignmentTransform(
            mgfx::math::vec3(0.0f, -0.28f, 0.0f));
        sanctuaryHost_.mergeSanctuaryLights(lights, alignM);
        engine_.setLights(lights);
    }
    return adopted;
}

SculpturePresentationDirector::~SculpturePresentationDirector() = default;

SculpturePresentationSemanticState
SculpturePresentationDirector::captureSemanticState() const noexcept {
    SculpturePresentationSemanticState state{};
    state.cameraMode = cameraMode_;
    state.renderParadigm = currentParadigm_;
    state.implicitRenderScale = implicitSession_.implicitRenderScale();
    state.implicitUpdateHz = implicitUpdateHz_;
    state.implicitFixedExtent = implicitSession_.implicitFixedExtent();
    state.exposureLeftLuminance = exposureState_.leftLuminance;
    state.exposureRightLuminance = exposureState_.rightLuminance;
    state.exposureUnifiedLuminance = exposureState_.unifiedLuminance;
    state.exposureMultiplier = exposureState_.exposureMultiplier;
    state.exposureTargetEV100 = exposureState_.targetEV100;
    state.tuningEnabled = activeTuning_.enabled;
    state.fogDensity = activeTuning_.fogDensity;
    state.emissionThreshold = activeTuning_.emissionThreshold;
    state.emissionMultiplier = activeTuning_.emissionMultiplier;
    state.sanctuaryConfig = sanctuaryHost_.semanticConfig();
    return state;
}

void SculpturePresentationDirector::restoreSemanticState(
    const SculpturePresentationSemanticState& state) {
    cameraMode_ = state.cameraMode;
    setRenderParadigm(state.renderParadigm);
    setImplicitRenderScale(state.implicitRenderScale);
    setImplicitUpdateHz(state.implicitUpdateHz);
    if (state.implicitFixedExtent) {
        setImplicitFixedExtent(*state.implicitFixedExtent);
    }
    exposureState_.leftLuminance = state.exposureLeftLuminance;
    exposureState_.rightLuminance = state.exposureRightLuminance;
    exposureState_.unifiedLuminance = state.exposureUnifiedLuminance;
    exposureState_.exposureMultiplier = state.exposureMultiplier;
    exposureState_.targetEV100 = state.exposureTargetEV100;
    setPresentationTuning(
        state.tuningEnabled,
        state.fogDensity,
        state.emissionThreshold,
        state.emissionMultiplier);
    configureSanctuaryEnvironment(state.sanctuaryConfig);
}

void SculpturePresentationDirector::setRenderParadigm(
    RenderParadigm paradigm) noexcept {
    if (currentParadigm_ == paradigm) return;
    currentParadigm_ = paradigm;
    if (paradigm == RenderParadigm::RaymarchedImplicit || paradigm == RenderParadigm::UnifiedComposite) {
        // A user-triggered retry should not inherit a previous resize backoff.
        implicitSession_.resetRetryBackoff();
    }
}

void SculpturePresentationDirector::setImplicitRenderScale(float scale) noexcept {
    implicitSession_.setImplicitRenderScale(scale);
}

void SculpturePresentationDirector::setImplicitFixedExtent(VkExtent2D extent) {
    implicitSession_.setImplicitFixedExtent(extent);
}

void SculpturePresentationDirector::setImplicitUpdateHz(float hertz) noexcept {
    implicitUpdateHz_ = std::isfinite(hertz) && hertz > 0.0f ? hertz : 0.0f;
}

ImplicitFrameIdentity SculpturePresentationDirector::implicitPublishedFrame() const noexcept {
    return implicitSession_.implicitPublishedFrame();
}

bool SculpturePresentationDirector::prepareImplicitObservation(
    const mgfx::Camera& camera, const SculptureProgramParams& params,
    float time, float morph, float deltaSeconds, std::uint32_t generation,
    ImplicitFrameRequest& outRequest) {
    return implicitSession_.prepareImplicitObservation(
        camera, params, time, morph, deltaSeconds, generation, outRequest);
}

void SculpturePresentationDirector::submitImplicitObservation(
    const ImplicitFrameRequest& request, ImplicitProductionAttemptObserver observer) {
    implicitSession_.submitImplicitObservation(request, observer);
}

void SculpturePresentationDirector::setImplicitQuality(ImplicitQualityConfig quality) noexcept {
    implicitSession_.setImplicitQuality(quality);
}

void SculpturePresentationDirector::setImplicitExternalTransportEnabled(bool enabled) noexcept {
    implicitSession_.setImplicitExternalTransportEnabled(enabled);
}


void SculpturePresentationDirector::updateCinematicMaterials(bool cinematic, float cinematicTime, bool sequencerActive) {
    if (!cinematic) return;
    for (std::size_t i = 0; i < 3; ++i) {
        float hue = std::fmod((cinematicTime * 20.0f) + (i * 120.0f), 360.0f);
        mgfx::math::vec3 c = hsvToRgb(hue, 0.8f, 1.0f);
        float surge = sequencerActive ? 3.0f : 1.0f;
        mgfx::math::vec3 emission = (i == 2) ? c * surge * 2.5f : mgfx::math::vec3(0.0f);
        
        mgfx::pbr::Material m{};
        m.baseColorFactor = mgfx::math::vec4(c, 1.0f);
        m.metallic = (i == 0) ? 0.9f : 0.3f;
        m.roughness = (i == 2) ? 0.1f : 0.3f;
        m.emissiveFactor = emission;
        m.emissiveStrength = emission == mgfx::math::vec3(0.0f) ? 0.0f : 1.0f;
        engine_.materials()->updateMaterial(sculptureMaterials_[i], m);
    }
}


mgfx::math::vec3 CameraSplineTrack::catmullRom(
    const mgfx::math::vec3& p0,
    const mgfx::math::vec3& p1,
    const mgfx::math::vec3& p2,
    const mgfx::math::vec3& p3,
    float u) noexcept {
    const float u2 = u * u;
    const float u3 = u2 * u;
    return 0.5f * ((2.0f * p1) +
                   (-p0 + p2) * u +
                   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * u2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * u3);
}

void CameraSplineTrack::addWaypoint(const CameraWaypoint& wp) {
    waypoints_.push_back(wp);
}

void CameraSplineTrack::clear() noexcept {
    waypoints_.clear();
}

std::size_t CameraSplineTrack::waypointCount() const noexcept {
    return waypoints_.size();
}

float CameraSplineTrack::totalDuration() const noexcept {
    if (waypoints_.empty()) return 0.0f;
    return waypoints_.back().time;
}

mgfx::Camera CameraSplineTrack::evaluate(float t) const noexcept {
    if (waypoints_.empty()) {
        return mgfx::Camera{
            .eye = mgfx::math::vec3(0.0f, 2.75f, 4.25f),
            .target = mgfx::math::vec3(0.0f, 1.35f, 0.0f),
            .up = mgfx::math::vec3(0.0f, 1.0f, 0.0f),
            .fovYRad = mgfx::math::radians(50.0f)
        };
    }
    if (waypoints_.size() == 1U) {
        return mgfx::Camera{
            .eye = waypoints_[0].position,
            .target = waypoints_[0].target,
            .up = mgfx::math::vec3(0.0f, 1.0f, 0.0f),
            .fovYRad = mgfx::math::radians(waypoints_[0].fovYDegrees)
        };
    }

    const float maxTime = waypoints_.back().time;
    const float wrappedTime = (maxTime > 0.0f) ? std::fmod(std::max(0.0f, t), maxTime) : 0.0f;

    std::size_t idx = 0;
    while (idx + 1 < waypoints_.size() && waypoints_[idx + 1].time <= wrappedTime) {
        ++idx;
    }
    const std::size_t nextIdx = std::min(idx + 1, waypoints_.size() - 1);
    const float segmentDt = waypoints_[nextIdx].time - waypoints_[idx].time;
    const float u = (segmentDt > 1.0e-5f) ? (wrappedTime - waypoints_[idx].time) / segmentDt : 0.0f;

    const std::size_t p0Idx = (idx == 0) ? 0 : idx - 1;
    const std::size_t p1Idx = idx;
    const std::size_t p2Idx = nextIdx;
    const std::size_t p3Idx = std::min(nextIdx + 1, waypoints_.size() - 1);

    const auto pos = catmullRom(waypoints_[p0Idx].position, waypoints_[p1Idx].position,
                                waypoints_[p2Idx].position, waypoints_[p3Idx].position, u);
    const auto target = catmullRom(waypoints_[p0Idx].target, waypoints_[p1Idx].target,
                                   waypoints_[p2Idx].target, waypoints_[p3Idx].target, u);
    const float fov = glm::mix(waypoints_[p1Idx].fovYDegrees, waypoints_[p2Idx].fovYDegrees, u);

    return mgfx::Camera{
        .eye = pos,
        .target = target,
        .up = mgfx::math::vec3(0.0f, 1.0f, 0.0f),
        .fovYRad = mgfx::math::radians(fov)
    };
}

std::array<mgfx::Camera, 2> SculpturePresentationDirector::composeStereoCameras(
    bool cinematic, float cinematicTime, const SculptureWorldState& state, float ipd) const {
    const mgfx::Camera monoCam = composeCamera(cinematic, cinematicTime, state);
    const auto rig = composeStereoRig(monoCam, ipd, 3.0f);
    return { rig.leftEye, rig.rightEye };
}

std::array<mgfx::Camera, 2> SculpturePresentationDirector::composeJitteredStereoCameras(
    bool cinematic, float cinematicTime, const SculptureWorldState& state,
    std::uint32_t frameIndex, float eyeWidth, float eyeHeight, float ipd) const {
    
    auto cameras = composeStereoCameras(cinematic, cinematicTime, state, ipd);
    const mgfx::math::vec2 ndcJitter = HaltonJitter::computeJitterNdc(frameIndex, eyeWidth, eyeHeight);
    
    for (int eye = 0; eye < 2; ++eye) {
        auto& cam = cameras[eye];
        const mgfx::math::vec3 toTarget = cam.target - cam.eye;
        const float dist = glm::length(toTarget);
        if (dist > 1e-4f) {
            const mgfx::math::vec3 forward = toTarget / dist;
            const mgfx::math::vec3 right = glm::normalize(glm::cross(forward, cam.up));
            const mgfx::math::vec3 up = glm::cross(right, forward);
            
            const float tanHalfFov = std::tan(cam.fovYRad * 0.5f);
            const float aspect = std::max(eyeWidth / std::max(eyeHeight, 1.0f), 0.1f);
            
            const float jitterX = ndcJitter.x * tanHalfFov * aspect * dist * 0.5f;
            const float jitterY = ndcJitter.y * tanHalfFov * dist * 0.5f;
            
            cam.target += (right * jitterX + up * jitterY);
        }
    }
    return cameras;
}

mgfx::Camera SculpturePresentationDirector::composeCamera(bool cinematic, float cinematicTime, const SculptureWorldState& state) const {
    if (state.hasCameraOverride) {
        // Captured views are explicit. Live route framing may also provide
        // coordinates, but it never steals manual/cinematic camera authority.
        return mgfx::Camera{
            .eye = mgfx::math::vec3(state.cameraEye[0], state.cameraEye[1], state.cameraEye[2]),
            .target = mgfx::math::vec3(state.cameraTarget[0], state.cameraTarget[1], state.cameraTarget[2]),
            .up = mgfx::math::vec3(state.cameraUp[0], state.cameraUp[1], state.cameraUp[2]),
            .fovYRad = mgfx::math::radians(45.0f)
        };
    }

    if (cameraMode_ == CameraMode::TopOrthographic) {
        return mgfx::Camera{
            .eye = mgfx::math::vec3(0.0f, 7.5f, 0.0f),
            .target = mgfx::math::vec3(0.0f, 1.35f, 0.0f),
            .up = mgfx::math::vec3(0.0f, 0.0f, -1.0f),
            .fovYRad = mgfx::math::radians(45.0f)
        };
    }
    if (cameraMode_ == CameraMode::HelicalFlythrough) {
        const float cT = cinematicTime;
        const float radius = 1.4f + 1.6f * (0.5f + 0.5f * std::cos(cT * 0.35f));
        const float height = 1.35f + 2.0f * std::sin(cT * 0.22f);
        const float theta = state.cameraAngle * 1.5f + cT * 0.45f;
        const mgfx::math::vec3 eye(radius * std::sin(theta), height, radius * std::cos(theta));
        const float nextTheta = theta + 0.4f;
        const mgfx::math::vec3 target(0.5f * radius * std::sin(nextTheta), height * 0.7f, 0.5f * radius * std::cos(nextTheta));
        return mgfx::Camera{ .eye = eye, .target = target, .fovYRad = mgfx::math::radians(55.0f) };
    }
    if (cameraMode_ == CameraMode::VillarceauTunnel) {
        const float cT = cinematicTime;
        const float majorR = 3.5f;
        const float minorR = 1.2f;
        const float omega1 = 0.3f;
        const float omega2 = 0.42426f;
        const mgfx::math::vec3 eye((majorR + minorR * std::cos(cT * omega1)) * std::sin(cT * omega2),
                                   1.35f + minorR * std::sin(cT * omega1),
                                   (majorR + minorR * std::cos(cT * omega1)) * std::cos(cT * omega2));
        const mgfx::math::vec3 target(0.0f, 1.35f, 0.0f);
        return mgfx::Camera{ .eye = eye, .target = target, .fovYRad = mgfx::math::radians(52.0f) };
    }
    if (cameraMode_ == CameraMode::PoincareReliefChamber) {
        const float cT = cinematicTime;
        const float sweep = state.cameraAngle * 0.75f + 0.35f * std::sin(cT * 0.2f);
        const float radius = 3.2f + 0.4f * std::cos(cT * 0.15f);
        const float height = 2.1f + 0.3f * std::sin(cT * 0.25f);
        const mgfx::math::vec3 eye(radius * std::sin(sweep), height, radius * std::cos(sweep));
        const mgfx::math::vec3 target(0.0f, 0.35f, 0.0f);
        return mgfx::Camera{ .eye = eye, .target = target, .fovYRad = mgfx::math::radians(48.0f) };
    }

    float time = state.cameraAngle / 0.16f;
    mgfx::math::vec3 camEye = mgfx::math::vec3(4.25f * std::sin(state.cameraAngle), 2.75f + 0.28f * std::sin(0.23f * time), 4.25f * std::cos(state.cameraAngle));
    mgfx::math::vec3 camTarget = mgfx::math::vec3(0.0f, 1.35f, 0.0f);
    
    if (cinematic || cameraMode_ == CameraMode::DroneCinematic) {
        float cT = cinematicTime;
        float radius = 1.8f + 2.8f * (0.5f + 0.5f * std::sin(cT * 0.4f));
        float height = 1.35f + 2.5f * std::sin(cT * 0.31f);
        float sweepAngle = state.cameraAngle + 0.8f * std::sin(cT * 0.25f);
        camEye = mgfx::math::vec3(radius * std::sin(sweepAngle), height, radius * std::cos(sweepAngle));
        camTarget = mgfx::math::vec3(0.3f * std::sin(cT * 0.7f), 1.35f + 0.4f * std::cos(cT * 0.5f), 0.3f * std::cos(cT * 0.9f));
    }
    const mgfx::Camera rawCam{ .eye = camEye, .target = camTarget, .fovYRad = mgfx::math::radians(50.0f) };
    return sanctuaryHost_.constrainCameraToKeepout(
        rawCam, mgfx::math::vec3(0.0f, -0.28f, 0.0f));
}

mgfx::Camera SculpturePresentationDirector::composeJitteredCamera(
    bool cinematic, float cinematicTime, const SculptureWorldState& state,
    std::uint32_t frameIndex, float viewportWidth, float viewportHeight) const {
    
    mgfx::Camera cam = composeCamera(cinematic, cinematicTime, state);
    
    // Halton (2,3) sub-pixel angular jitter
    const mgfx::math::vec2 ndcJitter = HaltonJitter::computeJitterNdc(frameIndex, viewportWidth, viewportHeight);
    
    // Convert NDC sub-pixel offset into camera space perturbation
    const mgfx::math::vec3 toTarget = cam.target - cam.eye;
    const float dist = glm::length(toTarget);
    if (dist > 1e-4f) {
        const mgfx::math::vec3 forward = toTarget / dist;
        const mgfx::math::vec3 right = glm::normalize(glm::cross(forward, cam.up));
        const mgfx::math::vec3 up = glm::cross(right, forward);
        
        const float tanHalfFov = std::tan(cam.fovYRad * 0.5f);
        const float aspect = std::max(viewportWidth / std::max(viewportHeight, 1.0f), 0.1f);
        
        const float jitterX = ndcJitter.x * tanHalfFov * aspect * dist * 0.5f;
        const float jitterY = ndcJitter.y * tanHalfFov * dist * 0.5f;
        
        cam.target += (right * jitterX + up * jitterY);
    }
    return cam;
}

SculpturePresentationDirector::StereoCameraRig SculpturePresentationDirector::composeStereoRig(
    const mgfx::Camera& centerCamera,
    float ipdMeters,
    float convergenceDistance) noexcept {
    return composeStereoRig(centerCamera, SculptureStereoRig{
        .interpupillaryDistance = ipdMeters,
        .focalConvergenceDistance = convergenceDistance,
        .toeInConvergence = true,
    });
}

SculpturePresentationDirector::StereoCameraRig SculpturePresentationDirector::composeStereoRig(
    const mgfx::Camera& centerCamera,
    const SculptureStereoRig& rig) noexcept {
    const float safeIpd = std::clamp(rig.interpupillaryDistance, 0.01f, 0.20f);
    const float safeDist = std::max(rig.focalConvergenceDistance, 0.1f);

    const mgfx::math::vec3 toTarget = centerCamera.target - centerCamera.eye;
    const float dist = glm::length(toTarget);
    const mgfx::math::vec3 forward = dist > 1e-4f ? (toTarget / dist) : mgfx::math::vec3(0.0f, 0.0f, -1.0f);
    const mgfx::math::vec3 right = glm::normalize(glm::cross(forward, centerCamera.up));
    const mgfx::math::vec3 up = glm::normalize(glm::cross(right, forward));

    const mgfx::math::vec3 lookDisplacement = forward * safeDist;
    const mgfx::math::vec3 convergencePoint = centerCamera.eye + lookDisplacement;

    // Offset left and right eye positions along the inter-ocular right axis
    const mgfx::math::vec3 leftEyePos = centerCamera.eye - right * (safeIpd * 0.5f);
    const mgfx::math::vec3 rightEyePos = centerCamera.eye + right * (safeIpd * 0.5f);

    mgfx::Camera leftCam = centerCamera;
    leftCam.eye = leftEyePos;
    // Converged views share one target. Parallel views translate the target
    // with each eye, preserving equal look directions and the existing lens.
    leftCam.target = rig.toeInConvergence ? convergencePoint : leftEyePos + lookDisplacement;
    leftCam.up = up;

    mgfx::Camera rightCam = centerCamera;
    rightCam.eye = rightEyePos;
    rightCam.target = rig.toeInConvergence ? convergencePoint : rightEyePos + lookDisplacement;
    rightCam.up = up;

    return StereoCameraRig{
        .leftEye = leftCam,
        .rightEye = rightCam,
        .ipdMeters = safeIpd,
        .convergenceDistance = safeDist,
        .toeInConvergence = rig.toeInConvergence,
    };
}

void SculpturePresentationDirector::updateUnifiedStereoExposure(
    float leftEyeLogLum,
    float rightEyeLogLum,
    float deltaTime) noexcept {
    exposureState_.leftLuminance = std::max(leftEyeLogLum, 1e-4f);
    exposureState_.rightLuminance = std::max(rightEyeLogLum, 1e-4f);
    const float targetUnified = std::sqrt(exposureState_.leftLuminance * exposureState_.rightLuminance);

    const float dt = std::clamp(deltaTime, 0.0f, 0.1f);
    const float alpha = 1.0f - std::exp(-dt * 3.0f);
    exposureState_.unifiedLuminance += (targetUnified - exposureState_.unifiedLuminance) * alpha;

    const float safeLum = std::clamp(exposureState_.unifiedLuminance, 0.001f, 1000.0f);
    exposureState_.exposureMultiplier = std::clamp(0.18f / safeLum, 0.05f, 20.0f);
    exposureState_.targetEV100 = std::log2(safeLum * 100.0f / 12.5f);
}

#include "SculptureIsosurface.h"


// -----------------------------------------------------------------------------
// Educational Annotation: DUAL-PARADIGM RENDER PIPELINE
// -----------------------------------------------------------------------------
// This function orchestrates the actual drawing. It branches based on the active
// RenderParadigm:
// 1. VolumetricInstancing: Dispatches Vulkan indirect draws using the 
//    SculptureRenderBridge (which manages instanced geometry).
// 2. RaymarchedImplicit: Pulls the 2D raymarched frames directly from the 
//    ImplicitFrameProducer. (Integration pending: compositor overlay).
// -----------------------------------------------------------------------------
void SculpturePresentationDirector::drawScene(mgfx::Drawable& draw, 
                                            const mgfx::Camera& camera,
                                            SculptureRenderBridge& batches, 
                                            const SculptureProgramParams& params,
                                            const SculptureSnapshotDelivery& delivery,
                                            float time,
                                            float morph,
                                            float deltaSeconds,
                                            const char* hudText,
                                            bool canonicalReliefRequested,
                                            const EpiFeedbackFieldDelivery* canonicalField,
                                            bool canonicalFeedbackUnavailable,
                                            const epi_sculpt::IsosurfaceMesh* isoMesh,
                                            const epi_sculpt::IsosurfaceMesh* tubeMesh) {
    // Implicit and canonical routes do not publish compact observations.
    batches.rebuild(nullptr);
    if (implicitSessionStopped()) return;
    const auto extent = draw.engine().swapchain().extent();
    const float aspect = (extent.height > 0) ? (static_cast<float>(extent.width) / static_cast<float>(extent.height)) : (16.0f / 9.0f);
    const mgfx::math::mat4 view = glm::lookAt(camera.eye, camera.target, camera.up);
    const bool implicitPresentation = currentParadigm_ == RenderParadigm::RaymarchedImplicit || currentParadigm_ == RenderParadigm::UnifiedComposite;
    const bool instancedPresentation = currentParadigm_ == RenderParadigm::VolumetricInstancing || currentParadigm_ == RenderParadigm::UnifiedComposite;
    // The director selects *what* is presented; transport ownership belongs to
    // SculptureRenderBridge.  Both delivery forms are valid OpenCL output:
    // host staging is the portable fallback, while an external lease lets the
    // bridge acquire the canonical image for Vulkan without a CPU copy.
    // The two paradigms have intentionally disjoint normal work. A fullscreen
    // OpenCL field does not benefit from instance culling, LOD selection,
    // recovery meshes, or a ground draw that its near-plane compositor covers.
    // Keep that CPU/Vulkan work on the instanced path only.
    mgfx::math::mat4 sculptureRoot{1.0f};
    CullingFrustum frustum{};
    InstanceFilterOptions filterOptions{};
    if (instancedPresentation) {
        if (sanctuaryHost_.hasAdoptedScene()) {
            sanctuaryHost_.updateDynamicModulation(time, morph);
            sanctuaryHost_.drawAlignedPortalCulled(
                draw, camera, aspect, mgfx::math::vec3(0.0f, -0.28f, 0.0f));
        } else {
            draw.draw(*groundMesh_, groundMaterial_,
                      glm::translate(mgfx::math::mat4(1.0f),
                                     mgfx::math::vec3(0.0f, -0.28f, 0.0f)));
        }
        sculptureRoot = glm::rotate(sculptureRoot, 0.10f * time,
                                    mgfx::math::vec3(0.0f, 1.0f, 0.0f));
        sculptureRoot = glm::scale(sculptureRoot, mgfx::math::vec3(1.38f));
        sculptureRoot = glm::translate(sculptureRoot, mgfx::math::vec3(0.0f, 1.55f, 0.0f));

        if (!canonicalReliefRequested) {
            const mgfx::math::mat4 proj = glm::perspective(
                camera.fovYRad, aspect, camera.znear, camera.zfar);
            frustum = SculptureRenderBridge::extractFrustum(proj * view);

            SculptureLodConfig lodCfg{};
            lodCfg.enableDynamicLod = true;
            // The focused live demos present at 1440p. Start decimating sooner
            // than the cinematic application while preserving silhouette radius.
            lodCfg.nearDistance = 0.8f;
            lodCfg.farDistance = 3.0f;
            lodCfg.cullDistance = 24.0f;
            filterOptions = {
                .minDensityThreshold = 0.32f,
                .frustum = &frustum,
                .cameraPosition = &camera.eye,
                .lodConfig = lodCfg,
            };
        }
    }

    if (implicitPresentation) {
        // Render the quad exactly spanning the near plane
        const float dist = camera.znear + 0.001f;
        const float halfH = dist * std::tan(camera.fovYRad * 0.5f);
        const float halfW = halfH * aspect;
        
        mgfx::math::mat4 quadModel = glm::inverse(view);
        quadModel = glm::translate(quadModel, mgfx::math::vec3(0.0f, 0.0f, -dist));
        quadModel = glm::rotate(quadModel, 1.57079633f, mgfx::math::vec3(1.0f, 0.0f, 0.0f)); // rotate XZ plane to XY plane
        quadModel = glm::scale(quadModel, mgfx::math::vec3(halfW, 1.0f, halfH));
        
        implicitSession_.drawObservation(draw, quadModel);
        if (hudVisible_) draw.drawText(hudText, mgfx::math::vec2(18.0f, 16.0f), 18.0f);

        if (const auto* transport = implicitExternalTransport()) {
            const auto diagnostics = transport->diagnostics();
            char transportOverlay[256]{};
            std::snprintf(transportOverlay, sizeof(transportOverlay),
                "IMPLICIT EXTERNAL | in-flight %u | quarantine %llu | rejected %llu | %s",
                diagnostics.inFlightSlotCount(),
                static_cast<unsigned long long>(diagnostics.quarantineCount),
                static_cast<unsigned long long>(diagnostics.rejectedTransitionCount),
                implicitDiagnostic().empty() ? "ok" : implicitDiagnostic().c_str());
            
            mgfx::math::vec4 hudColor = diagnostics.quarantineCount > 0 
                ? mgfx::math::vec4(1.0f, 0.5f, 0.0f, 1.0f) // Orange/Warning
                : mgfx::math::vec4(0.2f, 1.0f, 0.2f, 1.0f); // Green/Ok
            if (hudVisible_) draw.drawText(transportOverlay, mgfx::math::vec2(18.0f, 42.0f), 16.0f, hudColor);
        } else {
            char transportOverlay[256]{};
            std::snprintf(transportOverlay, sizeof(transportOverlay),
                "IMPLICIT STAGED | %s",
                implicitDiagnostic().empty() ? implicitTransportDiagnostic().c_str()
                                             : implicitDiagnostic().c_str());
            if (hudVisible_) draw.drawText(transportOverlay, mgfx::math::vec2(18.0f, 42.0f), 16.0f, mgfx::math::vec4(1.0f, 0.8f, 0.2f, 1.0f)); // Yellow
        }
        if (!instancedPresentation) return;
    }

    if (canonicalReliefRequested) {
        // The field contains both height-driving luminance and canonical epi2
        // color. The render bridge caches one surface per generation, so the
        // right eye observes the same world instead of rebuilding it.
        if (canonicalField) {
            const auto reliefOutcome = batches.drawRelief(
                draw, sculptureMaterials_[1], sculptureRoot, params,
                *canonicalField, true);

            // Learn Mode distinguishes the input field from draw collection.
            // Submission and presentation remain later, separately owned facts.
            char lineage[256]{};
            if (canonicalField->transport == EpiFeedbackFieldTransport::ExternalMemory
                && canonicalField->externalLease) {
                std::snprintf(lineage, sizeof(lineage),
                    "CANONICAL FIELD | %s | input gen %llu | epoch %llu | %ux%u | OpenCL->Vulkan slot %u, serial %llu",
                    reliefDrawOutcomeName(reliefOutcome),
                    static_cast<unsigned long long>(canonicalField->descriptor.generation),
                    static_cast<unsigned long long>(canonicalField->descriptor.resetEpoch),
                    canonicalField->descriptor.width, canonicalField->descriptor.height,
                    canonicalField->externalLease->slotIndex,
                    static_cast<unsigned long long>(canonicalField->externalLease->readySerial));
            } else {
                std::snprintf(lineage, sizeof(lineage),
                    "CANONICAL FIELD | %s | input gen %llu | epoch %llu | %ux%u | %s",
                    reliefDrawOutcomeName(reliefOutcome),
                    static_cast<unsigned long long>(canonicalField->descriptor.generation),
                    static_cast<unsigned long long>(canonicalField->descriptor.resetEpoch),
                    canonicalField->descriptor.width, canonicalField->descriptor.height,
                    canonicalField->transport == EpiFeedbackFieldTransport::HostStaged
                        ? "OpenCL staged delivery" : "unresolved delivery");
            }
            const auto lineageColor = reliefOutcome == ReliefDrawOutcome::Collected
                ? mgfx::math::vec4(0.35f, 0.95f, 0.80f, 1.0f)
                : reliefOutcome == ReliefDrawOutcome::Deferred
                    ? mgfx::math::vec4(1.0f, 0.70f, 0.20f, 1.0f)
                    : mgfx::math::vec4(1.0f, 0.35f, 0.25f, 1.0f);
            if (hudVisible_) draw.drawText(lineage, mgfx::math::vec2(18.0f, 42.0f), 16.0f,
                lineageColor);
        } else {
            // A canonical preset must remain visually truthful while its first
            // OpenCL generation is pending or its producer is unavailable.
            // Do not draw compact instances or synthesize a CPU image here:
            // either substitute would silently change the selected producer.
            if (hudVisible_) draw.drawText(
                canonicalFeedbackUnavailable
                    ? "CANONICAL FIELD UNAVAILABLE | producer unavailable (compact fallback disabled)"
                    : "CANONICAL FIELD WAITING | no completed OpenCL generation (fallback disabled)",
                mgfx::math::vec2(18.0f, 42.0f), 16.0f,
                mgfx::math::vec4(1.0f, 0.70f, 0.20f, 1.0f));
        }
        if (hudVisible_) draw.drawText(hudText, mgfx::math::vec2(18.0f, 16.0f), 18.0f);
        // Canonical intent never continues into the compact path. This applies
        // to collected, deferred, rejected and pending fields alike.
        return;
    }

    // Volumetric instancing is the only remaining healthy presentation path.
    const mgfx::vk::Mesh* activeMesh = &*beadMesh_;
    if (params.instanceMeshType == 1 && torusKnotMesh_) activeMesh = &*torusKnotMesh_;
    else if (params.instanceMeshType == 2 && cubeMesh_) activeMesh = &*cubeMesh_;
    else if (params.instanceMeshType == 3 && cylinderMesh_) activeMesh = &*cylinderMesh_;
    static_cast<void>(batches.drawDirectly(
        draw, *activeMesh, sculptureMaterials_, sculptureRoot,
        params, delivery, filterOptions));

    frameIndex_ = (frameIndex_ + 1) % 3;
    const std::uint64_t deliveryGen = static_cast<std::uint64_t>(delivery.generation);

    if (isoMesh && !isoMesh->empty()) {
        const std::uint64_t sig = isoMesh->contentSignature(deliveryGen);
        if (sig != 0U && isoMeshRing_[lastIsoSlot_] && isoMeshSignatures_[lastIsoSlot_] == sig) {
            draw.draw(*isoMeshRing_[lastIsoSlot_], sculptureMaterials_[1], sculptureRoot);
        } else {
            lastIsoSlot_ = (lastIsoSlot_ + 1U) % static_cast<std::uint32_t>(isoMeshRing_.size());
            isoVertexScratch_.resize(isoMesh->vertices.size());
            for (std::size_t i = 0; i < isoMesh->vertices.size(); ++i) {
                const auto& v = isoMesh->vertices[i];
                mgfx::vk::Vertex gv{};
                gv.position = mgfx::math::vec3(v.position.x, v.position.y, v.position.z);
                gv.normal = mgfx::math::vec3(v.normal.x, v.normal.y, v.normal.z);
                gv.uv = mgfx::math::vec2(v.materialCoord, v.density);
                gv.color = mgfx::math::vec4(
                    std::clamp(v.normal.x * 0.5f + 0.5f, 0.0f, 1.0f),
                    std::clamp(v.normal.y * 0.5f + 0.5f, 0.0f, 1.0f),
                    std::clamp(v.normal.z * 0.5f + 0.5f, 0.0f, 1.0f),
                    1.0f
                );
                isoVertexScratch_[i] = gv;
            }

            auto& slotMesh = isoMeshRing_[lastIsoSlot_];
            if (slotMesh) {
                slotMesh->updateDynamic(draw.engine().context(), isoVertexScratch_, isoMesh->indices);
            } else {
                slotMesh = mgfx::vk::Mesh::makeDynamic(draw.engine().context(), isoVertexScratch_, isoMesh->indices);
            }
            isoMeshSignatures_[lastIsoSlot_] = sig;
            draw.draw(*slotMesh, sculptureMaterials_[1], sculptureRoot);
        }
    }

    if (tubeMesh && !tubeMesh->empty()) {
        const std::uint64_t sig = tubeMesh->contentSignature(deliveryGen);
        if (sig != 0U && tubeMeshRing_[lastTubeSlot_] && tubeMeshSignatures_[lastTubeSlot_] == sig) {
            draw.draw(*tubeMeshRing_[lastTubeSlot_], sculptureMaterials_[2], sculptureRoot);
        } else {
            lastTubeSlot_ = (lastTubeSlot_ + 1U) % static_cast<std::uint32_t>(tubeMeshRing_.size());
            tubeVertexScratch_.resize(tubeMesh->vertices.size());
            for (std::size_t i = 0; i < tubeMesh->vertices.size(); ++i) {
                const auto& v = tubeMesh->vertices[i];
                mgfx::vk::Vertex gv{};
                gv.position = mgfx::math::vec3(v.position.x, v.position.y, v.position.z);
                gv.normal = mgfx::math::vec3(v.normal.x, v.normal.y, v.normal.z);
                gv.uv = mgfx::math::vec2(v.materialCoord, v.density);
                gv.color = mgfx::math::vec4(1.0f, 0.6f, 0.2f, 1.0f); // Copper-ish color for tubes
                tubeVertexScratch_[i] = gv;
            }

            auto& slotMesh = tubeMeshRing_[lastTubeSlot_];
            if (slotMesh) {
                slotMesh->updateDynamic(draw.engine().context(), tubeVertexScratch_, tubeMesh->indices);
            } else {
                slotMesh = mgfx::vk::Mesh::makeDynamic(draw.engine().context(), tubeVertexScratch_, tubeMesh->indices);
            }
            tubeMeshSignatures_[lastTubeSlot_] = sig;
            draw.draw(*slotMesh, sculptureMaterials_[2], sculptureRoot); // Use Material 2
        }
    }

    if (hudVisible_) draw.drawText(hudText, mgfx::math::vec2(18.0f, 16.0f), 18.0f);
}

bool SculpturePresentationDirector::drawWorldScene(
    mgfx::Drawable& draw,
    SculptureWorldRenderBridge& batches,
    const WorldSnapshotDelivery& delivery,
    float time,
    const char* hudText,
    mgfx::vk::Mesh* gridMesh) {
    // Rebuild only when a completed immutable staging record changes. The
    // renderer otherwise reuses its already batched instance matrices while
    // the OpenCL world lane is asynchronously producing the next generation.
    if (delivery && batches.generation() != delivery.generation) {
        batches.rebuild(delivery);
    }

    
    if (gridMesh) {
        mgfx::math::mat4 gridRoot = glm::translate(mgfx::math::mat4(1.0f), mgfx::math::vec3(0.0f, -0.5f, 0.0f));
        draw.draw(*gridMesh, groundMaterial_, gridRoot);
    }

    if (sanctuaryHost_.hasAdoptedScene()) {
        sanctuaryHost_.updateDynamicModulation(time, 0.5f);
        const auto& fv = draw.frameViews();
        if (fv.count > 0U) {
            const auto& v0 = fv.views[0];
            const mgfx::math::vec3 fwd(
                -v0.view[0][2], -v0.view[1][2], -v0.view[2][2]);
            const float aspect = (v0.sceneExtent.height > 0U)
                ? (static_cast<float>(v0.sceneExtent.width) / static_cast<float>(v0.sceneExtent.height))
                : (16.0f / 9.0f);
            const mgfx::Camera portalCam{
                .eye    = v0.eye,
                .target = v0.eye + fwd,
                .up     = mgfx::math::vec3(v0.view[0][1], v0.view[1][1], v0.view[2][1]),
            };
            sanctuaryHost_.drawAlignedPortalCulled(
                draw, portalCam, aspect, mgfx::math::vec3(0.0f, -0.28f, 0.0f));
        } else {
            sanctuaryHost_.drawAlignedToFloorOrigin(
                draw, mgfx::math::vec3(0.0f, -0.28f, 0.0f));
        }
    } else {
        draw.draw(*groundMesh_, groundMaterial_,
                  glm::translate(mgfx::math::mat4(1.0f),
                                 mgfx::math::vec3(0.0f, -0.28f, 0.0f)));
    }
    mgfx::math::mat4 root{1.0f};
    root = glm::rotate(root, 0.035f * time, mgfx::math::vec3(0.0f, 1.0f, 0.0f));
    root = glm::scale(root, mgfx::math::vec3(1.15f));

    SculptureWorldRenderBridge::PrimitiveRegistry primitives{};
    primitives.meshes = {
        beadMesh_ ? &*beadMesh_ : nullptr,
        torusKnotMesh_ ? &*torusKnotMesh_ : nullptr,
        cylinderMesh_ ? &*cylinderMesh_ : nullptr,
        tileMesh_ ? &*tileMesh_ : nullptr,
        cubeMesh_ ? &*cubeMesh_ : nullptr,
        cylinderMesh_ ? &*cylinderMesh_ : nullptr,
        cylinderMesh_ ? &*cylinderMesh_ : nullptr,
        trussStrutMesh_ ? &*trussStrutMesh_ : nullptr,
    };
    bool selected = false;
    if (const auto policy = SculptureWorldRenderBridge::makeViewFamilyLodPushConstants(
            draw.frameViews(), root)) {
        batches.setCompactionPushConstants(*policy);
        // Bootstrap still runs before publication selection for a valid family.
        // Invalid/absent views cannot admit an old policy as this observation.
        selected = batches.draw(draw, primitives, selectWorldMaterialPalette(delivery),
            root, delivery ? delivery.generation : 0U);
    }
    if (hudVisible_) draw.drawText(hudText, mgfx::math::vec2(18.0f, 16.0f), 18.0f);
    return selected;
}

mgfx::math::vec3 SculpturePresentationDirector::evaluateThinFilmReflectance(
    float cosTheta, float thicknessNm, float nFilm, float nSubstrate) noexcept {
    const float n1 = 1.0f; // Air
    const float n2 = std::max(nFilm, 1.001f);
    const float n3 = std::max(nSubstrate, 1.001f);

    const float cosTheta1 = std::clamp(std::abs(cosTheta), 0.0f, 1.0f);
    const float sinTheta1Sq = 1.0f - cosTheta1 * cosTheta1;
    
    // Snell refraction angles
    const float sinTheta2Sq = (n1 * n1 / (n2 * n2)) * sinTheta1Sq;
    if (sinTheta2Sq >= 1.0f) return mgfx::math::vec3(1.0f); // Total internal reflection
    const float cosTheta2 = std::sqrt(1.0f - sinTheta2Sq);

    const float sinTheta3Sq = (n1 * n1 / (n3 * n3)) * sinTheta1Sq;
    if (sinTheta3Sq >= 1.0f) return mgfx::math::vec3(1.0f);
    const float cosTheta3 = std::sqrt(1.0f - sinTheta3Sq);

    // Fresnel s-polarization (TE)
    const float rs12 = (n1 * cosTheta1 - n2 * cosTheta2) / (n1 * cosTheta1 + n2 * cosTheta2);
    const float rs23 = (n2 * cosTheta2 - n3 * cosTheta3) / (n2 * cosTheta2 + n3 * cosTheta3);

    // Fresnel p-polarization (TM)
    const float rp12 = (n2 * cosTheta1 - n1 * cosTheta2) / (n2 * cosTheta1 + n1 * cosTheta2);
    const float rp23 = (n3 * cosTheta2 - n2 * cosTheta3) / (n3 * cosTheta2 + n2 * cosTheta3);

    // Optical path difference phase factor: 4 * pi * n2 * d * cos(theta2)
    const float opd = 4.0f * 3.14159265358979323846f * n2 * std::max(thicknessNm, 10.0f) * cosTheta2;

    auto evalAiry = [&](float lambdaNm) -> float {
        const float delta = opd / lambdaNm;
        const float cosDelta = std::cos(delta);

        const float rs12Sq = rs12 * rs12;
        const float rs23Sq = rs23 * rs23;
        const float Rs = (rs12Sq + rs23Sq + 2.0f * rs12 * rs23 * cosDelta) /
                         (1.0f + rs12Sq * rs23Sq + 2.0f * rs12 * rs23 * cosDelta);

        const float rp12Sq = rp12 * rp12;
        const float rp23Sq = rp23 * rp23;
        const float Rp = (rp12Sq + rp23Sq + 2.0f * rp12 * rp23 * cosDelta) /
                         (1.0f + rp12Sq * rp23Sq + 2.0f * rp12 * rp23 * cosDelta);

        return std::clamp(0.5f * (Rs + Rp), 0.0f, 1.0f);
    };

    // Standard primary spectral wavelengths (Red: 650nm, Green: 532nm, Blue: 450nm)
    return mgfx::math::vec3{evalAiry(650.0f), evalAiry(532.0f), evalAiry(450.0f)};
}

std::pair<mgfx::math::vec3, mgfx::math::vec3> SculpturePresentationDirector::computeAnisotropicTangentFrame(
    const mgfx::math::vec3& normal, const mgfx::math::vec3& velocity) noexcept {
    mgfx::math::vec3 n = glm::length(normal) > 1e-5f ? glm::normalize(normal) : mgfx::math::vec3(0.0f, 1.0f, 0.0f);
    mgfx::math::vec3 t_raw = velocity - glm::dot(velocity, n) * n;
    mgfx::math::vec3 t;
    if (glm::length(t_raw) > 1e-4f) {
        t = glm::normalize(t_raw);
    } else {
        mgfx::math::vec3 up = (std::abs(n.y) < 0.9f) ? mgfx::math::vec3(0.0f, 1.0f, 0.0f) : mgfx::math::vec3(1.0f, 0.0f, 0.0f);
        t = glm::normalize(glm::cross(up, n));
    }
    mgfx::math::vec3 b = glm::normalize(glm::cross(n, t));
    return {t, b};
}

mgfx::math::vec2 SculpturePresentationDirector::computeScreenSpaceVelocity(
    const mgfx::math::vec3& worldPos,
    const mgfx::math::vec3& worldVelocity,
    const mgfx::math::mat4& currViewProj,
    const mgfx::math::mat4& prevViewProj,
    float deltaTime) noexcept {
    
    // Current world position in clip space
    const mgfx::math::vec4 currClip = currViewProj * mgfx::math::vec4(worldPos, 1.0f);
    if (std::abs(currClip.w) < 1e-6f) {
        return mgfx::math::vec2(0.0f);
    }
    const mgfx::math::vec2 currNdc = mgfx::math::vec2(currClip.x, currClip.y) / currClip.w;

    // Previous world position (reconstructed via reverse advection: p_prev = p - v * dt)
    const mgfx::math::vec3 prevWorldPos = worldPos - worldVelocity * deltaTime;
    const mgfx::math::vec4 prevClip = prevViewProj * mgfx::math::vec4(prevWorldPos, 1.0f);
    if (std::abs(prevClip.w) < 1e-6f) {
        return mgfx::math::vec2(0.0f);
    }
    const mgfx::math::vec2 prevNdc = mgfx::math::vec2(prevClip.x, prevClip.y) / prevClip.w;

    // Map NDC [-1, 1] delta to Screen/UV delta [0, 1]
    // uv = 0.5 * ndc + 0.5 => delta_uv = 0.5 * (currNdc - prevNdc)
    return 0.5f * (currNdc - prevNdc);
}

mgfx::math::vec3 SculpturePresentationDirector::evaluateAtmosphericInScattering(
    const mgfx::math::vec3& rayOrigin,
    const mgfx::math::vec3& rayDirection,
    float marchDistance,
    const mgfx::math::vec3& sunDirection,
    const mgfx::math::vec3& sunRadiance,
    float fogDensity,
    float fogAnisotropy) noexcept {

    AtmosphericVolumeConfig config{};
    config.sunDirection = {sunDirection.x, sunDirection.y, sunDirection.z};
    config.sunRadiance = {sunRadiance.x, sunRadiance.y, sunRadiance.z};
    config.fogBaseDensity = fogDensity;
    config.fogAnisotropy = fogAnisotropy;

    const auto result = SculptureAtmosphereEngine::integrateInScattering(
        {rayOrigin.x, rayOrigin.y, rayOrigin.z},
        {rayDirection.x, rayDirection.y, rayDirection.z},
        marchDistance,
        config
    );

    return mgfx::math::vec3(
        result.inScatteringLinear.x,
        result.inScatteringLinear.y,
        result.inScatteringLinear.z
    );
}

std::string SculpturePresentationDirector::formatTelemetryOverlay(
    const SculptureStageTelemetry& telemetry, float fps, float frameTimeMs) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "FPS: " << fps << " (" << frameTimeMs << " ms) | Mask: 0x" << std::hex << telemetry.activeStageMask << std::dec << "\n";
    ss << "Multirate: " << name(telemetry.multirate.seedScale) << " (" << telemetry.multirate.seedDivisor << "x div, " 
       << telemetry.multirate.auxiliaryVoxelDimension << "^3 vox, " << telemetry.multirate.effectiveCompressionRatio << "x comp)\n";
    ss << "LOD: L0=" << telemetry.lod.lod0Ratio * 100.0f << "% L1=" << telemetry.lod.lod1Ratio * 100.0f << "% L2=" << telemetry.lod.lod2Ratio * 100.0f << "%\n";
    ss << "Dynamics: Contraction=" << telemetry.transform.contractionRatio << " Lyapunov=" << telemetry.transform.lyapunovExponentEst << "\n";
    ss << "Samples: Valid=" << telemetry.blend.finiteSampleCount << "/" << kSampleCount << " Radius=" << telemetry.blend.snapshotRadius << "\n";
    return ss.str();
}

} // namespace epi_sculpt
