#pragma once

#include <epi/presentation/vulkan_feedback_external_transport.hpp>

namespace epi_sculpt {
// Compatibility name for the flagship. Native transport ownership is shared
// with the flat Vulkan feedback frontend; sculpture code adds scene usage, not
// a second OpenCL/Vulkan handoff implementation.
using EpiFeedbackFieldExternalTransport =
    epi::presentation::VulkanFeedbackExternalTransport;
} // namespace epi_sculpt
