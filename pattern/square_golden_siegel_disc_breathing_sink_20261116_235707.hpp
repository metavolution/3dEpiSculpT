#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Square (Advanced 3.5D): the Golden Siegel Disc Breathes into a Spiral
 * Sink and Back.
 *
 * Mathematics:
 * - f(w) = lambda w + w^2: fixed point 0 with multiplier lambda, critical
 *   point -lambda / 2. Take lambda = r e^(2 pi i g) with g = (3 - sqrt 5)/2,
 *   the golden mean, the rotation number hardest to approximate by
 *   fractions.
 * - r = 1: by Siegel's theorem f is conjugate to the rotation by the
 *   golden angle on a disc around 0, and for this rotation number the
 *   disc's boundary is a quasicircle through the critical point. Nothing
 *   inside converges; points circle forever on invariant curves.
 * - r < 1: 0 attracts, and every point of its basin spirals in, shrinking
 *   by r and turning by the golden angle on each pass.
 *
 * Pipeline: Square with zn0 = 1, zn2 = s = 1.15, zn1 = lambda / (2 s) and
 * zn3 = -lambda^2 / (4 s), so T(z) = lambda z + s z^2 = f(s z) / s. Four
 * Sine lanes in phase move r = 0.965 +- 0.035 at 0.006 Hz (about 2.8 min
 * per breath). zn1 is linear in r and exact; zn3 is quadratic in r, so its
 * lanes are exact at both ends (r = 0.93 and r = 1) and off by at most
 * 2.6e-4 in between. The WaveRing seed is centred on the fixed point and
 * the OKLab hue step is g turns, so successive generations never repeat a
 * hue. At r = 1 the disc keeps every ring generation, rotated by the golden
 * angle: in the numpy model an iridescent pearl, with its preimage beside
 * it. At r = 0.93 the disc forgets: everything is drawn to the ring centre
 * and the pearl becomes a dark eye with a bright pupil. The pupil carries
 * the WaveRing centre's sine pulse, slow and smooth rather than a flash.
 *
 * Fidelity:   exact at the two ends of the breath; second order between
 * Cell:       Square / Clamp / WaveRing / OKLabColorRotation
 * Nearest:    siegel_disk_golden_mean_linearizer holds a Siegel disc still
 *             (P3); spatial_wave_chiral_siegel_critical_vortex folds one on
 *             Torus; tp2_golden_siegel_cylinder_rainbow_comb is exponential.
 *             Here the disc is a phase of a breath between memory and
 *             forgetting.
 * Height:     age = passes; ring generations at the golden hue step.
 * Hue clock:  bounded; step g = 0.381966.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-16. At r = 1 the critical
 *             orbit stayed within |z| <= 0.4346 for 2e5 iterations; at
 *             r = 0.965 and 0.93 it converged to 0. At rest (lane offsets)
 *             the fixed point sits 1.4e-4 from the origin with |multiplier|
 *             0.9653. A numpy model of the pass was drawn at r = 1 and 0.93.
 *             Not built, rendered by the engine or measured.
 * @assert sing[0].period == 1 && sing[0].rho_abs within 1 % of 0.965
 */
inline Pattern square_golden_siegel_disc_breathing_sink() {
  constexpr float BREATH_HZ = 0.006f;
  Pattern p;
  p.name = "square_golden_siegel_disc_breathing_sink";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {-0.30937433f, 0.28341223f};  // 0.965 lambda / 2.3
  p.zn.zn[2] = {1.15f, 0.0f};
  p.zn.zn[3] = {-0.017721765f, 0.20193037f}; // mean of r^2 times -lambda^2 / 4.6

  p.color.hue_rotation = 0.381966f; // the golden mean, as on the rotation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.96f;
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

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.1f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.5f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("breath_zn1_re", // r from 0.93 to 1: linear in r, exact
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = BREATH_HZ,
                            .amplitude = -0.011220831f,
                            .offset = -0.30937433f},
           control::ModTarget::Zn1Re)
      .add("breath_zn1_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = BREATH_HZ,
                            .amplitude = 0.010279200f,
                            .offset = 0.28341223f},
           control::ModTarget::Zn1Im)
      .add("breath_zn3_re", // r^2 from 0.8649 to 1, linear in the sine
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = BREATH_HZ,
                            .amplitude = -0.0012838278f,
                            .offset = -0.017721765f},
           control::ModTarget::Zn3Re)
      .add("breath_zn3_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = BREATH_HZ,
                            .amplitude = 0.014628555f,
                            .offset = 0.20193037f},
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
