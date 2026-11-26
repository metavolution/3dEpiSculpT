#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace epi_sculpt {

enum class ImplicitSlotState : std::uint8_t {
    Free,
    Producing,
    Ready,
    Delivered,
};

// Pure ownership policy for asynchronous full-frame delivery. OpenCL events
// and buffers remain producer-owned; this class only proves legal transitions.
template <std::size_t SlotCount>
class ImplicitDeliveryRing {
    static_assert(SlotCount >= 2U);

public:
    struct SlotRecord {
        ImplicitSlotState state = ImplicitSlotState::Free;
        std::uint32_t generation = 0U;
    };

    [[nodiscard]] std::optional<std::size_t> acquireForProduction(
        std::uint32_t generation) noexcept {
        for (std::size_t index = 0U; index < SlotCount; ++index) {
            if (slots_[index].state != ImplicitSlotState::Free) continue;
            slots_[index] = {ImplicitSlotState::Producing, generation};
            return index;
        }
        return std::nullopt;
    }

    [[nodiscard]] bool acquireSpecificForProduction(
        std::size_t index, std::uint32_t generation) noexcept {
        if (index >= SlotCount || slots_[index].state != ImplicitSlotState::Free) {
            return false;
        }
        slots_[index] = {ImplicitSlotState::Producing, generation};
        return true;
    }

    [[nodiscard]] std::optional<std::size_t> nextFreeIndex() const noexcept {
        for (std::size_t index = 0U; index < SlotCount; ++index) {
            if (slots_[index].state == ImplicitSlotState::Free) return index;
        }
        return std::nullopt;
    }

    [[nodiscard]] bool markReady(std::size_t index) noexcept {
        if (index >= SlotCount
            || slots_[index].state != ImplicitSlotState::Producing) {
            return false;
        }
        slots_[index].state = ImplicitSlotState::Ready;
        return true;
    }

    [[nodiscard]] std::optional<std::size_t> deliverNewestReady(
        bool reclaimPriorDelivery = true) noexcept {
        // External-memory presentation keeps the delivered slot pinned until
        // Vulkan has submitted its final read/ownership release. Host-staged
        // presentation may continue to recycle its previously copied slot.
        if (!reclaimPriorDelivery) {
            if (deliveredIndex_) return std::nullopt;
            // Every external Ready slot carries an unconsumed binary signal.
            // Present oldest first; discarding intermediate frames would leak
            // those signals/ownership tickets and eventually exhaust the ring.
            std::optional<std::size_t> oldest;
            for (std::size_t index = 0U; index < SlotCount; ++index) {
                if (slots_[index].state != ImplicitSlotState::Ready) continue;
                if (!oldest || generationNewer(
                        slots_[*oldest].generation, slots_[index].generation)) {
                    oldest = index;
                }
            }
            if (!oldest) return std::nullopt;
            slots_[*oldest].state = ImplicitSlotState::Delivered;
            deliveredIndex_ = oldest;
            deliveredGeneration_ = slots_[*oldest].generation;
            return oldest;
        }
        std::optional<std::size_t> selected;
        for (std::size_t index = 0U; index < SlotCount; ++index) {
            if (slots_[index].state != ImplicitSlotState::Ready) continue;
            if (deliveredIndex_
                && !generationNewer(
                    slots_[index].generation, deliveredGeneration_)) {
                continue;
            }
            if (!selected || generationNewer(
                    slots_[index].generation, slots_[*selected].generation)) {
                selected = index;
            }
        }
        if (!selected) {
            // Late completion of an older generation is obsolete but must be
            // reclaimed so it cannot occupy a staging slot forever.
            if (deliveredIndex_) {
                for (auto& slot : slots_) {
                    if (slot.state == ImplicitSlotState::Ready
                        && !generationNewer(
                            slot.generation, deliveredGeneration_)) {
                        slot.state = ImplicitSlotState::Free;
                    }
                }
            }
            return std::nullopt;
        }

        for (auto& slot : slots_) {
            if ((reclaimPriorDelivery && slot.state == ImplicitSlotState::Delivered)
                || slot.state == ImplicitSlotState::Ready) {
                slot.state = ImplicitSlotState::Free;
            }
        }
        slots_[*selected].state = ImplicitSlotState::Delivered;
        deliveredIndex_ = selected;
        deliveredGeneration_ = slots_[*selected].generation;
        return selected;
    }

    // The caller must have completed the presentation-side ownership release
    // for this exact generation before freeing an external delivered slot.
    [[nodiscard]] bool releaseDelivered(std::uint32_t generation) noexcept {
        if (!deliveredIndex_ || deliveredGeneration_ != generation) return false;
        SlotRecord& slot = slots_[*deliveredIndex_];
        if (slot.state != ImplicitSlotState::Delivered
            || slot.generation != generation) {
            return false;
        }
        slot = {};
        deliveredIndex_.reset();
        return true;
    }
    void forceReset() noexcept {
        for (auto& slot : slots_) {
            slot = {};
        }
        deliveredIndex_.reset();
        deliveredGeneration_ = 0U;
    }

    [[nodiscard]] bool abandon(std::size_t index) noexcept {
        if (index >= SlotCount
            || slots_[index].state == ImplicitSlotState::Delivered) {
            return false;
        }
        slots_[index] = {};
        return true;
    }

    [[nodiscard]] const SlotRecord& slot(std::size_t index) const noexcept {
        return slots_[index];
    }

    [[nodiscard]] std::optional<std::size_t> deliveredIndex() const noexcept {
        return deliveredIndex_;
    }

    [[nodiscard]] std::uint32_t deliveredGeneration() const noexcept {
        return deliveredGeneration_;
    }

    [[nodiscard]] std::uint32_t pendingCount() const noexcept {
        std::uint32_t count = 0U;
        for (const auto& slot : slots_) {
            count += slot.state == ImplicitSlotState::Producing ? 1U : 0U;
        }
        return count;
    }

    [[nodiscard]] static bool generationNewer(
        std::uint32_t candidate, std::uint32_t reference) noexcept {
        return static_cast<std::int32_t>(candidate - reference) > 0;
    }

private:
    std::array<SlotRecord, SlotCount> slots_{};
    std::optional<std::size_t> deliveredIndex_;
    std::uint32_t deliveredGeneration_ = 0U;
};

} // namespace epi_sculpt
