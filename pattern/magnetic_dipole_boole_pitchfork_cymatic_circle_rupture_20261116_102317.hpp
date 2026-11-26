#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Magnetic Dipole (Advanced 3.5D): Boole's Map, a Pitchfork on the Circle
 * and the Rupture of a Julia Set.
 *
 * Mathematics:
 * - The even Blaschke products B_a(u) = (u^2 + a) / (a u^2 + 1) all map
 *   the unit circle to itself. Their fixed points are 1 and the roots of
 *   a u^2 + (a - 1) u + a, and B_a'(1) = 2 (1 - a) / (1 + a).
 * - a < 1/3: two sinks p < 1 < 1/p on the real axis, one for the disc and
 *   one for the outside, and 1 repels. The Julia set is the whole circle.
 * - a = 1/3: the three fixed points coincide. B = (3 u^2 + 1) / (u^2 + 3)
 *   has a triple fixed point at 1 with multiplier 1 and two attracting
 *   petals, the disc and the outside. Under the Cayley map
 *   h(u) = i (1 + u) / (1 - u) it is exactly Boole's transformation
 *   x -> x - 1/x of the real line (checked numerically), the standard
 *   example of a map that preserves an infinite measure.
 * - a > 1/3: 1 attracts with multiplier below 1, and the other two fixed
 *   points have moved onto the circle as a repelling pair at angles
 *   +-arccos((1 - a) / 2a). Disc and outside are now one basin, joined
 *   through an open arc around 1, and the Julia set is a Cantor subset of
 *   the circle. A pitchfork of sinks and a change of topology happen at
 *   the same parameter.
 *
 * Pipeline: MagneticDipole with zn0 = 2.1 sqrt a, zn2 = A / 2.1 and
 * zn3 = D / 2.1, A = (v - 1/v) / 2, D = (v + 1/v) / 2, v = 1 / a. Three
 * Sine lanes in phase move v = 3 + 0.2 sin: a from 0.3125 to 0.357. The
 * lanes are the tangent line to the Blaschke family at Boole's map, so
 * they are exact there and off by a second-order term elsewhere: at the
 * ends the circle is mapped to within about 0.5 % of itself and the
 * sinks sit within about 0.07 of those of the exact family (the single
 * sink is near 0.94, just inside the circle, rather than on it). The
 * circle has radius 0.476 in the frame. ChladniCymatics is the seed: a
 * vibrating plate covers disc and outside, and generation k is that plate
 * seen through a 2^k-fold conformal cover that is fine near the circle
 * and flat near the sinks. Near a = 1/3 convergence is algebraic rather
 * than geometric (about 150 passes instead of 30 in the check), so many
 * more generations stay distinct: critical slowing shows as a picture
 * that fills with plates and dims, then clears on the far side with a
 * single sink on the rim.
 *
 * Fidelity:   identity at rest; first order in the lanes (see above)
 * Cell:       MagneticDipole / Clamp / ChladniCymatics / SpatialWave
 * Nearest:    magnetic_dipole_minimal_golden_blaschke_circle_net is one
 *             member of this family, a = 0.309, held still and drawn as a
 *             net; spatial_wave_curie_weiss_tanh_pitchfork has a
 *             pitchfork without a Julia set. cube_chladni_rose_window_
 *             gear_train pulls the same seed through z^3, concentric.
 * Height:     age = number of passes; deep near the sinks.
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-16. The rest map was
 *             checked against (3 u^2 + 1) / (u^2 + 3) and against Boole's
 *             map; lane states at v = 2.7, 2.85, 3.15, 3.3 were iterated
 *             for their sinks and for |f| on the circle, and the figures
 *             quoted for the +-0.2 sweep are those errors scaled
 *             quadratically. Not built, rendered or measured.
 * @assert sing[0].period == 1 && p_C >= 0.5
 */
inline Pattern magnetic_dipole_boole_pitchfork_cymatic_circle_rupture() {
  constexpr float PRE = 1.2124356f;   // 2.1 / sqrt 3
  constexpr float POST = 0.6349206f;  // (4/3) / 2.1
  constexpr float SHIFT = 0.7936508f; // (5/3) / 2.1
  constexpr float SWEEP_HZ = 0.005f;
  Pattern p;
  p.name = "magnetic_dipole_boole_pitchfork_cymatic_circle_rupture";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {PRE, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {POST, 0.0f};
  p.zn.zn[3] = {SHIFT, 0.0f};

  p.color.hue_rotation = 0.05f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.96f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.25f;
  p.color.spiral_radius_coeff = 1.3f;
  p.color.chroma_breath_amp = 0.06f;
  p.color.chroma_breath_freq = 0.09f;
  p.color.chroma_breath_phase = 0.05f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.04f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.08f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.55f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("blaschke_tangent_pre", // 0.2 * 2.1 * d(v^-1/2)/dv at v = 3
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SWEEP_HZ,
                            .amplitude = -0.0404145f,
                            .offset = PRE},
           control::ModTarget::Zn0Re)
      .add("blaschke_tangent_post", // 0.2 * (5/9) / 2.1
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SWEEP_HZ,
                            .amplitude = 0.0529101f,
                            .offset = POST},
           control::ModTarget::Zn2Re)
      .add("blaschke_tangent_shift", // 0.2 * (4/9) / 2.1
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SWEEP_HZ,
                            .amplitude = 0.0423280f,
                            .offset = SHIFT},
           control::ModTarget::Zn3Re)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.03f,
                            .offset = 0.06f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
