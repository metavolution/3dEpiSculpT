#include "EpiFeedbackFieldProducer.h"
#include "SculptureModeProjection.h"
#include "epi/mode/visual_mode.hpp"

namespace epi_sculpt {

EpiFeedbackFieldProducer::EpiFeedbackFieldProducer()
    : EpiFeedbackFieldProducer(Config{}) {}

EpiFeedbackFieldProducer::EpiFeedbackFieldProducer(const Config& config)
    : config_(config), producer_(epi::CanonicalFeedbackProducer::Config{
          config.width, config.height, config.kernelDimension,
          config.kernelPath, config.sourceStateId, config.generatedKernelDirectory}) {}

EpiFeedbackFieldProducer::~EpiFeedbackFieldProducer() = default;
EpiFeedbackFieldProducer::EpiFeedbackFieldProducer(EpiFeedbackFieldProducer&&) noexcept = default;
EpiFeedbackFieldProducer& EpiFeedbackFieldProducer::operator=(EpiFeedbackFieldProducer&&) noexcept = default;

bool EpiFeedbackFieldProducer::initialize() { return producer_.initialize(); }
bool EpiFeedbackFieldProducer::ready() const noexcept { return producer_.ready(); }
const std::string& EpiFeedbackFieldProducer::diagnostic() const noexcept {
    return producer_.diagnostic();
}

epi::FeedbackInteropEndpointCapabilities
EpiFeedbackFieldProducer::interop_capabilities() const noexcept {
    return producer_.interop_capabilities();
}

bool EpiFeedbackFieldProducer::bind_external_presentation_target(
    epi::ExternalTextureImport& import, std::size_t slotIndex) {
    return producer_.bind_external_presentation_target(import, slotIndex);
}
bool EpiFeedbackFieldProducer::clear_external_presentation_target_after_native_retirement(
    std::size_t slotIndex) noexcept {
    return producer_.clear_external_presentation_target_after_native_retirement(slotIndex);
}
bool EpiFeedbackFieldProducer::has_external_presentation_target(std::size_t slotIndex) const noexcept {
    return producer_.has_external_presentation_target(slotIndex);
}
void EpiFeedbackFieldProducer::set_external_presentation_enabled(bool enabled) noexcept {
    producer_.set_external_presentation_enabled(enabled);
}
epi::QueueDrainResult EpiFeedbackFieldProducer::wait_idle() noexcept {
    return producer_.wait_idle();
}

epi::FeedbackExternalSubmissionResult EpiFeedbackFieldProducer::submit_external_feedback(
    const epi::FeedbackExternalSubmissionRequest& request,
    epi::FeedbackExternalSlotOwnership<EpiFeedbackFieldDeliveryRing::kSlotCount>& ownership) noexcept {
    return producer_.submit_external_feedback(request, ownership);
}

std::optional<epi::FeedbackFieldCheckpoint>
EpiFeedbackFieldProducer::capture_checkpoint() {
    return producer_.capture_checkpoint();
}

std::optional<epi::FeedbackFieldCheckpoint>
EpiFeedbackFieldProducer::checkpoint_from_host_delivery(
    const EpiFeedbackFieldDelivery& delivery) const {
    return producer_.checkpoint_from_host_delivery(delivery);
}

bool EpiFeedbackFieldProducer::restore_checkpoint(
    const epi::FeedbackFieldCheckpoint& checkpoint,
    const epi::VisualMode& mode,
    std::uint64_t resumedResetEpoch,
    std::uint64_t seedImageSelectionRevision,
    const std::shared_ptr<const epi::SeedImageInput>& seedImageInput) {
    return producer_.restore_checkpoint(checkpoint, mode, resumedResetEpoch, seedImageSelectionRevision,
        seedImageInput);
}

std::optional<EpiFeedbackFieldDelivery> EpiFeedbackFieldProducer::produce(
    const epi::VisualMode& mode, float deltaSeconds,
    std::uint64_t generation, std::uint64_t resetEpoch, std::size_t ringIndex,
    std::uint64_t seedImageSelectionRevision,
    const std::shared_ptr<const epi::SeedImageInput>& seedImageInput) {
    return producer_.produce(mode, deltaSeconds, generation, resetEpoch, ringIndex, seedImageSelectionRevision,
        seedImageInput);
}

epi::CanonicalFeedbackProductionResult EpiFeedbackFieldProducer::produce_result(
    const epi::VisualMode& mode, float deltaSeconds,
    std::uint64_t generation, std::uint64_t resetEpoch, std::size_t ringIndex,
    std::uint64_t seedImageSelectionRevision,
    const std::shared_ptr<const epi::SeedImageInput>& seedImageInput) {
    return producer_.produce_result(mode, deltaSeconds, generation, resetEpoch, ringIndex,
        seedImageSelectionRevision, seedImageInput);
}

bool EpiFeedbackFieldProducer::restore_checkpoint(
    const epi::FeedbackFieldCheckpoint& checkpoint,
    const SculptureProgramParams& params,
    std::uint64_t resumedResetEpoch) {
    epi::VisualMode mode{};
    mode_projection::sculptureParamsToVisualMode(
        params, mode, config_.sourceStateId);
    return restore_checkpoint(checkpoint, mode, resumedResetEpoch);
}

std::optional<EpiFeedbackFieldDelivery> EpiFeedbackFieldProducer::produce(
    const SculptureProgramParams& params, float deltaSeconds,
    std::uint64_t generation, std::uint64_t resetEpoch,
    std::size_t ringIndex) {
    epi::VisualMode mode{};
    mode_projection::sculptureParamsToVisualMode(
        params, mode, config_.sourceStateId);
    return produce(mode, deltaSeconds, generation, resetEpoch, ringIndex);
}

epi::CanonicalFeedbackProductionResult EpiFeedbackFieldProducer::produce_result(
    const SculptureProgramParams& params, float deltaSeconds,
    std::uint64_t generation, std::uint64_t resetEpoch,
    std::size_t ringIndex) {
    epi::VisualMode mode{};
    mode_projection::sculptureParamsToVisualMode(
        params, mode, config_.sourceStateId);
    return produce_result(mode, deltaSeconds, generation, resetEpoch, ringIndex);
}

} // namespace epi_sculpt
