#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Hexagonal grain mosaic preset.
 *
 * PHYSICS: Crystallographic grain boundary dynamics / EBSD orientation imaging
 *
 * In materials science, polycrystalline metals and ceramics consist of many
 * crystalline GRAINS — regions of identical crystal orientation separated by
 * grain boundaries.  In hexagonal close-packed (HCP) materials (titanium,
 * magnesium, zinc, ice), the crystal lattice has 6-fold rotational symmetry
 * from the hexagonal unit cell.
 *
 * ENGINE MAPPING:
 *
 *   CRYSTAL SYMMETRY = z³ (Cube) inner function
 *     z³ has intrinsic 3-fold rotational symmetry: z³ is invariant under
 *     rotation by 2π/3.  Combined with a π/3 (60°) pre-rotation, the
 *     effective inner transform has the cubed pre-angle: 3×60° = 180°,
 *     which combines with z³'s 3-fold to produce HEXAGONAL (6-fold)
 *     visual symmetry — matching the HCP crystal lattice group.
 *
 *   PERIODIC BOUNDARY CONDITIONS = Torus reduction
 *     In solid-state physics, periodic (Born-von Karman) boundary conditions
 *     are the standard way to model infinite crystal lattices from finite
 *     simulation cells.  Torus reduction implements exactly this: seamless
 *     mirrored wrapping creates an infinite repeating crystal without edge
 *     artifacts.
 *
 *   GRAIN MISORIENTATIONS = irrational post-rotation (-ln(2) rad)
 *     ln(2) is transcendental (and thus incommensurate with any rational
 *     multiple of π).  This means successive feedback iterations rotate
 *     the crystal lattice by an angle that never aligns with the hexagonal
 *     symmetry axes.  Result: each "depth layer" in the feedback represents
 *     a DIFFERENT grain orientation — like looking at a polycrystalline
 *     cross-section where adjacent grains have random misorientations.
 *
 *   ORIENTATION IMAGING = Spiral color (mode 1)
 *     In real electron backscatter diffraction (EBSD), grains are colored
 *     by their crystallographic orientation: hue ∝ angle, depth ∝ grain
 *     size.  Mode 1 does exactly this:
 *       h_spiral = spiral_a × angle + spiral_r × log(r)
 *     where angle = polar angle (crystal orientation) and log(r) = depth
 *     (grain structure scale).  Each grain sector gets a unique hue.
 *
 *   GRAIN COARSENING = fade (0.985)
 *     At elevated temperatures, grain boundaries migrate to reduce total
 *     boundary energy.  Small grains shrink, large grains grow (Ostwald
 *     ripening).  Fade < 1 causes old, small structures to decay while
 *     newer, larger structures persist — the visual analog of coarsening.
 *
 * - Pre-rotation at π/3 (60°): cubed → 3×60°=180°; combined with z³'s
 *   intrinsic 3-fold → 6-fold hexagonal symmetry.
 *
 * - Post-rotation at -ln(2) ≈ -0.6931 rad: the natural logarithm of 2
 *   is transcendental (Lindemann-Weierstrass theorem) and represents the
 *   information content of a binary choice — fitting for grain boundaries
 *   which represent the binary decision "which lattice orientation won."
 *
 * - |zn0| = 0.82: conservative for Cube (since |z³| = |z|³, amplification
 *   at domain edge: 0.82³ = 0.551).  |zn2| = 0.98: near-unity for
 *   crystalline persistence.  Net: 0.82³ × 0.98 = 0.540.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern hexagonal_grain_mosaic() {
  // π/3 (60 deg): hexagonal lattice symmetry (cubed → 180°, × 3-fold = 6-fold)
  constexpr float C3  = 0.5f;            // cos(π/3)
  constexpr float S3  = 0.8660254038f;   // sin(π/3) = √3/2
  // -ln(2) ≈ -0.6931 rad: transcendental grain misorientation
  constexpr float CL2  = 0.7691765f;     // cos(ln(2))
  constexpr float SL2N = -0.6390124f;    // sin(-ln(2))

  constexpr float PRE  = 0.82f;  // conservative for z³ (0.82³ = 0.551)
  constexpr float POST = 0.98f;  // near-unity: crystalline persistence

  Pattern p;
  p.name = "hexagonal_grain_mosaic";
  p.inner = InnerFunction::Cube;         // z³ = hexagonal symmetry group
  p.reduction = ReductionType::Torus;    // periodic crystal boundary conditions

  p.zn.zn[0] = {C3 * PRE, S3 * PRE};
  p.zn.zn[1] = {0.006f, -0.004f};       // lattice imperfection (vacancy/impurity)
  p.zn.zn[2] = {CL2 * POST, SL2N * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 1 (Spiral) — orientation imaging microscopy ──
  p.color.hue_rotation = 0.009f;         // slow: let spiral color dominate
  p.color.sat_scale = 1.07f;             // boosted: crystal vibrancy
  p.color.fade = 0.985f;                 // moderate-high: crystalline persistence
  p.color.color_mode = as_int(ColorMode::Spiral);               // Spiral: grain orientation → hue
  p.color.spiral_angle_coeff = 1.20f;   // STRONG: each grain sector → unique hue
  p.color.spiral_radius_coeff = 0.40f;  // depth rings = grain size distribution
  // Mode 1 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.18f;     // grain boundary contrast
  p.color.luma_shape_center = 1.4f;

  // ── Seed: THERMAL ACTIVATION ──
  p.seed.border_width = 0.022f;         // moderate: thermal activation energy
  p.seed.hue_speed = 0.06f;             // slow: metallurgical timescale
  p.seed.alpha = 0.88f;
  p.speed = 0.50f;                      // slow: grain coarsening is gradual
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.129f;
  p.zn.zn0_anim_amp = 0.034f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.157f;
  p.zn.zn1_anim_amp = 0.044f;
  p.zn.zn1_anim_k = 6.0f;
  p.seed.pulse_freq = 0.123f;
  p.seed.pulse_amp = 0.15f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.079f,
                            .amplitude = 0.120f,
                            .offset = 0.400f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.105f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.085f,
                            .amplitude = 0.030f,
                            .offset = 0.060f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
