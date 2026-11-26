#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Cosine Feigenbaum period-doubling web preset.
 *
 * CHAOS THEORY: FEIGENBAUM UNIVERSALITY & PERIOD-DOUBLING
 *
 * In 1975, Mitchell Feigenbaum discovered one of the most remarkable
 * universal constants in mathematics.  When a smooth map with a single
 * quadratic maximum undergoes period-doubling bifurcations, the RATIO
 * of successive bifurcation intervals converges to a UNIVERSAL constant:
 *
 *   δ = lim_{n→∞} (r_n - r_{n-1}) / (r_{n+1} - r_n) ≈ 4.66920...
 *
 * This constant is UNIVERSAL — it appears in the logistic map, the sine
 * map, the cosine map, the Gaussian map, the Hénon map, even in physical
 * systems like dripping faucets, fluid convection, and electronic circuits.
 * Nature uses the SAME constant for ALL period-doubling routes to chaos.
 *
 * The second Feigenbaum constant α_F ≈ 2.5029... is the scaling ratio of
 * the bifurcation widths: how the "windows" of periodic behavior shrink
 * at each level of the cascade.
 *
 * The COSINE inner function is the perfect choice: cos(z) has a quadratic
 * maximum at z = 0 (cos(z) ≈ 1 - z²/2), making it a CANONICAL map for
 * period-doubling.  The iteration z → r·cos(z) undergoes the exact same
 * cascade as the logistic map, with the exact same δ and α_F.
 *
 * ENGINE MAPPING TO PERIOD-DOUBLING:
 *
 *   THE QUADRATIC MAXIMUM = Cos inner function
 *     cos(z) has a smooth maximum at z = 0, making every iteration a
 *     period-doubling candidate.  The NODAL LINES of cos (zeros at
 *     z = ±π/2 + nπ) create standing-wave patterns: bright regions
 *     where cos(z) ≈ 1, dark nodes where cos(z) = 0.  Under iteration,
 *     these nodes BIFURCATE — one node splits into two, then four, then
 *     eight — the period-doubling cascade made visible as fractal nodal
 *     line splitting.
 *
 *   THE BIFURCATION TREE = Grid reduction
 *     Grid tiles are the PARAMETER CELLS of the bifurcation diagram.
 *     Each tile represents a different "parameter value" r in the map
 *     z → r·cos(z).  The tile boundaries are where bifurcations OCCUR:
 *     crossing a boundary triggers a period-doubling.  The web of nodal
 *     lines within and between tiles IS the bifurcation tree — the
 *     iconic "Feigenbaum tree" rendered as fractal architecture.
 *
 *   29-FOLD QUASI-SYMMETRY = 1/δ ≈ 12.27° pre-rotation
 *     1/δ ≈ 0.2142 radians — the INVERSE of the first Feigenbaum constant.
 *     This is the "shrinkage factor" per bifurcation: each level of the
 *     cascade occupies 1/δ of the previous level's parameter range.
 *
 *     cos(1/δ) ≈ 0.9772, sin(1/δ) ≈ 0.2126
 *     360°/12.27° ≈ 29.3 → quasi-29-fold symmetry.
 *     29 is prime, a Lucas number (2,1,3,4,7,11,18,29,...), and the
 *     number of days in a lunar month.  Each of the ~29 sectors is one
 *     "level" of the cascade: the first sector is period-1, the second
 *     is period-2, the third period-4, etc.  After ~29 sectors, the
 *     pattern transitions from periodic to CHAOTIC — the onset of chaos
 *     at the edge of the cascade.
 *
 *   THE SCALING ANGLE = -arctan(α_F) ≈ -68.2° post-rotation
 *     cos = 1/√(1+α_F²) ≈ 0.3711
 *     sin = -α_F/√(1+α_F²) ≈ -0.9286
 *     The angle whose tangent IS the second Feigenbaum constant α_F.
 *     Each depth layer contracts by the bifurcation width ratio: looking
 *     deeper into the web = looking at finer bifurcation levels.
 *
 *   BREATHING = the period-doubling oscillation (Mode 2)
 *     The system oscillates between period-1 (steady state) and period-2
 *     (alternating).  The BREATH IS the bifurcation: when saturation
 *     peaks, the system is in period-1 (single attractor).  When it
 *     dips, the system has bifurcated to period-2 (two attractors).
 *     The breathing frequency IS the bifurcation rate.
 *
 * WHAT TO WATCH:
 *   1. NODAL LINES: cos zeros create standing-wave patterns that bifurcate
 *   2. 29-FOLD: quasi-29 bifurcation levels spiraling into chaos
 *   3. BREATHING: period-1 ↔ period-2 oscillation visible as pulsing
 *   4. BIFURCATION TREE: Grid boundaries = parameter-space transitions
 *   5. UNIVERSALITY: the SAME δ governs dripping faucets and this web
 *   6. EDGE OF CHAOS: where periodic order meets chaotic complexity
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern cos_feigenbaum_period_web() {
  // 1/δ ≈ 0.2142 rad (12.27°): inverse of the first Feigenbaum constant
  // The shrinkage factor per bifurcation level: each cascade step × 1/δ
  // 360°/12.27° ≈ 29.3 → quasi-29-fold bifurcation cascade structure
  constexpr float CF   =  0.9772f;           // cos(1/δ)
  constexpr float SF   =  0.2126f;           // sin(1/δ)
  // -arctan(α_F) ≈ -1.1905 rad (-68.2°): second Feigenbaum constant angle
  // α_F ≈ 2.5029: the universal bifurcation WIDTH scaling ratio
  constexpr float CFA  =  0.3711f;           // cos(arctan(α_F)) = 1/√(1+α_F²)
  constexpr float SFAN = -0.9286f;           // sin(-arctan(α_F)) = -α_F/√(1+α_F²)

  constexpr float PRE  = 1.4119f;  // moderate: stable with cos nodal dynamics
  constexpr float POST = 1.2067f;  // gentle contraction: product 0.686

  Pattern p;
  p.name = "cos_feigenbaum_period_web";
  p.inner = InnerFunction::Cos;              // quadratic maximum → period-doubling
  p.reduction = ReductionType::Grid;         // bifurcation cells: parameter space tiles

  p.zn.zn[0] = {CF * PRE, SF * PRE};
  p.zn.zn[1] = {0.0546f, -0.0392f};           // perturbation: seed the cascade
  p.zn.zn[2] = {CFA * POST, SFAN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — the period-doubling oscillation ──
  p.color.hue_rotation = 0.008f;             // moderate: cascade color evolution
  p.color.sat_scale = 1.10f;                 // moderate vibrancy
  p.color.fade = 0.988f;                     // deep: ~83 frame persistence
  p.color.color_mode = as_int(ColorMode::Breathing);                    // Breathing: period-1 ↔ period-2
  p.color.spiral_angle_coeff = 0.80f;        // strong: shows 29 cascade levels
  p.color.spiral_radius_coeff = 0.45f;       // moderate: depth rings
  // THE BIFURCATION BREATH: period-doubling visible as pulsing
  p.color.chroma_breath_amp = 0.32f;         // strong: visible oscillation amplitude
  p.color.chroma_breath_freq = 0.09f;        // slow: period of the driving parameter
  p.color.chroma_breath_phase = 2.5f;        // phase: asymmetric bifurcation threshold
  // Mode 2 does not use hue velocity params
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: strong nodal line contrast
  p.color.luma_shape_depth = 0.28f;          // strong: crisp standing-wave nodes
  p.color.luma_shape_center = 2.0f;

  // ── Seed: moderate — injects perturbation to trigger cascades ──
  p.seed.border_width = 0.079f;              // moderate: boundary-driven bifurcation
  p.seed.hue_speed = 0.120f;
  p.seed.alpha = 0.92f;

  p.speed = 0.45f;                           // moderate: watch the cascade unfold
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.183f;
  p.zn.zn0_anim_amp = 0.040f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.193f;
  p.zn.zn1_anim_amp = 0.041f;
  p.zn.zn1_anim_k = 2.0f;
  p.seed.pulse_freq = 0.075f;
  p.seed.pulse_amp = 0.20f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.073f,
                            .amplitude = 0.126f,
                            .offset = 0.450f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.079f,
                            .amplitude = 0.120f,
                            .offset = 0.320f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.056f,
                            .amplitude = 0.042f,
                            .offset = 0.120f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
