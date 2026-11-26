#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Op-Art: Zoellner Illusion (Parallel Lines Bent by Oblique Hatching).
 *
 * Scientific foundation:
 * - J. K. F. Zoellner (1860): long parallel lines crossed by short oblique
 *   strokes, with the stroke direction alternating from line to line, look
 *   convergent and divergent. Acute angles are perceptually over-estimated
 *   (lateral inhibition between orientation columns in V1, Blakemore et al.
 *   1970), which rotates each long line away from its hatching. The effect
 *   peaks for crossing angles of roughly 10-30 deg.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Tan: poles at Re w = +/- pi/2 with period pi. Between poles tan
 *   sweeps the whole real line, so after the Grid wrap each period becomes a
 *   sheaf of ever denser parallel stripes: the long lines.
 * - Pre-affine at +20 deg, post-affine at -40 deg: the net turn per pass is
 *   -20 deg, so consecutive generations cross the previous stripes at
 *   alternating +/-20 deg, inside the peak range of the illusion. The
 *   herringbone comes from the feedback recurrence; it is not drawn.
 * - Motion: a triangle lane rocks the hatch angle (Zn2Im) through the
 *   10-30 deg band, so the "parallels" visibly lean one way, then the other.
 * - Color = QuasiPeriodicWave: three incommensurate hue waves (1, phi,
 *   sqrt 2) avoid a colour period that would compete with the stripes.
 */
inline Pattern op_art_zollner_herringbone_tilt() {
  constexpr float A0 = 0.34906585f;  // +20 deg
  constexpr float A2 = -0.69813170f; // -40 deg
  constexpr float PRE = 1.62f;  // reaches the tan poles at |z| ~ 0.97
  constexpr float POST = 1.15f;
  const float zn2_im = POST * std::sin(A2);

  Pattern p;
  p.name = "op_art_zollner_herringbone_tilt";
  p.inner = InnerFunction::Tan;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.02f, 0.008f};
  p.zn.zn[2] = {POST * std::cos(A2), zn2_im};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.07f;
  p.zn.zn0_anim_amp = 0.04f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.05f;
  p.zn.zn1_anim_amp = 0.025f;
  p.zn.zn1_anim_k = 2.0f;

  p.color.hue_rotation = 0.005f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.987f;
  p.color.color_mode = as_int(ColorMode::QuasiPeriodicWave);
  p.color.spiral_angle_coeff = 0.5f;
  p.color.spiral_radius_coeff = 0.25f;
  p.color.chroma_breath_amp = 0.12f;
  p.color.chroma_breath_freq = 0.10f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.011f;
  p.color.hue_velocity_breath = 0.007f;
  p.color.hue_velocity_drift = 0.005f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.09f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.08f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.08f;
  p.seed.pulse_amp = 0.15f;
  p.speed = 0.8f;

  p.motion
      .add("hatch_angle_rock",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.045f,
                            .amplitude = 0.09f,
                            .offset = zn2_im},
           control::ModTarget::Zn2Im)
      .add("parallel_spacing",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.06f,
                            .amplitude = 0.08f,
                            .offset = 0.25f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("tilt_chroma",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.08f,
                            .amplitude = 0.05f,
                            .offset = 0.12f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
