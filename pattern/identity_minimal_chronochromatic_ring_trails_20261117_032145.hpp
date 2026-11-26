#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Minimal 3.5D): Chronochromatic Rings; every Crest Drags a
 * Rainbow of its own Past.
 *
 * Mathematics:
 * - With T = id the loop is a pure delay line: generation n is the ring
 *   wave of n frames ago, S(r, t - n Delta), Delta = speed / fps of seed
 *   time per frame. OKLab rotates its hue by n h.
 * - A crest of the ring wave sin(20 r - 5 t) moves out at 5/20 = 0.25 units
 *   per second of seed time, so in generation n it sits a distance
 *   n x 0.25 Delta inside where it is now (0.0033 per frame at 60 fps,
 *   speed 0.8). Each crest is drawn at its present radius in the seed hue
 *   and, inside it, at its past radii, each n steps older hued n h later:
 *   a rainbow wake whose colour is the age of the light, a spectrum laid
 *   out along time. With fade 0.93 the trail is about 14 frames long,
 *   0.047 units (25 px at 1080p), spanning 0.42 of the hue circle at
 *   h = 0.03.
 * - Exact: no map, no reduction error.
 *
 * Pipeline: Identity, all slots trivial; Clamp (never reached); WaveRing at
 * alpha 0.5; fade 0.93; OKLab step 0.03.
 *
 * Fidelity:   identity
 * Cell:       Identity / Clamp / WaveRing / OKLabColorRotation
 * Nearest:    identity_lock_in_amplifier_tuned_by_the_hue_step (the same
 *             delay line in Opponent: a filter); here in OKLab: a trail.
 * Height:     age = passes = time since the light was emitted.
 * Hue clock:  bounded; step 0.03 +- 0.005.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. The trail length is
 *             0.2 x 14 / fps units: frame-rate dependent. Not built,
 *             rendered or measured.
 * @assert p_N >= 0.95
 */
inline Pattern identity_minimal_chronochromatic_ring_trails() {
  Pattern p;
  p.name = "identity_minimal_chronochromatic_ring_trails";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.03f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.93f;
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

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.5f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("trail_spectrum",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.005f,
                            .offset = 0.03f},
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
