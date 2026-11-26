#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Morphogenetic gradient wave preset.
 *
 * Scientific foundation:
 * - In developmental biology, morphogens are signaling molecules that
 *   diffuse through tissue to form concentration gradients specifying
 *   cell fate.  The steady-state morphogen concentration c(x) from a
 *   localized source satisfies the reaction-diffusion equation:
 *     D * d^2c/dx^2 - k*c + s*delta(x) = 0
 *   whose solution is proportional to cosh(x/L) / cosh(W/L) where
 *   L = sqrt(D/k) is the characteristic diffusion length.
 *
 * - cosh(z) IS the morphogen concentration profile.  The dome shape
 *   (cosh(0) = 1 at source, exponential decay outward) directly
 *   represents how morphogen concentration peaks at the source and
 *   drops symmetrically.  Unlike sin/cos, cosh has no zeros on the
 *   real line (minimum at cosh(0)=1), so there are no "dead zones"
 *   where cells receive zero signal -- matching biological reality.
 *
 * - Pre-rotation at the golden angle (2*pi*(2-phi) ~ 137.5 deg) connects
 *   to phyllotaxis: the arrangement of leaves, seeds, and floral organs
 *   in plants is driven by morphogen gradients at the shoot apical
 *   meristem, and the golden angle maximizes packing efficiency.  This
 *   IS the angle that auxin concentration peaks follow around the stem.
 *
 * - Post-rotation at 2*pi/5 (72 deg, pentagonal) reflects pentaradial
 *   symmetry found in echinoderm embryos (starfish, sea urchins) -- one
 *   of the most dramatic examples of morphogen-driven body plan formation.
 *
 * - Torus reduction creates smooth tissue continuity (cells in developing
 *   embryos do not have hard grid boundaries; tissue is continuous).
 *
 * - Color mode 1 (Spiral) reveals the morphogen gradient as concentric
 *   hue rings through the log-distance term, while angular variation
 *   shows the directional gradients that specify body axes.  The luma
 *   shaping creates bright peaks at high concentration and dark valleys
 *   at low concentration -- a direct visualization of the gradient field.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern morphogenetic_wave() {
  // Golden angle: 2*pi*(2-phi) ~ 137.5 deg -- phyllotaxis angle
  constexpr float CGA = -0.7373688781f;  // cos(golden angle)
  constexpr float SGA = 0.6754902943f;   // sin(golden angle)
  // 2*pi/5 (72 deg): pentaradial symmetry (echinoderm embryos)
  constexpr float C5  = 0.3090169944f;   // cos(2*pi/5)
  constexpr float S5  = 0.9510565163f;   // sin(2*pi/5)

  constexpr float PRE  = 0.64f;  // moderate: controlled diffusion gradient
  constexpr float POST = 0.86f;  // gentle contraction for gradient stability

  Pattern p;
  p.name = "morphogenetic_wave";
  p.inner = InnerFunction::Cosh;         // IS the morphogen concentration profile
  p.reduction = ReductionType::Torus;    // smooth tissue continuity

  p.zn.zn[0] = {CGA * PRE, SGA * PRE};
  p.zn.zn[1] = {-0.008f, 0.012f};       // left-right symmetry breaking (embryonic axis)
  p.zn.zn[2] = {C5 * POST, S5 * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.012f;
  p.color.sat_scale = 1.05f;
  p.color.fade = 0.984f;                // moderate-high: morphogen gradients persist
  p.color.color_mode = as_int(ColorMode::Spiral);               // Spiral: concentric gradient rings
  p.color.spiral_angle_coeff = 0.95f;   // angular variation shows directional gradients
  p.color.spiral_radius_coeff = 0.45f;  // log-distance rings = concentration levels
  // Mode 1 does not use breathing or velocity params (they're ignored)
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.20f;     // bright=high concentration, dark=low
  p.color.luma_shape_center = 1.5f;     // ring position in log-distance space

  p.seed.border_width = 0.028f;         // moderate: tissue boundary seed source
  p.seed.hue_speed = 0.08f;             // slow cycling: biological timescale
  p.seed.alpha = 0.90f;
  p.speed = 0.55f;                      // slow: developmental processes are gradual

  // ── Motion: slow wave propagation + chroma breath ─────────────────────────
  // hue_drift: developmental hue shift on a 40 s cycle — reads as a morphogen
  //   gradient sweeping through the tissue over a long biological timescale.
  // chroma_breath: chroma amplitude breathes ±0.06 on a 17 s cycle (prime,
  //   so it dephases from the 40 s hue cycle without repeating).
  using namespace epi::control;
  p.motion = PatternMotion::slow_color_drift(0.012f, 0.007f, 40.0f);
  p.motion.add("chroma_wave",
               ModLane{.wave         = Waveform::Sine,
                       .frequency_hz = 1.0f / 17.0f,
                       .amplitude    = 0.06f,
                       .offset       = 0.15f},  // resting chroma_breath_amp
               ModTarget::ChromaBreathAmp);
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.140f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.217f;
  p.zn.zn1_anim_amp = 0.048f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.103f;
  p.seed.pulse_amp = 0.19f;


  return p;
}

} // namespace epi::presets
