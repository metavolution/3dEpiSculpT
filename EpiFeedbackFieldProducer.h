#pragma once

#include "EpiFeedbackFieldDelivery.h"
#include "epi/pipeline/canonical_feedback_producer.hpp"
#include "SculptureProgramSchema.h"

#include <cstdint>
#include <optional>
#include <string>

namespace epi {
struct ExternalTextureImport;
}

namespace epi_sculpt {

// Sculpture host adapter over the shared strict OpenCL feedback producer.
// Preferred callers provide a complete resolved epi2 VisualMode; legacy
// sculpture-only presets retain an explicit compatibility projection. It is
// intentionally separate from Vulkan transport and per-eye presentation: this
// object owns only simulation and completed field delivery.
class EpiFeedbackFieldProducer {
public:
    struct Config {
        std::uint32_t width = 1024U;
        std::uint32_t height = 1024U;
        std::uint32_t kernelDimension = 1024U;
        std::string kernelPath = "kernels/main.cl";
        std::string sourceStateId = "sculpture-mode-bridge";
        // Explicit resolved generated-math directory; empty preserves legacy lookup.
        std::string generatedKernelDirectory;
    };

    explicit EpiFeedbackFieldProducer();
    explicit EpiFeedbackFieldProducer(const Config& config);
    ~EpiFeedbackFieldProducer();
    EpiFeedbackFieldProducer(EpiFeedbackFieldProducer&&) noexcept;
    EpiFeedbackFieldProducer& operator=(EpiFeedbackFieldProducer&&) noexcept;
    EpiFeedbackFieldProducer(const EpiFeedbackFieldProducer&) = delete;
    EpiFeedbackFieldProducer& operator=(const EpiFeedbackFieldProducer&) = delete;

    [[nodiscard]] bool initialize();
    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] const std::string& diagnostic() const noexcept;
    [[nodiscard]] epi::FeedbackInteropEndpointCapabilities
    interop_capabilities() const noexcept;

    // Binds an imported image only as a non-recursive observation target.
    // The caller retains responsibility for external semaphore transactions
    // and must publish an ExternalMemory delivery only after OpenCL signals it.
    [[nodiscard]] bool bind_external_presentation_target(
        epi::ExternalTextureImport& import, std::size_t slotIndex = 0);
    [[nodiscard]] bool clear_external_presentation_target_after_native_retirement(
        std::size_t slotIndex = 0) noexcept;
    [[nodiscard]] bool has_external_presentation_target(std::size_t slotIndex = 0) const noexcept;
    void set_external_presentation_enabled(bool enabled) noexcept;
    [[nodiscard]] epi::QueueDrainResult wait_idle() noexcept;
    [[nodiscard]] epi::FeedbackExternalSubmissionResult submit_external_feedback(
        const epi::FeedbackExternalSubmissionRequest& request,
        epi::FeedbackExternalSlotOwnership<EpiFeedbackFieldDeliveryRing::kSlotCount>& ownership) noexcept;

    // Renderer-neutral recovery state. Capture is intentionally explicit and
    // performs a host materialization; callers choose the retention cadence.
    [[nodiscard]] std::optional<epi::FeedbackFieldCheckpoint> capture_checkpoint();
    [[nodiscard]] std::optional<epi::FeedbackFieldCheckpoint>
    checkpoint_from_host_delivery(const EpiFeedbackFieldDelivery& delivery) const;
    // Preferred semantic seam: recovery and production consume the complete
    // resolved VisualMode owned by epi2. No sculpture projection is involved.
    [[nodiscard]] bool restore_checkpoint(
        const epi::FeedbackFieldCheckpoint& checkpoint,
        const epi::VisualMode& mode,
        std::uint64_t resumedResetEpoch = 0U,
        std::uint64_t seedImageSelectionRevision = 0U,
        const std::shared_ptr<const epi::SeedImageInput>& seedImageInput = {});

    [[nodiscard]] std::optional<EpiFeedbackFieldDelivery> produce(
        const epi::VisualMode& mode,
        float deltaSeconds,
        std::uint64_t generation,
        std::uint64_t resetEpoch,
        std::size_t ringIndex = 0,
        std::uint64_t seedImageSelectionRevision = 0U,
        const std::shared_ptr<const epi::SeedImageInput>& seedImageInput = {});

    // Control-plane callers need to distinguish accepted recursive math from
    // downstream publication. Keep that semantic distinction visible through
    // the sculpture adapter instead of collapsing it into optional delivery.
    [[nodiscard]] epi::CanonicalFeedbackProductionResult produce_result(
        const epi::VisualMode& mode,
        float deltaSeconds,
        std::uint64_t generation,
        std::uint64_t resetEpoch,
        std::size_t ringIndex = 0,
        std::uint64_t seedImageSelectionRevision = 0U,
        const std::shared_ptr<const epi::SeedImageInput>& seedImageInput = {});

    // Compatibility seam for legacy sculpture-authored canonical presets that
    // have no retained VisualMode. New canonical Pattern/relief paths must use
    // the complete-mode overloads above. Delivery simulationTime remains the
    // canonical producer/kernel clock; presentation time is not an input.
    [[nodiscard]] bool restore_checkpoint(
        const epi::FeedbackFieldCheckpoint& checkpoint,
        const SculptureProgramParams& params,
        std::uint64_t resumedResetEpoch = 0U);

    [[nodiscard]] std::optional<EpiFeedbackFieldDelivery> produce(
        const SculptureProgramParams& params,
        float deltaSeconds,
        std::uint64_t generation,
        std::uint64_t resetEpoch,
        std::size_t ringIndex = 0);

    [[nodiscard]] epi::CanonicalFeedbackProductionResult produce_result(
        const SculptureProgramParams& params,
        float deltaSeconds,
        std::uint64_t generation,
        std::uint64_t resetEpoch,
        std::size_t ringIndex = 0);

private:
    Config config_;
    epi::CanonicalFeedbackProducer producer_;
};

} // namespace epi_sculpt
