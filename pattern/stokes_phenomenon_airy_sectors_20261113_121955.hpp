#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Stokes Phenomenon: Dominant and Subdominant Exponentials of the Airy
 * Function.
 *
 * Scientific foundation:
 * - G. G. Stokes (1857): the asymptotic form of Ai(z) is a combination of
 *   exp(-zeta) and exp(+zeta), zeta = (2/3)*z^(3/2), whose coefficients jump
 *   as arg z crosses certain rays. On a Stokes line one exponential is
 *   maximally dominant and the coefficient of the hidden, subdominant one
 *   switches on. M. V. Berry (1989) showed the switch is a universal smooth
 *   error function. Anti-Stokes lines are where both are equal and interfere.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = TP4 (0.8*(cosh(z) + exp(z)) = 0.8*(1.5*exp(z) + 0.5*exp(-z))): a
 *   dominant plus a subdominant exponential with fixed 3:1 multipliers.
 *   Equivalently 1.386*cosh(z + ln(3)/2): the anti-Stokes line (equal
 *   moduli) sits at Re w = -0.549, visibly off-centre.
 * - Pre-affine rotated by 2*pi/3, the angle between the Airy Stokes rays.
 * - Post gain 0.70 puts the critical value 1.386*0.70 = 0.97 inside the seed
 *   border, so the saddle on the anti-Stokes line is lit rather than black.
 * - Color = PhantomFractal: the kernel quantizes the polar angle into 12
 *   wedges (sectorial asymptotics) and modulates rings in log-radius with
 *   pitch 2/3, the exponent of zeta.
 * - Motion: Stokes-multiplier switch (Zn2Im), fringe pitch, subdominant glow.
 */
inline Pattern stokes_phenomenon_airy_sectors() {
  constexpr float STOKES_RAY = 2.09439510f; // 2*pi/3
  constexpr float A2 = -0.61803399f;        // -1/phi rad
  constexpr float PRE = 1.18f;
  constexpr float POST = 0.70f;
  const float zn2_im = POST * std::sin(A2);

  Pattern p;
  p.name = "stokes_phenomenon_airy_sectors";
  p.inner = InnerFunction::TP4;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE * std::cos(STOKES_RAY), PRE * std::sin(STOKES_RAY)};
  p.zn.zn[1] = {0.014f, -0.009f};
  p.zn.zn[2] = {POST * std::cos(A2), zn2_im};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.095f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.07f;
  p.zn.zn1_anim_amp = 0.026f;
  p.zn.zn1_anim_k = 3.0f;

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::PhantomFractal);
  p.color.spiral_angle_coeff = 3.0f;      // three Stokes sectors per turn
  p.color.spiral_radius_coeff = 0.667f;   // 2/3: exponent of zeta
  p.color.chroma_breath_amp = 0.22f;
  p.color.chroma_breath_freq = 0.15f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.022f;
  p.color.hue_velocity_breath = 0.012f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.10f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.10f;
  p.seed.hue = 0.75f;
  p.seed.hue_speed = 0.10f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.10f;
  p.seed.pulse_amp = 0.20f;
  p.speed = 0.8f;

  p.motion
      .add("stokes_multiplier_switch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.058f,
                            .amplitude = 0.06f,
                            .offset = zn2_im},
           control::ModTarget::Zn2Im)
      .add("anti_stokes_fringe_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.045f,
                            .amplitude = 0.20f,
                            .offset = 0.667f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("subdominant_glow",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.09f,
                            .amplitude = 0.05f,
                            .offset = 0.10f},
           control::ModTarget::LumaShapeDepth);

  return p;
}

} // namespace epi::presets
