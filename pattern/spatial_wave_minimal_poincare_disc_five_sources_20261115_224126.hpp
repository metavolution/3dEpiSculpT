#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal 3.5D): Five Ring Sources on a Hyperbolic Circle.
 *
 * Mathematics:
 * - The unit disc with the Poincare metric has a rotation about every
 *   point. The rotation by 72 degrees about p = 1/2 is the Mobius map
 *       T = M^-1 o (e^{2 pi i / 5} .) o M,   M(z) = (z - p) / (1 - p z).
 *   It is an isometry of the disc of order 5: T^5 is the identity, and no
 *   orbit that starts in the disc leaves it.
 * - The seed is a ring wave centred at 0. Its five images are centred at
 *       0,  0.476 -+ 0.393 i,  0.771 -+ 0.150 i,
 *   which lie on the hyperbolic circle about p through 0: the Euclidean
 *   circle of centre 0.4 and radius 0.4. They are the vertices of a
 *   regular hyperbolic pentagon.
 * - Mobius maps send circles to circles, so every ring of every source is
 *   a true circle, though the five families are no longer concentric.
 *   Each is a pencil of circles about one source and its mirror image in
 *   the unit circle.
 * - In the four slots, T(z) = zn3 + zn2 / (z + zn1) with
 *   k = cot 36 deg = 1.3763819:
 *       zn1 = -((1 + p^2) + i k (1 - p^2)) / (2p) = (-1.25, -1.0322864)
 *       zn3 =  ((1 + p^2) - i k (1 - p^2)) / (2p) = ( 1.25, -1.0322864)
 *       zn2 = (1 - p^2)^2 (1 + k^2) / (4 p^2)     =   1.6281153 (real).
 *   The pole is at 1.25 + 1.03 i, outside the frame.
 *
 * Pipeline: Invert / Clamp / WaveRing. The geometry is fixed and exact;
 * the motion is the travelling rings. The hue step is 1/5 turn per pass,
 * so the five sources hold five fixed hues; one lane detunes the step by
 * +-0.008 and lets the hues drift past one another. The four corners of
 * the frame lie outside the disc and are outside the claim.
 *
 * Fidelity:   identity
 * Cell:       Invert / Clamp / WaveRing / SpatialWave
 * Nearest:    spatial_wave_minimal_anharmonic_triality (order 3, a
 *             rotation of the sphere that moves infinity into the frame);
 *             spatial_wave_heptagonal_ring_array_moire (a Euclidean
 *             array). This one is a hyperbolic rotation, and the disc is
 *             invariant.
 * Height:     age modulo 5.
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.026 Hz
 * Status:     designed from source on 2026-11-15; T^5 = identity to
 *             2e-6 and the invariance of the disc were confirmed in a
 *             separate arithmetic check; not built, rendered or measured.
 * @assert p_N >= 0.75 && rot.q == 5
 */
inline Pattern spatial_wave_minimal_poincare_disc_five_sources() {
  Pattern p;
  p.name = "spatial_wave_minimal_poincare_disc_five_sources";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {-1.25f, -1.0322864f};
  p.zn.zn[2] = {1.6281153f, 0.0f};
  p.zn.zn[3] = {1.25f, -1.0322864f};

  p.color.hue_rotation = 0.2f; // one fifth of a turn per pass
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.0f;
  p.color.chroma_breath_amp = 0.02f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.004f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.6f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.5f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("hue_step_detune",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.007f,
                            .amplitude = 0.008f,
                            .offset = 0.2f},
           control::ModTarget::HueRot)
      .add("source_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.011f,
                            .amplitude = 0.1f,
                            .offset = 0.6f},
           control::ModTarget::SeedHue)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.026f,
                            .amplitude = 0.012f,
                            .offset = 0.02f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
