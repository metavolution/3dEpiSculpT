#pragma once

#include <array>
#include <cstdint>
#include <memory>

namespace epi_sculpt {

// A bootstrap signal must be submitted before OpenCL can wait for it. Otherwise
// an early exit can strand an in-order queue behind a frame that never existed.
// Like presentation tickets, this ledger is application-thread-only, not a GPU
// fence. Importers receive a const view; only the native transport settles it.
class WorldBootstrapHandoff {
public:
    static constexpr std::uint32_t kSlotCount = 3U;
    enum class Status : std::uint8_t { Pending, Submitted, Failed };

    [[nodiscard]] Status status(std::uint32_t slot) const noexcept {
        return slot < kSlotCount ? slots_[slot] : Status::Failed;
    }
    [[nodiscard]] bool markSubmitted(std::uint32_t slot) noexcept {
        if (status(slot) != Status::Pending) return false;
        slots_[slot] = Status::Submitted;
        return true;
    }
    void fail(std::uint32_t slot) noexcept {
        if (slot < kSlotCount) slots_[slot] = Status::Failed;
    }

private:
    std::array<Status, kSlotCount> slots_{};
};

// Host ownership only: no GPU handles or procedural state. The dispatcher and
// FrameLoop submit callbacks run serially on the application thread. One shared
// ledger per dispatcher keeps late tickets alive without per-generation allocation.
class WorldPresentationHandoff {
public:
    static constexpr std::uint32_t kSlotCount = 3U;
    enum class Status : std::uint8_t { Unavailable, Ready, Scheduled, Released, Quarantined };

    [[nodiscard]] Status status(std::uint32_t slot, std::uint32_t generation) const noexcept {
        if (slot >= kSlotCount || generation == 0U || slots_[slot].generation != generation)
            return Status::Unavailable;
        return slots_[slot].status;
    }

    [[nodiscard]] bool offer(std::uint32_t slot, std::uint32_t generation) noexcept {
        if (slot >= kSlotCount || generation == 0U) return false;
        auto& entry = slots_[slot];
        if (entry.generation == generation) return entry.status == Status::Ready;
        if (entry.status != Status::Unavailable && entry.status != Status::Released) return false;
        entry = {generation, Status::Ready};
        return true;
    }

    [[nodiscard]] bool schedule(std::uint32_t slot, std::uint32_t generation) noexcept {
        if (status(slot, generation) != Status::Ready) return false;
        slots_[slot].status = Status::Scheduled;
        return true;
    }

    // Queue acceptance authorizes another OpenCL wait on Vulkan's release
    // semaphore. It does not claim that Vulkan has finished using the buffer.
    [[nodiscard]] bool settle(std::uint32_t slot, std::uint32_t generation, bool submitted) noexcept {
        if (status(slot, generation) != Status::Scheduled) return false;
        slots_[slot].status = submitted ? Status::Released : Status::Quarantined;
        return true;
    }

private:
    struct Slot {
        std::uint32_t generation = 0U;
        Status status = Status::Unavailable;
    };
    std::array<Slot, kSlotCount> slots_{};
};

// Identity is copied with immutable delivery metadata. The shared ledger is a
// separate synchronization sidecar; stale generations cannot settle newer work.
struct WorldPresentationTicket {
    std::shared_ptr<WorldPresentationHandoff> owner;
    std::uint32_t slotIndex = 0xFFFFFFFFU;
    std::uint32_t generation = 0U;

    [[nodiscard]] WorldPresentationHandoff::Status status() const noexcept {
        return owner ? owner->status(slotIndex, generation)
                     : WorldPresentationHandoff::Status::Unavailable;
    }
    [[nodiscard]] bool schedule() const noexcept {
        return owner && owner->schedule(slotIndex, generation);
    }
    [[nodiscard]] bool settle(bool submitted) const noexcept {
        return owner && owner->settle(slotIndex, generation, submitted);
    }
};

} // namespace epi_sculpt
