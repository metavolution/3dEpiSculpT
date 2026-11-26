#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Seashell Pigmentation: Meinhardt's Space-Time Record on an Accreting Lip.
 *
 * Scientific foundation:
 * - H. Meinhardt (The Algorithmic Beauty of Sea Shells, 1995): a mollusc only
 *   deposits shell and pigment along its growing edge. The 2D pattern on the
 *   shell is therefore the time record of a 1D activator-inhibitor reaction
 *   running on that edge: stripes, chevrons and the triangles of Conus
 *   textile are space-time diagrams. D'Arcy Thompson / Raup (1966): the shell
 *   itself grows as a logarithmic spiral with whorl expansion rate W.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - The pipeline shares this architecture: fresh structure is written only at
 *   the seed border (the lip), and feedback carries older rows inward and
 *   shrinks them (accretion). The frame is a space-time record.
 * - Inner = F14 (z + i*cos(sinh(z))): the identity (pure accretion) plus a
 *   bounded oscillatory displacement (the pigment oscillator). The offset
 *   f(0) = i sends the centre straight to the top border, so even with a
 *   contracting linear part (|zn0*zn2| = 0.79) every region is fed by the lip.
 * - Thin seed (0.02) with a pronounced pulse: pigment is laid down in bouts.
 * - Color = Alive; warm seed hue. Motion: whorl expansion rate on Zn2Re,
 *   pigment-oscillator chroma, inhibitor ring pitch.
 */
inline Pattern seashell_meinhardt_accretion_lip() {
  Pattern p;
  p.name = "seashell_meinhardt_accretion_lip";
  p.inner = InnerFunction::F14;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {0.82f, 0.12f};
  p.zn.zn[1] = {0.0f, -0.02f};
  p.zn.zn[2] = {0.95f, 0.03f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.10f;
  p.zn.zn1_anim_amp = 0.03f;
  p.zn.zn1_anim_k = 5.0f;

  p.color.hue_rotation = 0.009f;
  p.color.sat_scale = 1.14f;
  p.color.fade = 0.988f;
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = 0.45f;
  p.color.chroma_breath_amp = 0.20f;
  p.color.chroma_breath_freq = 0.11f;
  p.color.chroma_breath_phase = 0.30f;
  p.color.hue_velocity_base = 0.015f;   // activator
  p.color.hue_velocity_breath = 0.012f; // inhibitor
  p.color.hue_velocity_drift = 0.004f;  // substrate depletion
  p.color.luma_shape_depth = 0.20f;
  p.color.luma_shape_center = 1.8f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.02f;
  p.seed.hue = 0.08f;
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.86f;
  p.seed.pulse_freq = 0.09f;
  p.seed.pulse_amp = 0.20f;
  p.speed = 0.5f;

  p.motion
      .add("whorl_expansion_rate",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.04f,
                            .amplitude = 0.04f,
                            .offset = 0.95f},
           control::ModTarget::Zn2Re)
      .add("pigment_oscillator",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.09f,
                            .amplitude = 0.08f,
                            .offset = 0.20f},
           control::ModTarget::ChromaBreathAmp)
      .add("inhibitor_diffusion_rings",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.06f,
                            .amplitude = 0.12f,
                            .offset = 0.45f},
           control::ModTarget::SpiralRadiusCoeff);

  return p;
}

} // namespace epi::presets
