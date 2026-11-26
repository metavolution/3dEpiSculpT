#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Somitogenesis: the Clock-and-Wavefront Model of Vertebrate Segmentation.
 *
 * Scientific foundation:
 * - Cooke & Zeeman (1976); Palmeirim et al. (1997): cells of the presomitic
 *   mesoderm run a genetic oscillator (hairy/her genes, Notch-coupled). A
 *   determination front, set by opposing FGF/Wnt and retinoic-acid gradients,
 *   regresses toward the tail at constant speed. Where the front passes, the
 *   current clock phase is frozen into a segment boundary:
 *   somite length = front speed * clock period. Snakes get ~300 vertebrae by
 *   running a faster clock.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Tanh: a bistable switch, the sharp FGF threshold that separates
 *   oscillating (undetermined) from arrested (segmented) tissue.
 * - Reduction = Grid: the periodic series of segments.
 * - Motion lane "determination_front_regression": a SAWTOOTH of amplitude 1
 *   on Zn3Re. On the period-2 Grid wrap this is seamless constant-velocity
 *   travel: the wavefront sweeping steadily along the axis, forever.
 * - Color = PaletteMorph: the kernel morphs between a warm and a cool cosine
 *   palette as sin(2*pi*f*t + phase*angle), a two-state oscillation playing
 *   the segmentation clock (expression on / off); "her1_clock_period" drives
 *   its frequency.
 * - Seed pulse = clock ticks at the tail bud; rose k = 7 for cervical count.
 */
inline Pattern somite_clock_wavefront_segmentation() {
  constexpr float A0 = 0.15707963f;  // 9 deg
  constexpr float A2 = -0.29670597f; // -17 deg
  constexpr float PRE = 1.50f;
  constexpr float POST = 1.30f;

  Pattern p;
  p.name = "somite_clock_wavefront_segmentation";
  p.inner = InnerFunction::Tanh;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.21f, -0.17f};
  p.zn.zn[2] = {POST * std::cos(A2), POST * std::sin(A2)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.10f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.18f;
  p.zn.zn1_anim_amp = 0.04f;
  p.zn.zn1_anim_k = 7.0f;

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.86f;  // palette * luma * sat: ~unity mean gain
  p.color.fade = 0.989f;
  p.color.color_mode = as_int(ColorMode::PaletteMorph);
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = 0.60f;
  p.color.chroma_breath_amp = 0.20f;
  p.color.chroma_breath_freq = 0.12f;       // segmentation clock
  p.color.chroma_breath_phase = 6.2831853f; // one kinematic wave per turn
  p.color.hue_velocity_base = 0.05f;
  p.color.hue_velocity_breath = 0.20f;
  p.color.hue_velocity_drift = 0.40f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.95f;
  p.seed.hue_speed = 0.10f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.12f;
  p.seed.pulse_amp = 0.22f;
  p.speed = 0.75f;

  p.motion
      .add("determination_front_regression",
           control::ModLane{.wave = control::Waveform::Sawtooth,
                            .frequency_hz = 0.02f,
                            .amplitude = 1.0f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("her1_clock_period",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.03f,
                            .amplitude = 0.03f,
                            .offset = 0.12f},
           control::ModTarget::ChromaBreathFreq)
      .add("fgf_gradient_rings",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.055f,
                            .amplitude = 0.20f,
                            .offset = 0.60f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("notch_sync_saturation",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.07f,
                            .amplitude = 0.08f,
                            .offset = 1.86f},
           control::ModTarget::SatScale);

  return p;
}

} // namespace epi::presets
