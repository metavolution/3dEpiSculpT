#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Ramachandran Map: Protein Backbone Conformations on the Dihedral Torus.
 *
 * Scientific foundation:
 * - Ramachandran, Ramakrishnan & Sasisekharan (1963): each amino-acid residue
 *   has two free backbone torsions, phi and psi. Both are angles, so
 *   conformation space is a torus T^2. Steric clashes forbid most of it; the
 *   allowed basins are the alpha-helix (phi ~ -63 deg, psi ~ -43 deg),
 *   beta-sheet and left-handed helix regions. Torsional energies are Fourier
 *   series in the angles, with a 3-fold term for sp3 bonds.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Reduction = Torus: the domain of the map really is T^2.
 * - Inner = Sin: the fundamental Fourier mode of a torsional potential.
 * - Pre-affine rotated by phi_alpha = -63 deg, post-affine by
 *   psi_alpha = -43 deg: the feedback loop is tuned to the alpha-helix.
 * - Seed = ChladniCymatics: products cos(n*x)*cos(m*y), the torus Fourier
 *   modes whose nodal lines look like energy contours of the map.
 * - zn1 rose animation with k = 3: the 3-fold torsional barrier.
 * - Motion: independent librations of phi (Zn3Re) and psi (Zn3Im) at an
 *   irrational (golden) frequency ratio, an ergodic walk on the torus.
 * - Color = OKLab rotation (perceptual hue = conformational class).
 */
inline Pattern ramachandran_dihedral_torus_basin() {
  constexpr float PHI_ALPHA = -1.09955743f; // -63 deg
  constexpr float PSI_ALPHA = -0.75049158f; // -43 deg
  constexpr float PRE = 1.30f;
  constexpr float POST = 1.12f;
  constexpr float LIBRATION_HZ = 0.06f;
  constexpr float GOLDEN = 1.6180339887f;

  Pattern p;
  p.name = "ramachandran_dihedral_torus_basin";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE * std::cos(PHI_ALPHA), PRE * std::sin(PHI_ALPHA)};
  p.zn.zn[1] = {0.03f, -0.02f};
  p.zn.zn[2] = {POST * std::cos(PSI_ALPHA), POST * std::sin(PSI_ALPHA)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.11f;
  p.zn.zn0_anim_amp = 0.04f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.14f;
  p.zn.zn1_anim_amp = 0.04f;
  p.zn.zn1_anim_k = 3.0f;

  p.color.hue_rotation = 0.026f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = 0.30f;
  p.color.chroma_breath_amp = 0.25f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 6.2831853f;
  p.color.hue_velocity_base = 0.020f;
  p.color.hue_velocity_breath = 0.010f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.58f;
  p.seed.hue_speed = 0.10f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.09f;
  p.seed.pulse_amp = 0.18f;
  p.speed = 0.8f;

  p.motion
      .add("phi_torsion_libration",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = LIBRATION_HZ,
                            .amplitude = 0.045f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("psi_torsion_libration",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = LIBRATION_HZ * GOLDEN,
                            .amplitude = 0.045f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("steric_clash_chroma",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.08f,
                            .amplitude = 0.08f,
                            .offset = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("helix_sheet_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.035f,
                            .amplitude = 0.010f,
                            .offset = 0.026f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
