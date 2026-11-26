#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern square_logistic_map_cascade() {
  Pattern p;
  p.name = "square_logistic_map_cascade";

  // 1. Inner function: Square (z^2)
  // Represents the quadratic nonlinearity of the logistic map: x_{n+1} = r*x_n*(1 - x_n).
  // Under the conjugation x = -z/r + 1/2, this becomes z_{n+1} = z_n^2 + c,
  // where c = (r/2)*(1 - r/2).
  p.inner = InnerFunction::Square;

  // 2. Pre-affine: slight supercritical expansion + chiral tilt exposes Julia filaments
  p.zn.zn[0] = {1.06f, 0.04f};
  p.zn.zn[1] = {0.008f, -0.006f};

  // 3. Post-affine: Add the 'c' constant for the Feigenbaum accumulation point.
  // The Feigenbaum point (onset of chaos) corresponds to r = 3.5699456...
  // c = (3.5699456 / 2) * (1 - 3.5699456 / 2) = -1.401155.
  // Subtle ZN parameter modulation sweeps gently across period-doubling cascades.
  p.zn.zn[2] = {0.96f, 0.02f};
  p.zn.zn[3] = {-1.401155f, 0.0f}; // Focal point offset (c)
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.15f;
  p.zn.zn0_anim_amp = 0.05f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.11f;
  p.zn.zn1_anim_amp = 0.014f;
  p.zn.zn1_anim_k = 2.0f;

  // 4. Reduction
  // Grid reduction folds coordinates back, creating bounds and crystalline tile patterns
  // which works well for exposing the periodic structure.
  p.reduction = ReductionType::Grid;

  // 5. Color mode
  // Mode 3 (Alive) to reveal the phase portrait of the strange attractor
  p.color.color_mode = presets::as_int(presets::ColorMode::Alive);
  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.978f; // Balanced fade preserves crisp filament contrast without washout

  // Mode 3 parameters calibrated for log_r = log(0.01 * |t_z|) ≈ -4.3:
  p.color.spiral_angle_coeff = 0.55f;
  p.color.spiral_radius_coeff = 0.32f;
  p.color.hue_velocity_base = 0.014f;
  p.color.hue_velocity_breath = 0.009f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.045f;
  p.color.luma_shape_center = -4.3f;

  // 6. Seed (energy injection)
  p.seed.border_width = 0.045f;
  p.seed.hue_speed = 0.09f;
  p.seed.alpha = 0.94f;

  // 7. Speed
  p.speed = 0.72f;

  return p;
}

} // namespace epi::presets
