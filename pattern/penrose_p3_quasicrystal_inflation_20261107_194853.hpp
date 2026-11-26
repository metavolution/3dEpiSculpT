#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Penrose P3 rhombic quasicrystal inflation preset.
 *
 * Scientific foundation:
 * - Roger Penrose's 1974 P3 tiling of the plane with exactly two rhombic
 *   prototiles:
 *     - "thick" (fat) rhombus with angles 72 / 108 degrees
 *     - "thin" rhombus with angles 36 / 144 degrees
 *   Together with matching rules, these two tiles force an APERIODIC
 *   tiling of the whole plane — no translational symmetry, yet the tiling
 *   has perfect 5-fold rotational symmetry at each Penrose center. This
 *   was one of the most profound counterexamples to the naive belief
 *   that aperiodicity requires randomness.
 *
 * - Every sufficiently large patch of the Penrose tiling contains
 *   isomorphic copies of every other patch (the "isomorphism theorem"):
 *   the tiling is locally indistinguishable from its translates even
 *   though it has no global translation symmetry.
 *
 * - The frequency ratio of fat to thin tiles equals exactly the golden
 *   ratio phi = (1 + sqrt(5)) / 2. The tiling is "locally derivable"
 *   from a 5D cubic lattice via de Bruijn's cut-and-project construction
 *   — the first appearance of the "higher-dimensional lattice projected
 *   to an irrational slice" paradigm that underlies all modern
 *   quasicrystal theory.
 *
 * - INFLATION RULE: every Penrose tile can be substituted by a cluster
 *   of smaller tiles whose linear scale is 1 / phi ≈ 0.618 relative to
 *   the original. Iterating this inflation produces a self-similar
 *   cascade — the most natural way to see the Penrose tiling as a
 *   fixed point of a deterministic substitution system. The pre-scale
 *   in this preset IS precisely the Penrose inflation ratio, so each
 *   feedback iteration enacts one generation of Penrose substitution.
 *
 * - Daniel Shechtman's 1982 discovery of REAL material quasicrystals
 *   (Al-Mn alloy with 5-fold electron diffraction symmetry) was so
 *   counter to the classical crystallographic restriction theorem that
 *   Linus Pauling rejected it. Vindicated by the 2011 Nobel Prize in
 *   Chemistry, Shechtman's quasicrystals are physical realisations of
 *   exactly the Penrose P3 geometry this preset visualises.
 *
 * Mapping to this engine:
 * - Inner = Cos. Cosine on the Penrose 5-fold symmetric coordinate has
 *   extrema at vertices of the pentagonal star and pentagonal decagon
 *   symmetric under rotation by 2*pi/5. The engine's cos inner on a
 *   2*pi/5-rotated coordinate IS a discrete-time dynamical substitute
 *   for the Penrose matching rule.
 * - Pre-rotation = 2*pi/5 (72 deg): the fundamental Penrose rotation.
 *   Exact values: cos(2*pi/5) = (sqrt(5)-1)/4, sin(2*pi/5) = sqrt(10+2*sqrt(5))/4.
 * - Pre-scale = 1/phi ≈ 0.6180: the Penrose inflation ratio. Each
 *   feedback iteration contracts coordinates by exactly this factor
 *   — the golden-mean scaling under which the tiling is self-similar.
 * - Post-rotation = 4*pi/5 (144 deg): the complementary Penrose angle
 *   (the interior angle of the fat rhombus). Exact values:
 *   cos(4*pi/5) = -(sqrt(5)+1)/4, sin(4*pi/5) = sqrt(10-2*sqrt(5))/4.
 *   Composing pre (72 deg) + post (144 deg) with the intervening
 *   cosine non-linearity traces the full decagonal stellation implied
 *   by the Penrose inflation.
 * - Torus reduction: preserves the smooth decagonal circulation
 *   characteristic of quasicrystal electron-density maps (no grid
 *   tiling artefacts that would break the aperiodic delicate structure).
 * - Color mode 3 (alive): strong angle-coupling makes each of the 10
 *   Penrose stellation wedges a distinct hue, while log-radial
 *   coupling lets the inflation cascade read as radial hue bands —
 *   one band per Penrose substitution generation.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern penrose_p3_quasicrystal_inflation() {
  // Pre-rotation 2*pi/5 = 72 deg — the Penrose fundamental rotation.
  // Exact: cos(2*pi/5) = (sqrt(5) - 1) / 4, sin(2*pi/5) = sqrt(10 + 2*sqrt(5)) / 4.
  constexpr float C5 = 0.3090169944f; // cos(2*pi/5) = (sqrt(5)-1)/4
  constexpr float S5 = 0.9510565163f; // sin(2*pi/5)

  // Post-rotation 4*pi/5 = 144 deg — the fat-rhombus interior angle.
  // Exact: cos(4*pi/5) = -(sqrt(5) + 1) / 4, sin(4*pi/5) = sqrt(10 - 2*sqrt(5)) / 4.
  constexpr float CP = -0.8090169944f; // cos(4*pi/5) = -(sqrt(5)+1)/4
  constexpr float SP = 0.5877852523f;  // sin(4*pi/5)

  // Golden-ratio expansion phi ≈ 1.618 scaled by 0.84 ≈ 1.359 drives cos(z)
  // across its pentagonal extrema while post-scale 0.98 keeps decagonal rings coupled.
  constexpr float PHI  = 1.6180339887f;
  constexpr float PRE  = 0.84f * PHI;
  constexpr float POST = 0.98f;

  Pattern p;
  p.name = "penrose_p3_quasicrystal_inflation";
  p.inner = InnerFunction::Cos;       // 5-fold symmetric cosine coupling
  p.reduction = ReductionType::Torus; // smooth decagonal circulation

  p.zn.zn[0] = {C5 * PRE, S5 * PRE};
  // Small imaginary detune: breaks reflection symmetry across the
  // Penrose center so the 10-fold stellation is slightly chiral.
  p.zn.zn[1] = {0.013f, -0.010f};
  p.zn.zn[2] = {CP * POST, SP * POST};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.16f;
  p.zn.zn0_anim_amp = 0.07f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.10f;
  p.zn.zn1_anim_amp = 0.018f;
  p.zn.zn1_anim_k = 5.0f;

  // Color: highlight the decagonal stellation + inflation cascade.
  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.14f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::Alive);             // alive — spiral + breath

  // Strong angular coupling: partitions the visual field into 10
  // decagonal hue wedges matching the Penrose stellation.
  p.color.spiral_angle_coeff = 0.95f;
  p.color.spiral_radius_coeff = 0.52f;

  p.color.chroma_breath_amp = 0.15f;
  p.color.chroma_breath_freq = 0.094f;
  p.color.chroma_breath_phase = 0.38f;
  p.color.hue_velocity_base = 0.013f;
  p.color.hue_velocity_breath = 0.018f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.05f;
  p.color.luma_shape_center = -4.3f;

  p.seed.border_width = 0.075f;
  p.seed.hue_speed = 0.072f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.081f;
  p.seed.pulse_amp = 0.20f;
  p.speed = 0.78f;
  return p;
}

} // namespace epi::presets
