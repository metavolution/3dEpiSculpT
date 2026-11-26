#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Minimal 3.5D): a Triadic Mirror Kaleidoscope of Chladni Plates.
 *
 * Mathematics:
 * - Torus reduction is the triangle fold F: reflection in x = +-1 and
 *   y = +-1, period 4. F is symmetric about every odd integer, so for an
 *   odd multiplier F(3 F(x)) = F(3 x), and (F o 3)^n = F o 3^n exactly
 *   (checked to 2e-14 for n = 4). T(z) = 3 z with the fold is the
 *   threefold tent map in each coordinate: a mirror box seen in a mirror
 *   box.
 * - Generation n cuts the square into 3^n x 3^n cells and maps each onto
 *   the whole square by an affine reflection, so each cell holds a full
 *   copy of the plate, mirrored to its neighbours (the plate is even in x
 *   and y, so the mirror seams are invisible). The middle cell of every
 *   cell is concentric with it: each plate holds a plate a third its size
 *   at its centre and eight around it, without end.
 * - A translation slot would break the fold identity (it needs c = 0 mod 2),
 *   so the geometry is still; the plates breathe through the seed's own
 *   mode drift (n = 3 + sin 0.2t, m = 5 + cos 0.25t), the same in every
 *   cell at every scale.
 *
 * Pipeline: Identity with zn0 = 3; Torus; Chladni at alpha 0.6; fade 0.82
 * (four levels: 81 cells a side at level 4); OKLab step 1/6.
 *
 * Fidelity:   identity
 * Cell:       Identity / Torus / ChladniCymatics / OKLabColorRotation
 * Nearest:    identity_minimal_meno_doubling_squares_dyadic_parallax (a
 *             torus endomorphism by wrap); here by mirrors, factor 3.
 * Height:     age = passes; level n has 9^n plates.
 * Hue clock:  bounded; step 1/6 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; fold identity checked
 *             arithmetically. Level 5 plates are 4 px across at 1080p and
 *             alias. Not built, rendered or measured.
 * @assert p_E >= 0.95
 */
inline Pattern identity_minimal_triadic_mirror_plate_kaleidoscope() {
  Pattern p;
  p.name = "identity_minimal_triadic_mirror_plate_kaleidoscope";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {3.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.16666667f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.82f;
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
  p.seed.hue = 0.5f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.6f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("level_tint",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.01f,
                            .offset = 0.16666667f},
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
