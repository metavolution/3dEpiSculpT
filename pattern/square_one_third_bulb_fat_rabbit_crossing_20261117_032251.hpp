#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Square (Advanced 3.5D): through the Root of the 1/3 Bulb; a Quasicircle
 * becomes the Fat Rabbit, then the Rabbit.
 *
 * Mathematics:
 * - T(z) = 2 z^2 + c/2 (w -> w^2 + c, w = 2z), with c on the straight segment
 *   c(u) = c_r + u (c_c - c_r) from the root of the 1/3 bulb,
 *   c_r = e^(2 pi i/3)/2 - e^(4 pi i/3)/4 = -0.125 + 0.6495191i, toward the
 *   rabbit centre c_c = -0.1225612 + 0.7448618i (root of
 *   c^3 + 2c^2 + c + 1). A Sine lane moves u over [-0.6, 1.0].
 * - u < 0: inside the main cardioid (fixed-point multiplier modulus 0.93 at
 *   u = -0.6, 0.99 at -0.1): an attracting fixed point, J a quasicircle
 *   that grows three-lobed as the multiplier nears e^(2 pi i/3).
 * - u = 0 (the rest state): the multiplier is exactly e^(2 pi i/3), a
 *   parabolic fixed point with three petals, three basins meeting at it:
 *   the "fat rabbit".
 * - u > 0: inside the bulb, an attracting 3-cycle (checked at u = 0.02 ...
 *   1.2); at u = 1 it is superattracting: Douady's rabbit, three ears
 *   joined at the alpha fixed point, which turns them by a third.
 * - The three lobes of the quasicircle become the three ears; the
 *   topology changes only at u = 0, where the fixed point's basin splits
 *   into the 3-cycle's basins.
 * - Exactness: |c| <= 0.765, K lies in |z| <= 0.753, inside the Simplex
 *   band; border points escape and stay clamped.
 *
 * Pipeline: Square, zn0 = 2, zn2 = 1/2, zn3 = c(u)/2 on two Sine lanes with
 * the same phase (0.0015 Hz; amplitudes in the ratio of c_c - c_r); Clamp;
 * Simplex border; fade 0.95; OKLab step 1/3.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Square / Clamp / Simplex / OKLabColorRotation
 * Nearest:    square_period_three_triad_circle (passes the rabbit on a
 *             circle); square_parabolic_implosion_cauliflower_gate (the 1/1
 *             root); here the 1/3 root, crossed into its bulb.
 * Height:     age = passes before exit.
 * Hue clock:  bounded; step 1/3 +- 0.006.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; root multiplier
 *             (e^(2 pi i/3) to 1e-10) and cycle periods along the segment
 *             evaluated arithmetically. Near u = 0 convergence is parabolic
 *             (slow), so the basins fill in over more passes than the fade
 *             keeps. Not built, rendered or measured.
 * @assert sing[0].escapes == 0
 */
inline Pattern square_one_third_bulb_fat_rabbit_crossing() {
  constexpr float BULB_HZ = 0.0015f;
  constexpr float PHASE = 0.95978469f; // u = 0 at t = 0
  Pattern p;
  p.name = "square_one_third_bulb_fat_rabbit_crossing";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.5f, 0.0f};
  p.zn.zn[3] = {-0.0625f, 0.32475953f}; // c_r / 2: the fat rabbit

  p.color.hue_rotation = 0.33333333f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.4f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("bulb_crossing_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = BULB_HZ,
                            .amplitude = 0.00097553f,
                            .offset = -0.06225612f,
                            .phase = PHASE},
           control::ModTarget::Zn3Re)
      .add("bulb_crossing_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = BULB_HZ,
                            .amplitude = 0.03813709f,
                            .offset = 0.33429380f,
                            .phase = PHASE},
           control::ModTarget::Zn3Im)
      .add("ear_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.006f,
                            .offset = 0.33333333f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.025f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
