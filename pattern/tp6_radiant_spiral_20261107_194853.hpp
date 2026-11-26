#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * TP6 Radiant Spiral — showcase for GPU inner ID 19 (0.5·(cosh(z) + sinh(z))).
 *
 * MATHEMATICS: THE EXPONENTIAL IDENTITY
 *
 * The defining identity 0.5·(cosh(z) + sinh(z)) = 0.5·exp(z) collapses this
 * map to the *cleanest* nonlinear in the catalog: a pure complex exponential.
 *
 *   f(z) = 0.5·exp(z) = 0.5·exp(x)·(cos(y) + i·sin(y))
 *
 * Magnitude grows as 0.5·exp(Re(z)) along the real axis, and the argument
 * rotates linearly with Im(z).  The image of any horizontal strip y ∈ [a, b]
 * is a circular sector; the image of any vertical line is a logarithmic
 * spiral.  Iterating this map on a contracted grid traces a tightening
 * RADIANT SPIRAL — rays of constant phase folding into the origin while
 * shells of constant modulus shear outward.
 *
 * Because the dynamics are dominated by exp(Re(z)), the system is highly
 * sensitive to horizontal placement: a small Re(zn1) translate sweeps the
 * spiral's apparent "horizon" left or right by an exponential factor.
 *
 * COLOR MODE 5 — DEPTH ECHO:
 *
 *   brightness ← brightness · (1 + depth · echo(z))
 *
 * Depth Echo cascades brightness along iteration depth, naturally aligning
 * with the exp map's geometric shell structure.  Each "shell" of constant
 * |z| reads as a depth band, giving the spiral a layered, telescoping
 * recession into the frame — a true *radiant horizon*.
 *
 * AFFINE GEOMETRY:
 *
 * zn0 = (0.55, 0.20): aggressive contraction (|zn0| ≈ 0.585) is mandatory —
 * exp grows so fast that any |zn0| > ~0.7 sends iterates to numerical
 * infinity within a few steps.  The 20° tilt rotates each shell, accumulating
 * into the spiral arm sweep.
 *
 * zn1 = (-0.04, 0.02): tiny leftward translate parks the spiral apex
 * slightly inside frame center, biasing the radiant rays toward the right
 * half — the asymmetry reads as gravitational lensing toward a hidden source.
 *
 * zn2 = (0.86, -0.04): mild post-contraction with counter-rotation tames
 * accumulation; without it the spiral over-twists and loses readable arms.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode5) → seed → blend
 */
inline Pattern tp6_radiant_spiral() {
  Pattern p;
  p.name = "tp6_radiant_spiral";
  // Named-enum path: InnerFunction::TP6 ↔ GPU id 19 ((cosh(z)+sinh(z))/2)
  p.inner = InnerFunction::TP6;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus;

  // Strong contraction + 20° tilt: tames exp blow-up, drives spiral sweep
  p.zn.zn[0] = {1.1121f, 0.4044f};
  // Slight leftward apex offset: asymmetry reads as a lensed light source
  p.zn.zn[1] = {-0.04f, 0.02f};
  // Mild post-contraction with counter-rotation: prevents over-twist
  p.zn.zn[2] = {1.4940f, -0.2105f};
  p.zn.zn[3] = {0.0f,   0.0f};

  // ── Color: MODE 5 (Depth Echo) — geometric shells → telescoping bands ──
  p.color.hue_rotation      = 0.006f;   // slow base hue rotation
  p.color.sat_scale         = 1.14f;
  p.color.fade              = 0.982f;   // moderate persistence: shells linger
  p.color.color_mode        = as_int(ColorMode::DepthEcho);  // Depth Echo
  p.color.spiral_angle_coeff  = 1.40f;  // angle drives ray-coloring
  p.color.spiral_radius_coeff = 1.10f;  // log_r aligns with shell geometry
  p.color.chroma_breath_amp   = 0.18f;
  p.color.chroma_breath_freq  = 0.65f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base   = 0.0f;
  p.color.hue_velocity_breath = 0.10f;
  p.color.hue_velocity_drift  = 0.50f;
  // Luma shaping is gated on color_mode < 4 in both kernels; mode 5 has its
  // own brightness path (hv_base + breath_amp above). These fields are zero.
  p.color.luma_shape_depth    = 0.0f;
  p.color.luma_shape_center   = 0.0f;

  // ── Seed: medium border — refresh the spiral's outer rim ──
  p.seed.border_width = 0.090f;
  p.seed.hue          = 0.55f;   // cool teal seed against warm exponential
  p.seed.hue_speed    = 0.129f;
  p.seed.alpha        = 0.92f;

  p.speed = 0.36f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.144f;
  p.zn.zn0_anim_amp = 0.051f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.195f;
  p.zn.zn1_anim_amp = 0.049f;
  p.zn.zn1_anim_k = 6.0f;
  p.seed.pulse_freq = 0.104f;
  p.seed.pulse_amp = 0.15f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.063f,
                            .amplitude = 0.308f,
                            .offset = 1.100f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.122f,
                            .amplitude = 0.103f,
                            .offset = 0.180f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.044f,
                            .amplitude = 0.045f,
                            .offset = 0.129f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
