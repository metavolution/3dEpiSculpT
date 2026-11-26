#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Conformal Wing — Joukowski transform feedback with complex inversion
 *
 * MATHEMATICAL FOUNDATION:
 *
 * The Joukowski transform J(z) = z + 1/z maps the unit circle to the
 * real interval [−2, 2] and circles offset from the origin to Joukowski
 * airfoil profiles — wing-cross-sections used in classical aerodynamics.
 *
 * The key property: J is CONFORMAL (angle-preserving) everywhere except
 * at the critical points z = ±1 where J'(z) = 0.  Near these points
 * the map folds angles and produces the characteristic trailing-edge
 * cusp of an airfoil.
 *
 * In the feedback engine, using inner = Invert (f(z) = 1/z):
 *
 *   pre-affine:  z' = zn0·z                  (scale + slight rotation)
 *   inner:       z' = 1/z                     (conformal inversion)
 *   post-affine: z' = zn2·(1/z) + zn3        (rescale + offset)
 *
 * The inversion 1/z swaps interior and exterior of the unit circle,
 * creating a "mirror world" where large |z| maps to small |1/z|.
 * Combined with the asymmetric post-translate zn3, this generates
 * wing-profile conformal domains that tile into Penrose-like self-similar
 * patterns through the feedback accumulation.
 *
 * The pre-affine rotation (cos θ + i·sin θ) continuously rotates the
 * sampling frame, creating flowing, aerodynamic visual motion.
 *
 * WHY INVERT IS RARE IN PRESETS:
 * 1/z is the ONLY Möbius transformation in the inner catalog (all others
 * are transcendental).  It maps circles to circles (generalized Apollonian
 * circles theorem) and preserves the hyperbolic metric on ℍ².  The
 * feedback loop creates Apollonian circle-packing geometry.
 */
inline Pattern conformal_wing() {
  constexpr float THETA = 0.08f;           // Small pre-affine rotation
  constexpr float C = 0.99681f;            // cos(THETA) ≈ 0.9968
  constexpr float S = 0.07991f;            // sin(THETA) ≈ 0.0799
  constexpr float PRE_SCALE = 0.56f;       // Pre-affine magnitude scale

  Pattern p;
  p.name = "conformal_wing";
  p.inner = InnerFunction::Invert;         // f(z) = 1/z — Möbius inversion
  p.reduction = ReductionType::Torus;

  // Pre-affine: scale + slight rotation (generates sweeping motion)
  p.zn.zn[0] = {C * PRE_SCALE, S * PRE_SCALE};
  p.zn.zn[1] = {0.0f, 0.0f};
  // Post-affine: contract + asymmetric offset (the "wingroot")
  p.zn.zn[2] = {0.86f, 0.0f};
  // Asymmetric post-translate — offset toward the airfoil leading edge
  p.zn.zn[3] = {0.22f, 0.13f};

  p.color.hue_rotation = 0.011f;
  p.color.sat_scale = 1.025f;
  p.color.fade = 0.982f;

  // Mode 3 (alive): non-uniform hue velocity creates organic colour sweep
  // that follows the conformal flow lines.
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 0.75f;
  p.color.spiral_radius_coeff = 0.40f;
  p.color.chroma_breath_amp = 0.13f;
  p.color.chroma_breath_freq = 0.09f;
  p.color.chroma_breath_phase = 0.45f;
  p.color.hue_velocity_base = 0.016f;
  p.color.hue_velocity_breath = 0.009f;
  p.color.hue_velocity_drift = 0.0025f;
  p.color.luma_shape_depth = 0.14f;
  p.color.luma_shape_center = 1.9f;

  p.seed.border_width = 0.07f;
  p.seed.hue_speed = 0.07f;
  p.seed.alpha = 0.92f;
  p.speed = 0.75f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.100f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.152f;
  p.zn.zn1_anim_amp = 0.052f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.124f;
  p.seed.pulse_amp = 0.15f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.094f,
                            .amplitude = 0.120f,
                            .offset = 0.400f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.092f,
                            .amplitude = 0.085f,
                            .offset = 0.130f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.045f,
                            .amplitude = 0.030f,
                            .offset = 0.070f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
