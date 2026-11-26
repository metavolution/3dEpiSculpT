// Host-side identity and ordering contract for external-memory handoffs.
// Native API submissions and completion remain the caller's responsibility.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>

namespace epi_sculpt {

enum class FeedbackExternalSlotState {
    VulkanOwned,
    VulkanReleased,
    OpenClWriting,
    OpenClReleased,
    VulkanReading,
    Unavailable,
};

struct FeedbackExternalSlotTicket {
    std::uint64_t generation = 0U;
    std::uint64_t epoch = 0U;
    [[nodiscard]] constexpr bool valid() const noexcept { return generation != 0U && epoch != 0U; }
};

struct FeedbackExternalSlotSnapshot {
    FeedbackExternalSlotState state = FeedbackExternalSlotState::VulkanOwned;
    FeedbackExternalSlotTicket ticket{};
};

struct FeedbackExternalSlotHealth {
    std::uint32_t vulkanOwned = 0U;
    std::uint32_t vulkanReleased = 0U;
    std::uint32_t openClWriting = 0U;
    std::uint32_t openClReleased = 0U;
    std::uint32_t vulkanReading = 0U;
    std::uint32_t unavailable = 0U;
    bool untrackedVulkanRelease = false;
    bool quarantinedNativeWork = false;

    [[nodiscard]] std::uint32_t inFlightCount() const noexcept {
        return vulkanReleased + openClWriting + openClReleased + vulkanReading;
    }
    [[nodiscard]] bool readyForRebuild() const noexcept {
        return inFlightCount() == 0U && !untrackedVulkanRelease && !quarantinedNativeWork;
    }
};

template <std::size_t SlotCount = 3U>
class FeedbackExternalSlotOwnership {
public:
    [[nodiscard]] bool inspect(std::size_t index, FeedbackExternalSlotSnapshot* out) const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= SlotCount || out == nullptr) return false;
        *out = slots_[index];
        return true;
    }

    [[nodiscard]] FeedbackExternalSlotState state(std::size_t index) const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return index < SlotCount ? slots_[index].state : FeedbackExternalSlotState::Unavailable;
    }

    [[nodiscard]] bool releaseFromVulkan(std::size_t index, std::uint64_t* epoch = nullptr) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= SlotCount) return false;
        auto& slot = slots_[index];
        if (slot.state != FeedbackExternalSlotState::VulkanOwned || !advanceEpoch(slot)) return false;
        slot.ticket.generation = 0U;
        slot.state = FeedbackExternalSlotState::VulkanReleased;
        if (epoch != nullptr) *epoch = slot.ticket.epoch;
        return true;
    }

    [[nodiscard]] bool acquireForOpenCl(std::size_t index, FeedbackExternalSlotTicket ticket) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= SlotCount || ticket.generation == 0U || ticket.epoch == 0U) return false;
        auto& slot = slots_[index];
        if (slot.state != FeedbackExternalSlotState::VulkanReleased || slot.ticket.epoch != ticket.epoch) return false;
        slot.ticket = ticket;
        slot.state = FeedbackExternalSlotState::OpenClWriting;
        return true;
    }

    [[nodiscard]] bool releaseFromOpenCl(std::size_t index, FeedbackExternalSlotTicket ticket) noexcept {
        return transition(index, ticket, FeedbackExternalSlotState::OpenClWriting,
                          FeedbackExternalSlotState::OpenClReleased);
    }
    [[nodiscard]] bool readyForVulkan(std::size_t index, FeedbackExternalSlotTicket ticket) const noexcept {
        return matchesState(index, ticket, FeedbackExternalSlotState::OpenClReleased);
    }
    [[nodiscard]] bool readyTicket(std::size_t index, FeedbackExternalSlotTicket* out) const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= SlotCount || out == nullptr || slots_[index].state != FeedbackExternalSlotState::OpenClReleased) return false;
        *out = slots_[index].ticket;
        return true;
    }
    [[nodiscard]] bool acquireForVulkan(std::size_t index, FeedbackExternalSlotTicket ticket) noexcept {
        return transition(index, ticket, FeedbackExternalSlotState::OpenClReleased,
                          FeedbackExternalSlotState::VulkanReading);
    }
    [[nodiscard]] bool vulkanResident(std::size_t index, FeedbackExternalSlotTicket ticket) const noexcept {
        return matchesState(index, ticket, FeedbackExternalSlotState::VulkanReading);
    }
    [[nodiscard]] bool releaseFromVulkanRead(std::size_t index, FeedbackExternalSlotTicket ticket) noexcept {
        return transition(index, ticket, FeedbackExternalSlotState::VulkanReading,
                          FeedbackExternalSlotState::VulkanOwned);
    }
    // The same accepted graphics submission consumes the old read and
    // signals the next release. Commit both facts under one lock: on failure
    // the old ticket remains available for exact quarantine.
    [[nodiscard]] bool recycleFromVulkanRead(std::size_t index, FeedbackExternalSlotTicket current,
                                              std::uint64_t nextGeneration,
                                              FeedbackExternalSlotTicket* next) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= SlotCount || nextGeneration == 0U || next == nullptr) return false;
        auto& slot = slots_[index];
        if (slot.state != FeedbackExternalSlotState::VulkanReading || !matches(slot.ticket, current) ||
            slot.ticket.epoch == std::numeric_limits<std::uint64_t>::max()) return false;
        slot.ticket = {nextGeneration, slot.ticket.epoch + 1U};
        slot.state = FeedbackExternalSlotState::OpenClWriting;
        *next = slot.ticket;
        return true;
    }
    [[nodiscard]] bool quarantine(std::size_t index, FeedbackExternalSlotTicket ticket) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= SlotCount) return false;
        auto& slot = slots_[index];
        if (slot.state == FeedbackExternalSlotState::VulkanOwned
            || slot.state == FeedbackExternalSlotState::Unavailable || !matches(slot.ticket, ticket)) return false;
        // Keep the slot unavailable even if the epoch is exhausted.
        static_cast<void>(advanceEpoch(slot));
        slot.ticket.generation = 0U;
        slot.state = FeedbackExternalSlotState::Unavailable;
        return true;
    }
    [[nodiscard]] bool recoverQuarantined(std::size_t index) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= SlotCount) return false;
        auto& slot = slots_[index];
        if (slot.state != FeedbackExternalSlotState::Unavailable || !advanceEpoch(slot)) return false;
        slot.ticket.generation = 0U;
        slot.state = FeedbackExternalSlotState::VulkanOwned;
        return true;
    }

    // Only call after the native allocation boundary has been settled. Reset
    // invalidates prior host tickets; it cannot establish native completion.
    void forceReset() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& slot : slots_) {
            const bool advanced = advanceEpoch(slot);
            slot.ticket.generation = 0U;
            slot.state = advanced ? FeedbackExternalSlotState::VulkanOwned : FeedbackExternalSlotState::Unavailable;
        }
    }
    void noteUntrackedVulkanRelease() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        untrackedVulkanRelease_ = true;
    }
    [[nodiscard]] bool hasUntrackedVulkanRelease() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return untrackedVulkanRelease_;
    }
    void noteQuarantinedNativeWork() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        quarantinedNativeWork_ = true;
    }
    [[nodiscard]] FeedbackExternalSlotHealth health() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        FeedbackExternalSlotHealth value{};
        value.untrackedVulkanRelease = untrackedVulkanRelease_;
        value.quarantinedNativeWork = quarantinedNativeWork_;
        for (const auto& slot : slots_) {
            switch (slot.state) {
            case FeedbackExternalSlotState::VulkanOwned: ++value.vulkanOwned; break;
            case FeedbackExternalSlotState::VulkanReleased: ++value.vulkanReleased; break;
            case FeedbackExternalSlotState::OpenClWriting: ++value.openClWriting; break;
            case FeedbackExternalSlotState::OpenClReleased: ++value.openClReleased; break;
            case FeedbackExternalSlotState::VulkanReading: ++value.vulkanReading; break;
            case FeedbackExternalSlotState::Unavailable: ++value.unavailable; break;
            }
        }
        return value;
    }
    [[nodiscard]] bool readyForRebuild() const noexcept { return health().readyForRebuild(); }

private:
    static bool matches(FeedbackExternalSlotTicket a, FeedbackExternalSlotTicket b) noexcept {
        return a.generation == b.generation && a.epoch == b.epoch;
    }
    static bool advanceEpoch(FeedbackExternalSlotSnapshot& slot) noexcept {
        if (slot.ticket.epoch == std::numeric_limits<std::uint64_t>::max()) return false;
        ++slot.ticket.epoch;
        return true;
    }
    [[nodiscard]] bool matchesState(std::size_t index, FeedbackExternalSlotTicket ticket,
                                    FeedbackExternalSlotState expected) const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return index < SlotCount && slots_[index].state == expected && matches(slots_[index].ticket, ticket);
    }
    [[nodiscard]] bool transition(std::size_t index, FeedbackExternalSlotTicket ticket,
                                  FeedbackExternalSlotState from, FeedbackExternalSlotState to) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= SlotCount || slots_[index].state != from || !matches(slots_[index].ticket, ticket)) return false;
        slots_[index].state = to;
        return true;
    }

    mutable std::mutex mutex_;
    std::array<FeedbackExternalSlotSnapshot, SlotCount> slots_{};
    bool untrackedVulkanRelease_ = false;
    bool quarantinedNativeWork_ = false;
};

} // namespace epi_sculpt
