#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Minimal 3.5D): a Whirlpool of Nested Chladni Plates.
 *
 * Mathematics:
 * - T(z) = lambda z, lambda = 1.2 e^(i theta). Exact under Clamp: |T z| =
 *   1.2 |z|, so points with r >= 1 stay outside the plate's disc, and
 *   clamped points (on the border, r >= 1) too.
 * - Generation n is the plate shrunk by 1.2^-n and turned by -n theta, all
 *   centred on 0: a Droste nest. The plate's nodal set contains both
 *   diagonals x = +-y (there cos(n pi x)cos(m pi y) = cos(m pi x)cos(n pi y)
 *   for every n, m), so copy n carries four nodal rays from the centre to
 *   radius 0.8 x 1.2^-n at angles pi/4 + k pi/2 - n theta. Their endpoints
 *   lie on the logarithmic spirals r = 0.8 e^((phi - phi_k) ln 1.2 / theta):
 *   four whirlpool arms.
 * - Two quadrature lanes turn theta once round in 1000 s. Copy n then turns
 *   n times as fast as theta: the depths spin faster than the rim, like
 *   water draining. At theta = 0 (mod a quarter turn, the plate's own
 *   symmetry) all copies align, and the arms straighten into four radial
 *   spokes, four times per cycle.
 *
 * Pipeline: Identity, zn0 = 1.2 e^(i theta) on the lanes; Clamp; Chladni at
 * alpha 0.6; fade 0.9 (copy 20 has radius 0.026); OKLab step 1/16, so hue
 * reads depth.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Identity / Clamp / ChladniCymatics / OKLabColorRotation
 * Nearest:    invert_minimal_apollonian_droste_tunnel (a Mobius nest, off
 *             centre, static twist); identity_minimal_golden_angle_chladni_
 *             three_gap_stack (turns, no scaling).
 * Height:     age = passes = depth in the nest.
 * Hue clock:  bounded; step 1/16.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. Copy 20 turns at 0.02
 *             turn/s. Not built, rendered or measured.
 * @assert p_E >= 0.95
 */
inline Pattern identity_minimal_plate_whirlpool_droste() {
  constexpr float S = 1.2f;
  constexpr float TWIST_HZ = 0.001f;
  Pattern p;
  p.name = "identity_minimal_plate_whirlpool_droste";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {S, 0.0f}; // theta = 0 at rest: four spokes
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0625f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.9f;
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
  p.seed.hue = 0.55f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.6f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("twist_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TWIST_HZ,
                            .amplitude = S,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn0Re)
      .add("twist_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TWIST_HZ,
                            .amplitude = S,
                            .offset = 0.0f,
                            .phase = 0.0f},
           control::ModTarget::Zn0Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.02f,
                            .offset = 0.04f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
