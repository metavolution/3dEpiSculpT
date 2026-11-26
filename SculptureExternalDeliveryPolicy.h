// Backend-neutral policy for selecting a safe producer-to-renderer delivery.
// Vulkan/OpenCL adapters perform API calls; this file only decides which
// already-owned storage may be consumed at each submission phase.

#pragma once

#include <cstdint>
#include <string_view>

namespace epi_sculpt {

enum class SculptureDeliveryAction : std::uint8_t {
    UseHostStaged,
    UseExternal,
    DropUnsafeFrame,
    RetainCommittedExternal,
};

[[nodiscard]] constexpr std::string_view name(
    SculptureDeliveryAction action) noexcept {
    switch (action) {
    case SculptureDeliveryAction::UseHostStaged: return "host-staged";
    case SculptureDeliveryAction::UseExternal: return "external";
    case SculptureDeliveryAction::DropUnsafeFrame: return "drop-unsafe";
    case SculptureDeliveryAction::RetainCommittedExternal: return "retain-external";
    }
    return "unknown";
}

struct SculpturePreSubmitFacts {
    bool externalMetadataValid = false;
    bool externalResourcesReady = false;
    bool externalAcquireAccepted = false;
    bool safeHostSnapshotAvailable = false;
};

[[nodiscard]] constexpr SculptureDeliveryAction decidePreSubmitDelivery(
    SculpturePreSubmitFacts facts) noexcept {
    if (facts.externalMetadataValid && facts.externalResourcesReady
        && facts.externalAcquireAccepted) {
        return SculptureDeliveryAction::UseExternal;
    }
    return facts.safeHostSnapshotAvailable
        ? SculptureDeliveryAction::UseHostStaged
        : SculptureDeliveryAction::DropUnsafeFrame;
}

// After queue commitment the imported buffer is already referenced by Vulkan.
// A failed ownership release can only quarantine/retain that path; switching
// to a host copy at this point would create an unsynchronised second consumer.
[[nodiscard]] constexpr SculptureDeliveryAction decidePostSubmitDelivery(
    bool ownershipReleaseAccepted) noexcept {
    return ownershipReleaseAccepted
        ? SculptureDeliveryAction::UseExternal
        : SculptureDeliveryAction::RetainCommittedExternal;
}

} // namespace epi_sculpt
