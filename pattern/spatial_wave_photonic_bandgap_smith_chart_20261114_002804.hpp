#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Spatial Wave (Advanced): Photonic Band Gap on the Smith Chart.
 *
 * Scientific foundation:
 * - For a periodic layered medium (Bragg mirror, Kronig-Penney crystal) the
 *   complex reflection coefficient r after one more unit cell is a Mobius
 *   function of r before it: r' = e^{i phi} (r + rho) / (1 + rho r), with rho
 *   the Fresnel coefficient of the interface and phi the round-trip phase,
 *   i.e. the optical frequency. This is the Smith-chart rule of RF
 *   engineering and it maps the unit disk |r| < 1 onto itself.
 * - Its conjugacy class is the band structure. With trace
 *   2 cos(phi/2) / sqrt(1 - rho^2):
 *     |sin(phi/2)| < rho  hyperbolic: r runs to a point on |r| = 1, total
 *                         reflection, the stop band;
 *     |sin(phi/2)| > rho  elliptic: r circulates about an interior fixed
 *                         point, a propagating Bloch wave, the pass band;
 *     equality            parabolic: the band edge.
 *   As a circle map this is the 0/1 Arnold tongue of the Mobius family.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Invert. With the four affine slots,
 *       T(z) = zn3 + zn2 / (zn0 z + zn1)
 *   is a general Mobius map. Here zn0 = rho, zn1 = 1,
 *   zn2 = -e^{i phi} (1 - rho^2) / rho, zn3 = e^{i phi} / rho.
 * - Motion is the physics: four Sine lanes in quadrature turn zn2 and zn3
 *   together, so phi sweeps the full frequency axis once per 50 s. With
 *   rho = 0.32 the gap is |phi| < 0.65 rad, about a fifth of each sweep.
 * - Seed = WaveRing (concentric rings about r = 0, the matched load). In the
 *   pass band the rings are carried round an off-centre fixed point and beat
 *   against their own displaced copies; at the band edge the motion slows to
 *   horocycles; inside the gap everything streams into one boundary point.
 * - Color = SpatialWave. Corners outside the disk are folded by Torus.
 */
inline Pattern spatial_wave_photonic_bandgap_smith_chart() {
  constexpr float RHO = 0.32f;
  constexpr float POLE_GAIN = 2.805f;   // (1 - rho^2) / rho
  constexpr float SHIFT_GAIN = 3.125f;  // 1 / rho
  constexpr float SWEEP_HZ = 0.02f;
  constexpr float PHI0_CYCLES = 0.30f;  // start inside the pass band
  constexpr float PHI0 = 1.88495559f;   // 2 pi * 0.30

  Pattern p;
  p.name = "spatial_wave_photonic_bandgap_smith_chart";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {RHO, 0.0f};
  p.zn.zn[1] = {1.0f, 0.0f};
  p.zn.zn[2] = {-POLE_GAIN * std::cos(PHI0), -POLE_GAIN * std::sin(PHI0)};
  p.zn.zn[3] = {SHIFT_GAIN * std::cos(PHI0), SHIFT_GAIN * std::sin(PHI0)};

  p.color.hue_rotation = 0.011f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.985f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.3f;
  p.color.spiral_radius_coeff = 1.0f;
  p.color.chroma_breath_amp = 0.12f;
  p.color.chroma_breath_freq = 2.0f;
  p.color.chroma_breath_phase = 0.07f;
  p.color.hue_velocity_base = 0.015f;
  p.color.hue_velocity_breath = -0.009f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.07f;
  p.seed.hue = 0.55f;
  p.seed.hue_speed = 0.06f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.15f;
  p.speed = 0.9f;

  // zn2 = -G e^{i phi}, zn3 = S e^{i phi}, phi = 2 pi (f t + phi0).
  // Lane phase is in cycles; sin(2 pi (u + 0.25)) = cos(2 pi u).
  p.motion
      .add("frequency_sweep_pole_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SWEEP_HZ,
                            .amplitude = POLE_GAIN,
                            .offset = 0.0f,
                            .phase = PHI0_CYCLES + 0.75f},
           control::ModTarget::Zn2Re)
      .add("frequency_sweep_pole_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SWEEP_HZ,
                            .amplitude = POLE_GAIN,
                            .offset = 0.0f,
                            .phase = PHI0_CYCLES + 0.5f},
           control::ModTarget::Zn2Im)
      .add("frequency_sweep_shift_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SWEEP_HZ,
                            .amplitude = SHIFT_GAIN,
                            .offset = 0.0f,
                            .phase = PHI0_CYCLES + 0.25f},
           control::ModTarget::Zn3Re)
      .add("frequency_sweep_shift_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SWEEP_HZ,
                            .amplitude = SHIFT_GAIN,
                            .offset = 0.0f,
                            .phase = PHI0_CYCLES},
           control::ModTarget::Zn3Im)
      .add("bloch_wave_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.043f,
                            .amplitude = 0.05f,
                            .offset = 0.12f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
