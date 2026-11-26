#pragma once

#include <epi/pipeline/feedback_delivery.hpp>

namespace epi_sculpt {

// Compatibility vocabulary for the sculpture runtime. The canonical contract
// now lives in shared epi2 so 2D, 3D, capture, and future presenters describe
// the same mathematical field without depending on Vulkan/OpenGL types.
using EpiFeedbackFieldFormat = epi::FeedbackFieldFormat;
using EpiFeedbackFieldTransport = epi::FeedbackFieldTransport;
using EpiFeedbackFieldExternalLease = epi::FeedbackFieldExternalLease;
using EpiFeedbackFieldDescriptor = epi::FeedbackFieldDescriptor;
using EpiFeedbackFieldDelivery = epi::FeedbackFieldDelivery;
using EpiFeedbackFieldDeliveryRing = epi::FeedbackFieldDeliveryRing;

[[nodiscard]] inline bool publishCanonicalFeedbackDelivery(
    EpiFeedbackFieldDeliveryRing &ring,
    std::optional<EpiFeedbackFieldDelivery> delivery,
    EpiFeedbackFieldTransport expectedTransport) {
  return epi::publishFeedbackDelivery(
      ring, std::move(delivery), expectedTransport);
}

} // namespace epi_sculpt
