#pragma once

#include "EpiFeedbackFieldDelivery.h"

#include <cstdint>
#include <optional>

namespace epi_sculpt {

// Compact, renderer-neutral provenance for explaining exactly which completed
// mathematical field a presentation family is observing.
struct CanonicalFeedbackFrameLineage {
    std::uint64_t generation = 0U;
    std::uint64_t resetEpoch = 0U;
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    EpiFeedbackFieldTransport transport = EpiFeedbackFieldTransport::HostStaged;
    std::optional<EpiFeedbackFieldExternalLease> externalLease;
};

// A presentation family selects its canonical field once, before any eye is
// traversed. The selection borrows the delivery ring's immutable completed
// record; it never creates an eye-specific feedback buffer or a fallback.
class CanonicalFeedbackFrameSelection {
public:
    [[nodiscard]] static CanonicalFeedbackFrameSelection capture(
        bool canonicalRequested,
        const EpiFeedbackFieldDelivery* candidate) noexcept {
        return {
            canonicalRequested,
            canonicalRequested && candidate != nullptr && candidate->isConsumable()
                ? candidate : nullptr,
        };
    }

    [[nodiscard]] bool requested() const noexcept { return requested_; }
    [[nodiscard]] bool ready() const noexcept { return field_ != nullptr; }

    // Deliberately ignores eye index: every view samples the same completed
    // camera-independent generation. Per-eye camera state belongs to Vulkan.
    [[nodiscard]] const EpiFeedbackFieldDelivery* fieldForView(
        std::uint32_t /*eyeIndex*/) const noexcept {
        return field_;
    }

    [[nodiscard]] std::optional<CanonicalFeedbackFrameLineage> lineage() const {
        if (field_ == nullptr) return std::nullopt;
        return CanonicalFeedbackFrameLineage{
            .generation = field_->descriptor.generation,
            .resetEpoch = field_->descriptor.resetEpoch,
            .width = field_->descriptor.width,
            .height = field_->descriptor.height,
            .transport = field_->transport,
            .externalLease = field_->externalLease,
        };
    }

private:
    constexpr CanonicalFeedbackFrameSelection(
        bool requested, const EpiFeedbackFieldDelivery* field) noexcept
        : requested_(requested), field_(field) {}

    bool requested_ = false;
    const EpiFeedbackFieldDelivery* field_ = nullptr;
};

} // namespace epi_sculpt
