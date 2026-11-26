#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Hopf fibration drift preset.
 *
 * Scientific inspiration:
 * - The Hopf fibration is a map S^3 -> S^2 whose fibers are great circles.
 *   Under stereographic projection the fibers become Villarceau circles:
 *   nested tori of linked rings filling 3-space.
 * - Sinh inner function provides hyperbolic-spiral distortion that emulates
 *   the stereographic projection's radial stretching near the pole.
 * - Pre-rotation at pi/3 (hexagonal, 6-fold) gives the characteristic
 *   torus cross-section symmetry visible in Hopf fiber bundles.
 * - Post-rotation at 2*pi*(sqrt(5)-1)/2 = 2*pi/phi^2 (the golden angle)
 *   ensures the fiber orbits never close, densely filling the torus --
 *   exactly the property that makes the Hopf map non-trivial (pi_3(S^2)=Z).
 * - Torus reduction wraps the domain as a flat torus, the natural base
 *   space for visualizing fiber bundles.
 * - Alive color (mode 3) with breathing at 1/phi reveals the characteristic
 *   linked-ring topology through phase-coupled hue rotation.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern hopf_fibration_drift() {
  // pi/3 (60 deg): hexagonal symmetry
  constexpr float C3 = 0.5000000000f;   // cos(pi/3)
  constexpr float S3 = 0.8660254038f;   // sin(pi/3)
  // Golden angle: 2*pi*(2 - phi) = 2*pi*(sqrt(5)-1)/2 ≈ 2*pi*0.38197...
  // = 2.39996... rad
  constexpr float CGA = -0.7373688781f; // cos(golden angle)
  constexpr float SGA = 0.6754902943f;  // sin(golden angle)

  constexpr float INV_PHI = 0.6180339887f;
  constexpr float PHI = 1.6180339887f;

  constexpr float PRE  = 1.3393f; // near-unity for slow spiral convergence
  constexpr float POST = 1.1732f; // moderate contraction on output

  Pattern p;
  p.name = "hopf_fibration_drift";
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {C3 * PRE, S3 * PRE};
  p.zn.zn[1] = {-0.006f, 0.011f};       // small off-axis nudge
  p.zn.zn[2] = {CGA * POST, SGA * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.012f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.983f;
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: 3-velocity hue model
  p.color.spiral_angle_coeff = 0.88f;   // strong angle-hue coupling
  p.color.spiral_radius_coeff = 0.31f;  // moderate radial coupling
  p.color.chroma_breath_amp = 0.19f;    // breathing envelope
  p.color.chroma_breath_freq = INV_PHI; // golden-ratio breathing period
  p.color.chroma_breath_phase = 0.44f;  // phase offset
  p.color.hue_velocity_base = 0.011f;
  p.color.hue_velocity_breath = 0.018f; // dominant breath velocity
  p.color.hue_velocity_drift = 0.0032f;
  p.color.luma_shape_depth = 0.22f;
  p.color.luma_shape_center = PHI;      // luminance center at phi

  p.seed.border_width = 0.091f;
  p.seed.hue_speed = 0.104f;
  p.seed.alpha = 0.92f;
  p.speed = 0.58f;                      // slow, contemplative drift
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.183f;
  p.zn.zn0_anim_amp = 0.046f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.157f;
  p.zn.zn1_anim_amp = 0.051f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.123f;
  p.seed.pulse_amp = 0.14f;

  return p;
}

} // namespace epi::presets
