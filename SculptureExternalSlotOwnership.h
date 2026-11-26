// Sculpture vocabulary for the application-neutral external slot contract.
#pragma once

#include "FeedbackExternalSlotOwnership.h"

namespace epi_sculpt {
using SculptureExternalSlotState = FeedbackExternalSlotState;
using SculptureExternalSlotTicket = FeedbackExternalSlotTicket;
using SculptureExternalSlotSnapshot = FeedbackExternalSlotSnapshot;
using SculptureExternalSlotHealth = FeedbackExternalSlotHealth;

template <std::size_t SlotCount = 3U>
using SculptureExternalSlotOwnership = FeedbackExternalSlotOwnership<SlotCount>;
} // namespace epi_sculpt
