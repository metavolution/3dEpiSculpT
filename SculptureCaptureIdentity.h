#pragma once

#include <cstdint>

namespace epi_sculpt {

struct SculptureCaptureFrameIdentity final {
    std::uint64_t frameSequence = 0U;
    std::uint64_t deviceGeneration = 0U;
    std::uint64_t surfaceGeneration = 0U;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return frameSequence != 0U && deviceGeneration != 0U && surfaceGeneration != 0U;
    }
    constexpr bool operator==(const SculptureCaptureFrameIdentity&) const noexcept = default;
};

// Observation facts only. Admission freezes the native namespace; a returned
// accepted frame qualifies its serial; frame-exit finalization makes its copied
// provenance consumable. This object cannot submit, retire or reset GPU work.
class SculptureCaptureIdentity final {
public:
    explicit SculptureCaptureIdentity(SculptureCaptureFrameIdentity frame) noexcept
        : frame_(frame) {}

    [[nodiscard]] bool observeSubmission(SculptureCaptureFrameIdentity observed,
                                        std::uint64_t serial, bool submitted) noexcept {
        if (settled_ || finalized_) return false; // Never rebind a pending capture.
        settled_ = true;
        if (submitted && serial != 0U && frame_.valid() && observed == frame_)
            submissionSerial_ = serial;
        return submissionSerial_ != 0U;
    }

    void finalize() noexcept { finalized_ = true; }

    [[nodiscard]] bool matchesReadback(std::uint64_t serial, bool submitted) const noexcept {
        return finalized_ && submitted && submissionSerial_ != 0U
            && serial == submissionSerial_;
    }

    [[nodiscard]] bool matchesReadback(SculptureCaptureFrameIdentity frame,
                                      std::uint64_t serial, bool submitted) const noexcept {
        return frame == frame_ && matchesReadback(serial, submitted);
    }

    [[nodiscard]] std::uint64_t submissionSerial() const noexcept { return submissionSerial_; }

private:
    const SculptureCaptureFrameIdentity frame_;
    std::uint64_t submissionSerial_ = 0U;
    bool settled_ = false;
    bool finalized_ = false;
};

} // namespace epi_sculpt
