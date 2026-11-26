#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Cochlear Tonotopy: von Bekesy Travelling Wave on the Basilar Membrane.
 *
 * Scientific foundation:
 * - G. von Bekesy (Nobel 1961): sound launches a travelling wave along the
 *   basilar membrane that peaks at a place set by frequency. The place map is
 *   exponential (Greenwood 1961: f = A*(10^(a*x) - k)): equal distances are
 *   equal musical intervals. The membrane is coiled as a logarithmic spiral
 *   of ~2.75 turns, and outer hair cells amplify the wave actively.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Exp: position -> exp(position), the Greenwood place-frequency
 *   law; it also turns straight feedback into the log-spiral coil.
 * - Reduction = Grid: the modular wrap is the octave: after each doubling the
 *   pattern repeats one register higher (pitch-class periodicity).
 * - Color = SpectralPrism: RGB = cosine palette of
 *   (angle + pitch*log|T|), with the three channel phases at 0, 1/3, 2/3 of a
 *   turn. Hue is therefore a pure function of log-frequency: colour literally
 *   is pitch, repeating per octave. The palette is recomputed each frame
 *   (not accumulated), so the tonotopic map stays coherent.
 * - Motion: octave pitch sweep, helicotrema twist, outer-hair-cell gain on
 *   SatScale (the cochlear amplifier), spiral-turn sweep.
 */
inline Pattern cochlear_tonotopic_traveling_wave() {
  constexpr float A0 = -0.26179939f; // -15 deg
  constexpr float A2 = 0.10471976f;  // 6 deg
  constexpr float PRE = 1.74f;
  constexpr float POST = 1.10f;
  const float zn2_im = POST * std::sin(A2);

  Pattern p;
  p.name = "cochlear_tonotopic_traveling_wave";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.10f, -0.06f};
  p.zn.zn[2] = {POST * std::cos(A2), zn2_im};
  p.zn.zn[3] = {-0.02f, 0.02f};

  p.color.hue_rotation = 0.016f;
  p.color.sat_scale = 1.84f;  // palette * luma * sat: ~unity mean gain
  p.color.fade = 0.989f;
  p.color.color_mode = as_int(ColorMode::SpectralPrism);
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = 1.0f;
  p.color.chroma_breath_amp = 0.20f;
  p.color.chroma_breath_freq = 0.16f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;    // palette phase R
  p.color.hue_velocity_breath = 0.33f; // palette phase G
  p.color.hue_velocity_drift = 0.67f;  // palette phase B
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.075f;
  p.seed.hue = 0.10f;
  p.seed.hue_speed = 0.09f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.10f;
  p.seed.pulse_amp = 0.18f;
  p.speed = 0.85f;

  p.motion
      .add("greenwood_octave_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.05f,
                            .amplitude = 0.30f,
                            .offset = 1.0f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("helicotrema_twist",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.065f,
                            .amplitude = 0.08f,
                            .offset = zn2_im},
           control::ModTarget::Zn2Im)
      .add("outer_hair_cell_gain",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.07f,
                            .amplitude = 0.10f,
                            .offset = 1.84f},
           control::ModTarget::SatScale)
      .add("travelling_wave_envelope",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.08f,
                            .amplitude = 0.06f,
                            .offset = 0.18f},
           control::ModTarget::SeedPulseAmp);

  return p;
}

} // namespace epi::presets
