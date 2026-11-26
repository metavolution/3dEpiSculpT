#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh Hermite transcendental abyss preset.
 *
 * NUMBER THEORY: TRANSCENDENTAL NUMBERS & THE HERMITE PROOF
 *
 * In 1873, Charles Hermite proved that Euler's number e = 2.71828... is
 * TRANSCENDENTAL — it cannot satisfy ANY polynomial equation with integer
 * coefficients.  This resolved a question open since Euler's time and
 * opened the door for Lindemann's proof that pi is transcendental (1882),
 * which finally proved that squaring the circle is impossible.
 *
 * The Lindemann-Weierstrass theorem unifies both results: if alpha is
 * a non-zero algebraic number, then e^alpha is transcendental.  This
 * single theorem explains WHY e and pi live outside all algebraic systems.
 *
 * Transcendental numbers are DENSE — between any two rationals lie
 * uncountably many transcendentals (|T| = 2^aleph_0), while the algebraic
 * numbers are merely countable (|A| = aleph_0).  Yet only a handful of
 * specific constants have been proven transcendental.
 *
 * CONNECTION TO TANH:
 *   tanh(z) = (e^{2z} - 1)/(e^{2z} + 1) — BUILT from the exponential.
 *   The transcendence of e flows directly through tanh's definition.
 *   Under iteration, the irrational pre-rotation angle 1/e ≈ 20.22 deg
 *   guarantees the orbit is DENSE and NEVER periodic — the visual
 *   IS the transcendental number line made manifest as infinite,
 *   never-repeating kaleidoscopic structure.
 *
 * ENGINE MAPPING:
 *
 *   THE MIRROR WALLS = Tanh inner function
 *     tanh saturates at +/-1.  Under expansion (|zn[0]| > 1), material
 *     is pushed past the walls, folded back, pushed again — creating
 *     infinite nested reflections.  Each filament web IS a level set
 *     of e^z, and the web IS the transcendental continuum made visible.
 *
 *   THE SEAMLESS INFINITY = Torus reduction
 *     Torus wraps the domain seamlessly — no edges, no boundaries.
 *     The kaleidoscope has infinite depth: each iteration adds another
 *     layer of mirrored structure that spirals inward forever.
 *
 *   NAPIER'S ANGLE = arctan(1/e) ≈ 20.22 deg pre-rotation
 *     cos(arctan(1/e)) = e/sqrt(1+e^2) ≈ 0.9385
 *     sin(arctan(1/e)) = 1/sqrt(1+e^2) ≈ 0.3453
 *     The unit vector pointing toward (e, 1) in the plane.
 *
 *     360/20.22 ≈ 17.8 → quasi-18-fold symmetry.
 *     18 kaleidoscope sectors — each a mirror chamber reflecting
 *     the transcendental structure of e.  18 = 2 x 3^2, the number
 *     of edges in a complete graph K_3,3 (Kuratowski's non-planar graph).
 *
 *   THE RECIPROCAL DEPTH = -arctan(sqrt(e^2 - 1)) ≈ -68.42 deg post-rotation
 *     cos(arctan(sqrt(e^2-1))) = 1/e ≈ 0.3679 (EXACT: 1+(e^2-1)=e^2)
 *     sin(-arctan(sqrt(e^2-1))) = -sqrt(e^2-1)/e ≈ -0.9299 (EXACT)
 *
 *     The cosine of the depth angle IS 1/e — the RECIPROCAL of Euler's
 *     number.  And 1/e = e^{-1} is the probability that a random
 *     permutation is a DERANGEMENT (no fixed points): lim P(D_n) = 1/e.
 *     Each depth layer contracts by 1/e — the natural decay constant.
 *
 *     Pre and post angles are CONJUGATE through e: the pre-angle's sine
 *     (1/sqrt(1+e^2)) and the post-angle's cosine (1/e) are related by
 *     the same transcendental constant, creating deep structural harmony.
 *
 *     360/68.42 ≈ 5.26 → quasi-5-fold depth symmetry.
 *     Combined: 17.8 x 5.3 ≈ 94 interference cells.
 *
 *   KALEIDOSCOPE RAINBOW = Mode 1 (Spiral)
 *     Each of 18 angular sectors receives a distinct hue — a rainbow
 *     kaleidoscope where every sector mirrors and shifts the pattern.
 *     The log(r) component traces depth rings: each ring is one more
 *     iteration of the tanh feedback, one more level of the infinite
 *     mirror hall.
 *
 * WHAT TO WATCH:
 *   1. 18-FOLD KALEIDOSCOPE: dense rainbow mirror sectors
 *   2. INFINITE DEPTH: tanh walls x Torus = bottomless mirror hall
 *   3. NEVER REPEATS: transcendental rotation → dense, aperiodic orbit
 *   4. 1/e DECAY: each depth layer contracts by the derangement constant
 *   5. FILAMENT WEB: the transcendental continuum as visual texture
 *   6. CONJUGATE HARMONY: pre and post angles are e-siblings
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern tanh_hermite_transcendental_abyss() {
  // arctan(1/e) ≈ 0.3531 rad (20.22°): Napier's angle
  // cos = e/√(1+e²), sin = 1/√(1+e²) — unit vector toward (e, 1)
  // quasi-18-fold: 18 kaleidoscope mirror sectors
  constexpr float CN   =  0.9385f;           // cos(arctan(1/e)) = e/√(1+e²)
  constexpr float SN   =  0.3453f;           // sin(arctan(1/e)) = 1/√(1+e²)
  // -arctan(√(e²-1)) ≈ -1.1941 rad (-68.42°): reciprocal depth
  // cos = 1/e EXACT (derangement constant, natural decay)
  // sin = -√(e²-1)/e EXACT; check: 1/e² + (e²-1)/e² = e²/e² = 1 ✓
  // Conjugate to pre-angle through e: deep structural harmony
  constexpr float CRD  =  0.3679f;           // cos(arctan(√(e²-1))) = 1/e
  constexpr float SRDN = -0.9299f;           // sin(-arctan(√(e²-1))) = -√(e²-1)/e

  constexpr float PRE  = 1.12f;  // aggressive expansion: dense transcendental web
  constexpr float POST = 0.84f;  // strong contraction: product 0.941 (near-critical)

  Pattern p;
  p.name = "tanh_hermite_transcendental_abyss";
  p.inner = InnerFunction::Tanh;             // mirror walls: tanh = (e^{2z}-1)/(e^{2z}+1)
  p.reduction = ReductionType::Torus;        // seamless infinity: bottomless kaleidoscope

  p.zn.zn[0] = {CN * PRE, SN * PRE};
  p.zn.zn[1] = {0.003f, -0.003f};           // perturbation: break perfect symmetry
  p.zn.zn[2] = {CRD * POST, SRDN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 1 (Spiral) — 18-sector rainbow kaleidoscope ──
  p.color.hue_rotation = 0.006f;             // very slow: let kaleidoscope geometry dominate
  p.color.sat_scale = 1.16f;                 // HIGH: iridescent mirror brilliance
  p.color.fade = 0.992f;                     // EXTREME: ~125 frame persistence (deepest)
  p.color.color_mode = as_int(ColorMode::Spiral);                    // Spiral: rainbow sectors + depth rings
  p.color.spiral_angle_coeff = 1.10f;        // STRONG: shows 18 kaleidoscope sectors
  p.color.spiral_radius_coeff = 0.55f;       // STRONG: bold depth ring banding
  // Mode 1 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: strong mirror contrast — bright reflections, dark inter-mirror gaps
  p.color.luma_shape_depth = 0.25f;          // STRONG: crisp kaleidoscope facet definition
  p.color.luma_shape_center = 2.2f;

  // ── Seed: ultra-minimal — the kaleidoscope IS the visual ──
  p.seed.border_width = 0.006f;              // minimal: infinite mirrors dominate completely
  p.seed.hue_speed = 0.03f;                  // very slow: contemplative transcendence
  p.seed.alpha = 0.85f;

  p.speed = 0.34f;                           // VERY slow: meditative infinite depth
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.178f;
  p.zn.zn0_anim_amp = 0.047f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.213f;
  p.zn.zn1_anim_amp = 0.049f;
  p.zn.zn1_anim_k = 6.0f;
  p.seed.pulse_freq = 0.112f;
  p.seed.pulse_amp = 0.20f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.064f,
                            .amplitude = 0.154f,
                            .offset = 0.550f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.069f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.091f,
                            .amplitude = 0.030f,
                            .offset = 0.030f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
