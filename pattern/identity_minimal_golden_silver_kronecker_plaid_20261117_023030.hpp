#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Minimal 3.5D): a Golden-Silver Kronecker Plaid.
 *
 * Mathematics:
 * - T(z) = z + tau on the Grid torus with tau = 2 (g, s), g = (3 - sqrt 5)/2
 *   (golden) and s = sqrt 2 - 1 (silver). 1, g and s are linearly
 *   independent over the rationals, so the points n tau mod 2 fill the
 *   torus evenly (Kronecker-Weyl): an irrational line wound round a torus.
 * - The Cross seed on the torus is the line pair x, y in 2Z. Generation n
 *   moves it to x = -2 n g, y = -2 n s (mod 2): one vertical line of the
 *   seed's warm tint and one horizontal line of the complementary tint
 *   (the Cross seed adds 1/2 to the hue of its y-line) per generation.
 * - Three-gap theorem (Steinhaus): any first N positions n g mod 1 cut the
 *   circle into gaps of at most three lengths, and each new line splits
 *   one of the largest gaps in the golden ratio. So the verticals space
 *   themselves golden-evenly, the horizontals silver-evenly, never
 *   repeating, with brightness falling by the fade per generation: a
 *   tartan whose sett is two continued fractions, [0; 2, 1, 1, 1, ...] and
 *   [0; 2, 2, 2, ...].
 *
 * Pipeline: Identity with zn3 = tau; Grid; Cross at alpha 0.7; fade 0.9
 * (about ten line pairs in view); OKLab step 1/10, so the newest lines and
 * the tenth share a hue.
 *
 * Fidelity:   identity
 * Cell:       Identity / Grid / Cross / OKLabColorRotation
 * Nearest:    identity_minimal_golden_angle_chladni_three_gap_stack (three
 *             gaps in angle); here in two translations at once.
 * Height:     age = passes; generation n is the n-th line pair.
 * Hue clock:  bounded; step 1/10 +- 0.005.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. The geometry is static
 *             by design (a lane on tau would move generation n n times as
 *             fast). Not built, rendered or measured.
 * @assert p_N >= 0.95
 */
inline Pattern identity_minimal_golden_silver_kronecker_plaid() {
  Pattern p;
  p.name = "identity_minimal_golden_silver_kronecker_plaid";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.76393202f, 0.82842712f}; // 2 (g, sqrt 2 - 1)

  p.color.hue_rotation = 0.1f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.9f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.04f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.07f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.7f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("sett_tint",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.005f,
                            .offset = 0.1f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.02f,
                            .offset = 0.04f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
