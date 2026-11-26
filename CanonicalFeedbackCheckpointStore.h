#pragma once

#include <epi/pipeline/feedback_checkpoint.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

namespace epi_sculpt {

// Small host-owned retention policy for canonical recursive-field recovery.
//
// This is intentionally not a scheduler and never performs GPU readback. It
// merely decides when an already-host-staged field is worth retaining and
// keeps a tiny newest-first recovery window. Explicit readback remains a caller
// decision for cold-path recovery or persistence.
class CanonicalFeedbackCheckpointStore final {
public:
    static constexpr std::size_t kCapacity = 2U;
    static constexpr std::uint64_t kDefaultHostRetentionIntervalGenerations = 120U;

    explicit CanonicalFeedbackCheckpointStore(
        std::uint64_t hostRetentionIntervalGenerations =
            kDefaultHostRetentionIntervalGenerations) noexcept
        : hostRetentionIntervalGenerations_(hostRetentionIntervalGenerations) {}

    [[nodiscard]] bool shouldRetainHostDelivery(
        const epi::FeedbackFieldDescriptor& descriptor) const noexcept {
        if (descriptor.generation == 0U || descriptor.resetEpoch == 0U
            || descriptor.producerSession != 0U) return false;
        const epi::FeedbackFieldCheckpoint* newest = latest();
        if (newest == nullptr) return true;
        if (isOlder(descriptor, newest->descriptor)) return false;
        if (newest->descriptor.resetEpoch != descriptor.resetEpoch) return true;
        if (descriptor.generation == newest->descriptor.generation) return false;
        return hostRetentionIntervalGenerations_ == 0U
            || descriptor.generation - newest->descriptor.generation
                >= hostRetentionIntervalGenerations_;
    }

    [[nodiscard]] bool retain(epi::FeedbackFieldCheckpoint checkpoint) {
        epi::FeedbackFieldCheckpoint* newest = latestMutable();
        // Ordering is cheap and rejects obsolete large payloads before scanning.
        if (newest && isOlder(checkpoint.descriptor, newest->descriptor)) return false;
        if (!checkpoint.coherent()) return false;

        if (newest != nullptr) {
            if (checkpoint.descriptor.generation == newest->descriptor.generation
                && checkpoint.descriptor.resetEpoch == newest->descriptor.resetEpoch) {
                *newest = std::move(checkpoint);
                return true;
            }
        }

        slots_[nextSlot_] = std::move(checkpoint);
        newestSlot_ = nextSlot_;
        hasNewest_ = true;
        if (count_ < kCapacity) ++count_;
        nextSlot_ = (nextSlot_ + 1U) % kCapacity;
        return true;
    }

    [[nodiscard]] const epi::FeedbackFieldCheckpoint* latest() const noexcept {
        return hasNewest_ && slots_[newestSlot_]
            ? &*slots_[newestSlot_]
            : nullptr;
    }

    [[nodiscard]] const epi::FeedbackFieldCheckpoint* newestCompatible(
        std::string_view sourceStateId,
        std::uint32_t width,
        std::uint32_t height,
        epi::FeedbackFieldFormat format,
        std::uint64_t requiredResetEpoch = 0U) const noexcept {
        if (!hasNewest_) return nullptr;
        for (std::size_t offset = 0U; offset < count_; ++offset) {
            const std::size_t index = (newestSlot_ + kCapacity - offset) % kCapacity;
            const auto& slot = slots_[index];
            // Private slots are validated at admission and exposed only as const.
            // Avoid rescanning the entire host field during recovery selection.
            if (!slot) continue;
            if (requiredResetEpoch != 0U
                && slot->descriptor.resetEpoch != requiredResetEpoch) {
                continue;
            }
            if (slot->descriptor.width != width || slot->descriptor.height != height
                || slot->descriptor.format != format) {
                continue;
            }
            if (!sourceStateId.empty() && !slot->descriptor.sourceStateId.empty()
                && slot->descriptor.sourceStateId != sourceStateId) {
                continue;
            }
            return &*slot;
        }
        return nullptr;
    }

    [[nodiscard]] std::size_t size() const noexcept { return count_; }

private:
    [[nodiscard]] static bool isOlder(const epi::FeedbackFieldDescriptor& candidate,
                                      const epi::FeedbackFieldDescriptor& newest) noexcept {
        return candidate.generation < newest.generation
            || candidate.resetEpoch < newest.resetEpoch;
    }

    [[nodiscard]] epi::FeedbackFieldCheckpoint* latestMutable() noexcept {
        return hasNewest_ && slots_[newestSlot_]
            ? &*slots_[newestSlot_]
            : nullptr;
    }

    std::array<std::optional<epi::FeedbackFieldCheckpoint>, kCapacity> slots_{};
    std::size_t newestSlot_ = 0U;
    std::size_t nextSlot_ = 0U;
    std::size_t count_ = 0U;
    bool hasNewest_ = false;
    std::uint64_t hostRetentionIntervalGenerations_ =
        kDefaultHostRetentionIntervalGenerations;
};

} // namespace epi_sculpt
