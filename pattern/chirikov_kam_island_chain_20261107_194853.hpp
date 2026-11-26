#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Chirikov standard map / KAM island chain preset.
 *
 * Scientific foundation:
 * - The Chirikov standard map (Boris Chirikov, 1969) is the canonical model
 *   of Hamiltonian chaos. It describes a periodically kicked rotor:
 *       p_{n+1} = p_n + K sin(theta_n)
 *       theta_{n+1} = theta_n + p_{n+1}
 *   This is EXACTLY a composition of sin(z) with affine maps — the precise
 *   structure of this engine's per-pixel transform.
 *
 * - KAM theory (Kolmogorov 1954, Arnold 1963, Moser 1962) proves that at
 *   moderate coupling K, the phase portrait contains a hierarchy of structure:
 *   * INVARIANT TORI: smooth closed curves (KAM tori) where the rotation
 *     number is sufficiently irrational. These appear as concentric color
 *     bands in the feedback visualization.
 *   * ISLAND CHAINS: rings of n elliptic fixed points at each rational
 *     rotation number p/q. These appear as chains of bright spots.
 *   * CHAOTIC SEA: ergodic regions between broken tori where orbits mix.
 *     These appear as textured, flowing regions.
 *
 * - The critical coupling K_c ≈ 0.9716 marks the breaking of the last
 *   (golden-mean) KAM torus. At |zn0| = 0.97, we sit precisely at this
 *   threshold — the most beautiful coexistence of order and chaos.
 *
 * - Post-rotation at 2*pi/phi (phi = golden ratio) encodes the golden-mean
 *   winding number (sqrt(5)-1)/2 — the most irrational number and hence
 *   the rotation number of the last KAM torus to survive.
 *
 * Mapping to this engine:
 * - Sin inner IS the standard map's kick function
 * - |zn0| = 0.97 IS the critical coupling parameter K_c
 * - Post-rotation at golden angle IS the golden-mean winding number
 * - Torus reduction IS the phase space topology T^2
 * - Alive color reveals the island chain topology through angle→hue mapping
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern chirikov_kam_island_chain() {
  // Post-rotation: 2*pi*(sqrt(5)-1)/2 = 2*pi/phi ≈ 222.49 deg (golden rotation)
  // This is the winding number of the last surviving KAM torus.
  constexpr float CG = -0.7373688781f; // cos(2*pi/phi)
  constexpr float SG = -0.6754902943f; // sin(2*pi/phi)

  // K_c ≈ 0.9716: critical coupling where the golden KAM torus breaks
  constexpr float K_CRIT = 1.4514f;
  constexpr float POST   = 1.2592f;

  Pattern p;
  p.name = "chirikov_kam_island_chain";
  p.inner = InnerFunction::Sin;         // sin IS the standard map's kick
  p.reduction = ReductionType::Torus;   // T^2 phase space topology

  // Pre-affine: pure scaling at critical coupling, no pre-rotation.
  // The real-valued pre-scale controls the effective kick strength K.
  // At K = 0.97, we're at the exact threshold of global chaos onset.
  p.zn.zn[0] = {K_CRIT, 0.0f};
  p.zn.zn[1] = {0.018f, -0.011f};      // slight asymmetry breaks exact symmetry
  // Post-affine: golden-mean rotation with mild contraction.
  // The golden rotation ensures the last KAM torus is visible.
  p.zn.zn[2] = {CG * POST, SG * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.010f;
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.984f;
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: phase portrait dynamics
  // Strong angle→hue reveals island chains as color-distinct spots.
  // In the standard map, island chains at rotation number p/q appear
  // at specific angular positions — strong angle coupling makes them visible.
  p.color.spiral_angle_coeff = 0.85f;
  // Moderate radius→hue reveals KAM tori as concentric color bands.
  // Each torus has a unique rotation number → unique log-radius → unique color.
  p.color.spiral_radius_coeff = 0.40f;
  p.color.chroma_breath_amp = 0.14f;    // breathing envelope
  p.color.chroma_breath_freq = 0.10f;   // slow oscillation
  p.color.chroma_breath_phase = 0.33f;  // spatial phase offset
  p.color.hue_velocity_base = 0.013f;
  p.color.hue_velocity_breath = 0.016f; // breath-dominant → tori appear to pulse
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.20f;     // luminance rings enhance orbital structure
  p.color.luma_shape_center = 1.6f;

  p.seed.border_width = 0.080f;
  p.seed.hue_speed = 0.060f;
  p.seed.alpha = 0.92f;
  p.speed = 0.72f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.188f;
  p.zn.zn0_anim_amp = 0.051f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.130f;
  p.zn.zn1_anim_amp = 0.050f;
  p.zn.zn1_anim_k = 5.0f;
  p.seed.pulse_freq = 0.110f;
  p.seed.pulse_amp = 0.19f;

  return p;
}

} // namespace epi::presets
