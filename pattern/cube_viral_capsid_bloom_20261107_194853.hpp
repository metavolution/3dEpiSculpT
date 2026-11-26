#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Cube viral capsid bloom preset.
 *
 * STRUCTURAL VIROLOGY: ICOSAHEDRAL CAPSID SELF-ASSEMBLY
 *
 * Viruses solve an extraordinary engineering problem: enclosing a genome
 * in a protective protein shell using the MINIMUM amount of genetic
 * information.  Crick and Watson (1956) predicted that viral capsids must
 * use SYMMETRY to do this — identical protein subunits arranged in a
 * symmetric shell.  The ICOSAHEDRON emerged as nature's answer: 60
 * equivalent positions, the maximum symmetry achievable by identical
 * subunits on a closed surface.
 *
 * CASPAR-KLUG THEORY (1962, Nobel Prize 1982):
 * Larger viruses need more than 60 subunits.  Caspar and Klug showed
 * that capsids can accommodate 60×T subunits, where the TRIANGULATION
 * NUMBER T = h² + hk + k² (h,k non-negative integers):
 *
 *   T=1: 60 subunits in 12 pentamers (smallest, e.g. satellite tobacco necrosis)
 *   T=3: 180 subunits = 12 pentamers + 20 hexamers (e.g. tomato bushy stunt)
 *   T=4: 240 subunits (e.g. hepatitis B)
 *   T=7: 420 subunits (e.g. papillomavirus)
 *
 * The T=3 capsid has exactly 3 QUASI-EQUIVALENT protein chains (A, B, C)
 * in its asymmetric unit — three proteins that are chemically identical
 * but occupy slightly different environments.  This quasi-equivalence is
 * the MINIMAL DEVIATION from exact symmetry needed to close the shell.
 *
 * THE ICOSAHEDRON AND THE GOLDEN RATIO:
 * The icosahedron is BUILT from the golden ratio phi = (1+sqrt(5))/2:
 *   - Vertices lie on 3 mutually perpendicular GOLDEN RECTANGLES (phi x 1)
 *   - Edge length to circumradius: a/R = 1/sin(2*pi/5) = 2/sqrt(2+phi)
 *   - Dihedral angle: arccos(-sqrt(5)/3) ~ 138.19 degrees
 *   - 5-fold axes pass through opposite vertices (12 vertices, 6 axes)
 *   - 3-fold axes pass through opposite face centers (20 faces, 10 axes)
 *   - 2-fold axes pass through opposite edge midpoints (30 edges, 15 axes)
 *
 * ENGINE MAPPING TO CAPSID SELF-ASSEMBLY:
 *
 *   THE TRIMER UNIT = Cube (z^3) inner function
 *     z^3 creates EXACT 3-fold rotational symmetry.  In the T=3 capsid,
 *     the asymmetric unit contains exactly 3 quasi-equivalent protein
 *     chains (A, B, C).  z^3 IS this trimer: three identical nonlinear
 *     warps composed into one rotation.  Under iteration, the 3-fold
 *     creates interlocking trefoil patterns — the trimeric capsomers
 *     tiling the icosahedral surface.
 *
 *     z^3 TRIPLES the pre-rotation angle.  With pre = 2*pi/15 = 24 deg,
 *     z^3 produces 3 x 24 = 72 deg = 2*pi/5 — the PENTAGONAL axis of
 *     the icosahedron!  The 3-fold local trimer symmetry generates the
 *     5-fold global capsid symmetry through a single nonlinear step.
 *
 *   THE PROTEIN SHELL = Torus reduction
 *     The capsid is a CLOSED SURFACE — no edges, no seams.  Torus
 *     reduction provides seamless mirror wrapping, creating the smooth
 *     crystallographic symmetry documented in the design guide as the
 *     canonical Cube+Torus synergy.  The self-similar depth IS looking
 *     through nested shells — onion-layer matryoshka of capsid symmetry.
 *
 *   THE CAPSID LATTICE = 2*pi/15 = 24 deg pre-rotation
 *     15 = 3 x 5: the product of LOCAL trimer (3) and GLOBAL pentamer (5)
 *     symmetry orders.  This is the fundamental factorization of
 *     icosahedral quasi-equivalence.  z^3 triples 24 to 72 = 360/5,
 *     producing pentagonal emergent symmetry from trimeric input.
 *
 *     cos(2*pi/15) ~ 0.9135, sin(2*pi/15) ~ 0.4067
 *
 *     360/24 = 15 input sectors, but z^3 maps each group of 3 to one
 *     output sector -> 5-fold output.  15 total interference sectors =
 *     the number of 2-fold symmetry axes of the icosahedron (30 edges,
 *     15 edge-pairs), confirming the crystallographic mapping.
 *
 *   THE GOLDEN COMPLEMENT = -arctan(1/phi) ~ -31.72 deg post-rotation
 *     arctan(1/phi) + arctan(phi) = pi/2 (EXACT complementarity).
 *
 *     cos(arctan(1/phi)) = phi/sqrt(2+phi) ~ 0.8507
 *     sin(-arctan(1/phi)) = -1/sqrt(2+phi) ~ -0.5257
 *
 *     These are the EXACT COMPLEMENTS of the mitotic spindle aster
 *     preset (which uses arctan(phi)):
 *       spindle: cos = 1/sqrt(2+phi), sin = -phi/sqrt(2+phi)
 *       capsid:  cos = phi/sqrt(2+phi), sin = -1/sqrt(2+phi)
 *
 *     This is NOT coincidence.  Both structures use FIBONACCI GEOMETRY:
 *       - Microtubules: 13 protofilaments (Fibonacci) in a cylinder
 *       - Viral capsids: icosahedral symmetry built from phi
 *     arctan(phi) = INTERIOR view (looking along a filament into depth)
 *     arctan(1/phi) = SURFACE view (looking at the shell from outside)
 *     The complementary angles encode the duality between the INTERIOR
 *     structure (cytoskeleton) and SURFACE structure (viral shell) of
 *     biological self-assembly.
 *
 *     360/31.72 ~ 11.35 -> quasi-11-fold depth symmetry.
 *     Combined with 15-fold azimuthal: 15 x 11.3 ~ 170 interference
 *     cells.  170 ~ 180 - 10: nearly the T=3 capsid subunit count (180),
 *     minus the 10 three-fold axes of the icosahedron.
 *
 *   THREE ASSEMBLY TIMESCALES = Mode 3 (Alive)
 *     Capsid assembly proceeds through three kinetic phases:
 *     base  = 0.022 -> nucleation: FAST (initial trimer/pentamer docking)
 *     breath = 0.012 -> elongation: MEDIUM (shell growth by subunit addition)
 *     drift  = 0.003 -> maturation: SLOW (conformational rearrangement/cement)
 *     These correspond to the three phases observed in light-scattering
 *     kinetics of in-vitro capsid assembly.
 *
 * WHAT TO WATCH:
 *   1. PENTAGONAL BLOOMS: 5-fold floral symmetry emerging from z^3 trimers
 *   2. TREFOIL CAPSOMERS: 3-fold local patterns = trimeric protein units
 *   3. GOLDEN DEPTH: phi-encoded layer stacking (icosahedral constant)
 *   4. SMOOTH SHELL: Torus + Cube = seamless crystallographic tiling
 *   5. TRIPLE SHIMMER: nucleation/elongation/maturation dynamics
 *   6. COMPLEMENT DUALITY: arctan(1/phi) mirrors the spindle's arctan(phi)
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern cube_viral_capsid_bloom() {
  // 2*pi/15 = 24 deg (0.4189 rad): icosahedral capsid lattice
  // 15 = 3x5: trimer symmetry x pentamer symmetry
  // z^3 triples 24 to 72 = 2*pi/5 -> PENTAGONAL emergent symmetry
  // 15 sectors = 15 two-fold axes of the icosahedron (edge midpoints)
  constexpr float CV   =  0.9135f;           // cos(2*pi/15)
  constexpr float SV   =  0.4067f;           // sin(2*pi/15)
  // -arctan(1/phi) ~ -0.5536 rad (-31.72 deg): golden complement
  // cos = phi/sqrt(2+phi): the SURFACE view of Fibonacci geometry
  // COMPLEMENT of spindle aster's arctan(phi): interior vs surface duality
  // arctan(phi) + arctan(1/phi) = pi/2 EXACT — biological self-assembly pair
  constexpr float CGC  =  0.8507f;           // cos(arctan(1/phi)) = phi/sqrt(2+phi)
  constexpr float SGCN = -0.5257f;           // sin(-arctan(1/phi)) = -1/sqrt(2+phi)

  constexpr float PRE  = 0.88f;  // moderate: z^3 triples rotation, needs care
  constexpr float POST = 0.90f;  // moderate contraction: product 0.792

  Pattern p;
  p.name = "cube_viral_capsid_bloom";
  p.inner = InnerFunction::Cube;             // T=3 trimer: 3 quasi-equivalent chains
  p.reduction = ReductionType::Torus;        // closed protein shell: seamless capsid surface

  p.zn.zn[0] = {CV * PRE, SV * PRE};
  p.zn.zn[1] = {0.004f, -0.003f};           // perturbation: thermal subunit jostling
  p.zn.zn[2] = {CGC * POST, SGCN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // -- Color: MODE 3 (Alive) -- three capsid assembly kinetic phases --
  p.color.hue_rotation = 0.012f;             // moderate: assembly cycle drift
  p.color.sat_scale = 1.14f;                 // vivid: fluorescent capsid markers
  p.color.fade = 0.988f;                     // deep: ~83 frame persistence (stable shell)
  p.color.color_mode = as_int(ColorMode::Alive);                    // Alive: nucleation/elongation/maturation
  p.color.spiral_angle_coeff = 1.10f;        // STRONG: reveals 5-fold pentagonal blooms
  p.color.spiral_radius_coeff = 0.45f;       // moderate: capsid layer depth rings
  // Capsid breathing: conformational fluctuation observed in cryo-EM
  p.color.chroma_breath_amp = 0.18f;         // moderate: shell breathing amplitude
  p.color.chroma_breath_freq = 0.10f;        // conformational oscillation rate
  p.color.chroma_breath_phase = 0.3f;        // subunit phase coupling
  // THREE ASSEMBLY TIMESCALES
  p.color.hue_velocity_base = 0.022f;        // FAST: nucleation (trimer/pentamer docking)
  p.color.hue_velocity_breath = 0.012f;      // MEDIUM: elongation (shell growth)
  p.color.hue_velocity_drift = 0.003f;       // SLOW: maturation (post-assembly cement)
  // Luminance: capsomer boundary contrast
  p.color.luma_shape_depth = 0.22f;          // moderate-strong: trefoil capsomer edges
  p.color.luma_shape_center = 1.8f;

  // -- Seed: moderate -- injects initial nucleation event --
  p.seed.border_width = 0.012f;              // narrow: shell self-assembly dominates
  p.seed.hue_speed = 0.06f;                  // moderate: capsid maturation color drift
  p.seed.alpha = 0.88f;

  p.speed = 0.52f;                           // moderate: watch pentagonal blooms emerge
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.172f;
  p.zn.zn0_anim_amp = 0.035f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.120f;
  p.zn.zn1_anim_amp = 0.040f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.136f;
  p.seed.pulse_amp = 0.19f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.063f,
                            .amplitude = 0.126f,
                            .offset = 0.450f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.105f,
                            .amplitude = 0.103f,
                            .offset = 0.180f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.076f,
                            .amplitude = 0.030f,
                            .offset = 0.060f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
