#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Dedekind eta function modular tower ("cathedral") preset.
 *
 * Scientific foundation:
 * - Richard Dedekind's eta function (1877) is the prototype modular form
 *   of weight 1/2 on the upper half-plane H:
 *
 *       eta(tau) = q^(1/24) * prod_{n=1}^infty (1 - q^n),   q = exp(2*pi*i*tau)
 *
 *   It is nowhere zero on H, converges for Im(tau) > 0, and encodes some
 *   of the deepest identities in number theory (Euler's pentagonal number
 *   theorem, Jacobi triple product, the partition function generating
 *   identity 1/eta = q^(-1/24) * sum p(n) q^n, etc.).
 *
 * - The TWO modular transformation laws of eta are the mathematical
 *   heart of this preset:
 *
 *       eta(tau + 1)    = exp(i * pi / 12) * eta(tau)      (translation)
 *       eta(-1 / tau)   = sqrt(-i * tau)   * eta(tau)      (inversion)
 *
 *   The translation phase pi/12 = 15 degrees is a 24th root of unity
 *   (since (e^{i*pi/12})^24 = 1) — this is why the discriminant
 *   Delta(tau) = eta(tau)^24 is a fully-invariant weight-12 cusp form.
 *   Setting the engine's pre-rotation to exactly pi/12 makes each
 *   feedback iteration enact a LITERAL Dedekind-eta translation.
 *
 * - As tau -> i*infinity (the "cusp"), eta(tau) -> q^(1/24) = 0
 *   exponentially. Geometrically the fundamental domain of SL_2(Z)
 *   narrows to an infinite tower toward i*infinity — this is the
 *   visual "cathedral tower" the preset conjures: feedback iterations
 *   compound the pi/12 rotation until the contraction toward the cusp
 *   produces an infinitely nested dense kaleidoscopic tunnel of
 *   self-congruent fundamental-domain translates.
 *
 * - The post-rotation uses the transcendental angle 2*pi/e radians,
 *   which is incommensurate with pi, with every algebraic irrational
 *   (golden, silver), and with the pi/12 modular translation — so no
 *   rational periodicity can short-circuit the density of the tower.
 *   This is the finest available "most-irrational" companion to the
 *   eta translation angle among transcendental constants.
 *
 * - tanh inner: Dedekind eta restricted to a horizontal line Re(tau)
 *   = const is a bounded, smoothly oscillating function of Im(tau)
 *   whose envelope decays like exp(-pi*Im(tau)/12) toward the cusp.
 *   tanh is the canonical bounded smooth function for this regime —
 *   it saturates before diverging (unlike sinh/exp) and is symmetric
 *   (unlike cosh), reproducing eta's anti-symmetric behavior under
 *   Im(tau) reflection through the real axis.
 *
 * - Grid reduction realises the modular-group tessellation: each
 *   SL_2(Z) translate of the fundamental domain tiles the feedback
 *   space periodically, and at each tile boundary the accumulated
 *   pi/12 phase produces the characteristic multiplier-system seam
 *   that makes the Dedekind eta function so visually distinctive.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern tanh_dedekind_eta_modular_cathedral() {
  // Pre-rotation: pi/12 = 15 degrees — the Dedekind eta translation
  // phase (24th root of unity). Each feedback step applies this
  // rotation plus a tanh-bounded nonlinear warp, so 24 iterations
  // complete one full modular-group translation cycle.
  //
  // Exact values:
  //   cos(pi/12) = (sqrt(6) + sqrt(2)) / 4
  //   sin(pi/12) = (sqrt(6) - sqrt(2)) / 4
  constexpr float CE = 0.9659258263f; // cos(pi/12)
  constexpr float SE = 0.2588190451f; // sin(pi/12)

  // Post-rotation: 2*pi/e ≈ 2.31141 rad ≈ 132.43 degrees — the
  // transcendental irrational angle 2*pi divided by Euler's number.
  // Incommensurate with pi and with pi/12, so the composition of
  // pre- and post-rotations never closes into a finite cycle; the
  // kaleidoscopic tower is genuinely infinite, never repeating.
  //
  //   2*pi/e = 2.3114138007240...
  //   cos(2*pi/e) ≈ -0.6748176831
  //   sin(2*pi/e) ≈  0.7380128439
  constexpr float CT = -0.6748176831f; // cos(2*pi/e)
  constexpr float ST = 0.7380128439f;  // sin(2*pi/e)

  // Pre-scale very close to 1: dense saturation inside the unit disk
  // produces the characteristic tight "tunnel wall" tanh gives — the
  // tanh nonlinearity compresses all unbounded structure into a bright
  // boundary, which the 24-fold pi/12 rotation then tiles into a
  // rose-window cathedral.
  constexpr float PRE  = 1.5133f;
  // Post-scale slightly smaller: a gentle inward contraction per
  // iteration simulates the cathedral's perspective tower receding
  // toward the eta cusp at i*infinity.
  constexpr float POST = 1.3427f;

  Pattern p;
  p.name = "tanh_dedekind_eta_modular_cathedral";
  p.inner = InnerFunction::Tanh;
  p.reduction = ReductionType::Grid; // SL_2(Z) fundamental-domain tiling

  p.zn.zn[0] = {CE * PRE, SE * PRE};
  // Small imaginary detune — acts like Im(tau) offset, nudging the
  // visible slice of the modular tower along the imaginary axis so
  // the cusp approach is off-axis (breaks exact reflection symmetry
  // and enriches the fringe structure near the eta cusp).
  p.zn.zn[1] = {0.2229f, -0.1431f};
  p.zn.zn[2] = {CT * POST, ST * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // Color mode 3 (alive): strong spiral coupling turns the nested
  // fundamental-domain tower into a hue cathedral. Angle -> hue maps
  // each rotational pi/12 step to a distinct chromatic wedge, and
  // log-radius -> hue stratifies the cathedral into concentric
  // altitude bands that read as the tower's "floor levels".
  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.18f;  // vivid — stained-glass palette
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::Alive);

  // Spiral coefficients: both strong. High angle coupling plus high
  // radius coupling together trace the full conformal symmetry of
  // the SL_2(Z) fundamental-domain tessellation — which has a rich
  // 3-fold (tau = e^{2*pi*i/3}), 2-fold (tau = i), and cusp-like
  // (tau = i*infinity) symmetry that becomes visible as the
  // feedback compounds pi/12 rotations and tanh contractions.
  p.color.spiral_angle_coeff = 0.92f;
  p.color.spiral_radius_coeff = 0.48f;

  // Breathing: the modular discriminant Delta = eta^24 has a Fourier
  // expansion sum tau(n) q^n whose coefficients (Ramanujan tau) exhibit
  // highly-oscillatory sign changes. Modest chroma breathing at a
  // sub-unity frequency with significant spatial phase echoes this
  // rapid but bounded variation across the cathedral tower.
  p.color.chroma_breath_amp = 0.20f;
  p.color.chroma_breath_freq = 0.115f;
  p.color.chroma_breath_phase = 0.52f;
  p.color.hue_velocity_base = 0.016f;
  p.color.hue_velocity_breath = 0.022f; // breath-dominant → pulsing glow
  p.color.hue_velocity_drift = 0.0045f;
  p.color.luma_shape_depth = 0.29f;     // strong depth → deep tower feel
  p.color.luma_shape_center = 1.85f;

  p.seed.border_width = 0.088f;         // thin seed → preserves density
  p.seed.hue_speed = 0.105f;            // brisk chromatic cycling
  p.seed.alpha = 0.92f;
  // Periodic re-excitation simulates the q-expansion sum p(n) q^n of
  // the partition generating function 1/eta — the partition numbers
  // p(n) grow like exp(pi*sqrt(2n/3))/4n*sqrt(3) (Hardy-Ramanujan),
  // so a moderate pulse amplitude visually represents the partition
  // asymptotic density injection into the feedback loop.
  p.seed.pulse_freq = 0.092f;
  p.seed.pulse_amp = 0.24f;
  p.speed = 0.79f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.102f;
  p.zn.zn0_anim_amp = 0.047f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.193f;
  p.zn.zn1_anim_amp = 0.038f;
  p.zn.zn1_anim_k = 7.0f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.092f,
                            .amplitude = 0.134f,
                            .offset = 0.480f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.112f,
                            .amplitude = 0.110f,
                            .offset = 0.200f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.062f,
                            .amplitude = 0.037f,
                            .offset = 0.105f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
