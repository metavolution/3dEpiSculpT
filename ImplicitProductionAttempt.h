#pragma once

#include <cstdint>

namespace epi_sculpt {

// Presentation-owned resources may decide *how* an implicit request crosses
// the Vulkan/OpenCL boundary, but they should report only this compact fact
// back to the application control plane. The application then owns cadence,
// schedule, receipt, and causal-history mutation.
enum class ImplicitProductionAttemptOutcome : std::uint8_t {
    Accepted,
    ProducerBackpressure,
    ProducerRejected,
    FrameSubmissionRejected,
    TransportBackpressure,
    TransportRejected,
};

struct ImplicitProductionAttemptReport final {
    ImplicitProductionAttemptOutcome outcome =
        ImplicitProductionAttemptOutcome::ProducerRejected;
    std::uint32_t generation = 0U;

    [[nodiscard]] constexpr bool accepted() const noexcept {
        return outcome == ImplicitProductionAttemptOutcome::Accepted
            && generation != 0U;
    }
};

// Allocation-free observer used from FrameLoop submit callbacks. The token is
// an application-owned schedule identity; the director treats it as opaque.
struct ImplicitProductionAttemptObserver final {
    using Handler = void (*)(
        void* context,
        std::uint64_t token,
        const ImplicitProductionAttemptReport& report) noexcept;

    Handler handler = nullptr;
    void* context = nullptr;
    std::uint64_t token = 0U;

    void notify(const ImplicitProductionAttemptReport& report) const noexcept {
        if (handler != nullptr) handler(context, token, report);
    }
};

} // namespace epi_sculpt