#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Entorhinal Grid Cells: Hexagonal Firing Fields from Wave Interference.
 *
 * Scientific foundation:
 * - Hafting, Fyhn, Molden, Moser & Moser (2005, Nobel 2014): neurons in the
 *   medial entorhinal cortex fire on the vertices of a triangular lattice
 *   tiling the animal's environment. The firing map is well described by the
 *   sum of three cosine gratings 60 deg apart; in oscillatory-interference
 *   models (Burgess et al. 2007) these arise from velocity-controlled
 *   oscillators beating against the 8 Hz theta rhythm. Modules of different
 *   grid scale form a geometric progression along the dorso-ventral axis.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Cos: one cosine grating.
 * - Pre-affine rotated by +60 deg, post-affine by -60 deg: every feedback
 *   generation re-reads the previous field turned by 60 deg, so the
 *   recurrence stacks gratings at 0, 60, 120 deg... the three-wave
 *   interference construction, iterated across scales (grid modules).
 * - Seed = ChladniCymatics: products of cosines, i.e. standing-wave nodal
 *   lines throughout the interior: place-field-like energy everywhere.
 * - zn1 rose-curve animation = path integration (the animal's trajectory).
 * - Color = LivingPaletteAgent: OKLab rotation with a slow self-steering hue
 *   offset, theta-like lightness oscillation along log-radius.
 */
inline Pattern grid_cell_hexagonal_firing_field() {
  constexpr float HEX = 1.04719755f; // pi/3
  constexpr float PRE = 1.30f;
  constexpr float POST = 1.02f;
  const float zn2_re = POST * std::cos(-HEX);

  Pattern p;
  p.name = "grid_cell_hexagonal_firing_field";
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE * std::cos(HEX), PRE * std::sin(HEX)};
  p.zn.zn[1] = {0.012f, -0.016f};
  p.zn.zn[2] = {zn2_re, POST * std::sin(-HEX)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.10f;
  p.zn.zn0_anim_amp = 0.04f;
  p.zn.zn1_animate = true;       // path integration
  p.zn.zn1_anim_speed = 0.15f;
  p.zn.zn1_anim_amp = 0.05f;
  p.zn.zn1_anim_k = 3.0f;

  p.color.hue_rotation = 0.018f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::LivingPaletteAgent);
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = 0.20f;
  p.color.chroma_breath_amp = 0.30f;
  p.color.chroma_breath_freq = 0.06f;
  p.color.chroma_breath_phase = 6.2831853f;
  p.color.hue_velocity_base = 0.15f;
  p.color.hue_velocity_breath = 0.01f;
  p.color.hue_velocity_drift = 0.002f;
  p.color.luma_shape_depth = 0.05f;
  p.color.luma_shape_center = -4.25f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.45f;
  p.seed.hue_speed = 0.10f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.13f;
  p.seed.pulse_amp = 0.19f;
  p.speed = 0.8f;

  p.motion
      .add("grid_scale_module",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.045f,
                            .amplitude = 0.05f,
                            .offset = zn2_re},
           control::ModTarget::Zn2Re)
      .add("theta_phase_precession",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.03f,
                            .amplitude = 0.012f,
                            .offset = 0.018f},
           control::ModTarget::HueRot)
      .add("firing_field_chroma",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.075f,
                            .amplitude = 0.08f,
                            .offset = 0.30f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
