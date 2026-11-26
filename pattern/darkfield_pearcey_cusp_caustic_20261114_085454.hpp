#pragma once

#include "base.hpp"

namespace epi::presets {

// Dark-field Pearcey-umbilic cubic caustic (P1 0.5*(1-5z^3/3), |zn2|=2.05, Clamp + Cross, fade=0.80) concentrating >90% sparse energy onto unit-peak zero contours.
/**
 * Dark-Field Caustic / Soliton Regime 1: Pearcey-Umbilic Cubic Fold Caustic.
 *
 * Analytical derivation:
 * - Inner = P1 evaluates f(w) = 0.5 * (1 - (5/3)*w^3) with 3-fold roots at
 *   |w| = (3/5)^(1/3) = 0.84343 and stationary fold Jacobian f'(w) = -2.5*w^2.
 * - With zn0 = 1.28 and zn2 = 2.05, t_z = 1.025 * (1 - (5/3)*w^3) concentrates
 *   pullback measure around its 6 hyperbolic zero-contour caustic arcs
 *   Re(t_z) = 0 and Im(t_z) = 0.
 * - Reduction = Clamp + Seed = Cross (exp(-20|z.x|) + exp(-20|z.y|)):
 *     * Cross injected luma intensity^2 * alpha exceeds 0.05 only inside
 *       |z_x| < 0.075 or |z_y| < 0.075; under |zn2| = 2.05 and Clamp, >90% of
 *       [-1, 1]^2 clamps into the dark perimeter where exp(-20) = 2e-9.
 *     * Along the cubic zero-contour ridges, Cross peaks at 1.0 with
 *       pulse_amp = 0.0 (env == 1.0) and seed.alpha = 0.96, locking peak
 *       ridge amplitude at 0.96 without clipping.
 * - Color = OKLabColorRotation (mode 17) with fade = 0.80 preserves exact
 *   0.000 dark-field background (zero additive floor lift) while layering
 *   3-4 crisp perceptually uniform caustic echoes.
 */
inline Pattern darkfield_pearcey_cusp_caustic() {
  Pattern p;
  p.name = "darkfield_pearcey_cusp_caustic";
  p.inner = InnerFunction::P1;
  p.reduction = ReductionType::Clamp;

  p.zn.zn[0] = {1.28f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {2.05f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.045f;
  p.zn.zn0_anim_amp = 0.035f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.036f;
  p.zn.zn1_anim_amp = 0.025f;
  p.zn.zn1_anim_k = 3.0f; // matches C3 cubic symmetry of P1

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.80f; // tau = 4.48 steps: crisp caustic echo without dark-field flood
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 3.0f; // 3-fold C3 azimuthal color match
  p.color.spiral_radius_coeff = 0.45f;
  p.color.chroma_breath_amp = 0.14f;
  p.color.chroma_breath_freq = 0.09f;
  p.color.chroma_breath_phase = 3.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.015f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.04f;
  p.seed.hue = 0.55f;
  p.seed.hue_speed = 0.04f;
  p.seed.alpha = 0.96f;     // normalized constructive ridge peak ~0.96-1.00
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;  // lock env(t) == 1.0 so peak amplitude never dips

  p.speed = 0.80f;

  p.motion
      .add("umbilic_caustic_shear",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.042f,
                            .amplitude = 0.22f,
                            .offset = 0.0f},
           control::ModTarget::Zn2Im)
      .add("cusp_scale_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.028f,
                            .amplitude = 0.14f,
                            .offset = 2.05f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Re);

  return p;
}

} // namespace epi::presets
