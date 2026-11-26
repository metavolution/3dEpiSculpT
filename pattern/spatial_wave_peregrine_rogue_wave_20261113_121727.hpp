#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Advanced 7): Peregrine Soliton Rogue Wave.
 *
 * Scientific foundation:
 * - Peregrine (1983): the focusing nonlinear Schroedinger equation has the
 *   rational solution psi = [1 - 4*(1 + 2it) / (1 + 4x^2 + 4t^2)] * exp(it):
 *   a wave "that appears from nowhere and disappears without a trace",
 *   peaking at exactly 3x the background. Observed in optical fibre (Kibler
 *   et al. 2010) and water tanks (Chabchoub et al. 2011).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = MagneticDipole ((z^2-1)/(z^2+1) = 1 - 2/(1+z^2)): the same
 *   "constant background minus a Lorentzian" rational form as the Peregrine
 *   solution. Far field f -> 1 is the plane-wave background; the poles at
 *   z = +/-i are the two points where the field folds without bound (the
 *   rogue peaks).
 * - Both special values are seeded on purpose: f(0) = -1 and f(inf) = +1 give
 *   T ~ -/+ zn2, with |zn2| = 1.03 inside the wide seed border (0.12).
 * - Color = SpatialWave with wave2 at 3x wave1's wavenumber (the 3:1 peak).
 * - Motion: modulation-instability gain on Zn2Re, Benjamin-Feir sideband
 *   breathing, and a slow Lissajous drift of the focus point via Zn3.
 */
inline Pattern spatial_wave_peregrine_rogue_wave() {
  Pattern p;
  p.name = "spatial_wave_peregrine_rogue_wave";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.08f, 0.06f};
  p.zn.zn[1] = {0.02f, -0.015f};
  p.zn.zn[2] = {1.03f, -0.05f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.11f;
  p.zn.zn1_anim_amp = 0.03f;
  p.zn.zn1_anim_k = 3.0f;

  p.color.hue_rotation = 0.010f;
  p.color.sat_scale = 1.07f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.58f;
  p.color.spiral_radius_coeff = 0.75f;  // background carrier
  p.color.chroma_breath_amp = 0.18f;
  p.color.chroma_breath_freq = 2.25f;   // 3x carrier: Peregrine peak ratio
  p.color.chroma_breath_phase = 0.10f;
  p.color.hue_velocity_base = 0.018f;
  p.color.hue_velocity_breath = 0.012f;
  p.color.hue_velocity_drift = 0.005f;
  p.color.luma_shape_depth = 0.18f;
  p.color.luma_shape_center = 3.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.12f;
  p.seed.hue = 0.52f;
  p.seed.hue_speed = 0.08f;
  p.seed.alpha = 0.90f;
  p.seed.pulse_freq = 0.07f;  // the wave appears ...
  p.seed.pulse_amp = 0.28f;   // ... and disappears
  p.speed = 0.8f;

  p.motion
      .add("modulation_instability_gain",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.046f,
                            .amplitude = 0.05f,
                            .offset = 1.03f},
           control::ModTarget::Zn2Re)
      .add("benjamin_feir_sideband",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.083f,
                            .amplitude = 0.07f,
                            .offset = 0.18f},
           control::ModTarget::ChromaBreathAmp)
      .add("rogue_focus_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.039f,
                            .amplitude = 0.035f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("rogue_focus_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.063f,
                            .amplitude = 0.035f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im);

  return p;
}

} // namespace epi::presets
