#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spectral & Optical: Kosterlitz-Thouless Vortex-Antivortex Checkerboard.
 *
 * Scientific foundation:
 * - In the 2-D XY model the order parameter is a phase angle. Its point
 *   defects are vortices with integer winding number. Below the
 *   Berezinskii-Kosterlitz-Thouless temperature they exist only as bound
 *   vortex-antivortex pairs of zero net charge; above it the pairs unbind
 *   into a neutral plasma. Winding number is conserved under any smooth
 *   deformation of the field.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Color = Opponent with spiral_angle_coeff = 1. The opponent mode rotates
 *   the (Co, Cg) chroma vector, a linear U(1) action, by the polar angle of
 *   the pixel on every pass. The chroma angle is therefore a genuine phase
 *   field, and an integer coefficient makes it single-valued.
 * - After one more pass the phase is arg(F(z)) with F = Reduce(T(z)). Its
 *   singularities are the zeros of the folded map. The Torus fold vanishes
 *   at every T in 2Z[i] and reverses orientation in cells of odd parity, so
 *       winding = +1 at T^-1(2a + 2bi) with a + b even,
 *       winding = -1 with a + b odd.
 *   The reduction stage itself manufactures a neutral checkerboard of
 *   vortices and antivortices; the transform decides where they sit.
 * - Inner = MagneticDipole (w^2 - 1)/(w^2 + 1). Its two zeros hold the
 *   isolated vortices; towards each of its two poles T covers the whole
 *   lattice, so pairs of alternating sign crowd together without limit: a
 *   bound-pair gas far from the poles and a dense plasma at them.
 * - zn0 = 1.3 e^{-i pi/4}, zn2 = 1.06 i is a neighbour of the Lattes
 *   parameter (see lattes_torus_sphere_shadow), chosen so the luminance
 *   dynamics has no attracting basin and the whole frame stays lit.
 * - Motion: raising |zn2| drags more lattice points into range, which
 *   appears as pair creation; lowering it annihilates them again.
 */
inline Pattern kosterlitz_thouless_vortex_checkerboard() {
  constexpr float PRE = 0.91923882f; // 1.3 * cos(pi/4)
  constexpr float GAIN = 1.06f;

  Pattern p;
  p.name = "kosterlitz_thouless_vortex_checkerboard";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE, -PRE};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.0f, GAIN};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.02f;
  p.color.sat_scale = 1.03f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 1.0f;  // unit of topological charge per pass
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.10f;
  p.color.chroma_breath_freq = 0.07f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.008f;
  p.color.hue_velocity_breath = 0.004f;
  p.color.hue_velocity_drift = 0.001f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.10f;
  p.seed.hue = 0.33f;
  p.seed.hue_speed = 0.06f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.06f;
  p.seed.pulse_amp = 0.10f;
  p.speed = 0.85f;

  p.motion
      .add("pair_creation_gain",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.027f,
                            .amplitude = 0.12f,
                            .offset = GAIN},
           control::ModTarget::Zn2Im)
      .add("vortex_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.019f,
                            .amplitude = 0.05f,
                            .offset = 0.0f},
           control::ModTarget::Zn1Re)
      .add("stiffness_breath",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.035f,
                            .amplitude = 0.06f,
                            .offset = 0.10f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
