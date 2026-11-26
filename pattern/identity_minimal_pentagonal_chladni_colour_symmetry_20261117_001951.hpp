#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Minimal 3.5D): a Chladni Plate under a Fivefold Turn, with
 * Colour Symmetry.
 *
 * Mathematics:
 * - T(z) = e^(2 pi i / 5) z: a rotation of order 5, T^5 = id. It maps the
 *   unit disc onto itself, so 78.5 % of the frame (the inscribed disc) is
 *   never clamped and every point there has period 5.
 * - The ChladniCymatics seed is |cos(n X) cos(m Y) - cos(m X) cos(n Y)|
 *   near zero, X = pi x, Y = pi y, masked to the disc. A quarter turn
 *   (x, y) -> (-y, x) changes the sign inside the bars and leaves the
 *   modulus alone, so the plate is exactly fourfold symmetric. Turned by
 *   n 72 degrees it therefore shows only n 72 mod 90: 72, 54, 36, 18, 0
 *   for n = 1 .. 5. Five orientations, 18 degrees apart, together a
 *   twentyfold set.
 * - Generation n is the plate turned by -72 n degrees and carries the hue
 *   offset n / 5. So each of the five orientations has its own hue, and
 *   turning the picture by 72 degrees moves every plate onto the next
 *   orientation and every hue on by a fifth: colour symmetry in the sense
 *   of Escher's and Coxeter's coloured patterns (exact up to the
 *   generation weights, which fall off by fade (1 - alpha) per pass).
 *
 * Pipeline: Identity with zn0 = e^(2 pi i / 5); Clamp (the corners outside
 * the disc are clamped onto the border, where the plate is masked to zero,
 * so they stay dark); Chladni seed; OKLab hue step 1/5. The plate's own
 * modes drift (n = 3 + sin 0.2 t, m = 5 + cos 0.25 t), and all five
 * orientations morph together.
 *
 * Fidelity:   identity
 * Cell:       Identity / Clamp / ChladniCymatics / OKLabColorRotation
 * Nearest:    identity_minimal_octagonal_chladni_opponent_moire turns the
 *             same plate by 45 degrees and uses opponent colour instead;
 *             penrose_p3_quasicrystal_inflation reaches fivefold order
 *             through cos on Torus.
 * Height:     age = passes; generation index mod 5 is the hue.
 * Hue clock:  bounded; step 1/5 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17 (the symmetry of the
 *             seed and the orientation count are proved above). Not built,
 *             rendered or measured.
 * @assert p_N >= 0.75
 */
inline Pattern identity_minimal_pentagonal_chladni_colour_symmetry() {
  Pattern p;
  p.name = "identity_minimal_pentagonal_chladni_colour_symmetry";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.30901699f, 0.95105652f}; // e^(2 pi i / 5)
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.2f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.94f;
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

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.01f;
  p.seed.alpha = 0.6f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("fifth_hue_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.005f,
                            .amplitude = 0.01f,
                            .offset = 0.2f},
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
