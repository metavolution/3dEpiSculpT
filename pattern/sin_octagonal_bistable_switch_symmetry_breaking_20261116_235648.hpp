#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Sin (Advanced 3.5D): a Bistable Switch whose Symmetry is Broken by an
 * Octagonal Fold.
 *
 * Mathematics:
 * - On [-1, 1], x -> sin(pi x / 2) is a two-state memory: fixed points -1,
 *   0 and 1, where 0 repels with multiplier pi/2 and +-1 are critical
 *   points as well as fixed points, hence superattracting. Every x > 0
 *   settles on 1 and every x < 0 on -1.
 * - Off the axis sin grows like exp(pi |y| / 2) / 2 and the box fold
 *   returns it. The AmmannBeenker fold then rotates every point outside the
 *   wedge |y| <= tan(pi/8) |x| into the wedge |arg| <= pi/8, which lies
 *   on the side of +1 and never on the side of -1. The map is odd, the fold
 *   is not, and the switch is biased.
 * - Measured: 81.7 % of the frame settles on +1, 18.3 % on -1, and nothing
 *   else within 300 passes. The boundary between the two basins is where
 *   the image is busiest.
 *
 * Pipeline: Sin with zn0 = pi/2 (zn2 = 1) and AmmannBeenker. The Cross seed
 * lights the real axis, which holds both sinks, so every pixel ends lit and
 * its colour records how many passes it took. A HueRot lane sweeps the
 * per-pass step between 1/8 and 1/2 at 0.003 Hz: at 1/8 successive
 * generations walk round the hue circle (magenta and blue tiles under a
 * cyan web in the numpy model); at 1/2 they alternate between two
 * opponent hues (cyan and red). Hue changes, lightness does not, and a
 * pixel n passes deep turns n times faster than the lane: about 0.07 turns
 * per second at n = 20.
 *
 * Fidelity:   identity
 * Cell:       Sin / AmmannBeenker / Cross / OKLabColorRotation
 * Nearest:    devaney_sine_cantor_bouquet scales sin on Clamp, where
 *             orbits escape along hairs; sin_fourier_harmonic_web folds sin
 *             on Grid. Here nothing escapes and the subject is which of two
 *             states wins.
 * Height:     age = passes to the sink.
 * Hue clock:  bounded; step 0.3125 +- 0.1875.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-16. Basin fractions from a
 *             400 x 400 grid in double precision; the multipliers are
 *             analytic. A numpy model of the pass was drawn at steps 1/8
 *             and 1/2. Not built, rendered by the engine or measured.
 * @assert p_C >= 0.99
 */
inline Pattern sin_octagonal_bistable_switch_symmetry_breaking() {
  Pattern p;
  p.name = "sin_octagonal_bistable_switch_symmetry_breaking";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::AmmannBeenker;
  p.zn.zn[0] = {1.5707963f, 0.0f}; // pi / 2: the fixed points +-1 are critical
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.3125f;
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
  p.color.hue_velocity_drift = 0.0f;
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
      .add("generation_step_sweep", // 1/8 (hue walk) to 1/2 (opponent pair)
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.1875f,
                            .offset = 0.3125f},
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
