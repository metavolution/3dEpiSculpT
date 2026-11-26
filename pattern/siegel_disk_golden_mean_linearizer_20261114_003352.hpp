#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Nonlinear Dynamics: Golden-Mean Siegel Disk and its Linearizing Coordinate.
 *
 * Scientific foundation:
 * - Siegel (1942): if a holomorphic map has a fixed point with multiplier
 *   e^{2 pi i theta} and theta is Diophantine, it is conjugate to the rigid
 *   rotation by theta on a neighbourhood, the Siegel disk. The disk is
 *   foliated by invariant closed curves, the level sets |phi| = const of the
 *   linearizing coordinate phi. For the golden mean (the "most irrational"
 *   number) and a quadratic map the disk reaches out to the critical point,
 *   and its boundary is a quasi-circle that is nowhere smooth.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = P3: with zn0 = 1.92 and zn2 = (2 / 1.92) e^{2 pi i theta} the
 *   loop iterates f(w) = lambda w (1 + w), w = 1.92 z, with the indifferent
 *   fixed point at the centre of the frame. theta = (sqrt(5) - 1) / 2.
 * - Inside the disk the map neither expands nor contracts, so the feedback
 *   cannot build structure by escape. It does something else: each pass
 *   moves the picture one rotation step along the invariant curves, and the
 *   fade-weighted sum of passes is an ergodic average along each curve. The
 *   steady luminance is therefore constant on invariant curves. The loop
 *   computes the level sets of the linearizer by averaging; nothing in the
 *   preset knows phi.
 * - Seed = WaveRing centred on the fixed point. Close to the centre the
 *   invariant curves are circles and the rings pass through unchanged;
 *   further out they are averaged over curves that are no longer round, and
 *   the difference between the two families is what you see.
 * - Reduce = Clamp: the basin of infinity goes to the border, where WaveRing
 *   is nearly transparent, and stays dark. Every preimage of the disk shows
 *   the same ring structure pulled back.
 * - Color = ChromaBreathing: hue is a fixed function of log|T|, luminance is
 *   whatever the averaging produced.
 * - The multiplier is exact and is not modulated: off the golden mean the
 *   disk would shrink or vanish (any rational rotation number destroys it).
 */
inline Pattern siegel_disk_golden_mean_linearizer() {
  constexpr float GOLDEN_ROTATION = 3.88322208f; // 2 pi (sqrt(5) - 1) / 2
  constexpr float VIEW = 1.92f;
  constexpr float POST = 1.04166667f;            // 2 / VIEW

  Pattern p;
  p.name = "siegel_disk_golden_mean_linearizer";
  p.inner = InnerFunction::P3;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {VIEW, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {POST * std::cos(GOLDEN_ROTATION),
                POST * std::sin(GOLDEN_ROTATION)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f; // ChromaBreathing has no hue clock
  p.color.sat_scale = 1.05f;
  p.color.fade = 0.985f;
  p.color.color_mode = as_int(ColorMode::ChromaBreathing);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.6f; // hue from log|T|: rings of colour
  p.color.chroma_breath_amp = 0.15f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.002f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.07f;
  p.seed.hue = 0.5f;
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.90f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("ring_hue_pitch",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.013f,
                            .amplitude = 0.2f,
                            .offset = 0.6f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.021f,
                            .amplitude = 0.08f,
                            .offset = 0.15f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
