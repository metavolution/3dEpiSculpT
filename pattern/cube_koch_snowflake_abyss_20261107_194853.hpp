#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Cube Koch snowflake abyss preset.
 *
 * FRACTAL GEOMETRY & ICE CRYSTALLOGRAPHY: THE KOCH SNOWFLAKE
 *
 * In 1904, Helge von Koch constructed a curve of INFINITE LENGTH enclosing
 * FINITE AREA — the first published example of a continuous curve that is
 * nowhere differentiable.  The construction is deceptively simple:
 *
 *   Start with an equilateral triangle.
 *   On each side, replace the middle third with two sides of a smaller
 *   equilateral triangle (pointing outward).
 *   Repeat forever.
 *
 * After n iterations: 3 × 4^n sides, each of length s/3^n.
 * Perimeter: P_n = 3s × (4/3)^n → INFINITY.
 * Area: A = (2√3/5) × s² (FINITE, converges after ~6 iterations).
 *
 * THE FRACTAL DIMENSION:
 *
 *   d = log(4) / log(3) ≈ 1.26186...
 *
 * Each iteration replaces 1 segment with 4 segments at 1/3 scale.
 * The ratio log(4)/log(3) = 2·log₃(2) is PROVABLY IRRATIONAL — there
 * is no integer n,m such that 3^n = 4^m (since 3 is odd and 4 is even).
 * This irrationality means the fractal's self-similarity is APERIODIC:
 * zoom in by any ratio and you never see exact repetition, only
 * quasiperiodic echoes of the whole in every part.
 *
 * ICE CRYSTAL SYMMETRY:
 * The Koch snowflake has the same symmetry group as real snowflakes:
 * D₆ (the dihedral group of order 12 = 6 rotations + 6 reflections).
 * This is NOT a coincidence — water ice (ice Ih) crystallizes in a
 * HEXAGONAL lattice where each O atom makes 4 hydrogen bonds in
 * tetrahedral geometry, but the bulk crystal viewed along the c-axis
 * exhibits exact 6-fold rotational symmetry.
 *
 * Snowflake dendrite branching occurs because:
 *   - Faceted growth along the 6 prism faces is slow (bounded)
 *   - Tip growth is fast (unstable, Mullins-Sekerka instability)
 *   - Side-branches nucleate from perturbations on the main arms
 * This is a REACTION-DIFFUSION process: water vapor diffusion (field)
 * coupled with surface-energy-driven crystallization (reaction).
 *
 * ENGINE MAPPING TO THE KOCH SNOWFLAKE:
 *
 *   THE TRIPLING MAP = Cube inner function (z³)
 *     z³ is the GENERATOR of 3-fold rotational symmetry in ℂ.
 *     The cube roots of unity {1, ω, ω²} where ω = e^{2πi/3} form
 *     the vertices of an equilateral triangle — the SEED of the Koch
 *     construction.  Under iteration, z³ applied to a hexagonally-
 *     rotated input creates self-similar tripling at every depth level:
 *     each frame IS one Koch iteration, replacing structure with 3-fold
 *     refined sub-structure.
 *
 *     z³ = |z|³ · e^{i·3θ}: it TRIPLES the angle and CUBES the radius.
 *     Pre-rotation by π/3 (60°) gets tripled to π (180°).  Under Torus
 *     mirror reduction, 180° is a PERFECT BILATERAL REFLECTION — each
 *     hexagonal arm gains internal mirror symmetry.  The resulting
 *     symmetry group is EXACTLY D₆ = ⟨r₆₀, σ⟩: six rotations by 60°
 *     plus six reflection axes.  This IS the snowflake symmetry group.
 *
 *     ALGEBRAIC ELEGANCE: z³ maps the Eisenstein integers ℤ[ω] (the
 *     hexagonal lattice) to themselves.  The norm form N(a+bω) = a²-ab+b²
 *     is preserved under cubing.  Every point in the hexagonal lattice
 *     stays in the hexagonal lattice under z³ — the fractal respects
 *     the crystallography.
 *
 *   THE SEAMLESS CRYSTAL = Torus reduction
 *     Real ice crystals have no edges — the hexagonal facets wrap
 *     seamlessly around the crystal.  Torus reduction provides this:
 *     seamless mirror wrapping with no visible tile boundaries.  The
 *     abyss view IS looking along the c-axis of an infinite ice crystal,
 *     each depth layer a new Koch iteration of increasing complexity.
 *
 *   THE HEXAGONAL LATTICE = π/3 = 60° pre-rotation
 *     cos(π/3) = 1/2 (EXACT)
 *     sin(π/3) = √3/2 (EXACT)
 *
 *     60° is THE crystallographic angle of hexagonal symmetry:
 *       - Ice Ih basal plane: 60° between prism faces
 *       - Graphene C-C bond angles: 120° = 2×60°
 *       - Close-packed sphere layers: 60° stacking
 *       - Benzene: 60° internal angles
 *
 *     Under z³: 60° → 3×60° = 180° → Torus mirror → D₆ snowflake.
 *     Each of the 6 sectors IS one dendrite arm of the snowflake.
 *
 *   THE FRACTAL DIMENSION = -log₃(4) ≈ -72.28° post-rotation
 *     log₃(4) = log(4)/log(3) ≈ 1.2619 rad: the Koch fractal dimension.
 *     This is the number that encodes HOW MUCH more complex each Koch
 *     iteration becomes — the ratio of detail to scale.
 *
 *     cos(log₃4) ≈ 0.3034 (irrational)
 *     sin(-log₃4) ≈ -0.9529 (irrational)
 *
 *     Because log₃(4) is PROVABLY IRRATIONAL, the depth stacking is
 *     QUASIPERIODIC — each layer is offset by the fractal dimension
 *     itself, ensuring the abyss NEVER exactly repeats.  The fractal's
 *     self-similarity ratio IS the visual's depth rotation angle.
 *
 *     360°/72.28° ≈ 4.98 → quasi-5-fold depth symmetry.
 *     Combined with 6-fold azimuthal: 6 × 5 = 30 interference cells.
 *     30 = number of edges in an icosahedron, connecting hexagonal
 *     crystallography to icosahedral virus capsid geometry.
 *
 *   CRYSTALLOGRAPHIC COLORING = Mode 1 (Spiral)
 *     Mode 1 maps hue to angle + log(radius).  In the snowflake:
 *     - 6 angular sectors → 6 distinct hues → 6 dendrite arms
 *     - log(r) depth rings → crystal growth planes (basal layers)
 *     - The spiral IS the crystallographic Bragg diffraction pattern:
 *       concentric rings of constructive interference from the
 *       hexagonal lattice, colored by diffraction angle.
 *
 * WHAT TO WATCH:
 *   1. 6-FOLD SYMMETRY: exact D₆ snowflake — count the 6 dendrite arms
 *   2. BILATERAL ARMS: each arm has internal mirror symmetry (σ_v)
 *   3. FRACTAL DEPTH: each layer = one Koch iteration of refinement
 *   4. QUASIPERIODIC: layers never repeat (irrational depth angle)
 *   5. CRYSTAL RINGS: spiral coloring traces hexagonal growth planes
 *   6. BRANCHING: slight perturbation creates Mullins-Sekerka side-arms
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern cube_koch_snowflake_abyss() {
  // π/3 = 60° (0.5236 rad): THE hexagonal crystallographic angle
  // cos = 1/2 EXACT, sin = √3/2 EXACT — the most fundamental trig values
  // z³ triples 60° → 180° → Torus mirror → D₆ snowflake (order 12)
  // D₆ = ⟨r₆₀, σ⟩: the symmetry group of every snowflake ever formed
  constexpr float CH   =  0.5000f;           // cos(π/3) = 1/2
  constexpr float SH   =  0.8660f;           // sin(π/3) = √3/2
  // -log₃(4) ≈ -1.2619 rad (-72.28°): the Koch snowflake fractal dimension
  // d = log(4)/log(3): PROVABLY IRRATIONAL (3^n ≠ 4^m for any integers)
  // The self-similarity ratio of the Koch curve encoded as depth angle
  // Each depth layer rotates by the fractal dimension → APERIODIC stacking
  // quasi-5-fold depth: 360°/72.28° ≈ 4.98 → 6×5 = 30 interference cells
  constexpr float CKD  =  0.3034f;           // cos(log₃4)
  constexpr float SKDN = -0.9529f;           // sin(-log₃4)

  constexpr float PRE  = 0.94f;  // high: near kaleidoscope regime for rich z³ dynamics
  constexpr float POST = 0.88f;  // moderate contraction: product 0.827

  Pattern p;
  p.name = "cube_koch_snowflake_abyss";
  p.inner = InnerFunction::Cube;             // z³: 3-fold symmetry generator → D₆ snowflake
  p.reduction = ReductionType::Torus;        // seamless crystal: no facet boundaries

  p.zn.zn[0] = {CH * PRE, SH * PRE};
  p.zn.zn[1] = {0.005f, -0.003f};           // perturbation: Mullins-Sekerka dendrite nucleation
  p.zn.zn[2] = {CKD * POST, SKDN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 1 (Spiral) — crystallographic growth plane coloring ──
  p.color.hue_rotation = 0.008f;             // slow: crystal growth is meditative
  p.color.sat_scale = 1.12f;                 // moderate: clean crystalline clarity
  p.color.fade = 0.989f;                     // deep: ~91 frame persistence
  p.color.color_mode = as_int(ColorMode::Spiral);                    // Spiral: 6 dendrite arms + depth ring growth planes
  p.color.spiral_angle_coeff = 1.10f;        // STRONG: shows 6 hexagonal sectors distinctly
  p.color.spiral_radius_coeff = 0.50f;       // strong: crystal basal-plane depth rings
  // Mode 1 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: moderate crystal branch definition
  p.color.luma_shape_depth = 0.22f;          // moderate: soft hexagonal facet shading
  p.color.luma_shape_center = 1.8f;

  // ── Seed: minimal — fractal crystal growth dominates ──
  p.seed.border_width = 0.008f;              // very subtle: snowflake geometry IS the visual
  p.seed.hue_speed = 0.04f;                  // slow: frozen crystal contemplation
  p.seed.alpha = 0.86f;

  p.speed = 0.45f;                           // slow: watch Koch iterations accumulate
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.168f;
  p.zn.zn0_anim_amp = 0.041f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.181f;
  p.zn.zn1_anim_amp = 0.048f;
  p.zn.zn1_anim_k = 2.0f;
  p.seed.pulse_freq = 0.079f;
  p.seed.pulse_amp = 0.14f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.067f,
                            .amplitude = 0.140f,
                            .offset = 0.500f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.099f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.051f,
                            .amplitude = 0.030f,
                            .offset = 0.040f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
