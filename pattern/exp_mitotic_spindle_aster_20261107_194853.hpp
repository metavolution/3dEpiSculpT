#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential mitotic spindle aster preset.
 *
 * CELL BIOLOGY: MICROTUBULE DYNAMIC INSTABILITY & THE MITOTIC SPINDLE
 *
 * During cell division, the MITOTIC SPINDLE orchestrates chromosome
 * segregation — one of the most spectacular molecular machines in nature.
 * Two ASTERS (radial arrays of microtubules) grow outward from centrosomes,
 * their filaments searching for chromosomes via "search and capture."
 *
 * Each MICROTUBULE is a hollow cylinder of tubulin dimers (8 nm long, 5 nm
 * wide), arranged in exactly 13 PROTOFILAMENTS running parallel to the tube
 * axis.  These 13 protofilaments form a LEFT-HANDED 3-start HELIX:
 *
 *   13 protofilaments × 3-start helix × 8 nm rise per dimer
 *
 * 13 and 3 are CONSECUTIVE FIBONACCI NUMBERS (1,1,2,3,5,8,13,21,...).
 * The 8 nm dimer length and 5 nm width are ALSO Fibonacci numbers.  This
 * Fibonacci structure is NOT coincidence — it arises from the packing
 * geometry of protein subunits on a cylindrical surface, where the golden
 * ratio φ = (1+√5)/2 minimizes lattice strain.
 *
 * DYNAMIC INSTABILITY: Microtubules alternate stochastically between:
 *   - GROWTH (polymerization): ~1-2 μm/min, GTP-tubulin addition
 *   - CATASTROPHE: sudden switch to rapid shrinkage (~10-20 μm/min)
 *   - RESCUE: switch back from shrinkage to growth
 * This far-from-equilibrium process, powered by GTP hydrolysis, allows
 * microtubules to EXPLORE SPACE efficiently.  The "search and capture"
 * mechanism finds chromosomes in ~15 minutes — far faster than diffusion
 * alone — because dynamic instability is a BIASED RANDOM SEARCH.
 *
 * ENGINE MAPPING TO MICROTUBULE DYNAMICS:
 *
 *   THE ASTER = Exp inner function
 *     Exp maps Cartesian → polar: straight lines become concentric circles,
 *     vertical lines become radial rays.  This IS the aster: radial
 *     microtubules emanating from a central point (centrosome), each one
 *     an exponentially growing filament.  The exponential growth of Exp
 *     mirrors the polymerization rate: length grows as L(t) = v_g × t
 *     (linear in time, but exponential in the feedback loop because each
 *     iteration compounds the previous).
 *
 *   THE CELL MEMBRANE = Torus reduction
 *     Torus reduction provides seamless mirror wrapping — the cell
 *     boundary.  Microtubules that grow beyond the cell cortex are
 *     reflected back (cortical rescue/catastrophe), creating the
 *     characteristic aster-within-a-sphere geometry.  The tunnel
 *     view IS looking along a microtubule toward the centrosome.
 *
 *   13 PROTOFILAMENTS = 2π/13 ≈ 27.69° pre-rotation
 *     2π/13 gives EXACT 13-fold rotational symmetry.  Each of the 13
 *     sectors IS one protofilament, viewed end-on.  The 3-start helix
 *     means every 3rd protofilament aligns — look for 3 dominant spiral
 *     arms within the 13-fold pattern.
 *
 *     cos(2π/13) ≈ 0.8855, sin(2π/13) ≈ 0.4647
 *     13 is: the 7th prime, a Fibonacci number, a happy number,
 *     a star number (S₃ = 13), and the number of Archimedean solids.
 *
 *   THE FIBONACCI DEPTH = -arctan(φ) ≈ -58.28° post-rotation
 *     arctan(φ) is the angle whose tangent IS the golden ratio.
 *     Using the identity φ² = φ + 1:
 *
 *       1 + tan²(θ) = 1 + φ² = 2 + φ = 3.618...
 *       cos(arctan(φ)) = 1/√(2+φ) ≈ 0.5257
 *       sin(arctan(φ)) = φ/√(2+φ) ≈ 0.8507
 *
 *     Each depth layer rotates by arctan(φ) — the vertical:horizontal
 *     ratio at each level IS the golden ratio.  This creates the tightest
 *     possible spiral stacking, echoing the Fibonacci helix of the
 *     microtubule itself.
 *
 *     360°/58.28° ≈ 6.18 → quasi-6-fold depth symmetry.
 *     6 is a PERFECT NUMBER (6 = 1+2+3 = 1×2×3).  Combined with
 *     13-fold azimuthal: 13 × 6.18 ≈ 80 interference cells — a dense
 *     aster of interlocking Fibonacci spirals.
 *
 *   THE PROTOFILAMENT HELIX = Mode 1 (Spiral) coloring
 *     Mode 1 maps hue to angle + log(radius).  In the aster, this
 *     traces the HELICAL TRACKS along which motor proteins (kinesin,
 *     dynein) walk.  Each spiral color band IS one protofilament track,
 *     and the 13 bands create 13 spiral railways for molecular cargo.
 *
 * WHAT TO WATCH:
 *   1. 13-FOLD ASTER: 13 protofilament rays radiating from center
 *   2. 3-START HELIX: 3 dominant spiral arms within the 13-fold pattern
 *   3. FIBONACCI DEPTH: golden-ratio stacking → never-repeating layers
 *   4. EXPONENTIAL GROWTH: each ray diverges like polymerizing tubulin
 *   5. SPIRAL TRACKS: Mode 1 color traces motor protein pathways
 *   6. TUNNEL: looking along a microtubule toward the centrosome
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern exp_mitotic_spindle_aster() {
  // 2π/13 ≈ 0.4833 rad (27.69°): exact 13-fold protofilament symmetry
  // 13 is Fibonacci — the lattice geometry of tubulin protein packing
  // 3-start helix: every 3rd of 13 protofilaments aligns helically
  constexpr float CM   =  0.8855f;           // cos(2π/13)
  constexpr float SM   =  0.4647f;           // sin(2π/13)
  // -arctan(φ) ≈ -1.0172 rad (-58.28°): golden ratio depth rotation
  // φ² = φ+1 → 1+φ² = 2+φ → cos = 1/√(2+φ), sin = φ/√(2+φ)
  // Fibonacci spiral stacking: each depth layer offset by arctan(φ)
  constexpr float CFI  =  0.5257f;           // cos(arctan(φ)) = 1/√(2+φ)
  constexpr float SFIN = -0.8507f;           // sin(-arctan(φ)) = -φ/√(2+φ)

  constexpr float PRE  = 0.86f;  // HIGH for Exp: dense aster filaments
  constexpr float POST = 0.91f;  // moderate contraction: product 0.783

  Pattern p;
  p.name = "exp_mitotic_spindle_aster";
  p.inner = InnerFunction::Exp;              // radial aster: exponential polymerization
  p.reduction = ReductionType::Torus;        // cell membrane: seamless boundary wrapping

  p.zn.zn[0] = {CM * PRE, SM * PRE};
  p.zn.zn[1] = {0.004f, -0.003f};           // perturbation: break exact symmetry (thermal noise)
  p.zn.zn[2] = {CFI * POST, SFIN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 1 (Spiral) — protofilament helix tracks ──
  p.color.hue_rotation = 0.010f;             // moderate: mitotic cycle drift
  p.color.sat_scale = 1.14f;                 // vivid: fluorescent tubulin markers
  p.color.fade = 0.990f;                     // deep: ~100 frame persistence (stable aster)
  p.color.color_mode = as_int(ColorMode::Spiral);                    // Spiral: hue traces helical motor tracks
  p.color.spiral_angle_coeff = 1.30f;        // STRONG: shows 13 protofilament sectors
  p.color.spiral_radius_coeff = 0.55f;       // strong: aster radial depth rings
  // Mode 1 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: strong filament contrast (bright microtubules, dark gaps)
  p.color.luma_shape_depth = 0.24f;          // strong: crisp filament visibility
  p.color.luma_shape_center = 2.0f;

  // ── Seed: moderate — injects centrosome nucleation ──
  p.seed.border_width = 0.010f;              // moderate: cortical boundary visible
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.86f;

  p.speed = 0.46f;                           // moderate: watch polymerization unfold
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.117f;
  p.zn.zn0_anim_amp = 0.046f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.173f;
  p.zn.zn1_anim_amp = 0.054f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.099f;
  p.seed.pulse_amp = 0.17f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.092f,
                            .amplitude = 0.154f,
                            .offset = 0.550f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.120f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.063f,
                            .amplitude = 0.030f,
                            .offset = 0.050f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
