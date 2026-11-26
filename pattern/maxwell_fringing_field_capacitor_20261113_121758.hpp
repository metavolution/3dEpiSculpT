#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Maxwell's Curves: Fringing Field at the Edge of a Parallel-Plate Capacitor.
 *
 * Scientific foundation:
 * - J. C. Maxwell (Treatise, 1873, Art. 202): the conformal map
 *   z = w + exp(w) carries the uniform field between two infinite plates onto
 *   the exact field of semi-infinite plates. Deep inside (Re w << 0) the map
 *   is the identity (straight, parallel field lines); outside, the exponential
 *   takes over and the field lines balloon out around the plate edge. The
 *   same map describes the Borda mouthpiece and Saffman-Taylor fingers.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = TP2 ((z + exp(z))/2): Maxwell's map itself. Its derivative
 *   (1 + exp(z))/2 only vanishes at z = i*pi*(2k+1), outside the reachable
 *   pre-image disk (|zn0|*sqrt(2) < pi), so the frame has no critical point
 *   and no attracting basin; the left half is near-identity feedback, the
 *   right half fans out exponentially.
 * - The Simplex border pulled back through T draws the equipotentials and
 *   field lines of the fringe directly.
 * - Color = Alive: three hue velocities read as conduction, displacement and
 *   leakage currents; luma shaping brightens the high-|T| fringe.
 * - Motion: plate-voltage swing on Zn2Re, fringe-ring pitch, dielectric chroma.
 */
inline Pattern maxwell_fringing_field_capacitor() {
  constexpr float A0 = 0.20943951f;  // 12 deg
  constexpr float A2 = -0.13962634f; // -8 deg
  constexpr float PRE = 1.34f;
  constexpr float POST = 1.14f;
  const float zn2_re = POST * std::cos(A2);

  Pattern p;
  p.name = "maxwell_fringing_field_capacitor";
  p.inner = InnerFunction::TP2;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.04f, -0.03f};
  p.zn.zn[2] = {zn2_re, POST * std::sin(A2)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.12f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.17f;
  p.zn.zn1_anim_amp = 0.04f;
  p.zn.zn1_anim_k = 5.0f;

  p.color.hue_rotation = 0.012f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = 0.44f;
  p.color.chroma_breath_amp = 0.20f;
  p.color.chroma_breath_freq = 0.12f;
  p.color.chroma_breath_phase = 0.35f;
  p.color.hue_velocity_base = 0.017f;
  p.color.hue_velocity_breath = 0.015f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.22f;
  p.color.luma_shape_center = 1.8f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.085f;
  p.seed.hue = 0.60f;
  p.seed.hue_speed = 0.09f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.10f;
  p.seed.pulse_amp = 0.18f;
  p.speed = 0.78f;

  p.motion
      .add("plate_voltage_swing",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.052f,
                            .amplitude = 0.06f,
                            .offset = zn2_re},
           control::ModTarget::Zn2Re)
      .add("fringe_ring_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.071f,
                            .amplitude = 0.13f,
                            .offset = 0.44f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("dielectric_chroma",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.088f,
                            .amplitude = 0.08f,
                            .offset = 0.20f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
