#pragma once

#include "base.hpp"

namespace epi::presets {

// [2B] Quadratic golden Siegel map w^2+c, c=e^{2pi i/phi^2}/2-e^{4pi i/phi^2}/4: fixed-point multiplier exactly unimodular (golden rotation) -> smooth chiral rotation disk inside Torus-fractured cells.
/**
 * 4x3 Orthogonal Spatial Wave Matrix -- cell 2B.
 *
 * Row 2 (Chiral / Phase-Vortex) x Column B (Critical Bifurcation).
 * - View-conjugated quadratic map: w = s z with s = 3/2, T(z) = (w^2 + c)/s
 *   (Square, zn0 = s, zn2 = 1/s, zn3 = c/s).
 * - c on the main-cardioid boundary c = lambda/2 - lambda^2/4 gives a fixed
 *   point with multiplier lambda; lambda = e^{2 pi i theta}, theta = 1/phi^2
 *   (golden, Brjuno) => c = -0.390541 + 0.586788i: |lambda| = 1 exactly,
 *   the threshold between an attracting sink (smooth basin, |lambda| < 1)
 *   and a repelling source (Cantor-like cells, |lambda| > 1).
 * - At |lambda| = 1 with golden rotation the fixed point sits in a Siegel
 *   rotation disk: orbits circulate on invariant curves (a chiral phase
 *   vortex, no mirror symmetry since Im c != 0), bounded by a fractal Julia
 *   set; escaping orbits are Torus-folded into fractured bright cells.
 * - Lanes sweep the multiplier modulus |lambda| = 1 +- 0.03 at f_4 via
 *   dc = (1 - lambda)/2 * d(lambda): the disk alternately implodes
 *   (parabolic/repelling) and re-forms (attracting) -- critical breathing.
 * - fade = e^{-1/24}: tau = 24 steps. Opponent YCoCg (mode 6) accumulates a
 *   chroma rotation of spiral_a = 1/tau turns per step: net charge-1 vortex.
 *
 * Temporal ladder: f_4 = 0.06 * 2^(4/12) = 0.07560 Hz (12-TET; no two
 * cells share a frequency). Seed hue = 4/12 on the matrix hue wheel.
 */
inline Pattern spatial_wave_chiral_siegel_critical_vortex() {
  Pattern p;
  p.name = "spatial_wave_chiral_siegel_critical_vortex";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.5f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.6666667f, 0.0f};
  p.zn.zn[3] = {-0.2603606f, 0.3911919f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.9591895f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0416667f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.12f;
  p.color.chroma_breath_freq = 0.0755953f;
  p.color.chroma_breath_phase = 12.5663706f;
  p.color.hue_velocity_base = 0.0f; // O9 bounded hue clock
  p.color.hue_velocity_breath = 0.0018999f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.1f;
  p.seed.hue = 0.3333333f;
  p.seed.hue_speed = 0.012f;
  p.seed.alpha = 0.9f;
  p.seed.pulse_freq = 0.0755953f;
  p.seed.pulse_amp = 0.0f; // envelope == 1: normalized, non-strobing injection

  p.speed = 0.5f;

  p.motion
      .add("siegel_modulus_sweep_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0755953f,
                            .amplitude = 0.0082479f,
                            .offset = -0.2603606f,
                            .phase = 0.5f},
           control::ModTarget::Zn3Re)
      .add("siegel_modulus_sweep_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0755953f,
                            .amplitude = 0.0167166f,
                            .offset = 0.3911919f,
                            .phase = 0.0f},
           control::ModTarget::Zn3Im);

  return p;
}

} // namespace epi::presets
