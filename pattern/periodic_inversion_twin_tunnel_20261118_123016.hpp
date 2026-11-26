#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Periodic twin inversion: a tunnel seen through two rotated mirrors
 *
 * Generator A = 1/z behind a strong zoom (|zn0| ~ 6.3), generator B = 1/z in a near-
 * identity frame (|zn0| ~ 1.03); the two inversions do not commute (commutator 0.67).
 * The periodic word ABAB (hold 4 -> AAAABBBB frames) alternates the two inversions, giving a Torus-folded
 * Kluver tunnel whose walls swap chirality every four frames. Opponent colour, slow
 * bounded hue clock. Census hole H6.
 *
 * Discovered by tools/discover_subspaces (MAP-Elites), promoted by
 * tools/promote_discovery_draft.py. Discovery metrics:
 *
 * Cell: aq6:invert/Tunnel/periodic   lineage: invert_modular_wave_meets_its_mirror_image   born: iteration 659
 * Score 0.95472164 = fitness 0.825260719 + novelty 0.369888344
 *   edge_of_chaos 0.707335056, lyapunov 0.980744133 (lambda_med 0.0488042034), memory 0.966789793, spectral 0.959244967,
 *   form 1, semigroup 0.732113801 (commutator 0.665760448, word_fill 0.16796875)
 *
 * Generator A: 1/z
 * Generator B: 1/z  word periodic, hold 4
 *
 * Passed: native admission, static Calm motion-safety passport.
 */
inline Pattern periodic_inversion_twin_tunnel() {
  Pattern p;
  p.name = "periodic_inversion_twin_tunnel";
  p.inner = static_cast<InnerFunction>(10); // 1/z
  p.reduction = static_cast<ReductionType>(1);
  p.zn.zn[0] = {5.98302126f, -1.90443277f};
  p.zn.zn[1] = {0.0338635966f, -0.0107000414f};
  p.zn.zn[2] = {4.13779354f, -0.455497772f};
  p.zn.zn[3] = {0.969447076f, -0.0296177138f};
  p.zn.zn0_animate = false; p.zn.zn0_anim_speed = 0.00499999989f; p.zn.zn0_anim_amp = 0.100000001f;
  p.zn.zn1_animate = false; p.zn.zn1_anim_speed = 0.00400000019f; p.zn.zn1_anim_amp = 0.100000001f; p.zn.zn1_anim_k = 5.0f;
  p.color.hue_rotation = 0.508336961f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.967864811f;
  p.color.use_hsls = false;
  p.color.distance_modulate = true;
  p.color.color_mode = presets::as_int(presets::ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.0399999991f;
  p.color.chroma_breath_freq = 0.100000001f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.00039999999f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;
  p.color.spatial_mode = static_cast<ColorSpatialMode>(0);
  p.seed.type = static_cast<SeedType>(3);
  p.seed.border_width = 0.0500000007f;
  p.seed.hue = 0.491241455f;
  p.seed.saturation = 1.0f;
  p.seed.alpha = 0.25f;
  p.seed.smooth_mask = 50.0f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_freq = 0.0500000007f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.800000012f;
  p.semigroup.word = static_cast<PatternSemigroupWord>(1); // periodic
  p.semigroup.hold_steps = 4;
  p.semigroup.seed = 14058026980159906113ull;
  p.semigroup.has_partner = true;
  p.semigroup.partner_inner = static_cast<InnerFunction>(10); // 1/z
  p.semigroup.partner_zn.zn[0] = {1.02934241f, 0.101016045f};
  p.semigroup.partner_zn.zn[1] = {0.0f, 0.0f};
  p.semigroup.partner_zn.zn[2] = {0.937372327f, -0.0353596769f};
  p.semigroup.partner_zn.zn[3] = {0.0f, 0.0f};
  return p;
}

} // namespace epi::presets
