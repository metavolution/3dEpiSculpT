#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * F5 Nested Bloom — showcase for GPU inner ID 25 (1.3·cos(sin(z))).
 *
 * MATHEMATICS: NESTED TRIGONOMETRIC COMPOSITION
 *
 * The map f(z) = 1.3·cos(sin(z)) is a two-stage nested composition:
 *
 *   Stage 1: w = sin(z)   — maps ℂ onto a horizontal strip via entire function
 *   Stage 2: z' = 1.3·cos(w) — maps the strip through cosine's oscillation
 *
 * For real z: f(x) = 1.3·cos(sin(x)).  Since |sin(x)| ≤ 1, this evaluates
 * cos on a bounded real range → f(x) ∈ [1.3·cos(1), 1.3] ≈ [0.703, 1.3].
 * The function is bounded on the real axis but grows along the imaginary axis
 * as |sin(iy)| = sinh(|y|) → ∞, driving cos into hyperbolic growth:
 *
 *   f(iy) ≈ 1.3·cosh(sinh(y))  — double hyperbolic amplification
 *
 * This creates NESTED BLOOM RINGS: near the origin the function traces slow
 * circles (bounded real regime), while off-axis it explodes into expanding
 * hyperbolic sheets — forming concentrically nested petals.
 *
 * COLOR MODE 2 — BREATHING:
 *
 *   brightness += breath_amp × sin(breath_freq × t + breath_phase)
 *
 * The time-oscillating brightness resonates with the concentric ring structure:
 * each ring appears to pulse in and out of visibility, giving the impression
 * of a slowly respiring bloom.  The mild spiral coefficients add rotation
 * to prevent stasis.
 *
 * AFFINE GEOMETRY:
 *
 * zn0 = (0.78, 0.08): |zn0| ≈ 0.784, arg ≈ 5.8°.  The contraction keeps
 * iterates near the bounded real-axis regime, suppressing the double-hyperbolic
 * blow-up.  The small tilt rotates the bloom rings slightly each step,
 * accumulating into a slow spiral over time.
 *
 * zn2 = (0.92, −0.05): post-affine contraction with counter-tilt prevents
 * angular drift accumulation → stable, non-rotating bloom at long time.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode2) → seed → blend
 */
inline Pattern f5_nested_bloom() {
  Pattern p;
  p.name = "f5_nested_bloom";
  // Named-enum path: InnerFunction::F5 ↔ GPU id 25 (1.3·cos(sin(z)))
  p.inner = InnerFunction::F5;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus;

  // Moderate contraction + 5.8° tilt: suppresses hyperbolic blow-up, adds bloom spin
  p.zn.zn[0] = {0.78f,  0.08f};
  // Zero translate: preserve concentric symmetry (bloom needs center-origin)
  p.zn.zn[1] = {0.0f,   0.0f};
  // Counter-tilt: prevents net angular drift → long-term bloom stability
  p.zn.zn[2] = {0.92f, -0.05f};
  p.zn.zn[3] = {0.0f,   0.0f};

  // ── Color: MODE 2 (Breathing) — pulsing concentric ring visibility ──
  p.color.hue_rotation      = 0.005f;   // slow base hue cycle
  p.color.sat_scale         = 1.10f;
  p.color.fade              = 0.995f;   // high persistence: rings accumulate
  p.color.color_mode        = as_int(ColorMode::Breathing);        // Breathing
  p.color.spiral_angle_coeff  = 0.90f;  // mild rotation — don't dominate the rings
  p.color.spiral_radius_coeff = 1.20f;  // strong radius term → concentric hue rings
  p.color.chroma_breath_amp   = 0.55f;  // strong breathing: rings pulse in/out
  p.color.chroma_breath_freq  = 0.60f;  // slow breathing cycle
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base   = 0.22f;
  p.color.hue_velocity_breath = 0.20f;  // breathing modulates hue velocity
  p.color.hue_velocity_drift  = 0.45f;
  p.color.luma_shape_depth    = 0.22f;
  p.color.luma_shape_center   = 2.0f;   // wide center: preserves bloom core detail

  // ── Seed: narrow border — bloom is center-driven, not edge-driven ──
  p.seed.border_width = 0.010f;
  p.seed.hue          = 0.12f;   // warm seed hue contrasts with the cool blooms
  p.seed.hue_speed    = 0.010f;
  p.seed.alpha        = 0.70f;

  p.speed = 0.22f;  // very slow: nested rings need time to build up
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.101f;
  p.zn.zn0_anim_amp = 0.041f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.187f;
  p.zn.zn1_anim_amp = 0.050f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.127f;
  p.seed.pulse_amp = 0.19f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.093f,
                            .amplitude = 0.336f,
                            .offset = 1.200f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.071f,
                            .amplitude = 0.120f,
                            .offset = 0.550f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.091f,
                            .amplitude = 0.030f,
                            .offset = 0.010f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
