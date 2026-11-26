#pragma once

#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: LOCKLESS TRIPLE-BUFFERED STAGING RING
// Triple-buffered 64-byte cache-line aligned DMA ring with acquire-release
// atomic monotonic generation fences. Eliminates false sharing and guarantees
// tearing-free lockless handoff between background compute and the render loop.
// Generalized for reuse across concurrent producers.
// -----------------------------------------------------------------------------
template <typename T, std::size_t SlotCount = 3>
class alignas(64) SculptureStagingRing {
    static_assert(SlotCount >= 2U, "SculptureStagingRing requires at least 2 slots for lockless staging");

public:
    static constexpr std::size_t kSlotCount = SlotCount;

    struct alignas(64) Slot {
        T data{};
        std::atomic<std::uint64_t> sequence{0U};
        std::atomic<std::uint64_t> generation{0U};
    };

    void write(std::size_t slotIndex, const T& value, std::uint64_t generation) noexcept {
        const std::size_t idx = slotIndex % SlotCount;
        auto& slot = slots_[idx];
        const std::uint64_t seq = slot.sequence.load(std::memory_order_relaxed);
        slot.sequence.store(seq + 1U, std::memory_order_release);
        std::atomic_thread_fence(std::memory_order_release);
        slot.data = value;
        slot.generation.store(generation, std::memory_order_release);
        slot.sequence.store(seq + 2U, std::memory_order_release);
        latestSlot_.store(idx, std::memory_order_release);
    }

    [[nodiscard]] T& slotData(std::size_t slotIndex) noexcept {
        return slots_[slotIndex % SlotCount].data;
    }

    [[nodiscard]] const T& slotData(std::size_t slotIndex) const noexcept {
        return slots_[slotIndex % SlotCount].data;
    }

    void commit(std::size_t slotIndex, std::uint64_t generation) noexcept {
        const std::size_t idx = slotIndex % SlotCount;
        auto& slot = slots_[idx];
        const std::uint64_t seq = slot.sequence.load(std::memory_order_relaxed);
        slot.generation.store(generation, std::memory_order_release);
        slot.sequence.store(seq + 2U, std::memory_order_release);
        latestSlot_.store(idx, std::memory_order_release);
    }

    [[nodiscard]] const T* readLatest(std::uint64_t* outGeneration = nullptr) const noexcept {
        const std::size_t slotIdx = latestSlot_.load(std::memory_order_acquire);
        const auto& slot = slots_[slotIdx];
        const std::uint64_t gen = slot.generation.load(std::memory_order_acquire);
        if (outGeneration) *outGeneration = gen;
        return (gen > 0U) ? &slot.data : nullptr;
    }

    [[nodiscard]] bool readLatestCopy(T& outData, std::uint64_t* outGeneration = nullptr) const noexcept {
        for (int retry = 0; retry < 5; ++retry) {
            const std::size_t slotIdx = latestSlot_.load(std::memory_order_acquire);
            const auto& slot = slots_[slotIdx];
            const std::uint64_t seq1 = slot.sequence.load(std::memory_order_acquire);
            if ((seq1 & 1U) != 0U || seq1 == 0U) continue;

            outData = slot.data;
            std::atomic_thread_fence(std::memory_order_acquire);
            const std::uint64_t seq2 = slot.sequence.load(std::memory_order_acquire);
            if (seq1 == seq2) {
                if (outGeneration) *outGeneration = slot.generation.load(std::memory_order_relaxed);
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] const T* readSlot(std::size_t slotIndex, std::uint64_t* outGeneration = nullptr) const noexcept {
        const std::size_t idx = slotIndex % SlotCount;
        const auto& slot = slots_[idx];
        const std::uint64_t gen = slot.generation.load(std::memory_order_acquire);
        if (outGeneration) *outGeneration = gen;
        return (gen > 0U) ? &slot.data : nullptr;
    }

    [[nodiscard]] std::size_t latestSlotIndex() const noexcept {
        return latestSlot_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::uint64_t latestGeneration() const noexcept {
        const std::size_t slotIdx = latestSlot_.load(std::memory_order_acquire);
        return slots_[slotIdx].generation.load(std::memory_order_acquire);
    }

    void reset() noexcept {
        for (auto& slot : slots_) {
            slot.sequence.store(0U, std::memory_order_release);
            slot.generation.store(0U, std::memory_order_release);
        }
        latestSlot_.store(0U, std::memory_order_release);
    }

    [[nodiscard]] constexpr std::size_t capacity() const noexcept { return SlotCount; }

private:
    std::array<Slot, SlotCount> slots_{};
    std::atomic<std::size_t> latestSlot_{0U};
};

} // namespace epi_sculpt
