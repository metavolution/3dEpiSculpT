#pragma once

#include <cstdint>

namespace epi_sculpt {

// One-shot observation policy. Only producer acceptance at the exact target
// latches a hold; completion polling and presentation retain their own owners.
class SculptureGenerationPause {
public:
    [[nodiscard]] bool arm(std::uint32_t target, std::uint64_t leaseEpoch) noexcept {
        if (target == 0U || leaseEpoch == 0U) return false;
        target_ = target;
        leaseEpoch_ = leaseEpoch;
        holding_ = false;
        return true;
    }

    [[nodiscard]] bool onAccepted(std::uint32_t generation, std::uint64_t leaseEpoch) noexcept {
        if (!armed() || holding_ || leaseEpoch != leaseEpoch_ || generation != target_)
            return false;
        holding_ = true;
        return true;
    }

    void invalidateIfLeaseChanged(std::uint64_t leaseEpoch) noexcept {
        if (armed() && leaseEpoch != leaseEpoch_) release();
    }

    void release() noexcept {
        target_ = 0U;
        leaseEpoch_ = 0U;
        holding_ = false;
    }

    [[nodiscard]] bool armed() const noexcept { return target_ != 0U; }
    [[nodiscard]] bool holding() const noexcept { return holding_; }
    [[nodiscard]] std::uint32_t target() const noexcept { return target_; }
    [[nodiscard]] std::uint64_t leaseEpoch() const noexcept { return leaseEpoch_; }

private:
    std::uint32_t target_ = 0U;
    std::uint64_t leaseEpoch_ = 0U;
    bool holding_ = false;
};

} // namespace epi_sculpt
