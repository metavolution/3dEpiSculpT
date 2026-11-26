#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Faraday Rotation: Circular Birefringence in a Magnetized Medium.
 *
 * Scientific foundation:
 * - M. Faraday (1845): a magnetic field along the propagation direction makes
 *   left- and right-circular light travel with different indices n_L != n_R.
 *   Linear light, a sum of both, emerges with its plane rotated by
 *   beta = V*B*d (Verdet constant V). Unequal circular amplitudes give an
 *   ellipse whose orientation is the longitude on the Poincare sphere.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = F4 (0.8*(cos(z) + exp(iz)) = 0.8*(1.5*exp(iz) + 0.5*exp(-iz))):
 *   literally a 3:1 superposition of two counter-rotating circular waves, an
 *   elliptically polarized field. It grows as exp(|Im z|) on one side only,
 *   so the frame is handed (chiral), like the medium.
 * - Color = Opponent: the kernel rotates the (Co, Cg) chroma plane by an angle
 *   that depends on screen angle and log-radius. A rotation of a 2D chroma
 *   vector is the direct colour analogue of rotating a polarization vector.
 *   spiral_angle_coeff = 2: polarization is a director, so the Poincare
 *   longitude is twice the physical angle.
 * - Seed = OrganicNoise: depolarized thermal light entering at the boundary.
 * - Motion: Verdet rotation sweep, B-field sweep on Zn2Im, ellipticity pulse.
 */
inline Pattern faraday_rotation_circular_birefringence() {
  constexpr float A0 = 0.34906585f;  // 20 deg
  constexpr float A2 = -0.24434610f; // -14 deg
  constexpr float PRE = 1.36f;
  constexpr float POST = 1.12f;
  const float zn2_im = POST * std::sin(A2);

  Pattern p;
  p.name = "faraday_rotation_circular_birefringence";
  p.inner = InnerFunction::F4;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.06f, -0.04f};
  p.zn.zn[2] = {POST * std::cos(A2), zn2_im};
  p.zn.zn[3] = {0.02f, 0.03f};

  p.color.hue_rotation = 0.030f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.988f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 2.0f;
  p.color.spiral_radius_coeff = 0.85f;
  p.color.chroma_breath_amp = 0.26f;
  p.color.chroma_breath_freq = 0.16f;
  p.color.chroma_breath_phase = 6.2831853f; // one full turn: seamless in angle
  p.color.hue_velocity_base = 0.040f;
  p.color.hue_velocity_breath = 0.020f;
  p.color.hue_velocity_drift = 0.009f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.085f;
  p.seed.hue = 0.14f;
  p.seed.hue_speed = 0.10f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.20f;
  p.seed.pulse_amp = 0.22f;
  p.speed = 1.0f;

  p.motion
      .add("verdet_rotation",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.10f,
                            .amplitude = 0.40f,
                            .offset = 2.0f},
           control::ModTarget::SpiralAngleCoeff)
      .add("magnetic_field_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.07f,
                            .amplitude = 0.08f,
                            .offset = zn2_im},
           control::ModTarget::Zn2Im)
      .add("ellipticity_pulse",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.09f,
                            .amplitude = 0.09f,
                            .offset = 0.26f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
