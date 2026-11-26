#pragma once

#include "base.hpp"
#include <cmath>

namespace epi::presets {

inline Pattern exp_ulam_prime_spiral() {
  Pattern p;
  p.name = "exp_ulam_prime_spiral";

  // 1. Inner function: Exp
  // The exponential function maps lines to circles and rays, driving the spiral nature
  // of the Ulam spiral construction.
  p.inner = InnerFunction::Exp;

  // 2. Pre-affine: Prime 7-fold symmetry rotation
  // Angle = 2*pi / 7 ~ 0.8975979 rad
  const float th1 = 2.0f * static_cast<float>(M_PI) / 7.0f;
  const float r1 = 1.22f;
  p.zn.zn[0] = {r1 * std::cos(th1), r1 * std::sin(th1)};

  // 3. Post-affine: Prime 11-fold symmetry rotation
  // Angle = 2*pi / 11 ~ 0.5711986 rad
  const float th2 = 2.0f * static_cast<float>(M_PI) / 11.0f;
  const float r2 = 0.98f;
  p.zn.zn[2] = {r2 * std::cos(th2), -r2 * std::sin(th2)};

  // 4. Reduction: Grid
  // Grid tiling creates the square lattice boundaries, mirroring the discrete square 
  // grid layout upon which the Ulam integer spiral is written.
  p.reduction = ReductionType::Grid;

  // 5. Color mode: Spatial Wave (4)
  // By using mode 4 with prime-numbered spatial frequencies, we create diagonal
  // interference lines that represent the unexpected diagonal prime alignments
  // found in the Ulam spiral.
  p.color.color_mode = presets::as_int(presets::ColorMode::SpatialWave);
  p.color.hue_rotation = 0.02f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.985f; // High persistence to let the lattice interference build

  // Mode 4 mappings (dual wave interference):
  p.color.spiral_angle_coeff = 1.0f;       // hue_x_slope
  p.color.spiral_radius_coeff = 7.0f;      // wave1_spatial_freq = Prime 7
  p.color.chroma_breath_amp = 0.3f;        // wave1_amplitude
  p.color.chroma_breath_freq = 11.0f;      // wave2_spatial_freq = Prime 11
  p.color.chroma_breath_phase = 0.25f;     // wave2_amplitude
  p.color.hue_velocity_base = 0.015f;      // wave1_temporal_speed
  p.color.hue_velocity_breath = 0.011f;    // wave2_temporal_speed
  p.color.hue_velocity_drift = 0.005f;     // base_rotation_speed
  p.color.luma_shape_depth = 0.2f;         // sat_breath_amp
  p.color.luma_shape_center = 13.0f;       // sat_breath_freq = Prime 13

  // 6. Seed (energy injection)
  // Moderate border for steady energy injection into the prime lattice.
  p.seed.border_width = 0.081f;
  p.seed.hue_speed = 0.15f;
  p.seed.alpha = 0.92f;

  // 7. Speed
  p.speed = 0.80f;
  p.zn.zn[1] = {0.0451f, -0.0303f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.159f;
  p.zn.zn0_anim_amp = 0.046f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.172f;
  p.zn.zn1_anim_amp = 0.042f;
  p.zn.zn1_anim_k = 5.0f;
  p.seed.pulse_freq = 0.127f;
  p.seed.pulse_amp = 0.14f;


  return p;
}

} // namespace epi::presets
