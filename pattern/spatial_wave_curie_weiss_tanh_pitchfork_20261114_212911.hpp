#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Curie-Weiss Pitchfork, z -> tanh(K z).
 *
 * Physics and mathematics:
 * - The mean-field magnetisation of a ferromagnet solves m = tanh(K m),
 *   K = J / (k_B T). Iterating the right-hand side is the standard way to
 *   solve it. For K < 1 the only solution is m = 0; at K = 1 a pitchfork
 *   bifurcation creates two stable states +-m*(K).
 * - In the complex plane the same iteration has two attracting basins for
 *   K > 1, separated by a Julia set that runs through the poles of tanh at
 *   z = i pi (2k + 1) / (2K). Near a pole the image leaves the square and is
 *   clamped. tanh(w) is the Cayley image of e^(2w), so the picture is a
 *   member of the tangent family seen in the rotated plane.
 *
 * Pipeline: Tanh, zn0 = K (with a small imaginary part so the two basins
 * spiral instead of mirroring exactly), zn2 = 1; Clamp; ChladniCymatics seed
 * filling the unit disc. The K lane sweeps 0.9 .. 2.3 slowly through the
 * critical point: one basin, then two, with the poles entering the window
 * from the top and bottom edges as K grows (at K = 1.6 they sit at y = 0.98).
 *
 * Mechanism assertion: contracting class dominant for all K in the sweep;
 * number of attracting fixed points on the real axis 1 for K < 1, 2 above.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_curie_weiss_tanh_pitchfork() {
  Pattern p;
  p.name = "spatial_wave_curie_weiss_tanh_pitchfork";
  p.inner = InnerFunction::Tanh;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.6f, 0.12f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.05f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.8f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.62f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.75f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("coupling_K_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.021f,
                            .amplitude = 0.7f,
                            .offset = 1.6f},
           control::ModTarget::Zn0Re)
      .add("basin_twist",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.013f,
                            .amplitude = 0.06f,
                            .offset = 0.12f},
           control::ModTarget::Zn0Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.02f,
                            .offset = 0.05f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
