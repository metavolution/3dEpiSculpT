#pragma once

#include <cstdint>
#include <memory>

namespace epi_sculpt {

// Cross-API import description intentionally contains no Vulkan or OpenCL
// handles/types. The native transport exports POSIX descriptors; OpenCL-side
// dispatchers consume the value and establish their own imported objects.
inline constexpr std::uint32_t kSculptureExternalSlotCount = 3U;

struct SculptureOpenClImportSlot {
    int memoryFd = -1;
    int releaseSemaphoreFd = -1;
    int openClReadySemaphoreFd = -1;
    std::uint32_t byteSize = 0U;

    [[nodiscard]] bool valid() const noexcept {
        return memoryFd >= 0 && releaseSemaphoreFd >= 0
            && openClReadySemaphoreFd >= 0 && byteSize != 0U;
    }
};

class WorldBootstrapHandoff;

// World production additionally requires proof that the matching Vulkan
// bootstrap release is still pending for this exact slot. Keep that host-only
// synchronization identity beside the exported descriptors instead of making
// the OpenCL dispatcher depend on the Vulkan transport owner.
struct WorldOpenClImportSlot {
    SculptureOpenClImportSlot handles{};
    std::shared_ptr<const WorldBootstrapHandoff> bootstrap;
    std::uint32_t slotIndex = 0xFFFFFFFFU;
};

} // namespace epi_sculpt
