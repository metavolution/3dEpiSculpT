#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Markov cathedral cobweb: z^2 frames chosen by a seeded random walk
 *
 * Generators A, B = z^2 in two rotated frames (commutator 0.65), selected by a seeded
 * 2-state Markov walk (hold 2). The cathedral octahedral fold turns the stochastic
 * composition into a cobweb form constant whose strands re-weave without repeating.
 * Opponent colour, slow seed hue. Census hole H6.
 *
 * Discovered by tools/discover_subspaces (MAP-Elites), promoted by
 * tools/promote_discovery_draft.py. Discovery metrics:
 *
 * Cell: aq1:square/Cobweb/markov_walk   lineage: spatial_wave_chiral_siegel_critical_vortex   born: iteration 1648
 * Score 0.901674606 = fitness 0.787919411 + novelty 0.325014841
 *   edge_of_chaos 0.711226408, lyapunov 0.951174493 (lambda_med 0.0783075626), memory 0.593626834, spectral 0.991074367,
 *   form 1, semigroup 0.845622835 (commutator 0.648613413, word_fill 0.53125)
 *
 * Generator A: z²
 * Generator B: z²  word markov_walk, hold 2
 *
 * Passed: native admission, static Calm motion-safety passport.
 */
inline Pattern markov_square_cathedral_cobweb() {
  Pattern p;
  p.name = "markov_square_cathedral_cobweb";
  p.inner = static_cast<InnerFunction>(1); // z²
  p.reduction = static_cast<ReductionType>(4);
  p.zn.zn[0] = {1.32596624f, 0.608671725f};
  p.zn.zn[1] = {0.0735131651f, 2.22781673e-05f};
  p.zn.zn[2] = {0.643053532f, -0.222205624f};
  p.zn.zn[3] = {-0.0976359695f, 0.444821388f};
  p.zn.zn0_animate = false; p.zn.zn0_anim_speed = 0.00499999989f; p.zn.zn0_anim_amp = 0.100000001f;
  p.zn.zn1_animate = false; p.zn.zn1_anim_speed = 0.00400000019f; p.zn.zn1_anim_amp = 0.100000001f; p.zn.zn1_anim_k = 5.0f;
  p.color.hue_rotation = 0.00563657796f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.960510612f;
  p.color.use_hsls = false;
  p.color.distance_modulate = true;
  p.color.color_mode = presets::as_int(presets::ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0416667014f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.119999997f;
  p.color.chroma_breath_freq = 0.0755952969f;
  p.color.chroma_breath_phase = 12.566371f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.00189990003f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;
  p.color.spatial_mode = static_cast<ColorSpatialMode>(0);
  p.seed.type = static_cast<SeedType>(0);
  p.seed.border_width = 0.100000001f;
  p.seed.hue = 0.333302706f;
  p.seed.saturation = 1.0f;
  p.seed.alpha = 0.899999976f;
  p.seed.smooth_mask = 50.0f;
  p.seed.hue_speed = 0.0120000001f;
  p.seed.pulse_freq = 0.0755952969f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.5f;
  p.semigroup.word = static_cast<PatternSemigroupWord>(4); // markov_walk
  p.semigroup.hold_steps = 2;
  p.semigroup.seed = 12362058880066398253ull;
  p.semigroup.has_partner = true;
  p.semigroup.partner_inner = static_cast<InnerFunction>(1); // z²
  p.semigroup.partner_zn.zn[0] = {1.12999046f, -0.869907498f};
  p.semigroup.partner_zn.zn[1] = {0.0289418288f, 0.0102017391f};
  p.semigroup.partner_zn.zn[2] = {0.445348978f, 0.591174722f};
  p.semigroup.partner_zn.zn[3] = {-0.408098936f, 0.00867719762f};
  return p;
}

} // namespace epi::presets
