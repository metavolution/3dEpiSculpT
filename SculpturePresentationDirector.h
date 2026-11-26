#pragma once

#include <optional>
#include <memory>
#include <array>
#include <string>
#include "mgfx/Engine.h"
#include "mgfx/PostProcessFrameIntent.h"
#include "mgfx/spaceshipDojo/SpaceshipDojoRoomHost.h"

#include "SculptureRenderBridge.h"
#include "SculptureWorldRenderBridge.h"
#include "SculptureAcousticMistBridge.h"
#include "SculptureImplicitObservationSession.h"
#include "SculptureStereoRig.h"

#include "mgfx/vk/Image.h"
#include "mgfx/pbr/Material.h"

#include <variant>

#include "SculptureTelemetry.h"

namespace epi_sculpt {

class ImplicitExternalImageTransport;

// -----------------------------------------------------------------------------
// Educational Annotation: Presentation Director
// Decouples the mathematical generation (Model) from the rendering (View).
// This class manages the Vulkan-backed meshes, PBR materials, cameras, and
// scene atmosphere, making the core application strictly about logic flow.
// -----------------------------------------------------------------------------
enum class CameraMode : std::uint8_t {
    Orbit = 0,
    DroneCinematic = 1,
    TopOrthographic = 2,
    HelicalFlythrough = 3,
    VillarceauTunnel = 4,
    PoincareReliefChamber = 5
};

inline constexpr std::size_t kCameraModeCount = 6U;

[[nodiscard]] constexpr std::string_view name(CameraMode mode) noexcept {
    switch (mode) {
    case CameraMode::Orbit: return "Orbit";
    case CameraMode::DroneCinematic: return "DroneCinematic";
    case CameraMode::TopOrthographic: return "TopOrthographic";
    case CameraMode::HelicalFlythrough: return "HelicalFlythrough";
    case CameraMode::VillarceauTunnel: return "VillarceauTunnel";
    case CameraMode::PoincareReliefChamber: return "PoincareReliefChamber";
    }
    return "Orbit";
}

struct CameraWaypoint {
    mgfx::math::vec3 position{0.0f};
    mgfx::math::vec3 target{0.0f};
    float fovYDegrees = 50.0f;
    float time = 0.0f;
};

class CameraSplineTrack {
public:
    void addWaypoint(const CameraWaypoint& wp);
    void clear() noexcept;
    [[nodiscard]] std::size_t waypointCount() const noexcept;
    [[nodiscard]] mgfx::Camera evaluate(float t) const noexcept;
    [[nodiscard]] float totalDuration() const noexcept;

    [[nodiscard]] static mgfx::math::vec3 catmullRom(
        const mgfx::math::vec3& p0,
        const mgfx::math::vec3& p1,
        const mgfx::math::vec3& p2,
        const mgfx::math::vec3& p3,
        float u) noexcept;

private:
    std::vector<CameraWaypoint> waypoints_;
};

struct IsosurfaceMesh;


// -----------------------------------------------------------------------------
// Educational Annotation: RENDERING PARADIGM SWITCHER
// -----------------------------------------------------------------------------
// This enumerates the fundamental display modes. 
// - VolumetricInstancing: Uses OpenCL to generate points, Vulkan renders 3D meshes
// - RaymarchedImplicit: Uses OpenCL to fully raymarch an SDF into a 2D image
// -----------------------------------------------------------------------------
enum class RenderParadigm : std::uint8_t {
    VolumetricInstancing,
    RaymarchedImplicit,
    UnifiedComposite
};

// Value-only presentation choices that survive replacement of Vulkan/OpenCL
// presentation resources. Native handles, material handles, descriptor sets,
// and transient producer slots are deliberately absent.
struct SculpturePresentationSemanticState final {
    CameraMode cameraMode = CameraMode::Orbit;
    RenderParadigm renderParadigm = RenderParadigm::VolumetricInstancing;
    float implicitRenderScale = 0.50f;
    float implicitUpdateHz = 30.0f;
    std::optional<VkExtent2D> implicitFixedExtent{};
    float exposureLeftLuminance = 1.0f;
    float exposureRightLuminance = 1.0f;
    float exposureUnifiedLuminance = 1.0f;
    float exposureMultiplier = 1.0f;
    float exposureTargetEV100 = 0.0f;
    bool tuningEnabled = false;
    float fogDensity = 0.0f;
    float emissionThreshold = 1.0f;
    float emissionMultiplier = 1.0f;
    mgfx::procedural::DojoSanctuarySemanticConfig sanctuaryConfig{};
};

class SculpturePresentationDirector {

public:
    explicit SculpturePresentationDirector(mgfx::Engine& engine,
        const ImplicitFieldProgram& implicitProgram = {});
    // The injected session must belong to this Engine and outlive the director.
    SculpturePresentationDirector(mgfx::Engine& engine,
        SculptureImplicitObservationSession& implicitSession);
    ~SculpturePresentationDirector();
    void setHudVisible(bool visible) noexcept { hudVisible_ = visible; }
    [[nodiscard]] bool hudVisible() const noexcept { return hudVisible_; }
    
    // Dynamically mutates the lighting and materials for cinematic sequences
    void updateCinematicMaterials(bool cinematic, float cinematicTime, bool sequencerActive);
    
    // -------------------------------------------------------------------------
    // Educational Annotation: HALTON(2,3) LOW-DISCREPANCY SUB-PIXEL JITTER
    // -------------------------------------------------------------------------
    // Evaluates 8-phase Halton sequence offsets within [-0.5, +0.5] pixel space
    // to drive Temporal Anti-Aliasing (TAA) and sub-pixel edge reconstruction.
    // -------------------------------------------------------------------------
    struct HaltonJitter {
        static constexpr std::size_t kPhaseCount = 8U;
        
        [[nodiscard]] static constexpr mgfx::math::vec2 getOffset(std::uint32_t frameIndex) noexcept {
            // Centered 8-phase Halton(2,3) low-discrepancy offsets in [-0.5, +0.5] pixel space
            constexpr std::array<mgfx::math::vec2, kPhaseCount> kHalton8 = {{
                {  0.0f,     -0.166667f },
                { -0.25f,     0.166667f },
                {  0.25f,    -0.388889f },
                { -0.375f,   -0.055556f },
                {  0.125f,    0.277778f },
                { -0.125f,   -0.277778f },
                {  0.375f,    0.055556f },
                { -0.5f,      0.388889f }
            }};
            return kHalton8[frameIndex % kPhaseCount];
        }

        [[nodiscard]] static mgfx::math::vec2 computeJitterNdc(std::uint32_t frameIndex, float width, float height) noexcept {
            const auto pixelOffset = getOffset(frameIndex);
            const float w = std::max(width, 1.0f);
            const float h = std::max(height, 1.0f);
            return mgfx::math::vec2((2.0f * pixelOffset.x) / w, (2.0f * pixelOffset.y) / h);
        }
    };

    // Evaluates a continuous camera spline or fixed camera based on state
    [[nodiscard]] std::array<mgfx::Camera, 2> composeStereoCameras(bool cinematic, float cinematicTime, const SculptureWorldState& state, float ipd = 0.065f) const;

    [[nodiscard]] std::array<mgfx::Camera, 2> composeJitteredStereoCameras(
        bool cinematic, float cinematicTime, const SculptureWorldState& state,
        std::uint32_t frameIndex, float eyeWidth = 1920.0f, float eyeHeight = 1080.0f, float ipd = 0.065f) const;

    [[nodiscard]] mgfx::Camera composeCamera(
        bool cinematic, float cinematicTime, const SculptureWorldState& state) const;

    [[nodiscard]] mgfx::Camera composeJitteredCamera(
        bool cinematic, float cinematicTime, const SculptureWorldState& state,
        std::uint32_t frameIndex, float viewportWidth, float viewportHeight) const;
    
    [[nodiscard]] CameraMode cameraMode() const noexcept { return cameraMode_; }
    void setCameraMode(CameraMode mode) noexcept { cameraMode_ = mode; }

    // Reconstruction boundary: freeze/restore only semantic presentation facts.
    // Device resources are rebuilt by the owning presentation bindings island.
    [[nodiscard]] SculpturePresentationSemanticState captureSemanticState() const noexcept;
    void restoreSemanticState(const SculpturePresentationSemanticState& state);
    void cycleCameraMode() noexcept {
        cameraMode_ = static_cast<CameraMode>((static_cast<std::uint8_t>(cameraMode_) + 1U) % kCameraModeCount);
    }

    [[nodiscard]] CameraSplineTrack& splineTrack() noexcept { return splineTrack_; }
    [[nodiscard]] const CameraSplineTrack& splineTrack() const noexcept { return splineTrack_; }
    
    void drawScene(mgfx::Drawable& draw, 
                   const mgfx::Camera& camera,
                   SculptureRenderBridge& batches, 
                   const SculptureProgramParams& params,
                   const SculptureSnapshotDelivery& delivery,
                   float time,
                   float morph,
                   float deltaSeconds,
                   const char* hudText,
                   bool canonicalReliefRequested = false,
                   const EpiFeedbackFieldDelivery* canonicalField = nullptr,
                   bool canonicalFeedbackUnavailable = false,
                   const epi_sculpt::IsosurfaceMesh* isoMesh = nullptr,
                   const epi_sculpt::IsosurfaceMesh* tubeMesh = nullptr);

    // Dedicated worlds use shared static primitives plus OpenCL-authored full
    // transforms. This is intentionally a separate route from compact sample
    // filtering: neither path interprets the other's ABI.
    // Reports world draw admission independently of ground/HUD recording.
    bool drawWorldScene(mgfx::Drawable& draw,
                        SculptureWorldRenderBridge& batches,
                        const WorldSnapshotDelivery& delivery,
                        float time,
                        const char* hudText, mgfx::vk::Mesh* gridMesh = nullptr);

    // Pure observation selection from completed world metadata and application
    // tuning. Pass the result to Engine's frame entry point; no baseline is edited.
    [[nodiscard]] mgfx::PostProcessFrameIntent worldPresentationIntent(
        const WorldSnapshotDelivery& delivery) const noexcept;
    void setPresentationTuning(bool enabled, float fogDensity, float emissionThreshold, float emissionMultiplier) noexcept;

    // Configures or retires the zero-asset Procedural Spaceship Zen Dojo Sanctuary
    // environment framing the central sculpture/world origin at (0, -0.28, 0).
    // Must be called on the owner thread outside frame recording.
    bool configureSanctuaryEnvironment(
        const mgfx::procedural::DojoSanctuarySemanticConfig& config);
    [[nodiscard]] const mgfx::procedural::DojoSanctuarySemanticConfig&
    sanctuaryEnvironmentConfig() const noexcept {
        return sanctuaryHost_.semanticConfig();
    }
    [[nodiscard]] const mgfx::procedural::SpaceshipDojoRoomHost&
    sanctuaryEnvironmentHost() const noexcept {
        return sanctuaryHost_;
    }

    // Formats a multi-line HUD telemetry overlay string combining render performance, multirate, and stage diagnostics
    [[nodiscard]] static std::string formatTelemetryOverlay(
        const SculptureStageTelemetry& telemetry, float fps = 60.0f, float frameTimeMs = 16.667f);

    // -------------------------------------------------------------------------
    // Physical Thin-Film Multi-Beam Optical Wave Interference & Anisotropy
    // -------------------------------------------------------------------------
    [[nodiscard]] static mgfx::math::vec3 evaluateThinFilmReflectance(
        float cosTheta, float thicknessNm, float nFilm = 1.45f, float nSubstrate = 1.0f) noexcept;

    [[nodiscard]] static std::pair<mgfx::math::vec3, mgfx::math::vec3> computeAnisotropicTangentFrame(
        const mgfx::math::vec3& normal, const mgfx::math::vec3& velocity) noexcept;

    // -------------------------------------------------------------------------
    // Screen-Space Velocity Buffer & Reprojection for TAA and VRS
    // -------------------------------------------------------------------------
    [[nodiscard]] static mgfx::math::vec2 computeScreenSpaceVelocity(
        const mgfx::math::vec3& worldPos,
        const mgfx::math::vec3& worldVelocity,
        const mgfx::math::mat4& currViewProj,
        const mgfx::math::mat4& prevViewProj,
        float deltaTime = 1.0f / 60.0f) noexcept;

    // -------------------------------------------------------------------------
    // Atmospheric Participating Media In-Scattering & Sun-Shafts
    // -------------------------------------------------------------------------
    [[nodiscard]] static mgfx::math::vec3 evaluateAtmosphericInScattering(
        const mgfx::math::vec3& rayOrigin,
        const mgfx::math::vec3& rayDirection,
        float marchDistance,
        const mgfx::math::vec3& sunDirection,
        const mgfx::math::vec3& sunRadiance,
        float fogDensity = 0.45f,
        float fogAnisotropy = 0.55f) noexcept;

    // -------------------------------------------------------------------------
    // Stereoscopic Dual-Eye Frustum Generation (VR & Side-By-Side SBS)
    // -------------------------------------------------------------------------
    struct StereoCameraRig {
        mgfx::Camera leftEye;
        mgfx::Camera rightEye;
        float ipdMeters = 0.065f;          // 65mm standard human interpupillary distance
        float convergenceDistance = 3.0f; // Focal convergence plane
        bool toeInConvergence = true;     // Qualifies whether that plane is used
    };

    // Consume the complete frozen configuration so its convergence selection
    // is retained when adapting application state into native eye cameras.
    [[nodiscard]] static StereoCameraRig composeStereoRig(
        const mgfx::Camera& centerCamera,
        const SculptureStereoRig& rig) noexcept;

    // Existing convenience calls keep their converged-camera defaults.
    [[nodiscard]] static StereoCameraRig composeStereoRig(
        const mgfx::Camera& centerCamera,
        float ipdMeters = 0.065f,
        float convergenceDistance = 3.0f) noexcept;

    // -------------------------------------------------------------------------
    // Multi-View Presentation Resource Keying & Combined-Eye Auto-Exposure
    // -------------------------------------------------------------------------
    struct PresentationResourceKey {
        std::uint32_t frameSlot = 0U;  // Triple-buffering frame index (0..2)
        std::uint32_t viewSlot = 0U;   // 0 = Mono / Left Eye, 1 = Right Eye

        [[nodiscard]] constexpr bool operator==(const PresentationResourceKey& other) const noexcept = default;
        [[nodiscard]] constexpr std::size_t flatIndex(std::size_t maxViews = 2U) const noexcept {
            return static_cast<std::size_t>(frameSlot) * maxViews + static_cast<std::size_t>(viewSlot);
        }
    };

    struct CombinedEyeExposureState {
        float leftLuminance = 1.0f;
        float rightLuminance = 1.0f;
        float unifiedLuminance = 1.0f;
        float exposureMultiplier = 1.0f;
        float targetEV100 = 0.0f;
    };

    // Evaluates combined dual-eye auto-exposure ensuring identical tone-mapping scales across stereo views
    [[nodiscard]] static inline float computeUnifiedStereoExposure(
        float leftEyeLogLum,
        float rightEyeLogLum,
        float currentUnifiedLum,
        float deltaTime,
        float adaptationRate = 3.0f,
        float keyMiddleGrey = 0.18f) noexcept {
        const float safeLeft = std::max(leftEyeLogLum, 1e-4f);
        const float safeRight = std::max(rightEyeLogLum, 1e-4f);
        const float targetUnified = std::sqrt(safeLeft * safeRight);

        const float dt = std::clamp(deltaTime, 0.0f, 0.1f);
        const float alpha = 1.0f - std::exp(-dt * std::max(adaptationRate, 0.1f));
        const float adaptedLum = currentUnifiedLum + (targetUnified - currentUnifiedLum) * alpha;

        const float safeLum = std::clamp(adaptedLum, 0.001f, 1000.0f);
        return std::clamp(keyMiddleGrey / safeLum, 0.05f, 20.0f);
    }

    void updateUnifiedStereoExposure(float leftEyeLogLum, float rightEyeLogLum, float deltaTime) noexcept;
    [[nodiscard]] const CombinedEyeExposureState& exposureState() const noexcept { return exposureState_; }
    [[nodiscard]] float unifiedExposureScale() const noexcept { return exposureState_.exposureMultiplier; }

    void setRenderParadigm(RenderParadigm paradigm) noexcept;
    RenderParadigm renderParadigm() const { return currentParadigm_; }
    // Applies only to the OpenCL implicit image; the Vulkan desktop window
    // remains fixed at 2560x1440. This bounds raymarch cost independently of
    // presentation resolution.
    void setImplicitRenderScale(float scale) noexcept;
    // Startup-only fixed observation extent for reproducible optical replay.
    // Window resize may stretch presentation, but cannot change field sampling.
    void setImplicitFixedExtent(VkExtent2D extent);
    [[nodiscard]] float implicitRenderScale() const noexcept {
        return implicitSession_.implicitRenderScale();
    }
    void setImplicitUpdateHz(float hertz) noexcept;

    // Delegates view-dependent materialization to the implicit resource session.
    // Cadence/admission authority remains in the calling application.
    [[nodiscard]] bool prepareImplicitObservation(
        const mgfx::Camera& camera,
        const SculptureProgramParams& params,
        float time,
        float morph,
        float deltaSeconds,
        std::uint32_t generation,
        ImplicitFrameRequest& outRequest);

    // Attempts a request using the already-owned transport resources and
    // reports only a compact acceptance/deferral fact to the caller. For the
    // external path the report arrives from FrameLoop's submit-commit boundary.
    void submitImplicitObservation(
        const ImplicitFrameRequest& request,
        ImplicitProductionAttemptObserver observer);

    // Configured before first implicit presentation. This changes OpenCL SDF
    // sampling quality only; it neither changes the camera-space compositor
    // nor introduces a CPU rendering route.
    void setImplicitQuality(ImplicitQualityConfig quality) noexcept;

    // Selects the transport only; OpenCL remains the sole implicit-field
    // producer in both modes. Disable this before first presentation to use
    // the proven host-staged Vulkan upload path.
    void setImplicitExternalTransportEnabled(bool enabled) noexcept;

    void setInstanceMesh(mgfx::vk::Mesh&& mesh);
    void setInstanceMaterial(mgfx::MaterialHandle material);

    [[nodiscard]] float implicitUpdateHz() const noexcept {
        return implicitUpdateHz_;
    }

    [[nodiscard]] ImplicitFrameProducer* implicitProducer() noexcept {
        return implicitSession_.implicitProducer();
    }
    [[nodiscard]] const ImplicitExternalImageTransport* implicitExternalTransport() const noexcept {
        return implicitSession_.implicitExternalTransport();
    }
    [[nodiscard]] const std::string& implicitDiagnostic() const noexcept {
        return implicitSession_.implicitDiagnostic();
    }
    [[nodiscard]] bool implicitSessionStopped() const noexcept {
        return implicitSession_.implicitSessionStopped();
    }
    [[nodiscard]] VkResult implicitRetirementResult() const noexcept {
        return implicitSession_.implicitRetirementResult();
    }
    // For a capture queued after drawScene in this same frame. This records the
    // staged image selected for drawing, not the latest requested simulation.
    [[nodiscard]] std::optional<ImplicitFrameRequest> stagedImplicitObservation() const noexcept {
        return implicitSession_.stagedImplicitObservation();
    }
    [[nodiscard]] const std::string& implicitTransportDiagnostic() const noexcept {
        return implicitSession_.implicitTransportDiagnostic();
    }
    // Latest completed producer output, independent of whether the renderer
    // selects it this frame. This is the production-publication seam.
    [[nodiscard]] ImplicitFrameIdentity implicitPublishedFrame() const noexcept;
    [[nodiscard]] ImplicitPresentationSelection implicitPresentationSelection() const noexcept {
        return implicitSession_.implicitPresentationSelection();
    }

private:
    bool hudVisible_ = true;
    // Dedicated generators communicate a stable generator id, 0..2 material
    // band, and recipe flags through the shared world ABI.  Topology comes
    // from those immutable flags, so the material cache remains a pure
    // presentation concern rather than renderer-specific OpenCL state.
    using MaterialBands = std::array<mgfx::MaterialHandle, 3>;
    static constexpr std::size_t kWorldMaterialPaletteCount = kWorldGeneratorCount;

    RenderParadigm currentParadigm_ = RenderParadigm::VolumetricInstancing;
    // The flagship injects a device-island owner. Standalone hosts retain the
    // original construction API through this one optional private owner.
    std::unique_ptr<SculptureImplicitObservationSession> ownedImplicitSession_;
    SculptureImplicitObservationSession& implicitSession_;
    float implicitUpdateHz_ = 30.0f;

    void initializeSceneResources();

    mgfx::MaterialHandle createMaterial(mgfx::Engine& engine, const mgfx::math::vec4& color, float metallic, float roughness, const mgfx::math::vec3& emission);
    [[nodiscard]] const MaterialBands& selectWorldMaterialPalette(
        const WorldSnapshotDelivery& delivery);

    mgfx::Engine& engine_;
    std::uint32_t frameIndex_ = 0;
    std::array<std::unique_ptr<mgfx::vk::Mesh>, 3> isoMeshRing_;
    std::array<std::uint64_t, 3> isoMeshSignatures_{};
    std::uint32_t lastIsoSlot_ = 0U;
    std::vector<mgfx::vk::Vertex> isoVertexScratch_;
    std::array<std::unique_ptr<mgfx::vk::Mesh>, 3> tubeMeshRing_;
    std::array<std::uint64_t, 3> tubeMeshSignatures_{};
    std::uint32_t lastTubeSlot_ = 0U;
    std::vector<mgfx::vk::Vertex> tubeVertexScratch_;
    std::optional<mgfx::vk::Mesh> beadMesh_;
    std::optional<mgfx::vk::Mesh> torusKnotMesh_;
    std::optional<mgfx::vk::Mesh> cubeMesh_;
    std::optional<mgfx::vk::Mesh> cylinderMesh_;
    // Long, fine cylinder for graph and mirror-chamber cage members. It is
    // intentionally distinct from the general cylinder used by other worlds.
    std::optional<mgfx::vk::Mesh> trussStrutMesh_;
    std::optional<mgfx::vk::Mesh> tileMesh_;
    std::optional<mgfx::vk::Mesh> groundMesh_;
    MaterialBands sculptureMaterials_{};
    MaterialBands worldMaterialPalette_{};
    std::optional<std::size_t> worldMaterialPaletteKey_{};
    std::uint64_t worldMaterialPaletteAttempt_ = 0U;
    
    struct ActivePresentationTuning {
        bool enabled = false;
        float fogDensity = 0.0f;
        float emissionThreshold = 1.0f;
        float emissionMultiplier = 1.0f;
    };
    ActivePresentationTuning activeTuning_{};
    
    mgfx::MaterialHandle groundMaterial_;
    mgfx::procedural::SpaceshipDojoRoomHost sanctuaryHost_{};
    CameraMode cameraMode_ = CameraMode::Orbit;
    CameraSplineTrack splineTrack_;
    CombinedEyeExposureState exposureState_{};
};

} // namespace epi_sculpt
