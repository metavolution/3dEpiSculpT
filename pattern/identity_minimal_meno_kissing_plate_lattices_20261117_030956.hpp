#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Minimal 3.5D): Meno's Kissing Lattices of Chladni Plates.
 *
 * Mathematics:
 * - T(z) = (1 + i) z + c on the Grid torus: the exact degree-2 torus
 *   endomorphism of identity_minimal_meno_doubling_squares_dyadic_parallax,
 *   now with the Chladni seed. The plate is zero for r >= 1, so on the
 *   torus it is a lattice of discs of radius 1 on 2Z^2, touching at the
 *   edge midpoints: a kissing square lattice (packing density pi/4).
 * - Generation n is that lattice pulled back by (1 + i)^n: discs of radius
 *   2^(-n/2) on the lattice (1 + i)^-n 2Z^2, whose nearest distance is
 *   2 x 2^(-n/2). So every generation is again a kissing square lattice,
 *   turned by n pi/4: odd generations put plates exactly in the four-cornered
 *   gaps of the even ones, and each generation holds the previous one's
 *   centres. Every plate holds, at its centre and in its gaps, the plates of
 *   all finer generations.
 * - Moving c translates generation n by (1 - (1+i)^-n) z*, z* = -i c: an
 *   exact parallax, deep layers moving with z*, generation 0 still.
 *
 * Pipeline: Identity, zn0 = 1 + i, zn2 = 1, zn3 = c on quadrature lanes
 * (radius 0.2, 333 s); Grid; Chladni at alpha 0.6; fade 0.8; OKLab step 1/8
 * (hue = turn of the lattice).
 *
 * Fidelity:   identity for every lane value
 * Cell:       Identity / Grid / ChladniCymatics / OKLabColorRotation
 * Nearest:    identity_minimal_meno_doubling_squares_dyadic_parallax (the
 *             same map with line grids); here discs, kissing.
 * Height:     age = passes; layer n has 2^n times as many plates.
 * Hue clock:  bounded; step 1/8.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; lattice distances
 *             evaluated arithmetically. Plates of generation 8 are 67 px
 *             across at 1080p, generation 12 about 17 px. Not built, rendered
 *             or measured.
 * @assert p_E >= 0.95
 */
inline Pattern identity_minimal_meno_kissing_plate_lattices() {
  constexpr float C_RADIUS = 0.2f;
  constexpr float ORBIT_HZ = 0.003f;
  Pattern p;
  p.name = "identity_minimal_meno_kissing_plate_lattices";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.0f, 1.0f}; // 1 + i
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {C_RADIUS, 0.0f};

  p.color.hue_rotation = 0.125f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.8f;
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
  p.seed.hue = 0.42f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.6f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("parallax_centre_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = C_RADIUS,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("parallax_centre_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = C_RADIUS,
                            .offset = 0.0f,
                            .phase = 0.0f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.02f,
                            .offset = 0.04f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
