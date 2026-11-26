#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Square (Minimal 3.5D): the Feigenbaum Point, the Edge of Chaos.
 *
 * Mathematics:
 * - T(z) = 2 z^2 + c/2: in w = 2z it is w -> w^2 + c, with
 *   c = c_inf = -1.401155189092, the accumulation point of the
 *   period-doubling cascade on the real axis of the Mandelbrot set, where
 *   periodic order ends and chaos begins.
 * - The map is infinitely renormalizable: the Julia set is self-similar
 *   about the critical point, with Feigenbaum's scaling alpha = -2.5029
 *   (each zoom by alpha flips and reproduces it), and a chain of
 *   smaller and smaller copies runs along the real axis. The filled Julia
 *   set has no interior: there are no dark lakes, only the set itself.
 * - Exactness: K_c lies in |w| <= (1 + sqrt(1 + 4|c|))/2 = 1.785, i.e.
 *   |z| <= 0.893, inside the Simplex band (|x| or |y| > 0.94). Border points
 *   have |w| >= 2 and escape monotonically, so Clamp holds them on the
 *   border. Colour is exact escape time.
 *
 * Pipeline: Square with zn0 = 2, zn1 = 0, zn2 = 1/2, zn3 = c_inf / 2;
 * Clamp; Simplex border, so a pixel's colour is the generation at which its
 * orbit reached the border, in OKLab steps of 1/6; the Julia set itself is
 * dark. Nothing moves but the hue lane.
 *
 * Fidelity:   identity
 * Cell:       Square / Clamp / Simplex / OKLabColorRotation
 * Nearest:    spatial_wave_feigenbaum_julia_cascade walks the cascade and
 *             stops short of c_inf; this row rests exactly on it, static.
 * Height:     age = passes before exit.
 * Hue clock:  bounded; step 1/6 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; the bound above
 *             evaluated arithmetically. Not built, rendered or measured.
 * @assert sing[0].escapes == 0
 */
inline Pattern square_minimal_feigenbaum_point_edge_of_chaos() {
  Pattern p;
  p.name = "square_minimal_feigenbaum_point_edge_of_chaos";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.5f, 0.0f};
  p.zn.zn[3] = {-0.70057759f, 0.0f}; // c_inf / 2

  p.color.hue_rotation = 0.16666667f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.94f;
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
  p.seed.hue = 0.02f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("escape_band_tint",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.01f,
                            .offset = 0.16666667f},
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
