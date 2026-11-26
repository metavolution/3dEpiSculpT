#pragma once

#include "ImplicitFrameProducer.h"
#include "ImplicitProductionAttempt.h"
#include "ImplicitRetirementGate.h"
#include "mgfx/Engine.h"
#include "mgfx/vk/Image.h"
#include "mgfx/pbr/Material.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace epi_sculpt {

class ImplicitExternalImageTransport;

// Authored optical program; transport and presentation remain shared. A glow
// integral has no unique surface, so it must not inject synthetic G-buffer depth.
struct ImplicitFieldProgram {
    std::string kernelPath;
    std::string kernelEntryPoint = "raymarch_sculpture_chamber";
    bool surfaceComposition = true;
};

// Scalar identity of one completed implicit frame. Publication and presentation
// selection deliberately reuse the same resource-free identity type: the two
// *events* remain distinct, while their join key stays compact and typed.
struct ImplicitFrameIdentity final {
    std::uint32_t generation = 0U;
    std::uint32_t slotIndex = ImplicitExternalColorSlot::kInvalidSlot;
    std::uint64_t allocationGeneration = 0U;
    bool externalMemory = false;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return generation != 0U;
    }
};

using ImplicitPresentationSelection = ImplicitFrameIdentity;

// Native resource/transport owner for one implicit observation session.
// Application cadence, schedules and accepted-generation settlement stay outside.
// Keep this object at a stable address until FrameLoop has settled its callbacks;
// the Engine/device must outlive it and native work must already be retired.
class SculptureImplicitObservationSession final {
public:
    explicit SculptureImplicitObservationSession(
        mgfx::Engine& engine, const ImplicitFieldProgram& program = {});
    ~SculptureImplicitObservationSession();
    SculptureImplicitObservationSession(const SculptureImplicitObservationSession&) = delete;
    SculptureImplicitObservationSession& operator=(const SculptureImplicitObservationSession&) = delete;
    SculptureImplicitObservationSession(SculptureImplicitObservationSession&&) = delete;
    SculptureImplicitObservationSession& operator=(SculptureImplicitObservationSession&&) = delete;

    [[nodiscard]] bool usesEngine(const mgfx::Engine& engine) const noexcept {
        return &engine_ == &engine;
    }

    void setImplicitRenderScale(float scale) noexcept;
    void setImplicitFixedExtent(VkExtent2D extent);
    void setImplicitQuality(ImplicitQualityConfig quality) noexcept;
    void setImplicitExternalTransportEnabled(bool enabled) noexcept;
    void resetRetryBackoff() noexcept { implicitResizeRetryCountdown_ = 0U; }
    [[nodiscard]] float implicitRenderScale() const noexcept { return implicitRenderScale_; }
    [[nodiscard]] std::optional<VkExtent2D> implicitFixedExtent() const noexcept {
        return implicitFixedExtent_;
    }

    [[nodiscard]] bool prepareImplicitObservation(
        const mgfx::Camera& camera, const SculptureProgramParams& params,
        float time, float morph, float deltaSeconds, std::uint32_t generation,
        ImplicitFrameRequest& outRequest);
    void submitImplicitObservation(
        const ImplicitFrameRequest& request, ImplicitProductionAttemptObserver observer);

    // Copies/selects completed output and draws its persistent compositor quad.
    // The director supplies the camera-space transform; no cadence is admitted here.
    // A Drawable from another Engine rejects before any handoff is recorded.
    void drawObservation(mgfx::Drawable& draw, const mgfx::math::mat4& quadModel);

    [[nodiscard]] ImplicitFrameProducer* implicitProducer() noexcept {
        return implicitProducer_.get();
    }
    [[nodiscard]] const ImplicitExternalImageTransport* implicitExternalTransport() const noexcept {
        return implicitExternalTransport_.get();
    }
    [[nodiscard]] const std::string& implicitDiagnostic() const noexcept {
        return implicitResizeDiagnostic_;
    }
    [[nodiscard]] const std::string& implicitTransportDiagnostic() const noexcept {
        return implicitTransportDiagnostic_;
    }
    [[nodiscard]] bool implicitSessionStopped() const noexcept {
        return implicitRetirementGate_.stopped();
    }
    [[nodiscard]] VkResult implicitRetirementResult() const noexcept {
        return implicitRetirementGate_.result();
    }
    [[nodiscard]] std::optional<ImplicitFrameRequest> stagedImplicitObservation() const noexcept {
        if (!implicitTextureReady_ || implicitExternalTransport_ || implicitRetirementGate_.stopped())
            return std::nullopt;
        return stagedImplicitObservation_;
    }
    [[nodiscard]] ImplicitFrameIdentity implicitPublishedFrame() const noexcept;
    [[nodiscard]] ImplicitPresentationSelection implicitPresentationSelection() const noexcept {
        return implicitPresentationSelection_;
    }

private:
    [[nodiscard]] bool ensureImplicitExtent(VkExtent2D swapchainExtent);

    mgfx::Engine& engine_;
    bool implicitSurfaceComposition_ = true;
    ImplicitFrameProducer::Config implicitConfig_{};
    std::unique_ptr<ImplicitFrameProducer> implicitProducer_;
    std::unique_ptr<ImplicitExternalImageTransport> implicitExternalTransport_;

    std::unique_ptr<mgfx::vk::Image> implicitTexture_; // Color
    std::unique_ptr<mgfx::vk::Image> implicitDepthTexture_;
    std::unique_ptr<mgfx::vk::Image> implicitNormalTexture_;
    std::unique_ptr<mgfx::vk::Image> implicitVelocityTexture_;

    mgfx::MaterialHandle implicitMaterial_ = mgfx::kInvalidMaterial;
    std::optional<mgfx::vk::Mesh> implicitQuadMesh_;
    std::uint32_t lastImplicitUploadGeneration_ = 0U;
    bool implicitTextureReady_ = false;
    bool implicitExternalTransportEnabled_ = true;
    bool implicitExternalRecoveryPending_ = false;
    bool implicitExternalDisabled_ = false;
    std::uint32_t rejectedImplicitWidth_ = 0U;
    std::uint32_t rejectedImplicitHeight_ = 0U;
    std::uint32_t implicitResizeRetryCountdown_ = 0U;
    std::string implicitResizeDiagnostic_;
    std::string implicitTransportDiagnostic_;
    float implicitRenderScale_ = 0.50f;
    std::optional<VkExtent2D> implicitFixedExtent_;
    // Resource-allocation identity is independent from mathematical frame
    // generation. A resize/rebuild may happen without accepting any new math.
    std::uint64_t implicitAllocationGeneration_ = 0U;
    // Every successfully published ImplicitFrameProducer lifetime receives a
    // new epoch so external cadence owners can detect rebuilds without owning
    // or inspecting the producer itself.
    std::uint64_t implicitProducerEpoch_ = 0U;
    ImplicitRetirementGate implicitRetirementGate_;
    std::optional<ImplicitFrameRequest> stagedImplicitObservation_;
    ImplicitPresentationSelection implicitPresentationSelection_{};
};

} // namespace epi_sculpt
