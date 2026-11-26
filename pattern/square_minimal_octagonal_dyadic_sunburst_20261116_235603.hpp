#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Square (Minimal 3.5D): an Octagonal Dyadic Sunburst.
 *
 * Mathematics:
 * - T(z) = z^2, then the AmmannBeenker reduction: the box fold, and then
 *   every point outside the wedge |y| <= tan(pi/8) |x| is rotated by a
 *   multiple of pi/4 into the wedge |arg| <= pi/8.
 * - Inside the unit disc the box fold does nothing, and rotations by
 *   multiples of pi/4 pass through squaring as other such rotations, so
 *   T^n(z) = w_n z^(2^n) with w_n an eighth root of unity: |T^n z| is
 *   exactly |z|^(2^n), 0 is superattracting, and the argument doubles
 *   modulo pi/4 on every pass. The frame corners outside the disc (22 %)
 *   are expanded, folded and brought back.
 *
 * Pipeline: Square with bare slots and AmmannBeenker. The Cross seed lights
 * the horizontal axis, which lies inside the wedge, so a pixel is lit at
 * pass n when 2^n arg z is near a multiple of pi/4: 16 rays at the first
 * pass, 32 at the second, each generation finer and given its own hue by
 * the OKLab step of 1/8. Every orbit ends on the lit centre of the Cross,
 * so brightness falls off with the number of passes needed to get there:
 * a bright core and rings spaced by log log of the radius.
 *
 * Fidelity:   identity
 * Cell:       Square / AmmannBeenker / Cross / OKLabColorRotation
 * Nearest:    radial_cleavage_angle_doubling_rays doubles angles on Clamp
 *             (8 primary rays, no fold); cube_chladni_rose_window_gear_train
 *             pulls a plate through z^3. Here the octagonal fold doubles the
 *             ray count of each generation.
 * Height:     age = passes; log log depth to the core.
 * Hue clock:  bounded; step 1/8 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-16. On a 400 x 400 grid,
 *             100 % of points reached 0 within 300 passes, the corners
 *             included. A numpy model of the pass was drawn to check
 *             composition. Not built, rendered by the engine or measured.
 * @assert sing[0].period == 1 && p_C >= 0.99
 */
inline Pattern square_minimal_octagonal_dyadic_sunburst() {
  Pattern p;
  p.name = "square_minimal_octagonal_dyadic_sunburst";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::AmmannBeenker;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.125f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.52f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("ray_generation_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.005f,
                            .amplitude = 0.01f,
                            .offset = 0.125f},
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
