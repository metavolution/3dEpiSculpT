#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh ribosome codon abyss preset.
 *
 * MOLECULAR BIOLOGY: THE GENETIC CODE & PROTEIN TRANSLATION
 *
 * The RIBOSOME is the molecular machine that translates genetic information
 * (mRNA) into protein — the central process of the Central Dogma.  Every
 * protein in every cell of every living organism is built by ribosomes
 * reading the genetic code, one CODON (triplet of nucleotides) at a time.
 *
 * THE GENETIC CODE:
 *   - 4 nucleotide bases: A, U, G, C
 *   - 4^3 = 64 possible codons (triplets)
 *   - 20 standard amino acids (+ 1 stop signal)
 *   - Degeneracy: 64/20 ≈ 3.2 codons per amino acid
 *   - The code is UNIVERSAL: the same 64→20 mapping is shared by
 *     virtually all life on Earth, from bacteria to humans
 *
 * THE 20 AMINO ACIDS are the ALPHABET of protein structure:
 *   Ala, Arg, Asn, Asp, Cys, Gln, Glu, Gly, His, Ile,
 *   Leu, Lys, Met, Phe, Pro, Ser, Thr, Trp, Tyr, Val
 *
 * RIBOSOME MECHANICS:
 *   - The ribosome has two subunits: large (60S) and small (40S) in humans
 *   - mRNA threads through the small subunit
 *   - tRNAs deliver amino acids to the A (aminoacyl) site
 *   - Peptide bond formation occurs in the P (peptidyl) site
 *   - The exit tunnel (~80 A long) in the large subunit guides the
 *     nascent polypeptide chain out of the ribosome
 *   - Elongation rate: ~5-20 amino acids per second
 *   - Error rate: ~1 in 10,000 (proofreading via kinetic selection)
 *
 * ENGINE MAPPING TO PROTEIN TRANSLATION:
 *
 *   THE CODON SWITCH = Tanh inner function
 *     Each codon is either CORRECT (matches the tRNA anticodon, delivers
 *     the right amino acid) or INCORRECT (rejected by kinetic proofreading).
 *     This binary decision is SIGMOIDAL: the free energy difference between
 *     cognate and near-cognate tRNA binding creates a sharp discrimination.
 *     tanh's ±1 saturation IS the codon-anticodon recognition switch:
 *     +1 = cognate (correct amino acid delivered), -1 = rejected.
 *
 *     Under iteration, tanh creates a dense web of ±1 filaments — each
 *     filament IS one reading frame of the genetic code, and the web IS
 *     the network of all possible codon→amino acid mappings.
 *
 *   THE ROUGH ER = Grid reduction
 *     Ribosomes that synthesize secretory/membrane proteins are attached
 *     to the ROUGH ENDOPLASMIC RETICULUM — the ER membrane studded with
 *     ribosomes.  Grid tiles ARE the ER cisternae, with ribosomes sitting
 *     at each tile boundary.  The nascent protein threads through the
 *     translocon into the ER lumen at each tile junction.
 *
 *   20 AMINO ACIDS = pi/10 = 18° pre-rotation
 *     360°/20 = 18° = pi/10 → EXACT 20-fold symmetry.
 *     Each sector IS one amino acid in the genetic alphabet.
 *
 *     cos(pi/10) = sqrt(2+phi)/2 ≈ 0.9511 (where phi = golden ratio)
 *     sin(pi/10) = (sqrt(5)-1)/4 = 1/(2*phi) ≈ 0.3090
 *
 *     The GOLDEN RATIO in the genetic code!  sin(18°) = 1/(2*phi):
 *     the sine of the amino acid angle is the RECIPROCAL GOLDEN RATIO
 *     divided by 2.  This is one of the deepest identities in
 *     trigonometry, connecting the 20-fold symmetry of the genetic
 *     alphabet to the golden ratio that governs biological growth.
 *
 *     The golden ratio appears throughout protein structure:
 *       - Alpha-helix: 3.6 residues/turn = 18/5 (pentagonal connection)
 *       - Beta-sheet pleating angle relates to phi
 *       - Icosahedral virus capsids (5-fold axes) use phi
 *       - Protein folding kinetics involve phi-value analysis
 *
 *     cos(18°) = sqrt(2+phi)/2: note that (2+phi) = (1+phi^2) = phi^3/phi
 *     appears in the mitotic spindle preset (cos(arctan(phi)) = 1/sqrt(2+phi)),
 *     connecting the RIBOSOME (protein synthesis) to the SPINDLE
 *     (chromosome segregation) through the same algebraic identity.
 *
 *   THE CODON TRIPLET = -arctan(3) ≈ -71.57° post-rotation
 *     3 = the CODON LENGTH: each amino acid is encoded by exactly 3
 *     nucleotides.  The triplet code is the most fundamental number
 *     in molecular biology — it determines the reading frame, and a
 *     single insertion/deletion shifts the frame, scrambling the entire
 *     downstream protein (frameshift mutation).
 *
 *     cos(arctan(3)) = 1/sqrt(10) ≈ 0.3162 (EXACT: 1^2 + 3^2 = 10)
 *     sin(-arctan(3)) = -3/sqrt(10) ≈ -0.9487 (EXACT)
 *     sqrt(10) = sqrt(1+9): the Pythagorean hypotenuse of the 1-3 codon.
 *
 *     360°/71.57° ≈ 5.03 → quasi-5-fold depth symmetry.
 *     5-fold connects to the pentagon, and the pentagonal symmetry of
 *     icosahedral viruses — which USE the genetic code to replicate.
 *
 *     Combined: 20-fold × 5-fold = 100 interference cells — one for
 *     each possible amino acid × reading position combination.
 *
 *   THREE TRANSLATION TIMESCALES = Mode 3 (Alive)
 *     base  = 0.024 → elongation: FAST (~5-20 aa/sec, peptide bond)
 *     breath = 0.012 → decoding: MEDIUM (codon-anticodon selection)
 *     drift  = 0.004 → translocation: SLOW (ribosome ratchet movement)
 *     These are the three mechanical steps of each elongation cycle.
 *
 * WHAT TO WATCH:
 *   1. 20-FOLD: amino acid alphabet sectors (count them!)
 *   2. GOLDEN RATIO: sin(18°) = 1/(2*phi) — biology's deepest number
 *   3. CODON WEB: tanh ±1 filaments = reading frame network
 *   4. TRIPLE DEPTH: 3-nucleotide codon length structures each layer
 *   5. ER TILES: Grid boundaries = ribosome attachment sites on rough ER
 *   6. TRIPLE SHIMMER: elongation/decoding/translocation dynamics
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern tanh_ribosome_codon_abyss() {
  // pi/10 = 18° (0.3142 rad): exact 20-fold symmetry — 20 amino acids
  // cos(pi/10) = sqrt(2+phi)/2 ≈ 0.9511 (phi = golden ratio)
  // sin(pi/10) = (sqrt(5)-1)/4 = 1/(2*phi) ≈ 0.3090 — GOLDEN RATIO!
  // The genetic alphabet encoded in pentagonal trigonometry
  constexpr float CA   =  0.9511f;           // cos(π/10) = √(2+φ)/2
  constexpr float SA   =  0.3090f;           // sin(π/10) = 1/(2φ)
  // -arctan(3) ≈ -1.2490 rad (-71.57°): the codon TRIPLET
  // 3 nucleotides per codon — the fundamental unit of the genetic code
  // cos = 1/√10, sin = -3/√10 — EXACT algebraic (1² + 3² = 10)
  constexpr float CCT  =  0.3162f;           // cos(arctan(3)) = 1/√10
  constexpr float SCTN = -0.9487f;           // sin(-arctan(3)) = -3/√10

  constexpr float PRE  = 1.5547f;  // aggressive Tanh: dense codon web
  constexpr float POST = 1.3023f;  // strong contraction: product 0.918

  Pattern p;
  p.name = "tanh_ribosome_codon_abyss";
  p.inner = InnerFunction::Tanh;             // codon switch: cognate/non-cognate ±1
  p.reduction = ReductionType::Grid;         // rough ER: ribosome-studded membrane tiles

  p.zn.zn[0] = {CA * PRE, SA * PRE};
  p.zn.zn[1] = {0.2548f, -0.2286f};           // perturbation: thermal decoding noise
  p.zn.zn[2] = {CCT * POST, SCTN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 3 (Alive) — three elongation cycle timescales ──
  p.color.hue_rotation = 0.010f;             // moderate: protein sequence evolution
  p.color.sat_scale = 1.12f;                 // moderate vibrancy: ribosomal warmth
  p.color.fade = 0.991f;                     // VERY deep: ~111 frames (long mRNA lifetime)
  p.color.color_mode = as_int(ColorMode::Alive);                    // Alive: elongation triple dynamics
  p.color.spiral_angle_coeff = 0.90f;        // STRONG: shows 20 amino acid sectors
  p.color.spiral_radius_coeff = 0.50f;       // strong: mRNA threading depth
  // Ribosome oscillation (peptidyl-transferase center ratchet)
  p.color.chroma_breath_amp = 0.20f;         // moderate: ratchet amplitude
  p.color.chroma_breath_freq = 0.10f;        // elongation cycle rate
  p.color.chroma_breath_phase = 0.3f;        // tRNA accommodation phase
  // THREE TRANSLATION TIMESCALES
  p.color.hue_velocity_base = 0.024f;        // FAST: elongation (peptide bond formation)
  p.color.hue_velocity_breath = 0.012f;      // MEDIUM: decoding (codon-anticodon selection)
  p.color.hue_velocity_drift = 0.004f;       // SLOW: translocation (ribosome ratchet)
  // Luminance: strong codon boundary contrast
  p.color.luma_shape_depth = 0.24f;          // strong: crisp reading frame boundaries
  p.color.luma_shape_center = 2.0f;

  // ── Seed: moderate — injects start codon (AUG) ──
  p.seed.border_width = 0.087f;              // moderate: ER junction boundaries
  p.seed.hue_speed = 0.096f;
  p.seed.alpha = 0.92f;

  p.speed = 0.40f;                           // moderate: watch translation unfold
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.165f;
  p.zn.zn0_anim_amp = 0.041f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.125f;
  p.zn.zn1_anim_amp = 0.043f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.134f;
  p.seed.pulse_amp = 0.20f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.079f,
                            .amplitude = 0.140f,
                            .offset = 0.500f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.071f,
                            .amplitude = 0.110f,
                            .offset = 0.200f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.049f,
                            .amplitude = 0.034f,
                            .offset = 0.096f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
