#pragma once

#include <array>

namespace epi_sculpt {

// Explicit startup view coordinates; the application uses a Y-up camera.
struct SculptureCameraPose {
    std::array<float, 3> eye;
    std::array<float, 3> target;
};

} // namespace epi_sculpt
