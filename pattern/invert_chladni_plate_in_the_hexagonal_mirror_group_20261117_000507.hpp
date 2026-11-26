#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Advanced 3.5D): a Chladni Plate Resonating in Every Tile of a
 * Hyperbolic Mirror Group.
 *
 * Mathematics:
 * - The same map as invert_minimal_hexagonal_corner_mirror_arcs: inversion
 *   in the circle of radius 2 sec(pi/6) about the corner 1 + i, composed
 *   with the box mirrors. The circle meets the far mirrors at pi/6, so the
 *   group is the discrete (2, 6, 6) triangle group, and every pixel lands
 *   on a 2-cycle within a few passes (100 % of a 300 x 300 grid).
 * - The ChladniCymatics seed is a vibrating square plate,
 *   |cos(n x) cos(m y) - cos(m x) cos(n y)| near zero, with n = 3 + sin(0.2 t)
 *   and m = 5 + cos(0.25 t) drifting so that the nodal lines slowly morph.
 *   Because every pass is conformal, each tile holds an exact conformal
 *   image of a piece of the plate: angles between nodal lines are kept,
 *   lengths are not.
 *
 * Pipeline: Invert with zn0 = 1, zn1 = -(1 + i), zn2 = 16/3, zn3 = 1 + i;
 * Torus; Chladni seed at alpha 0.8; OKLab hue step 1/10 by depth. The
 * standing wave is copied into every tile, all copies morph in step as the
 * plate's mode drifts, and the copies shrink towards the limit set in the
 * upper right. In the numpy model a lace of gold and red nodal curves over
 * black, with a six-way crossing at every vertex of the tiling.
 *
 * Fidelity:   identity
 * Cell:       Invert / Torus / ChladniCymatics / OKLabColorRotation
 * Nearest:    spatial_wave_mobius_horocycle_bouquet pulls the plate through
 *             one parabolic Mobius map; magnetic_dipole_boole_pitchfork_
 *             cymatic_circle_rupture through a Blaschke family. Here through
 *             a whole hyperbolic group.
 * Height:     age = passes since landing.
 * Hue clock:  bounded; step 1/10 +- 0.015.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-17. Landing measured as for
 *             the arcs preset. A numpy model of the pass with the seed's
 *             own formula was drawn. Not built, rendered by the engine or
 *             measured.
 * @assert p_X == 0 && p_C <= 0.05
 */
inline Pattern invert_chladni_plate_in_the_hexagonal_mirror_group() {
  Pattern p;
  p.name = "invert_chladni_plate_in_the_hexagonal_mirror_group";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {-1.0f, -1.0f};
  p.zn.zn[2] = {5.3333333f, 0.0f};
  p.zn.zn[3] = {1.0f, 1.0f};

  p.color.hue_rotation = 0.1f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.09f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.1f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.8f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("nodal_hue_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.005f,
                            .amplitude = 0.015f,
                            .offset = 0.1f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.025f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
