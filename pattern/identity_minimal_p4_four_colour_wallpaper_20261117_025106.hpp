#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Minimal 3.5D): a Four-Colour p4 Wallpaper of Chladni Plates.
 *
 * Mathematics:
 * - T(z) = i z + c on the Grid torus, c = 0.6 + 0.2i: a quarter turn about
 *   z* = c/(1 - i) = 0.2 + 0.4i, exact on the torus (i is a Gaussian
 *   unit). With the plate lattice of the Chladni seed this generates the
 *   wallpaper group p4: order-4 centres at z* and z* + 1 + i, order-2
 *   centres at z* + 1 and z* + i.
 * - With fade 0.99 and alpha 0.15 the four generations weigh almost the
 *   same, and OKLab step 1/4 gives generation j the hue j/4. A quarter turn
 *   about either order-4 centre maps the picture onto itself with every
 *   hue advanced by 1/4: a colour symmetry (a perfect colouring in the
 *   sense of Escher's coloured tilings), the colour group a 4-cycle of
 *   hues. Every point shows four plates, one per hue, turned against one
 *   another about centres that are not the plates' own centres.
 * - The plates breathe through the seed's mode drift; the wallpaper
 *   symmetry holds at every instant.
 *
 * Pipeline: Identity, zn0 = i, zn2 = 1, zn3 = c; Grid; Chladni at alpha
 * 0.15; fade 0.99; OKLab step 1/4. Static geometry.
 *
 * Fidelity:   identity
 * Cell:       Identity / Grid / ChladniCymatics / OKLabColorRotation
 * Nearest:    identity_p4_orbifold_vortex_antivortex_crystal (the same
 *             group with Opponent colour: vortices); here OKLab: a coloured
 *             wallpaper.
 * Height:     age = passes; the four generations are the four group
 *             elements.
 * Hue clock:  bounded; step 1/4 +- 0.004.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. The colour symmetry is
 *             exact for equal weights. Per pass the weight is
 *             fade (1 - alpha I): 0.99 where the seed is faint, 0.84 on its
 *             brightest nodal lines, so the symmetry is near-exact in the
 *             dim field and only approximate on the bright lines.
 *             Not built, rendered or measured.
 * @assert p_N >= 0.95
 */
inline Pattern identity_minimal_p4_four_colour_wallpaper() {
  Pattern p;
  p.name = "identity_minimal_p4_four_colour_wallpaper";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {0.0f, 1.0f}; // i
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.6f, 0.2f}; // c; order-4 centre 0.2 + 0.4i

  p.color.hue_rotation = 0.25f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.99f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.04f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.05f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.15f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("colour_group_tuning",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.004f,
                            .offset = 0.25f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.02f,
                            .offset = 0.04f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
