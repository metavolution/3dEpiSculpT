#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Advanced 3.5D): a Golden Turn of the Sphere; the Hue Sweep Gives
 * a Vortex Pair Every Integer Charge.
 *
 * Mathematics:
 * - T is the elliptic Mobius map with fixed points +-p, p = 0.5i, and
 *   multiplier L = e^(2 pi i g) at p, g = 0.38196601: a turn of the Riemann
 *   sphere by the golden angle about the axis through +-p. Slots from
 *   zn0 = 1 - L, zn1 = p (1 + L), zn2 = -4 L p^2 / (1 - L),
 *   zn3 = p (1 + L)/(1 - L) (checked: T(+-p) = +-p to 2e-16).
 * - Opponent with step h: near p the seed's Taylor term d^a conj(d)^b picks
 *   up e^(2 pi i n (h + g (a - b))) per pass, so the resonant terms are those
 *   with a - b = k where h = -k g mod 1. On that line the chroma winds k
 *   times round p: a vortex of charge k, and -k at -p (where the turn is
 *   reversed). The WaveRing seed is centred at 0, not at a pole, so all
 *   integer k occur (rosette_spectrum's plate allowed only k in 4Z).
 * - The lines in the sweep [0.1, 0.9]: k = 3 at 0.854, -2 at 0.764,
 *   6 at 0.708, -7 at 0.674, 1 at 0.618, -4 at 0.528, 4 at 0.472,
 *   -1 at 0.382, 7 at 0.326, -6 at 0.292, 2 at 0.236, -3 at 0.146.
 *   The pair's charges jump in a three-gap order, never repeating, with
 *   darkness between lines (width (1 - rho)/2 pi = 0.0024).
 * - Exactness: a point on the Apollonian circle |zeta| = R about p stays on
 *   it; for |zeta| < 1/3 (a disc of radius about 0.37 round p, and its
 *   mirror about -p) these circles lie inside the frame, so near the
 *   vortex cores nothing is clamped and the analysis is exact.
 *
 * Pipeline: Invert with the slots below; Clamp; WaveRing at alpha 0.08;
 * fade 0.985; Opponent with h on a Sine lane (offset 1/2, amplitude 0.4,
 * 0.0005 Hz, 2000 s), starting dark between the k = +-4 lines.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Invert / Clamp / WaveRing / Opponent
 * Nearest:    invert_mobius_colour_vortex_charge_ladder (order 6: charges 1
 *             and 2 only); identity_golden_rotation_rosette_charge_spectrum
 *             (one centre, k in 4Z).
 * Height:     age = passes; generation n is the seed turned n g about the
 *             axis.
 * Hue clock:  bounded; step swept over [0.1, 0.9].
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; slots, lines and the
 *             exact discs evaluated arithmetically. Charge k shows only where
 *             harmonic k dominates; high |k| are weak near the cores
 *             (amplitude ~ r^|k|). Outside the exact discs, points the
 *             turn throws past the square are clamped. Generation n's colour
 *             turns n times as fast as h (0.13 turn/s at n = 100). Not built,
 *             rendered or measured.
 * @assert p_E <= 0.05
 */
inline Pattern invert_golden_sphere_turn_vortex_pair_spectrum() {
  Pattern p;
  p.name = "invert_golden_sphere_turn_vortex_pair_spectrum";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.73736887f, -0.6754903f};  // 1 - L
  p.zn.zn[1] = {-0.33774515f, 0.13131556f}; // p (1 + L)
  p.zn.zn[2] = {-0.5f, 0.19440037f};
  p.zn.zn[3] = {-0.19440037f, 0.0f};

  p.color.hue_rotation = 0.5f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.985f;
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
  p.seed.alpha = 0.08f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("charge_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0005f,
                            .amplitude = 0.4f,
                            .offset = 0.5f},
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
