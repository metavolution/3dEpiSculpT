#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Minimal 3.5D): the Pythagorean Star Lattice, Grids Turned by an
 * Irrational Angle.
 *
 * Mathematics:
 * - T(z) = G z + c with G = 2 + i on the Grid torus: a Gaussian integer, so
 *   the wrap is exact (the torus endomorphism of degree |G|^2 = 5).
 * - Generation n is the line grid x, y in 2Z (the Cross seed on the torus)
 *   pulled back by G^n: a square grid of spacing 2 / 5^(n/2), turned by
 *   -n arctan(1/2). G^2 = 3 + 4i: the second generation runs along the
 *   3-4-5 triangle, the third along 2-11 (G^3 = 2 + 11i), every one along a
 *   Pythagorean direction. arctan(1/2) / pi is irrational (the only
 *   rational multiples of pi with rational tangent are multiples of
 *   pi/4), so no two generations are parallel.
 * - G^n 2Z^2 lies in 2Z^2, so every node of generation 0 is a node of
 *   every generation: all the grids pass through common points, a lattice
 *   of stars of lines in ever new directions.
 * - The fixed point is z* = -c (1 - i)/2; moving c translates generation n
 *   by (1 - G^-n) z*: exact parallax as in identity_minimal_meno_doubling_
 *   squares_dyadic_parallax.
 *
 * Pipeline: Identity with zn0 = 2 + i, zn2 = 1, zn3 = c on quadrature lanes
 * (radius 0.15, 400 s); Grid; Cross at alpha 0.7; fade 0.8 (spacing falls
 * by sqrt 5 per pass, so about five generations are resolved); OKLab step
 * arctan(1/2) / 2 pi, so hue reads the turn of the grid.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Identity / Grid / Cross / OKLabColorRotation
 * Nearest:    identity_minimal_meno_doubling_squares_dyadic_parallax
 *             (G = 1 + i: rational angle, two orientations); here the angle
 *             is irrational and every orientation is new.
 * Height:     age = passes; layer n is 5^(n/2) times finer.
 * Hue clock:  bounded; step 0.0738.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. Generations past the
 *             sixth fall under 10 px spacing at 1080p and alias into moire.
 *             Not built,
 *             rendered or measured.
 * @assert p_E >= 0.95
 */
inline Pattern identity_minimal_pythagorean_star_lattice() {
  constexpr float C_RADIUS = 0.15f;
  constexpr float ORBIT_HZ = 0.0025f;
  Pattern p;
  p.name = "identity_minimal_pythagorean_star_lattice";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {2.0f, 1.0f}; // G = 2 + i
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {C_RADIUS, 0.0f};

  p.color.hue_rotation = 0.07379181f; // arctan(1/2) / 2 pi
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

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.13f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.7f;
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
