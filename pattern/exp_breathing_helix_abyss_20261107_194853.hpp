#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential breathing helix abyss preset.
 *
 * MATHEMATICS: logarithmic spiral + helicoid + breathing depth
 *
 * The complex exponential e^z = e^x(cos y + i sin y) is the UNIVERSAL
 * COVERING MAP of C* = C\{0}.  Under iterated rotation, orbits trace
 * LOGARITHMIC SPIRALS — the same curves found in nautilus shells,
 * hurricane arms, spiral galaxies, and the Fibonacci phyllotaxis of
 * sunflower seeds.  The logarithmic spiral is the ONLY curve that is
 * self-similar under both rotation AND scaling: zoom in on any part
 * and you see the same spiral at a finer scale.
 *
 * With Torus reduction (seamless mirrored wrap), the exponential's
 * radial growth is folded into a smooth infinite-depth tunnel where
 * each layer is a seamlessly-tiled copy of the pattern.  Unlike Grid
 * (which creates visible faceted structure), Torus gives SMOOTH depth
 * recession — like looking down the inside of a spiral seashell.
 *
 * ENGINE MAPPING:
 *
 *   LOGARITHMIC SPIRAL = Exp inner function
 *     e^z maps vertical lines → circles, horizontal lines → rays.
 *     Under iteration with rotation, these compose into logarithmic
 *     spirals.  The EQUIANGULAR property of the log spiral means
 *     the angle between the curve and any radial line is constant —
 *     this is why the pattern maintains its shape at every depth level.
 *
 *   TRANSCENDENTAL PRE-ROTATION = 2π/e ≈ 132.4°
 *     This angle involves BOTH fundamental transcendental numbers
 *     (π and e).  By the Lindemann-Weierstrass theorem, e^(2πi/e)
 *     is transcendental — the orbit will NEVER close.  The continued
 *     fraction expansion of 2/e has small partial quotients, making
 *     the orbit fill angular space DENSELY and uniformly.  This is
 *     the mathematical guarantee of visual richness at every scale.
 *
 *   GOLDEN POST-ROTATION = -√φ ≈ -72.9°
 *     √φ = √(1.618...) = 1.2720... is a QUADRATIC IRRATIONAL
 *     (root of x⁴ - x² - 1 = 0).  Each depth layer is rotated by
 *     this irrational angle from the previous, creating quasiperiodic
 *     depth stacking where the ratio between adjacent layers involves
 *     the golden proportion.  The √φ scaling appears naturally in the
 *     diagonal of a golden rectangle, connecting this depth structure
 *     to Renaissance proportion theory.
 *
 *   BREATHING SPIRAL = Mode 2 (Breathing)
 *     breath = 1 + amp × sin(freq × 2πt + phase × angle)
 *     With strong phase coupling (phase=2.5), the breathing envelope
 *     follows the spiral arms: as a spiral arm passes through a point,
 *     saturation increases (arm) then decreases (gap between arms).
 *     This IS the helical twist — like DNA unwinding or a barber-pole
 *     rotating.  The breathing propagates INWARD (toward the abyss
 *     center), like water spiraling into a drain.
 *
 *     breath_amp = 0.30: strong enough to see the spiral pulsation
 *     breath_freq = 0.09: slow cycle → mesmerizing, meditative
 *     breath_phase = 2.5: strong angular coupling → the breathing
 *       ROTATES with the spiral rather than pulsing uniformly
 *
 *   DEPTH PERSISTENCE = fade 0.987
 *     At 60fps, structure persists for ~77 frames ≈ 1.3 seconds.
 *     This means ~8-10 spiral generations are visible simultaneously,
 *     creating the sensation of an infinite helicoidal staircase
 *     descending into darkness.
 *
 * - |zn0| = 0.82: moderate for Exp (avoids explosive growth while
 *   maintaining rich spiral structure).  This puts the system in the
 *   "structured tunnel" regime — less chaotic than |zn0| > 0.90 but
 *   more complex than the gentle |zn0| < 0.60 range.
 *
 * - |zn2| = 0.88: mild contraction.  Net: 0.82 × 0.88 = 0.722.
 *   The low product means the system is VERY stable — it will never
 *   blow up, and structure accumulates slowly over many frames.
 *
 * WHAT TO WATCH:
 *   1. The spiral arms emerge from the edges and WIND inward
 *   2. The breathing makes the arms visibly pulse — brighter, dimmer
 *   3. At the center: infinite depth recession (the abyss)
 *   4. The spiral NEVER repeats (transcendental rotation) but is
 *      EVERYWHERE DENSE (every angle is eventually visited)
 *   5. Depth rings appear as concentric hue bands within the spiral
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern exp_breathing_helix_abyss() {
  // 2π/e ≈ 2.311 rad (132.4°): transcendental angle (Lindemann-Weierstrass)
  constexpr float CE  = -0.6745f;            // cos(2π/e)
  constexpr float SE  =  0.7383f;            // sin(2π/e)
  // -√φ ≈ -1.272 rad (-72.9°): golden quadratic irrational depth layering
  constexpr float CPH  =  0.2944f;           // cos(√φ)
  constexpr float SPHN = -0.9557f;           // sin(-√φ)

  constexpr float PRE  = 1.1654f;  // moderate: structured tunnel (not chaotic)
  constexpr float POST = 0.9396f;  // mild contraction for depth stability

  Pattern p;
  p.name = "exp_breathing_helix_abyss";
  p.inner = InnerFunction::Exp;             // logarithmic spiral orbits
  p.reduction = ReductionType::Torus;       // seamless: smooth spiral tunnel

  p.zn.zn[0] = {CE * PRE, SE * PRE};
  p.zn.zn[1] = {0.0361f, -0.0329f};             // tiny imaginary perturbation
  p.zn.zn[2] = {CPH * POST, SPHN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — the helix pulsation ──
  p.color.hue_rotation = 0.010f;            // slow: let spiral color dominate
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.987f;                    // HIGH: ~77 frame persistence, deep abyss
  p.color.color_mode = as_int(ColorMode::Breathing);                   // Breathing: spiral arm pulsation
  p.color.spiral_angle_coeff = 0.92f;       // strong: angular spiral sectors visible
  p.color.spiral_radius_coeff = 0.50f;      // moderate: concentric depth ring hues
  // BREATHING HELIX (slow, strong, spatially rotating)
  p.color.chroma_breath_amp = 0.30f;        // STRONG: visible spiral pulsation
  p.color.chroma_breath_freq = 0.09f;       // SLOW: mesmerizing, meditative cycle
  p.color.chroma_breath_phase = 2.5f;       // STRONG coupling: breathing rotates with spiral
  // Mode 2 does not use hue velocity params (set to 0 for clarity)
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: bright spiral arms, dark gaps → helicoidal depth contrast
  p.color.luma_shape_depth = 0.24f;         // strong: visible depth rings
  p.color.luma_shape_center = 2.0f;

  // ── Seed: minimal — abyss self-generates from spiral feedback ──
  p.seed.border_width = 0.078f;             // MINIMAL: feedback-dominated depth
  p.seed.hue_speed = 0.108f;                 // slow: contemplative
  p.seed.alpha = 0.92f;

  p.speed = 0.42f;                          // very slow: mesmerizing spiral descent
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.131f;
  p.zn.zn0_anim_amp = 0.049f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.189f;
  p.zn.zn1_anim_amp = 0.040f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.127f;
  p.seed.pulse_amp = 0.20f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.055f,
                            .amplitude = 0.140f,
                            .offset = 0.500f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.086f,
                            .amplitude = 0.120f,
                            .offset = 0.300f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.055f,
                            .amplitude = 0.038f,
                            .offset = 0.108f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
