#pragma once

#include "SculptureProgramSchema.h"
#include "mgfx/Camera.h"

namespace epi_sculpt {

// Copy the pose passed to mono rendering into the existing capture format.
// State has no projection fields: FOV, clip planes and temporal history are
// intentionally not represented by this pose-only contract.
[[nodiscard]] inline SculptureWorldState captureCameraPose(
    SculptureWorldState state, const mgfx::Camera& renderedCamera) noexcept {
    for (int axis = 0; axis < 3; ++axis) {
        state.cameraEye[axis] = renderedCamera.eye[axis];
        state.cameraTarget[axis] = renderedCamera.target[axis];
        state.cameraUp[axis] = renderedCamera.up[axis];
    }
    state.hasCameraOverride = true;
    return state;
}

} // namespace epi_sculpt
