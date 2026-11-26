#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Minimal 3.5D): Meno's Doubling Squares on the Gaussian Torus,
 * with Dyadic Parallax.
 *
 * Mathematics:
 * - T(z) = G z + c with G = 1 + i, Grid reduction (z mod 2Z^2). G is a
 *   Gaussian integer, so G 2Z^2 is inside 2Z^2 and the wrap commutes with
 *   T: (R o T)^n z = G^n z + (G^n - 1) c / (G - 1) mod 2, exactly. This is
 *   the expanding torus endomorphism of degree |G|^2 = 2, the lift of the
 *   degree-2 Lattes map of the square lattice.
 * - The Cross seed is zero (e^-20) on the wrap edges, so on the torus it is
 *   the line grid x, y in 2Z. Generation n is that grid pulled back by
 *   G^n: a square grid turned by n pi / 4 with spacing 2 / 2^(n/2) and line
 *   width shrinking with it, so every generation is the same picture
 *   scaled. Even generations are axis grids, each a refinement of the one
 *   before; odd ones are their diagonals: the doubling of the square in
 *   Plato's Meno, repeated, cell area halved at each pass.
 * - The fixed point is z* = -i c. The nodes of generation n are
 *   z* + G^-n (2Z^2 - z*): generation n is generation 0 scaled by
 *   2^(-n/2) and turned about z*, so moving c translates layer n by
 *   (1 - G^-n) z*, a factor that spirals onto 1. Deep layers move almost
 *   rigidly with z*, generation 0 not at all: an exact parallax.
 *
 * Pipeline: Identity with zn0 = 1 + i, zn1 = 0, zn2 = 1, zn3 = c; Grid;
 * Cross at alpha 0.7; fade 0.84; OKLab step 1/8, so the hue of a layer is
 * arg(G^n) / 2 pi: hue reads the turn of the grid. Two lanes in quadrature
 * move c round a circle of radius 0.2 in 333 s.
 *
 * Fidelity:   identity for every lane value (the wrap is exact on the torus)
 * Cell:       Identity / Grid / Cross / OKLabColorRotation
 * Nearest:    square_minimal_octagonal_dyadic_sunburst (z^2, eightfold);
 *             here the map is linear and the dyadic structure is a lattice.
 * Height:     age = passes; layer n is 2^(n/2) times finer.
 * Hue clock:  bounded; step 1/8 +- 0.005.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. Below about 2 px the
 *             deep grids alias into moire; fade 0.84 keeps their weight
 *             under 0.2 past generation 9. Not built, rendered or measured.
 * @assert p_E >= 0.95
 */
inline Pattern identity_minimal_meno_doubling_squares_dyadic_parallax() {
  constexpr float C_RADIUS = 0.2f;
  constexpr float ORBIT_HZ = 0.003f;
  Pattern p;
  p.name = "identity_minimal_meno_doubling_squares_dyadic_parallax";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.0f, 1.0f}; // G = 1 + i
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {C_RADIUS, 0.0f}; // c; fixed point z* = -i c

  p.color.hue_rotation = 0.125f; // hue = arg(G^n) / 2 pi
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.84f;
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
  p.seed.hue = 0.08f;
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
      .add("hue_turn_lock",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.005f,
                            .offset = 0.125f},
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
