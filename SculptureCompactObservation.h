// Host observation policy for compact GPU draws. The fixed GPU analytics
// sidecar is sampled and cannot supply the full-population measurements here.
#pragma once

#include "SculptureProgramSchema.h"

namespace epi_sculpt {

inline void publishMappedCompactObservation(
    std::size_t admittedCount, SculptureBoundingVolume& bounds,
    SculptureMotionKinematics& kinematics) noexcept {
    bounds = {};
    kinematics = {};
    kinematics.activeCount = admittedCount;
}

} // namespace epi_sculpt
