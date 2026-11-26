#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Wolfgang Paul RF Quadrupole Ion Trap — Dynamic Mathieu Saddle Confinement.
 *
 * Scientific foundation:
 * - Earnshaw's theorem forbids static 3D electrostatic trapping in free space
 *   (nabla^2 Phi = 0 implies saddle points only). Wolfgang Paul (Nobel Prize
 *   1989) proved that oscillating a quadrupole potential Phi ~ Re(z^2) at radio
 *   frequency Omega stabilizes charged ions inside the first Mathieu stability
 *   tongue (a, q) via ponderomotive secular pseudopotential confinement!
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = MagneticDipole ((z^2 - 1)/(z^2 + 1), id 44): conformal rational
 *   quadrupole/dipole map with zeros at +/-1 and poles at +/-i (the 4 hyperbolic
 *   rod electrodes!).
 * - Reduction = PareidoliaCathedral + Seed = Cross: 4-electrode saddle symmetry
 *   with supercritical |zn2| = 1.46 expanding trajectories across the electrodes.
 * - ZN animation: rapid 4-lobe RF micromotion orbit dynamically traps fluorescent
 *   cyan/magenta plasma ribbons (ColorMode::Spiral / Cyberpunk electric pulse).
 */
inline Pattern quadrupole_paul_ion_trap() {
  Pattern p;
  p.name = "quadrupole_paul_ion_trap";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::PareidoliaCathedral;

  p.zn.zn[0] = {1.38f, 0.32f};
  p.zn.zn[1] = {0.04f, -0.04f};
  p.zn.zn[2] = {1.46f, -0.38f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.24f;
  p.zn.zn0_anim_amp = 0.065f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.29f;
  p.zn.zn1_anim_amp = 0.072f;
  p.zn.zn1_anim_k = 4.0f; // 4-rod quadrupole RF drive

  p.color.hue_rotation = 0.016f;
  p.color.sat_scale = 1.04f;
  p.color.fade = 0.964f;
  p.color.color_mode = as_int(ColorMode::Spiral); // Kernel ID 1: electric cyan/magenta pulse
  p.color.spiral_angle_coeff = 2.0f;
  p.color.spiral_radius_coeff = 0.45f;
  p.color.chroma_breath_amp = 0.22f;
  p.color.chroma_breath_freq = 0.24f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.85f;
  p.color.hue_velocity_breath = 0.015f;
  p.color.hue_velocity_drift = 0.005f;
  p.color.luma_shape_depth = 0.20f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.055f;
  p.seed.hue = 0.52f;
  p.seed.hue_speed = 0.14f;
  p.seed.alpha = 0.90f;
  p.seed.pulse_freq = 0.15f;
  p.seed.pulse_amp = 0.22f;

  p.speed = 1.04f;

  p.motion
      .add("mathieu_rf_q_drive",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.115f,
                            .amplitude = 0.095f,
                            .offset = 1.46f},
           control::ModTarget::Zn2Re)
      .add("secular_micromotion_twist",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.071f,
                            .amplitude = 0.11f,
                            .offset = -0.38f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Im)
      .add("electrode_harmonic_pitch",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.088f,
                            .amplitude = 0.55f,
                            .offset = 2.0f},
           control::ModTarget::SpiralAngleCoeff)
      .add("plasma_discharge_pulse",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.132f,
                            .amplitude = 0.07f,
                            .offset = 0.20f},
           control::ModTarget::LumaShapeDepth);

  return p;
}

} // namespace epi::presets
