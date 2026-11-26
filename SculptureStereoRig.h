#pragma once

namespace epi_sculpt {

// Plain presentation configuration shared by application intent and CPU optics.
// It owns no camera, renderer resources or clock. Commands edit the live value;
// frame plans retain a copy so an admitted frame cannot observe later edits.
struct SculptureStereoRig {
    float interpupillaryDistance = 0.064f;  // Standard 64mm IPD
    float focalConvergenceDistance = 2.0f;  // Convergence distance in metres
    bool toeInConvergence = true;  // Converged versus parallel eye directions
};

} // namespace epi_sculpt
