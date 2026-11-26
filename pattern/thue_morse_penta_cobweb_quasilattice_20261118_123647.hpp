#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Thue-Morse penta cobweb: an overlap-free braid of two quasilattice warps
 *
 * Generator A = (z + e^z)/2 and generator B = (cosh z + z^2)/2 (commutator 0.57) are
 * composed along the Thue-Morse word ABBABAAB... (hold 2), which never repeats any
 * block three times. Descends from spatial_wave_penta_quasilattice_cascade; the Torus
 * fold turns the braid into a Kluver cobweb whose strands keep re-weaving (word-orbit
 * fill 0.44). Census hole H6.
 *
 * Discovered by tools/discover_subspaces (MAP-Elites), promoted by
 * tools/promote_discovery_draft.py. Discovery metrics:
 *
 * Cell: aq18:tp2/Cobweb/thue_morse   lineage: spatial_wave_penta_quasilattice_cascade   born: iteration 2862
 * Score 0.967872791 = fitness 0.896137149 + novelty 0.204958979
 *   edge_of_chaos 0.915064379, lyapunov 0.999974193 (lambda_med -0.00177801635), memory 0.982631155, spectral 0.790404954,
 *   form 1, semigroup 1 (commutator 0.56886873, word_fill 0.44140625)
 *
 * Generator A: (z+e^z)/2
 * Generator B: 0.5·(cosh(z)+z²)  word thue_morse, hold 2
 *
 * Passed: native admission, static Calm motion-safety passport.
 */
inline Pattern thue_morse_penta_cobweb_quasilattice() {
  Pattern p;
  p.name = "thue_morse_penta_cobweb_quasilattice";
  p.inner = static_cast<InnerFunction>(27); // (z+e^z)/2
  p.reduction = static_cast<ReductionType>(1);
  p.zn.zn[0] = {1.01826775f, -0.220152184f};
  p.zn.zn[1] = {-0.0281741992f, -0.0321420319f};
  p.zn.zn[2] = {0.710879207f, 0.713808715f};
  p.zn.zn[3] = {-0.0467009731f, -0.0450852513f};
  p.zn.zn0_animate = false; p.zn.zn0_anim_speed = 0.00499999989f; p.zn.zn0_anim_amp = 0.100000001f;
  p.zn.zn1_animate = false; p.zn.zn1_anim_speed = 0.00400000019f; p.zn.zn1_anim_amp = 0.100000001f; p.zn.zn1_anim_k = 5.0f;
  p.color.hue_rotation = -0.020941807f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.98713845f;
  p.color.use_hsls = false;
  p.color.distance_modulate = true;
  p.color.color_mode = presets::as_int(presets::ColorMode::QuasiPeriodicWave);
  p.color.spiral_angle_coeff = 3.53553391f;
  p.color.spiral_radius_coeff = 0.5f;
  p.color.chroma_breath_amp = 0.25f;
  p.color.chroma_breath_freq = 0.0952441022f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.149608999f;
  p.color.hue_velocity_drift = 0.00300000003f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;
  p.color.spatial_mode = static_cast<ColorSpatialMode>(0);
  p.seed.type = static_cast<SeedType>(2);
  p.seed.border_width = 1.0f;
  p.seed.hue = 0.617536843f;
  p.seed.saturation = 1.0f;
  p.seed.alpha = 0.0599999987f;
  p.seed.smooth_mask = 50.0f;
  p.seed.hue_speed = 0.00800000038f;
  p.seed.pulse_freq = 0.0952441022f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.75f;
  p.semigroup.word = static_cast<PatternSemigroupWord>(3); // thue_morse
  p.semigroup.hold_steps = 2;
  p.semigroup.seed = 14136971179981109339ull;
  p.semigroup.has_partner = true;
  p.semigroup.partner_inner = static_cast<InnerFunction>(33); // 0.5·(cosh(z)+z²)
  p.semigroup.partner_zn.zn[0] = {0.777641058f, 0.574610114f};
  p.semigroup.partner_zn.zn[1] = {-0.0742748752f, -0.058213748f};
  p.semigroup.partner_zn.zn[2] = {0.967109203f, 0.305093884f};
  p.semigroup.partner_zn.zn[3] = {-0.0650343001f, -0.025417354f};
  return p;
}

} // namespace epi::presets
