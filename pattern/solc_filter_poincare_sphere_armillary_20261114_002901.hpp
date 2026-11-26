#pragma once

#include "base.hpp"

#include <cmath>
#include <complex>

namespace epi::presets {

/**
 * Spectral & Optical: Solc Filter on the Poincare Sphere (SU(2) Armillary).
 *
 * Scientific foundation:
 * - The polarisation state of light is a point of the Poincare sphere; in the
 *   complex ratio z = E_y / E_x that sphere is the Riemann sphere. A lossless
 *   retarder acts on z as a Mobius map that is a rigid rotation of the sphere
 *   about the retarder's fast axis (Jones calculus, SU(2) -> SO(3), the same
 *   double cover that underlies the Hopf fibration and qubit Rabi rotations).
 * - A Solc or Lyot filter is a stack of identical retarders: the state after
 *   n plates is the n-th iterate of one rotation. Its orbits are circles of
 *   latitude about the axis, which stereographic projection draws as
 *   Apollonian circles around the two antipodal fixed points a and -1/conj(a).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Invert; zn0..zn3 are solved from the two fixed points and the
 *   multiplier e^{i theta} so that T(z) = zn3 + zn2 / (zn0 z + zn1) is exactly
 *   that sphere rotation. theta = 2 pi / 9: nine plates return the state.
 * - Seed = Cross. The two axes are stereographic images of two great circles;
 *   a rotation sends great circles to great circles. Each frame adds the
 *   pair turned by one more plate, so the picture is an armillary sphere of
 *   9 x 2 great circles, the newest drawn over the older ones.
 * - The map is an isometry of the sphere: nothing contracts and nothing
 *   escapes. Structure comes only from the seed being re-drawn, which is why
 *   this preset needs a seed with interior support.
 * - Color = OKLab rotation with hue step 1/9: after nine plates a circle
 *   returns to its place with its original hue and reinforces itself.
 * - Motion: zn2 and zn3 are turned together by a small angle (first-order
 *   lanes), which composes the rotation with a slight spin about the vertical
 *   axis. The axis and angle drift, and the armillary precesses.
 * - Points carried outside the square are folded back by Torus, adding
 *   mirror arcs near the edges.
 */
inline Pattern solc_filter_poincare_sphere_armillary() {
  using C = std::complex<float>;
  constexpr float THETA = 0.69813170f;  // 2 pi / 9
  constexpr float PRECESS = 0.012f;     // spin amplitude, radians
  constexpr float PRECESS_HZ = 0.015f;

  const C axis{0.30f, 0.22f};                    // fast axis on the sphere
  const C antipode = -1.0f / std::conj(axis);    // the opposite fixed point
  const C mult{std::cos(THETA), std::sin(THETA)};
  // Mobius map with fixed points (axis, antipode) and multiplier mult at axis.
  const C ma = axis - antipode * mult;
  const C mb = axis * antipode * (mult - 1.0f);
  const C mc = 1.0f - mult;
  const C md = mult * axis - antipode;
  const C post = (mb * mc - ma * md) / mc;
  const C shift = ma / mc;

  Pattern p;
  p.name = "solc_filter_poincare_sphere_armillary";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {mc.real(), mc.imag()};
  p.zn.zn[1] = {md.real(), md.imag()};
  p.zn.zn[2] = {post.real(), post.imag()};
  p.zn.zn[3] = {shift.real(), shift.imag()};

  p.color.hue_rotation = 0.11111111f; // 1/9: one hue per plate
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.988f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.08f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // bounded hue clock: step stays near 1/9
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.05f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.90f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  // d/d(delta) of zn * e^{i delta} at delta = 0 is i * zn.
  p.motion
      .add("precession_post_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = PRECESS_HZ,
                            .amplitude = -post.imag() * PRECESS,
                            .offset = post.real()},
           control::ModTarget::Zn2Re)
      .add("precession_post_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = PRECESS_HZ,
                            .amplitude = post.real() * PRECESS,
                            .offset = post.imag()},
           control::ModTarget::Zn2Im)
      .add("precession_shift_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = PRECESS_HZ,
                            .amplitude = -shift.imag() * PRECESS,
                            .offset = shift.real()},
           control::ModTarget::Zn3Re)
      .add("precession_shift_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = PRECESS_HZ,
                            .amplitude = shift.real() * PRECESS,
                            .offset = shift.imag()},
           control::ModTarget::Zn3Im);

  return p;
}

} // namespace epi::presets
