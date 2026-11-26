#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Kerr Spinning Black Hole Ergosphere & Relativistic Photon Ring.
 *
 * Scientific foundation:
 * - Roy Kerr (1963, Phys. Rev. Lett.) & James Bardeen (1973): a rotating black
 *   hole with dimensionless spin a* = J c / (G M^2) drags inertial frames
 *   (Lense-Thirring frame dragging) inside its oblate ergosphere, compressing
 *   the prograde critical photon orbit into an asymmetric D-shaped photon ring
 *   with nested Lyapunov-exponent demagnified subrings (n = 0, 1, 2, ...) and
 *   intense relativistic Doppler beaming on the approaching limb!
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = SpiralGalaxy (exp(i*z) * z / cos(z^2), id 41): combines chiral
 *   frame-dragging spiral phase exp(i*z) with hyperbolic critical-curve poles
 *   where cos(z^2) -> 0!
 * - Off-center zn1 = (0.14, -0.05) produces the signature Bardeen D-ring
 *   prograde/retrograde asymmetry.
 * - Color = Breathing (Kernel ID 2: Inferno Magma blackbody spectrum): renders
 *   magnetohydrodynamic plasma in realistic crimson -> amber -> white-gold
 *   synchrotron emission!
 */
inline Pattern kerr_photon_ring_ergosphere() {
  Pattern p;
  p.name = "kerr_photon_ring_ergosphere";
  p.inner = InnerFunction::SpiralGalaxy;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.16f, 0.34f};
  p.zn.zn[1] = {0.14f, -0.05f}; // spin-induced D-ring centroid shift
  p.zn.zn[2] = {0.92f, -0.28f};
  p.zn.zn[3] = {-0.06f, 0.03f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.16f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.21f;
  p.zn.zn1_anim_amp = 0.056f;
  p.zn.zn1_anim_k = 3.0f; // orbiting ISCO hot-spot flare

  p.color.hue_rotation = 0.008f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.958f;
  p.color.color_mode = as_int(ColorMode::Breathing); // Kernel ID 2: blackbody Inferno Magma
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = 0.38f;
  p.color.chroma_breath_amp = 0.24f;
  p.color.chroma_breath_freq = 0.18f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.012f;
  p.color.hue_velocity_breath = 0.008f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.22f; // Doppler beaming hot-spot boost
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.06f;
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.12f;
  p.seed.pulse_amp = 0.20f;

  p.speed = 0.95f;

  p.motion
      .add("lense_thirring_spin",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.086f,
                            .amplitude = 0.095f,
                            .offset = -0.28f},
           control::ModTarget::Zn2Im)
      .add("photon_subring_demag",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.053f,
                            .amplitude = 0.065f,
                            .offset = 0.92f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Re)
      .add("doppler_limb_beaming",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.074f,
                            .amplitude = 0.08f,
                            .offset = 0.22f},
           control::ModTarget::LumaShapeDepth)
      .add("synchrotron_flare_amp",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.109f,
                            .amplitude = 0.08f,
                            .offset = 0.24f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
