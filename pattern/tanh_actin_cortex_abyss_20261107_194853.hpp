#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh actin cortex abyss preset.
 *
 * CELL BIOLOGY: THE ACTIN CYTOSKELETON & CORTICAL DYNAMICS
 *
 * The ACTIN CORTEX is a thin (~200 nm), dense meshwork of actin filaments
 * and myosin motors beneath the cell membrane.  It is the cell's
 * MECHANICAL SKELETON — responsible for cell shape, division, migration,
 * and force generation.  Without the cortex, cells would be formless bags
 * of cytoplasm.  Every time a cell divides, migrates, or changes shape,
 * the actin cortex is rebuilt from scratch in seconds.
 *
 * F-ACTIN (filamentous actin) is a DOUBLE HELIX:
 *   - Each strand is a head-to-tail polymer of G-actin monomers (~42 kDa)
 *   - Two strands wind around each other with a LEFT-HANDED twist
 *   - The helix has exactly 13 subunits per 6 complete turns (the 13/6 HELIX)
 *   - Crossover (half-repeat distance): ~36 nm
 *   - Rotation per subunit: 6×360°/13 = 2160°/13 ≈ 166.15° = 12π/13 rad
 *
 * The numbers 13 and 8 (the dimer-equivalent spacing in nm) are FIBONACCI
 * NUMBERS.  Like microtubules (13 protofilaments), the Fibonacci structure
 * arises from optimal packing of protein subunits on a cylindrical surface,
 * where the golden ratio φ = (1+√5)/2 minimizes inter-subunit strain.
 *
 * TREADMILLING: Actin filaments are POLAR.  The barbed (+) end grows at
 * ~1-2 μm/min while the pointed (-) end shrinks at ~0.3 μm/min.  At
 * steady state, the filament appears to MOVE through the cytoplasm like
 * a treadmill belt, even though individual monomers are relatively fixed.
 * This requires continuous ATP hydrolysis — a far-from-equilibrium process
 * driven by the free energy of nucleotide exchange.
 *
 * ENGINE MAPPING TO ACTIN DYNAMICS:
 *
 *   THE POLYMERIZATION SWITCH = Tanh inner function
 *     Actin exists in two states: G-actin (globular monomer, OFF) and
 *     F-actin (filamentous polymer, ON).  The transition is COOPERATIVE
 *     and SIGMOIDAL — above the critical concentration (~0.1 μM),
 *     polymerization switches ON explosively.  tanh's ±1 saturation IS
 *     this binary switch: -1 = G-actin pool, +1 = F-actin network.
 *     The dense filament web created by iterated tanh IS the cortical
 *     meshwork — a self-organizing tangle of cross-linked filaments.
 *
 *   THE CELL CORTEX = Torus reduction
 *     The cortex wraps the entire cell surface like a shell.  Torus
 *     reduction creates seamless wrapping — the cortex has no edges,
 *     no seams, no boundaries.  The tunnel view is looking THROUGH
 *     the cortical meshwork, deeper and deeper into the filament
 *     tangle — an abyss of interlocking protein helices.
 *
 *   THE 13/6 HELIX = 12π/13 ≈ 166.15° pre-rotation
 *     Each actin subunit is rotated 12π/13 radians from the previous.
 *     This near-180° rotation means adjacent subunits are nearly
 *     OPPOSITE — creating the characteristic double-helix appearance
 *     where two strands interleave.  Under iteration, the near-inversion
 *     creates BILATERAL PATTERNS with progressive twist — the actin
 *     double helix made visible as fractal interference.
 *
 *     cos(12π/13) = -cos(π/13) ≈ -0.9709
 *     sin(12π/13) = sin(π/13) ≈ 0.2393
 *
 *     This is the ONLY preset with cos < 0 for Tanh — each iteration
 *     nearly INVERTS the complex plane, then tanh folds the result back
 *     into [-1,+1].  The alternating near-inversion creates standing-
 *     wave interference between successive helix turns — pairs of
 *     iterations produce quasi-bilateral symmetry, reflecting actin's
 *     two-strand structure.
 *
 *   THE GOLDEN ROOT = -arctan(√5) ≈ -65.91° post-rotation
 *     √5 = φ + 1/φ = 2φ - 1: the golden ratio's ALGEBRAIC CORE.
 *     Actin's 13/6 Fibonacci helix is governed by φ, and √5 is the
 *     discriminant of the golden quadratic x² - x - 1 = 0.
 *
 *     cos(arctan(√5)) = 1/√6 ≈ 0.4082 (EXACT: 1² + (√5)² = 6)
 *     sin(-arctan(√5)) = -√5/√6 ≈ -0.9129 (EXACT)
 *
 *     The number 6 appears in BOTH places: the denominator √6 AND the
 *     actin helix (13/6 = 13 subunits per 6 turns).  1/√6 IS the
 *     ratio of unit to helix-length — the cosine of the depth angle
 *     LITERALLY encodes the helical geometry of F-actin.
 *     360°/65.91° ≈ 5.46 → quasi-5.5-fold depth symmetry.
 *
 *   THE DOUBLE HELIX COLORING = Mode 1 (Spiral)
 *     Mode 1 maps hue to angle + log(radius).  With the near-180°
 *     pre-rotation, the spiral coloring reveals the DOUBLE HELIX:
 *     two color bands winding in opposite directions, interleaving
 *     like the two strands of F-actin.  The log(radius) component
 *     shows TREADMILLING: radial depth rings that migrate as monomers
 *     cycle through the filament from barbed to pointed end.
 *
 * WHAT TO WATCH:
 *   1. DOUBLE HELIX: near-180° rotation → bilateral spiral patterns
 *   2. CORTICAL MESH: tanh ±1 folding = dense actin filament network
 *   3. TREADMILLING: depth rings shift as the filament "walks"
 *   4. FIBONACCI: 13/6 helix from golden-ratio protein packing
 *   5. √5 DEPTH: golden ratio's algebraic core structures each layer
 *   6. TUNNEL: looking through the cortex into the cell interior
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern tanh_actin_cortex_abyss() {
  // 12π/13 ≈ 2.9002 rad (166.15°): the actin 13/6 helix rotation per subunit
  // 13 subunits per 6 turns → 12π/13 rad rotation between adjacent monomers
  // cos < 0: UNIQUE near-inversion creates bilateral double-helix patterns
  constexpr float CA   = -0.9709f;           // cos(12π/13) = -cos(π/13)
  constexpr float SA   =  0.2393f;           // sin(12π/13) = sin(π/13)
  // -arctan(√5) ≈ -1.1503 rad (-65.91°): golden ratio's algebraic core
  // √5 = φ + 1/φ: discriminant of x²-x-1=0, the golden quadratic
  // cos = 1/√6 EXACT: 6 = number of turns in the 13/6 helix
  constexpr float CGR  =  0.4082f;           // cos(arctan(√5)) = 1/√6
  constexpr float SGRN = -0.9129f;           // sin(-arctan(√5)) = -√5/√6

  constexpr float PRE  = 1.06f;  // aggressive Tanh expansion: dense cortical web
  constexpr float POST = 0.86f;  // strong contraction: product 0.912

  Pattern p;
  p.name = "tanh_actin_cortex_abyss";
  p.inner = InnerFunction::Tanh;             // polymerization switch: G-actin ↔ F-actin
  p.reduction = ReductionType::Torus;        // cell cortex: seamless membrane wrapping

  p.zn.zn[0] = {CA * PRE, SA * PRE};
  p.zn.zn[1] = {0.003f, -0.004f};           // perturbation: thermal Brownian motion
  p.zn.zn[2] = {CGR * POST, SGRN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 1 (Spiral) — double helix winding coloring ──
  p.color.hue_rotation = 0.008f;             // moderate: treadmilling drift
  p.color.sat_scale = 1.12f;                 // moderate vibrancy: cytoplasmic warmth
  p.color.fade = 0.990f;                     // deep: ~100 frame persistence (stable cortex)
  p.color.color_mode = as_int(ColorMode::Spiral);                    // Spiral: hue traces double helix strands
  p.color.spiral_angle_coeff = 1.40f;        // VERY STRONG: shows bilateral helix winding
  p.color.spiral_radius_coeff = 0.50f;       // strong: treadmilling radial depth rings
  // Mode 1 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: strong filament contrast (bright F-actin, dark gaps)
  p.color.luma_shape_depth = 0.22f;          // moderate-strong: meshwork visibility
  p.color.luma_shape_center = 2.0f;

  // ── Seed: minimal — cortex self-assembles from tanh feedback ──
  p.seed.border_width = 0.008f;              // minimal: web dominates
  p.seed.hue_speed = 0.04f;                  // slow: cytoskeletal meditation
  p.seed.alpha = 0.86f;

  p.speed = 0.42f;                           // moderate: watch cortical remodeling
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.105f;
  p.zn.zn0_anim_amp = 0.049f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.180f;
  p.zn.zn1_anim_amp = 0.056f;
  p.zn.zn1_anim_k = 2.0f;
  p.seed.pulse_freq = 0.114f;
  p.seed.pulse_amp = 0.15f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.092f,
                            .amplitude = 0.140f,
                            .offset = 0.500f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.089f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.088f,
                            .amplitude = 0.030f,
                            .offset = 0.040f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
