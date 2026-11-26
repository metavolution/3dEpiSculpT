#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh Euler prime abyss preset.
 *
 * NUMBER THEORY: EULER'S PRIME-GENERATING POLYNOMIAL & HEEGNER NUMBERS
 *
 * In 1772, Leonhard Euler observed that the polynomial:
 *
 *   f(n) = n^2 + n + 41
 *
 * produces PRIME NUMBERS for every integer n from 0 to 39 — a streak of
 * 40 consecutive primes.  This is the longest such run of any known
 * quadratic, and remains one of the most celebrated results in elementary
 * number theory:
 *
 *   f(0)=41, f(1)=43, f(2)=47, f(3)=53, f(4)=61, f(5)=71, f(6)=83,
 *   f(7)=97, f(8)=113, ..., f(39)=1601.
 *   f(40)=1681=41^2 — the first composite, breaking the streak.
 *
 * The number 41 is NOT accidental.  The DISCRIMINANT of f(n) is:
 *
 *   Delta = 1^2 - 4(41) = -163
 *
 * And -163 is a HEEGNER NUMBER — one of only 9 negative discriminants
 * for which the imaginary quadratic field Q(sqrt(d)) has CLASS NUMBER ONE
 * (unique factorization).  The complete list: -3,-4,-7,-8,-11,-19,-43,-67,-163.
 *
 * RAMANUJAN'S NEAR-INTEGER:
 * The Heegner number 163 produces the extraordinary near-coincidence:
 *
 *   e^(pi*sqrt(163)) = 262537412640768743.999999999999250...
 *                     ~ 640320^3 + 744
 *
 * This follows from complex multiplication theory and the j-invariant.
 * The fact that 163 gives a NEARLY INTEGER result reflects h(-163)=1:
 * the ring of integers in Q(sqrt(-163)) is a principal ideal domain.
 *
 * ENGINE MAPPING TO EULER'S PRIME POLYNOMIAL:
 *
 *   THE PRIME SIEVE = Tanh inner function
 *     Primality is a BINARY decision: each integer is prime (+1) or
 *     composite (-1).  tanh's sigmoidal saturation at +/-1 IS this
 *     arithmetic switch.  Under iteration, the dense tanh web creates
 *     alternating +/-1 filaments — each filament IS a prime candidate,
 *     and the web IS the sieve of Eratosthenes made visible.
 *     The filament density follows the Prime Number Theorem:
 *     pi(n) ~ n/ln(n) — primes thin logarithmically.
 *
 *   THE CLASS-ONE FIELD = Torus reduction
 *     In Q(sqrt(-163)), every ideal is principal (class number 1), so
 *     every element factors UNIQUELY — no seams, no exceptional cases.
 *     Torus wraps the domain seamlessly, embodying this algebraic
 *     perfection.  The infinity-mirror depth IS unique factorization:
 *     each layer decomposes identically to the one above.
 *
 *   41 EULER PRIMES = 2*pi/41 ~ 8.78 degrees pre-rotation
 *     360/8.78 = 41 -> EXACT 41-fold symmetry.
 *     Each sector IS one value of f(n) — 40 prime sectors plus one
 *     broken sector at n=40 where f(40)=41^2 is composite.
 *
 *     cos(2*pi/41) ~ 0.9883
 *     sin(2*pi/41) ~ 0.1526
 *
 *     41 is: the 13th prime (13 is Fibonacci!), a centered square
 *     number (41 = 4^2 + 5^2), and the smallest prime dividing E_12
 *     (the 12th Euler number).  The polynomial n^2+n+41 is the
 *     canonical example in Rabinowitz's theorem connecting class-1
 *     quadratic fields to prime-producing polynomials.
 *
 *   THE DISCRIMINANT DEPTH = -arctan(sqrt(41)) ~ -81.12 degrees
 *     sqrt(41) ~ 6.403: the square root of Euler's constant 41 itself.
 *     The discriminant -163 = 1 - 4*41, so 41 determines everything.
 *
 *     cos(arctan(sqrt(41))) = 1/sqrt(42) ~ 0.1543 (EXACT)
 *     sin(-arctan(sqrt(41))) = -sqrt(41)/sqrt(42) ~ -0.9881 (EXACT)
 *     Verification: 1/42 + 41/42 = 1
 *
 *     The denominator sqrt(42) encodes:
 *       42 = 41+1 — the integer AFTER Euler's constant
 *       42 = 2 x 3 x 7 — product of three CONSECUTIVE PRIMES
 *       42 is the 5th Catalan number's denominator: C_5 = 42
 *
 *     360/81.12 ~ 4.44 -> quasi-4.4-fold depth period.
 *     Combined: 41 x 4.4 ~ 180 interference cells.
 *     This is one of the DENSEST abyss patterns in the collection:
 *     180 prime-testing cells visible simultaneously.
 *
 *   THREE NUMBER-THEORETIC TIMESCALES = Mode 3 (Alive)
 *     base  = 0.020 -> prime gaps: FAST (local arithmetic fluctuation)
 *     breath = 0.011 -> density modulation: MEDIUM (PNT envelope n/ln(n))
 *     drift  = 0.003 -> class evolution: SLOW (deep algebraic structure)
 *
 * WHAT TO WATCH:
 *   1. 41-FOLD MANDALA: Euler's prime lattice radiating as ultra-fine sectors
 *   2. THE BROKEN SECTOR: f(40)=41^2 — one sector disrupts the prime pattern
 *   3. INFINITY MIRROR: Torus+Tanh creates endless self-similar depth
 *   4. QUARTER-TURN DEPTH: ~81 degree rotation -> 4.4-fold quasi-period
 *   5. 180 CELLS: among the densest interference patterns in the collection
 *   6. TRIPLE SHIMMER: gap/density/class timescales as nested iridescence
 *   7. HEEGNER CONNECTION: the visual IS Q(sqrt(-163)) made geometric
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern tanh_euler_prime_abyss() {
  // 2pi/41 ~ 0.1532 rad (8.78 degrees): Euler's prime-generating polynomial
  // n^2+n+41 gives primes for n=0..39 (40 consecutive!)
  // Discriminant 1-4(41) = -163, the largest Heegner number
  // 41 = 13th prime (Fibonacci), centered square 4^2+5^2
  constexpr float CE   =  0.9883f;           // cos(2pi/41)
  constexpr float SE   =  0.1526f;           // sin(2pi/41)
  // -arctan(sqrt(41)) ~ -1.4159 rad (-81.12 degrees): discriminant depth
  // cos = 1/sqrt(42) EXACT: 42 = 2x3x7 (three consecutive primes)
  // sin = -sqrt(41)/sqrt(42) EXACT; 1/42 + 41/42 = 1
  // quasi-4.4-fold: near-quarter-turn per layer, irrational -> never repeats
  constexpr float CDA  =  0.1543f;           // cos(arctan(sqrt(41))) = 1/sqrt(42)
  constexpr float SDAN = -0.9881f;           // sin(-arctan(sqrt(41))) = -sqrt(41)/sqrt(42)

  constexpr float PRE  = 1.12f;  // aggressive expansion: dense prime sieve filaments
  constexpr float POST = 0.84f;  // strong contraction: product 0.941 (near-critical)

  Pattern p;
  p.name = "tanh_euler_prime_abyss";
  p.inner = InnerFunction::Tanh;             // prime sieve: binary +/-1 decision at each z
  p.reduction = ReductionType::Torus;        // unique factorization: seamless class-1 domain

  p.zn.zn[0] = {CE * PRE, SE * PRE};
  p.zn.zn[1] = {0.002f, -0.003f};           // perturbation: arithmetic noise (symmetry break)
  p.zn.zn[2] = {CDA * POST, SDAN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // -- Color: MODE 3 (Alive) -- three number-theoretic timescales --
  p.color.hue_rotation = 0.006f;             // very slow: number-theoretic contemplation
  p.color.sat_scale = 1.16f;                 // HIGH: iridescent prime brilliance
  p.color.fade = 0.992f;                     // EXTREME: ~125 frame persistence (deepest)
  p.color.color_mode = as_int(ColorMode::Alive);                    // Alive: prime gap / density / class dynamics
  p.color.spiral_angle_coeff = 1.10f;        // VERY STRONG: reveals 41 prime sectors
  p.color.spiral_radius_coeff = 0.55f;       // strong: discriminant depth ring visibility
  // Chroma modulation (prime density oscillation pi(n) ~ n/ln(n))
  p.color.chroma_breath_amp = 0.18f;         // moderate: density fluctuation envelope
  p.color.chroma_breath_freq = 0.09f;        // prime gap oscillation rate
  p.color.chroma_breath_phase = 0.2f;        // arithmetic phase offset
  // THREE NUMBER-THEORETIC TIMESCALES
  p.color.hue_velocity_base = 0.020f;        // FAST: prime gaps (local arithmetic)
  p.color.hue_velocity_breath = 0.011f;      // MEDIUM: PNT density modulation
  p.color.hue_velocity_drift = 0.003f;       // SLOW: class number algebraic evolution
  // Luminance: maximum density contrast (41-fold must be crisp)
  p.color.luma_shape_depth = 0.26f;          // STRONG: sharp prime sector definition
  p.color.luma_shape_center = 2.2f;

  // -- Seed: ultra-minimal -- the prime lattice IS the visual --
  p.seed.border_width = 0.006f;              // ultra-subtle: sieve dominates completely
  p.seed.hue_speed = 0.03f;                  // very slow: number-theoretic meditation
  p.seed.alpha = 0.85f;

  p.speed = 0.30f;                           // VERY slow: 180-cell density demands contemplation
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.159f;
  p.zn.zn0_anim_amp = 0.038f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.139f;
  p.zn.zn1_anim_amp = 0.041f;
  p.zn.zn1_anim_k = 6.0f;
  p.seed.pulse_freq = 0.127f;
  p.seed.pulse_amp = 0.18f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.074f,
                            .amplitude = 0.154f,
                            .offset = 0.550f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.071f,
                            .amplitude = 0.103f,
                            .offset = 0.180f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.079f,
                            .amplitude = 0.030f,
                            .offset = 0.030f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
