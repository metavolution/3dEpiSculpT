#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh calcium wave abyss preset.
 *
 * CELL PHYSIOLOGY: CALCIUM SIGNALING & INTRACELLULAR WAVES
 *
 * Calcium is the most versatile second messenger in cell biology.  The
 * cytoplasmic Ca²⁺ concentration is maintained at ~100 nM (resting) —
 * 10,000× LOWER than outside the cell (~2 mM).  This enormous gradient
 * is a loaded spring.  When IP3 receptors on the endoplasmic reticulum
 * (ER) membrane open, Ca²⁺ floods into the cytoplasm, triggering:
 *
 *   CALCIUM-INDUCED CALCIUM RELEASE (CICR):
 *   Local [Ca²⁺] rise → opens nearby IP3 receptors → more Ca²⁺ released
 *   → opens MORE receptors → REGENERATIVE WAVE propagation at 10-100 μm/s
 *
 * This is a POSITIVE FEEDBACK EXCITABLE SYSTEM, mathematically identical
 * to nerve impulse propagation (FitzHugh-Nagumo) and forest fire spreading.
 * The Ca²⁺ wave sweeps across the cell like a detonation front, followed
 * by a refractory period as SERCA pumps re-sequester Ca²⁺ into the ER.
 *
 * The IP3 RECEPTOR is a TETRAMER (4 identical subunits) with both Ca²⁺
 * activation and Ca²⁺ INACTIVATION sites.  The dose-response curve is
 * BIPHASIC and SIGMOIDAL — at low [Ca²⁺] the channel opens (positive
 * feedback), at high [Ca²⁺] it closes (negative feedback).  This is
 * precisely the tanh function: monotonic sigmoidal saturation at ±1.
 *
 * CALCIUM OSCILLATIONS: Many cells show periodic Ca²⁺ spikes, typically
 * 0.1-1 Hz.  These oscillations encode information in their FREQUENCY
 * (frequency modulation), not amplitude.  Faster spiking = stronger
 * signal.  The oscillation arises from the interplay between CICR
 * (positive feedback) and pump/inactivation (negative feedback) — a
 * RELAXATION OSCILLATOR.
 *
 * ENGINE MAPPING TO CALCIUM DYNAMICS:
 *
 *   THE SIGMOIDAL GATE = Tanh inner function
 *     tanh(z) saturates at ±1 — the binary calcium switch.  Below
 *     threshold: resting state (tanh → -1, low Ca²⁺).  Above threshold:
 *     activated state (tanh → +1, high Ca²⁺).  The transition between
 *     states is SHARP and SIGMOIDAL, exactly like the IP3 receptor's
 *     dose-response curve.  Under iteration, tanh creates dense filament
 *     webs of alternating ±1 domains — each filament IS a calcium wave
 *     front, and the web IS the branching ER network through which
 *     waves propagate.
 *
 *   THE ER MEMBRANE = Grid reduction
 *     The endoplasmic reticulum forms a continuous membrane network of
 *     tubes and sheets spanning the entire cell.  Grid tiles are the
 *     ER CISTERNAE — membrane-bound calcium stores.  Each tile boundary
 *     is a junction between ER segments where Ca²⁺ waves must jump from
 *     one store to the next.  The tile boundaries ARE the IP3 receptor
 *     clusters that gate inter-cisterna wave propagation.
 *
 *   23 CHROMOSOME PAIRS = 2π/23 ≈ 15.65° pre-rotation
 *     Every human cell carries 23 pairs of chromosomes — the complete
 *     genetic blueprint that encodes ALL cellular machinery, including
 *     the IP3 receptors, SERCA pumps, and calcium channels that drive
 *     this pattern.  23 is the fundamental number of human cell identity.
 *
 *     cos(2π/23) ≈ 0.9625, sin(2π/23) ≈ 0.2714
 *     360°/15.65° = 23.0 → exact 23-fold symmetry.
 *     23 is prime, a factorial prime (23 = 4!-1), the smallest number
 *     of integer-sided boxes that tile a rectangle, and the number of
 *     axioms in Euclid's geometry.  Each of the 23 sectors represents
 *     one chromosome pair's worth of calcium channel gene expression.
 *
 *   THE SILVER RATIO = -ln(1+√2) ≈ -50.5° post-rotation
 *     The SILVER RATIO δ_S = 1+√2 ≈ 2.4142 is the positive root of
 *     x² - 2x - 1 = 0.  It governs the PELL NUMBERS (0,1,2,5,12,29,...),
 *     the diagonal-to-side ratio of a regular octagon, and optimal
 *     network branching patterns.
 *
 *     ln(1+√2) = arcsinh(1) ≈ 0.8814 rad:
 *       - arcsinh(1): the hyperbolic arcsine at unity — the threshold
 *         value in reaction-diffusion wave nucleation theory
 *       - In excitable media, arcsinh(1) appears as the CRITICAL
 *         PERTURBATION SIZE needed to trigger a propagating wave
 *       - (1+√2)(1-√2) = -1: conjugate units in ℤ[√2] (negative Pell)
 *
 *     cos(ln(1+√2)) ≈ 0.6361, sin(-ln(1+√2)) ≈ -0.7717
 *     360°/50.5° ≈ 7.13 → quasi-7-fold depth symmetry.
 *     Combined with 23-fold azimuthal: 23 × 7.1 ≈ 163 interference cells.
 *     163 is a HEEGNER NUMBER — j((-1+√-163)/2) = -640320³, the basis
 *     of Ramanujan's near-integer e^(π√163) ≈ 640320³ + 744.
 *
 *   CALCIUM OSCILLATION = Mode 2 (Breathing)
 *     The breathing IS the calcium spike: saturation peaks when [Ca²⁺]
 *     is high (wave crest), dips when [Ca²⁺] returns to baseline
 *     (refractory period).  The frequency IS the calcium oscillation
 *     frequency.  The phase term creates SPATIAL GRADIENT in spike
 *     timing — different parts of the cell fire at different phases,
 *     exactly as in a propagating calcium wave.
 *
 * WHAT TO WATCH:
 *   1. WAVE FRONTS: tanh ±1 filaments = calcium detonation waves
 *   2. 23-FOLD: chromosome-count sectors × dense tanh folding
 *   3. BREATHING: calcium oscillation visible as rhythmic pulsing
 *   4. ER NETWORK: Grid tiles = ER cisternae, boundaries = IP3R clusters
 *   5. SILVER DEPTH: Pell number stacking → optimal network branching
 *   6. THRESHOLD: tanh sigmoid = IP3 receptor dose-response curve
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern tanh_calcium_wave_abyss() {
  // 2π/23 ≈ 0.2732 rad (15.65°): 23 chromosome pairs — human cell identity
  // 23 = 4!-1 (factorial prime), smallest non-trivial box-tiling number
  // Exact 23-fold symmetry → dense calcium channel lattice
  constexpr float CC   =  0.9625f;           // cos(2π/23)
  constexpr float SC   =  0.2714f;           // sin(2π/23)
  // -ln(1+√2) = -arcsinh(1) ≈ -0.8814 rad (-50.5°): silver ratio threshold
  // 1+√2 ≈ 2.4142: the silver ratio, governs Pell numbers & optimal networks
  // arcsinh(1): critical perturbation size for wave nucleation in excitable media
  constexpr float CSR  =  0.6361f;           // cos(ln(1+√2))
  constexpr float SSRN = -0.7717f;           // sin(-ln(1+√2))

  constexpr float PRE  = 1.4945f;  // aggressive Tanh expansion: dense calcium web
  constexpr float POST = 1.3152f;  // strong contraction: near-critical (product 0.924)

  Pattern p;
  p.name = "tanh_calcium_wave_abyss";
  p.inner = InnerFunction::Tanh;             // sigmoidal gate: binary Ca²⁺ switch ±1
  p.reduction = ReductionType::Grid;         // ER membrane compartments: cisterna tiles

  p.zn.zn[0] = {CC * PRE, SC * PRE};
  p.zn.zn[1] = {0.2297f, -0.1860f};           // perturbation: thermal fluctuation
  p.zn.zn[2] = {CSR * POST, SSRN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — the calcium oscillation ──
  p.color.hue_rotation = 0.010f;             // moderate: calcium cycle color evolution
  p.color.sat_scale = 1.12f;                 // moderate vibrancy: biological warmth
  p.color.fade = 0.990f;                     // deep: ~100 frame persistence (wave memory)
  p.color.color_mode = as_int(ColorMode::Breathing);                    // Breathing: Ca²⁺ spike oscillation
  p.color.spiral_angle_coeff = 0.90f;        // STRONG: shows 23 calcium channel sectors
  p.color.spiral_radius_coeff = 0.50f;       // strong: ER depth layers
  // THE CALCIUM PULSE: CICR positive feedback → refractory recovery
  p.color.chroma_breath_amp = 0.35f;         // strong: visible Ca²⁺ spike amplitude
  p.color.chroma_breath_freq = 0.08f;        // slow: calcium oscillation period (~0.1-1 Hz)
  p.color.chroma_breath_phase = 1.8f;        // phase: spatial wave propagation gradient
  // Mode 2 does not use hue velocity params
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: strong wave front contrast (bright fronts, dark refractory zones)
  p.color.luma_shape_depth = 0.26f;          // strong: crisp calcium wave fronts
  p.color.luma_shape_center = 2.0f;

  // ── Seed: moderate — injects initial Ca²⁺ perturbation ──
  p.seed.border_width = 0.083f;              // moderate: ER junction boundaries
  p.seed.hue_speed = 0.131f;
  p.seed.alpha = 0.92f;

  p.speed = 0.40f;                           // moderate: watch calcium waves propagate
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.129f;
  p.zn.zn0_anim_amp = 0.037f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.137f;
  p.zn.zn1_anim_amp = 0.043f;
  p.zn.zn1_anim_k = 5.0f;
  p.seed.pulse_freq = 0.124f;
  p.seed.pulse_amp = 0.19f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.065f,
                            .amplitude = 0.140f,
                            .offset = 0.500f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.120f,
                            .amplitude = 0.120f,
                            .offset = 0.350f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.048f,
                            .amplitude = 0.046f,
                            .offset = 0.131f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
