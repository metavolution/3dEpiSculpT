#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Square (Advanced 3.5D): a Walk round the Rim of the Basilica Bulb, through
 * Every Resonance.
 *
 * Mathematics:
 * - P_c(u) = u^2 + c has the 2-cycle {p, q} with p + q = -1 and p q = c + 1,
 *   whose multiplier is mu = 4 p q = 4 (c + 1). The set where it attracts,
 *   |c + 1| < 1/4, is an exact disc: the period-2 component of the
 *   Mandelbrot set, centred on the basilica c = -1.
 * - On its rim, c = -1 + e^(2 pi i theta) / 4, the 2-cycle is neutral with
 *   rotation number theta. At theta = p/q it is parabolic and each cycle
 *   point grows a flower of q petals (Leau-Fatou); there the period-2q
 *   satellite bulb is attached. theta = 0 is the root on the main cardioid
 *   (c = -3/4), theta = 1/2 the doubling into the period-4 bulb
 *   (c = -5/4). At the golden mean the cycle carries a pair of Siegel
 *   discs.
 * - The rim is a circle in c, so two lanes in quadrature move along it
 *   exactly, at a uniform rotation number theta = f t.
 *
 * Pipeline: Square with zn0 = 1, zn1 = 0, zn2 = s = 1.9 and zn3 = c / s, so
 * T(z) = s z^2 + c / s = P_c(s z) / s. Lanes: Zn3Re = (-1 + cos) / (4 s),
 * Zn3Im = sin / (4 s), at 0.004 Hz (one walk per 250 s, from the root).
 * The filled Julia set lies within |u| <= (1 + sqrt(1 + 4|c|)) / 2 = 1.72
 * for |c| <= 1.25, inside the frame half-width 1.9, so every escaping
 * orbit reaches the Simplex border and is coloured by its exit time (OKLab
 * step 1/10). The basilica's two lakes are dark; as theta passes p/q they
 * pinch at the cycle points into q-petal flowers, most visibly at
 * q = 2, 3, 4, 5.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Square / Clamp / Simplex / OKLabColorRotation
 * Nearest:    spatial_wave_cardioid_multiplier_tour moves along the main
 *             cardioid; square_golden_siegel_disc_breathing_sink breathes
 *             radially at one rotation number. Here the whole rim of the
 *             period-2 disc, all rotation numbers in turn.
 * Height:     age = passes before exit.
 * Hue clock:  bounded; step 1/10.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. At rest (base slots)
 *             c = -1, the basilica, with a superattracting 2-cycle. Not
 *             built, rendered or measured.
 * @assert sing[0].period == 2 && sing[0].rho_abs <= 0.01
 */
inline Pattern square_basilica_bulb_rim_resonance_walk() {
  constexpr float S = 1.9f;
  constexpr float RIM = 0.13157895f;   // (1/4) / s
  constexpr float CENTRE = -0.52631579f; // -1 / s
  constexpr float WALK_HZ = 0.004f;
  Pattern p;
  p.name = "square_basilica_bulb_rim_resonance_walk";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {S, 0.0f};
  p.zn.zn[3] = {CENTRE, 0.0f}; // rest: c = -1, the basilica

  p.color.hue_rotation = 0.1f;
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

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.08f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("rim_walk_re", // c = -1 + e^(2 pi i f t) / 4, exact
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = RIM,
                            .offset = CENTRE,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("rim_walk_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = RIM,
                            .offset = 0.0f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.025f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
