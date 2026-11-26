#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Advanced): Chebyshev Overtone Ladder of a Square Plate.
 *
 * Scientific foundation:
 * - The Helmholtz eigenmodes of a free square plate are products
 *   cos(n pi x) cos(m pi y); Chladni figures are the nodal lines of their
 *   combinations.
 * - Chebyshev semiconjugacy: T_k(cos t) = cos(k t). In the angle variable the
 *   Chebyshev polynomial T_k is the k-fold tent map, so composing a cosine
 *   mode with the tent map multiplies its mode number by k exactly:
 *       cos(n pi * tent_k(x)) = cos(k n pi x)   for integer n.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Identity with zn0 = 2, Reduce = Torus. The Torus reduction is a
 *   triangle wave, so Reduce(2 z) is precisely the 2-fold tent map on each
 *   axis. No other reduction has this property.
 * - Seed = ChladniCymatics, which draws the nodal lines of
 *   cos(n pi x) cos(m pi y) - cos(m pi x) cos(n pi y).
 * - Result: generation k of the feedback shows the same Chladni figure with
 *   mode numbers (2^k n, 2^k m). The frame is the whole octave ladder of
 *   overtones at once, one hue step per octave, each octave visible through
 *   the gaps between the nodal lines of the one below.
 * - Exactness: the identity holds when n and m are integers. The seed
 *   animates them continuously (n = 3 + sin, m = 5 + cos), so between
 *   integers each octave is the mirror-continued (Neumann) extension, still
 *   continuous but creased on the fold lines.
 * - Fade = 0.90 plays the role of a 1/f spectral roll-off: octave 8 would be
 *   at pixel scale and must be faint.
 * - Motion: Zn3 shifts the sampling phase of every octave (a phase-shifted
 *   cosine is still an eigenfunction), so the overtones slide against the
 *   fundamental.
 */
inline Pattern spatial_wave_chebyshev_overtone_ladder() {
  Pattern p;
  p.name = "spatial_wave_chebyshev_overtone_ladder";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {2.0f, 0.0f}; // integer degree: one octave per pass
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.11f; // hue step per octave
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.90f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 2.0f;
  p.color.chroma_breath_phase = 0.03f;
  p.color.hue_velocity_base = 0.010f;
  p.color.hue_velocity_breath = 0.006f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.09f;
  p.seed.hue_speed = 0.04f;
  p.seed.alpha = 0.95f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("overtone_phase_x",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.30f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("overtone_phase_y",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.023f,
                            .amplitude = 0.30f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("octave_hue_step",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.013f,
                            .amplitude = 0.04f,
                            .offset = 0.11f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
