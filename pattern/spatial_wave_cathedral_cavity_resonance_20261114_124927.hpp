#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Advanced, Slot A1, Hole H3): Cathedral Cavity Resonance.
 *
 * Mathematical & dynamical structure:
 * - Dihedral Fabry-Perot interference cavity near the 8-fold Ammann-Beenker pi/8 wedge angle
 *   (theta = pi/8 + 0.015 = 0.4077 rad, |zn0| = 1.12) coupling Cos + AmmannBeenker + Cross.
 * - Successive 22.5-deg sector reflections lock Cross-seed wavefronts into crisp octagonal
 *   cavity harmonics (Law L12), gently rocked across resonance via Zn0Im motion.
 * - Value-linear SpatialWave (mode 4) + O9 bounded -cos hue clock (hv_base = 0).
 *
 * Atlas v4 metrics (fidelity: Class P dihedral sector-fold cavity, N = 512):
 * - Regime: ExpandingTiling (O = 3.2126, W = 0.6689, tau1 = 8.3417, g2/g0 = 1.0009, exp_m = 0.2221).
 * - Law L4r predicted pressure slope alpha_L4r = 2.7372 vs RTX 3090 measured V-slope = 4.7874
 *   (luma_std = 0.0928, mean_luma = 0.0602, hf_fraction = 0.0001, 0% clipped).
 * - Novelty: fills Hole H3 (Cos x AmmannBeenker x Cross x bounded cocycle); max staged
 *   pairwise |corr| = 0.1560.
 */
inline Pattern spatial_wave_cathedral_cavity_resonance() {
  // theta = pi/8 + 0.015 = 0.407699 rad, |zn0| = 1.12
  constexpr float ZN0_RE = 1.028197f; // 1.12 * cos(0.407699)
  constexpr float ZN0_IM = 0.443970f; // 1.12 * sin(0.407699)

  Pattern p;
  p.name = "spatial_wave_cathedral_cavity_resonance";
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::AmmannBeenker;

  p.zn.zn[0] = {ZN0_RE, ZN0_IM};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.18f, 0.0f};
  p.zn.zn[3] = {0.08f, -0.05f};

  p.color.hue_rotation = 0.02f;
  p.color.sat_scale = 1.05f;
  p.color.fade = 0.910f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.48f;
  p.color.spiral_radius_coeff = 0.58f;
  p.color.chroma_breath_amp = 0.17f;
  p.color.chroma_breath_freq = 0.077f;
  p.color.chroma_breath_phase = 0.13f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.058f;
  p.color.hue_velocity_drift = 0.011f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.04f;
  p.seed.hue = 0.64f;
  p.seed.hue_speed = 0.035f;
  p.seed.alpha = 0.48f;
  p.seed.pulse_freq = 0.077f;
  p.seed.pulse_amp = 0.0f;

  p.speed = 0.80f;

  p.motion
      .add("cavity_dihedral_detune",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.048f,
                            .amplitude = 0.06f,
                            .offset = ZN0_IM},
           control::ModTarget::Zn0Im)
      .add("cavity_hue_precession",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.033f,
                            .amplitude = 0.025f,
                            .offset = 0.02f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
