#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spectral & Optical: Earnshaw Quadrupole, Equipotentials with a Saddle.
 *
 * Scientific foundation:
 * - In two dimensions the potential of a line charge is logarithmic, so the
 *   potential of charges q_k at a_k is Re log prod (z - a_k)^q_k = log|f(z)|
 *   for a polynomial or rational f. It is harmonic away from the charges.
 * - Earnshaw's theorem (1842): a harmonic function has no interior maximum
 *   or minimum, so every equilibrium point of an electrostatic field is a
 *   saddle. No static arrangement of charges can trap a charge. The
 *   equilibria are exactly the critical points of f.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = F12, 0.7 (w^4 - 1): four equal charges at the fourth roots of
 *   unity. The colour stage receives log_r = log(0.01 |T(z)|), which is this
 *   potential up to a constant.
 * - Color = LumaLockedHueDrift, which sets hue = clock + spiral_radius_coeff
 *   * log_r on every pass and leaves luminance alone. The hue is therefore an
 *   exact equipotential map: closed bands around each charge, and the
 *   four-armed saddle at the centre, the single critical point of w^4 - 1.
 *   The hue clock makes the bands drain steadily into the four charges.
 * - Luminance is the feedback of the same quartic under the Torus fold. zn2
 *   is turned by 135 degrees so that the image of the saddle falls between
 *   two charges and is sent out to the seed frame; otherwise the critical
 *   orbit would close up into a dark attracting cycle at the centre.
 * - Motion: band density, and a small circular displacement of the charge
 *   cluster (Zn1) that drags the saddle with it.
 */
inline Pattern earnshaw_quadrupole_equipotential_saddle() {
  constexpr float POST = 0.77781746f; // 1.1 * cos(45 deg)

  Pattern p;
  p.name = "earnshaw_quadrupole_equipotential_saddle";
  p.inner = InnerFunction::F12;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.25f, 0.0f}; // charges at radius 0.8
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {-POST, POST}; // 1.1 * e^{i 135 deg}
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 0.92f; // LumaLocked uses this directly as saturation
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::LumaLockedHueDrift);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.9f; // equipotential bands per e-fold of |T|
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.012f; // bands flow into the charges
  p.color.hue_velocity_breath = 0.006f;
  p.color.hue_velocity_drift = 0.002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.10f;
  p.seed.hue = 0.15f;
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.06f;
  p.seed.pulse_amp = 0.10f;
  p.speed = 0.85f;

  p.motion
      .add("equipotential_density",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.019f,
                            .amplitude = 0.30f,
                            .offset = 0.9f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("charge_cluster_x",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.031f,
                            .amplitude = 0.06f,
                            .offset = 0.0f},
           control::ModTarget::Zn1Re)
      .add("charge_cluster_y",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.031f,
                            .amplitude = 0.06f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn1Im);

  return p;
}

} // namespace epi::presets
