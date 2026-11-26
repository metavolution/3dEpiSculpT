#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Nonlinear Dynamics: Douady's Rabbit, Three-Coloured by its Own Cycle.
 *
 * Scientific foundation:
 * - For c = -0.122561 + 0.744862 i the critical orbit of Z -> Z^2 + c is
 *   0 -> c -> c^2 + c -> 0: a superattracting cycle of period 3. The filled
 *   Julia set is Douady's rabbit. Its interior components are permuted by
 *   the map; the three touching the alpha fixed point (body and two ears)
 *   form the cycle, and every other component is a preimage of them. Near
 *   the critical point the Bottcher coordinate conjugates the third iterate
 *   to squaring, and its level curves are the equipotentials of the basin.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Square, zn2 = 1.6, zn3 = c / 1.6: this is Z -> Z^2 + c in the
 *   coordinate Z = 1.6 z, so the whole rabbit fits the frame.
 * - Seed = WaveRing, strongest at the centre of the frame, which is the
 *   critical point 0. It works as a circular orbit trap sitting exactly on
 *   one point of the attracting cycle.
 * - Reduce = Clamp. The exterior escapes to the border, where WaveRing has
 *   almost no alpha, so the outside stays dark and only the interior is lit.
 * - Color = OKLab rotation with hue step 1/3. A component that reaches the
 *   trap after k steps has had its hue turned k/3 of the wheel, so all
 *   components are coloured by k mod 3: body, ear and ear take three hues
 *   120 degrees apart, and every preimage inherits the hue of the phase it
 *   will join. The pipeline derives the classical three-colouring of the
 *   rabbit from the cycle itself.
 * - The travelling rings sample |F^(3m)(z)|, so inside each component they
 *   trace the equipotentials of its Bottcher coordinate. The trap centre is
 *   on a ring crest only half the time, so the whole rabbit pulses at the
 *   ring frequency (about 0.8 Hz at speed 1).
 * - Motion: c circles the centre of the period-3 bulb at radius 0.0096,
 *   about a tenth of the bulb radius; the cycle stays attracting and the
 *   ears lean.
 */
inline Pattern douady_rabbit_three_phase_ring_trap() {
  constexpr float C_RE = -0.07660063f; // -0.122561 / 1.6
  constexpr float C_IM = 0.46553875f;  //  0.744862 / 1.6

  Pattern p;
  p.name = "douady_rabbit_three_phase_ring_trap";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.6f, 0.0f};
  p.zn.zn[3] = {C_RE, C_IM};

  p.color.hue_rotation = 0.33333333f; // one third of the wheel per step
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.955f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.06f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // bounded hue clock: step stays near 1/3
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.07f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.04f;
  p.seed.alpha = 0.95f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("bulb_parameter_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.006f,
                            .offset = C_RE},
           control::ModTarget::Zn3Re)
      .add("bulb_parameter_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.006f,
                            .offset = C_IM,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("trap_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.011f,
                            .amplitude = 0.15f,
                            .offset = 0.0f},
           control::ModTarget::SeedHue);

  return p;
}

} // namespace epi::presets
