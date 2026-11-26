#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Square (Advanced 3.5D): Seahorse Valley, where pi Hides in the Mandelbrot
 * Set.
 *
 * Mathematics:
 * - T(z) = 2 z^2 + c/2 is w -> w^2 + c in w = 2z, with c = -3/4 + i y and a
 *   Sine lane on y over [-0.08, 0.08]. c = -3/4 is the neck between the main
 *   cardioid and the period-2 disc, a parabolic point with multiplier -1.
 *   For y != 0 the line Re c = -3/4 runs outside both (the cardioid
 *   boundary there is at Re = -3/4 + 3y^2/4, the disc's at about
 *   -3/4 - 2y^2): seahorse valley.
 * - Boll (1991): the critical orbit of c = -3/4 + i eps takes about pi/eps
 *   passes to escape. Checked: 33, 63, 158, 315, 629 passes at eps = 0.1,
 *   0.05, 0.02, 0.01, 0.005, so n eps = 3.30, 3.15, 3.16, 3.15, 3.145.
 * - In the escape-time picture the orbits that pass near the ghost
 *   2-cycle swap sides each pass and creep round it, so the exit-time bands
 *   wind into double spirals, the "seahorse tails". The number of turns
 *   grows like 1/|y|; the sign of y sets the sense of winding, so the
 *   spirals unwind through y = 0 (the parabolic "fat basilica", the rest
 *   state) and rewind the other way.
 * - Exactness: |c| <= 0.755, so K lies in |w| <= 1.5, |z| <= 0.75, inside
 *   the Simplex band; border points escape and stay clamped.
 *
 * Pipeline: Square, zn0 = 2, zn1 = 0, zn2 = 1/2, zn3 = c/2 with Im on the lane
 * (amplitude 0.04, 0.0015 Hz); Clamp; Simplex border; fade 0.97 (about 33
 * passes, so the spirals at |y| = 0.08, transit 39, are resolved); OKLab
 * step 1/20.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Square / Clamp / Simplex / OKLabColorRotation
 * Nearest:    square_parabolic_implosion_cauliflower_gate (the cusp, where
 *             the law is pi / sqrt(eps)); square_basilica_bulb_rim_resonance_
 *             walk (round the period-2 disc, not across its neck).
 * Height:     age = passes before exit; in the valley, the transit time.
 * Hue clock:  bounded; step 1/20 +- 0.003.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; Boll's law checked
 *             arithmetically. The line brushes tiny bulbs of rotation number
 *             near 1/2, where the Julia set is briefly connected. The Sine
 *             lane crosses y = 0 at its fastest, where the transit diverges;
 *             check that crossing against the photosensitivity gate once
 *             rendered. Not built, rendered or measured.
 * @assert sing[0].escapes == 0
 */
inline Pattern square_seahorse_valley_bolls_pi() {
  Pattern p;
  p.name = "square_seahorse_valley_bolls_pi";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.5f, 0.0f};
  p.zn.zn[3] = {-0.375f, 0.0f}; // c/2 at c = -3/4

  p.color.hue_rotation = 0.05f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.97f;
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
  p.seed.hue = 0.5f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("valley_crossing",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0015f,
                            .amplitude = 0.04f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Im)
      .add("transit_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.003f,
                            .offset = 0.05f},
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
