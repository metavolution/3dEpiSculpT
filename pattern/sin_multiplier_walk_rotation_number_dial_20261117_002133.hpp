#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Sin (Advanced 3.5D): the Sine Family's Multiplier Walks the Unit Circle,
 * and a Cross Becomes a Rotation-Number Dial.
 *
 * Mathematics:
 * - For any inner function g, T(z) = zn2 g(zn0 z + zn1) + zn3 with
 *   zn2 g(zn1) + zn3 = 0 fixes 0 with multiplier zn2 g'(zn1) zn0, which is
 *   linear in zn0. So two lanes in quadrature on zn0 walk the multiplier
 *   round the unit circle exactly, whatever g is.
 * - Here g = sin, zn1 = zn3 = 0, zn0 = s mu, zn2 = 1/s: T(z) = sin(s mu z)/s,
 *   conjugate (u = s mu z) to u -> mu sin u, the sine family, with
 *   mu = e^(2 pi i theta). 0 is a neutral fixed point of rotation number
 *   theta. At theta = p/q it is parabolic with a flower of petals; at
 *   bounded-type irrationals (the golden mean among them) it is the
 *   centre of a Siegel disc whose boundary is a quasicircle through a
 *   critical point (G. Zhang, "On the dynamics of e^(2 pi i theta) sin z").
 *   sin is odd, so the two critical values +-1 behave alike.
 * - Near 0, T is the rotation by theta to first order. The Cross seed is
 *   four rays from 0, so generation n shows them turned by -n theta. At
 *   theta = p/q the generations repeat after q passes and the star has
 *   lcm(4, q) rays; between rationals the rays fan out into a disc. The
 *   star's ray count reads off the rotation number: a dial.
 *
 * Pipeline: Sin with zn2 = 1/s, s = 3.6 (the frame holds the components at
 * 0 and +-pi), and zn0 = s mu on two Sine lanes in quadrature at 0.002 Hz
 * (one turn of mu per 500 s, starting at the golden mean). Clamp: orbits
 * that run up the imaginary direction, where sin grows like e^|Im| / 2,
 * leave through the border. OKLab hue step 0.05, so neighbouring
 * generations of rays differ slightly in hue. At rest (base slots) mu is
 * the golden rotation.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Sin / Clamp / Cross / OKLabColorRotation
 * Nearest:    devaney_sine_cantor_bouquet uses a real multiplier, where
 *             orbits escape along hairs; spatial_wave_cardioid_multiplier_
 *             tour walks the quadratic family. Here the walk is exact for
 *             the sine family and the seed is chosen to show the rotation.
 * Height:     age = passes; the angle of a ray is its generation times
 *             theta.
 * Hue clock:  bounded; step 0.05 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. A ray of generation n
 *             turns n times as fast as mu: about 0.25 rad/s at n = 20.
 *             Not built, rendered or measured.
 * @assert sing[0].escapes == 0
 */
inline Pattern sin_multiplier_walk_rotation_number_dial() {
  constexpr float S = 3.6f;
  constexpr float GOLDEN = 0.38196601f; // rotation number at t = 0
  constexpr float WALK_HZ = 0.002f;
  Pattern p;
  p.name = "sin_multiplier_walk_rotation_number_dial";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {-2.6545280f, 2.4317650f}; // s e^(2 pi i g)
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.27777778f, 0.0f}; // 1 / s
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.05f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.96f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.52f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("multiplier_walk_re", // s cos(2 pi (f t + g))
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = S,
                            .offset = 0.0f,
                            .phase = GOLDEN + 0.25f},
           control::ModTarget::Zn0Re)
      .add("multiplier_walk_im", // s sin(2 pi (f t + g))
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = S,
                            .offset = 0.0f,
                            .phase = GOLDEN},
           control::ModTarget::Zn0Im)
      .add("dial_hue_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.007f,
                            .amplitude = 0.01f,
                            .offset = 0.05f},
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
