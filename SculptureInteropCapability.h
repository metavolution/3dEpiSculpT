#pragma once

#include <cstdint>
#include <string_view>

namespace epi_sculpt {

// Data-only application boundary: Vulkan reports export capability, OpenCL
// reports import capability, and only a verified handoff may select it.
struct SculptureInteropCapabilities {
    bool vulkanExternalMemoryFd = false;
    bool vulkanExternalSemaphoreFd = false;
    bool matchingDeviceUuid = false;
    bool openClExternalMemoryFd = false;
    bool synchronizationVerified = false;
};

enum class SculptureInteropPath : unsigned char { HostStaged, ExternalMemory };

// The performer-facing lifecycle is intentionally more precise than the
// capability decision: eligible means all prerequisites are present, while
// active means every external-memory slot was imported successfully.
enum class SculptureInteropState : unsigned char {
    HostStaged,
    ExternalMemoryEligible,
    ExternalMemoryActive,
};

[[nodiscard]] constexpr std::string_view name(
    SculptureInteropState state) noexcept {
    switch (state) {
    case SculptureInteropState::HostStaged: return "host-staged";
    case SculptureInteropState::ExternalMemoryEligible: return "eligible";
    case SculptureInteropState::ExternalMemoryActive: return "active";
    }
    return "unknown";
}

// Stable, backend-neutral event vocabulary for performer diagnostics and
// focused fault tests. Detailed driver text remains in `diagnostic()`; this
// enum is the machine-readable reason that policies and HUDs can trust.
enum class SculptureInteropEvent : std::uint8_t {
    NotAttempted,
    OpenClDisabled,
    CapabilityHostStaged,
    ExternalMemoryEligible,
    ExternalMemoryActivated,
    IncompleteSlotContract,
    ImportRejected,
    ImportFailed,
    ProducerUnavailable,
    AcquireRejected,
    ReleaseRejected,
    DispatchFailed,
    WaitFailed,
};

[[nodiscard]] constexpr std::string_view name(
    SculptureInteropEvent event) noexcept {
    switch (event) {
    case SculptureInteropEvent::NotAttempted: return "not-attempted";
    case SculptureInteropEvent::OpenClDisabled: return "opencl-disabled";
    case SculptureInteropEvent::CapabilityHostStaged: return "capability-staged";
    case SculptureInteropEvent::ExternalMemoryEligible: return "eligible";
    case SculptureInteropEvent::ExternalMemoryActivated: return "activated";
    case SculptureInteropEvent::IncompleteSlotContract: return "incomplete-contract";
    case SculptureInteropEvent::ImportRejected: return "import-rejected";
    case SculptureInteropEvent::ImportFailed: return "import-failed";
    case SculptureInteropEvent::ProducerUnavailable: return "producer-unavailable";
    case SculptureInteropEvent::AcquireRejected: return "acquire-rejected";
    case SculptureInteropEvent::ReleaseRejected: return "release-rejected";
    case SculptureInteropEvent::DispatchFailed: return "dispatch-failed";
    case SculptureInteropEvent::WaitFailed: return "wait-failed";
    }
    return "unknown";
}

// One authoritative lifecycle/health record. It deliberately counts events,
// not frames, so live diagnostics remain meaningful under stale-frame reuse.
struct SculptureInteropHealth {
    SculptureInteropState state = SculptureInteropState::HostStaged;
    SculptureInteropEvent lastEvent = SculptureInteropEvent::NotAttempted;
    std::uint32_t activationCount = 0U;
    std::uint32_t acquireRejectionCount = 0U;
    std::uint32_t releaseRejectionCount = 0U;
    std::uint32_t fallbackTransitionCount = 0U;
    std::uint32_t unavailableSlotCount = 0U;

    void noteHostStaged(SculptureInteropEvent event) noexcept {
        state = SculptureInteropState::HostStaged;
        lastEvent = event;
        unavailableSlotCount = 0U;
    }

    void markEligible() noexcept {
        state = SculptureInteropState::ExternalMemoryEligible;
        lastEvent = SculptureInteropEvent::ExternalMemoryEligible;
    }

    void markActivated() noexcept {
        if (state != SculptureInteropState::ExternalMemoryActive) {
            ++activationCount;
        }
        state = SculptureInteropState::ExternalMemoryActive;
        lastEvent = SculptureInteropEvent::ExternalMemoryActivated;
        unavailableSlotCount = 0U;
    }

    void noteAcquireRejected() noexcept {
        ++acquireRejectionCount;
        lastEvent = SculptureInteropEvent::AcquireRejected;
    }

    void noteReleaseRejected(bool newlyUnavailable) noexcept {
        ++releaseRejectionCount;
        if (newlyUnavailable) ++unavailableSlotCount;
        lastEvent = SculptureInteropEvent::ReleaseRejected;
    }

    void transitionToHostStaged(SculptureInteropEvent event) noexcept {
        if (state != SculptureInteropState::HostStaged) {
            ++fallbackTransitionCount;
        }
        noteHostStaged(event);
    }
};

struct SculptureInteropDecision {
    SculptureInteropPath path = SculptureInteropPath::HostStaged;
    std::string_view reason = "Vulkan external-memory FD export is unavailable";

    [[nodiscard]] constexpr bool usesExternalMemory() const noexcept {
        return path == SculptureInteropPath::ExternalMemory;
    }
};

[[nodiscard]] constexpr SculptureInteropDecision decideSculptureInterop(
    const SculptureInteropCapabilities& capabilities) noexcept {
    if (!capabilities.vulkanExternalMemoryFd) {
        return {SculptureInteropPath::HostStaged,
                "Vulkan external-memory FD export is unavailable"};
    }
    if (!capabilities.matchingDeviceUuid) {
        return {SculptureInteropPath::HostStaged,
                "Vulkan and OpenCL device UUIDs are not verified as matching"};
    }
    if (!capabilities.openClExternalMemoryFd) {
        return {SculptureInteropPath::HostStaged,
                "OpenCL external-memory FD import is unavailable"};
    }
    if (!capabilities.vulkanExternalSemaphoreFd) {
        return {SculptureInteropPath::HostStaged,
                "Vulkan external-semaphore FD export is unavailable"};
    }
    if (!capabilities.synchronizationVerified) {
        return {SculptureInteropPath::HostStaged,
                "cross-API acquire/release synchronization is not verified"};
    }
    return {SculptureInteropPath::ExternalMemory, "external-memory handoff eligible"};
}

} // namespace epi_sculpt
