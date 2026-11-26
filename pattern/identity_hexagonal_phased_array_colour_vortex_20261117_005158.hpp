#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Advanced 3.5D): a Hexagonal Phased Array of Ring Sources and the
 * Colour Vortex at its Centre.
 *
 * Mathematics:
 * - T(z) = e^(i pi / 3) (z - c) + c with c = 0.3: a sixth turn about c,
 *   T^6 = id. The WaveRing seed is centred on 0, not on c, so the six
 *   generations see six copies of the ring wave, centred on the six points
 *   c + e^(-i pi n / 3) (0 - c): a regular hexagon of point sources of
 *   radius 0.3 round c, all emitting in step.
 * - Opponent colour is linear in RGB. With a hue step h = 1/6 and weights
 *   near 1, the chroma is C(z) = sum_j e^(2 pi i j / 6) S(T^j z): source j
 *   is given the phase j/6 of a turn. A ring of emitters with a phase that
 *   advances by one full turn round the ring is how vortex beams with
 *   orbital angular momentum are made from antenna and laser arrays.
 * - Expanding S about c in d = z - c and conj(d), the term d^a conj(d)^b
 *   survives only if a - b = -1 mod 6; the first survivor is conj(d), so the
 *   colour winds once round c: a phase singularity of charge -1, with no
 *   colour exactly at c.
 *
 * Pipeline: Identity with zn0 = e^(i pi / 3), zn1 = -e^(i pi / 3) c,
 * zn2 = 1, zn3 = c; Clamp (points whose turn leaves the square are clamped
 * onto the border); WaveRing at alpha 0.12; fade 0.99; Opponent with step
 * 1/6. The light (unrotated) shows the six-source superposition, the
 * colour shows the vortex. The hue lane stays within 0.005 of the
 * resonance.
 *
 * Fidelity:   identity
 * Cell:       Identity / Clamp / WaveRing / Opponent
 * Nearest:    invert_mobius_colour_vortex_charge_ladder is the spherical
 *             version (two poles, charge 1 to 2); spatial_wave_laguerre_
 *             gauss_vortex paints a vortex directly. Here it is produced by
 *             the sum over generations.
 * Height:     age = passes; the six generations are the six emitters.
 * Hue clock:  bounded; step 1/6 +- 0.005.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. Exact for equal
 *             weights; with rho = fade (1 - seed alpha) < 1 the core is
 *             displaced by a distance of order 1 - rho, its charge
 *             unchanged. Not built, rendered or measured.
 * @assert p_N >= 0.5
 */
inline Pattern identity_hexagonal_phased_array_colour_vortex() {
  Pattern p;
  p.name = "identity_hexagonal_phased_array_colour_vortex";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.5f, 0.86602540f};       // e^(i pi / 3)
  p.zn.zn[1] = {-0.15f, -0.25980762f};    // -e^(i pi / 3) c
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.3f, 0.0f};              // c: the centre of the array

  p.color.hue_rotation = 0.16666667f; // one phase turn round the six sources
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.99f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.03f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.12f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("vortex_tuning",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.005f,
                            .offset = 0.16666667f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.015f,
                            .offset = 0.03f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
