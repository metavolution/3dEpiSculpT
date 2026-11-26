#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Sturmian quasicrystal: two warps braided by the Fibonacci word
 *
 * Generator A = sin(z)cosh(z), generator B = z cos(z) in a nearby rotated frame
 * (commutator 0.56); the
 * Fibonacci (Sturmian) word ABAAB... with hold 8 interleaves them aperiodically, so the
 * Grid-folded attractor never repeats yet stays ordered: a honeycomb form constant that
 * breathes as letters change. Census hole H6 (2-letter semigroup words).
 *
 * Discovered by tools/discover_subspaces (MAP-Elites), promoted by
 * tools/promote_discovery_draft.py. Discovery metrics:
 *
 * Cell: aq12:f2/HoneycombLattice/sturmian_fibonacci   lineage: milankovitch_waterfall_stratigraphy   born: iteration 447
 * Score 0.971053429 = fitness 0.860306099 + novelty 0.316420943
 *   edge_of_chaos 0.886792169, lyapunov 0.999980576 (lambda_med 0.00154256056), memory 0.844249756, spectral 0.962269508,
 *   form 0.668309341, semigroup 1 (commutator 0.557361269, word_fill 0.5859375)
 *
 * Generator A: sin(z)·cosh(z)
 * Generator B: z·cos(z)  word sturmian_fibonacci, hold 8
 *
 * Passed: native admission, static Calm motion-safety passport.
 */
inline Pattern sturmian_honeycomb_quasicrystal_breath() {
  Pattern p;
  p.name = "sturmian_honeycomb_quasicrystal_breath";
  p.inner = static_cast<InnerFunction>(19); // sin(z)·cosh(z)
  p.reduction = static_cast<ReductionType>(2);
  p.zn.zn[0] = {-0.353274912f, -0.983915806f};
  p.zn.zn[1] = {-0.00290399045f, 0.0430345647f};
  p.zn.zn[2] = {-0.189831078f, 0.971881688f};
  p.zn.zn[3] = {0.0275717229f, 0.0149955284f};
  p.zn.zn0_animate = false; p.zn.zn0_anim_speed = 0.00499999989f; p.zn.zn0_anim_amp = 0.100000001f;
  p.zn.zn1_animate = false; p.zn.zn1_anim_speed = 0.00400000019f; p.zn.zn1_anim_amp = 0.100000001f; p.zn.zn1_anim_k = 5.0f;
  p.color.hue_rotation = -0.00332197407f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.987806141f;
  p.color.use_hsls = false;
  p.color.distance_modulate = true;
  p.color.color_mode = presets::as_int(presets::ColorMode::Classic);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.100000001f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.000600000028f;
  p.color.hue_velocity_drift = 0.000199999995f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;
  p.color.spatial_mode = static_cast<ColorSpatialMode>(0);
  p.seed.type = static_cast<SeedType>(0);
  p.seed.border_width = 0.0599999987f;
  p.seed.hue = 0.570059896f;
  p.seed.saturation = 1.0f;
  p.seed.alpha = 0.75f;
  p.seed.smooth_mask = 50.0f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_freq = 0.0500000007f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 1.0f;
  p.semigroup.word = static_cast<PatternSemigroupWord>(2); // sturmian_fibonacci
  p.semigroup.hold_steps = 8;
  p.semigroup.seed = 1ull;
  p.semigroup.has_partner = true;
  p.semigroup.partner_inner = static_cast<InnerFunction>(29); // z·cos(z)
  p.semigroup.partner_zn.zn[0] = {0.229522958f, -1.00977051f};
  p.semigroup.partner_zn.zn[1] = {-0.0160223022f, 0.0540169999f};
  p.semigroup.partner_zn.zn[2] = {0.29905194f, 0.96644181f};
  p.semigroup.partner_zn.zn[3] = {0.0268484261f, -0.00384904118f};
  return p;
}

} // namespace epi::presets
