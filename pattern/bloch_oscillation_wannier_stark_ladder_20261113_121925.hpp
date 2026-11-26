#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Bloch Oscillations and the Wannier-Stark Ladder.
 *
 * Scientific foundation:
 * - F. Bloch (1929), C. Zener (1934): an electron in a periodic lattice under
 *   a constant force F does not accelerate away. Its crystal momentum grows
 *   linearly, hbar*dk/dt = F, but k lives on the periodic Brillouin zone, so
 *   with the band E(k) = -2J*cos(k*a) the velocity dE/dk oscillates and the
 *   electron swings back and forth with period h/(F*a). The spectrum becomes
 *   the equally spaced Wannier-Stark ladder. Seen in semiconductor
 *   superlattices (1992) and cold atoms in optical lattices (1996).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Cos: the tight-binding band dispersion.
 * - Reduction = Grid: plain modular wrap with period 2, the Brillouin zone
 *   with its k ~ k + 2*pi/a identification.
 * - Motion lane "constant_force_k_drift": a SAWTOOTH of amplitude exactly 1 on
 *   Zn3Re. Because Grid wraps with period 2, a ramp from -1 to +1 followed by
 *   the jump back is perfectly seamless: zn3 drifts at constant velocity
 *   forever, exactly dk/dt = F on a periodic zone. The uniform drift of k
 *   through the cosine band makes the picture swing instead of scroll away.
 * - Color = AtmosphericDrift: hue ramp along x (the Stark tilt of the ladder)
 *   with luma-preserving recolouring.
 */
inline Pattern bloch_oscillation_wannier_stark_ladder() {
  constexpr float A0 = 0.26179939f;  // 15 deg
  constexpr float A2 = -0.41887902f; // -24 deg
  constexpr float PRE = 1.32f;
  constexpr float POST = 1.06f;

  Pattern p;
  p.name = "bloch_oscillation_wannier_stark_ladder";
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.03f, -0.02f};
  p.zn.zn[2] = {POST * std::cos(A2), POST * std::sin(A2)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.10f;
  p.zn.zn0_anim_amp = 0.04f;

  p.color.hue_rotation = 0.008f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::AtmosphericDrift);
  p.color.spiral_angle_coeff = 0.5f;   // hue tilt along x: the Stark ladder
  p.color.spiral_radius_coeff = 0.24f;
  p.color.chroma_breath_amp = 0.16f;
  p.color.chroma_breath_freq = 0.21f;
  p.color.chroma_breath_phase = 0.04f;
  p.color.hue_velocity_base = 0.016f;
  p.color.hue_velocity_breath = 0.008f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.075f;
  p.seed.hue = 0.33f;
  p.seed.hue_speed = 0.08f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.09f;
  p.seed.pulse_amp = 0.16f;
  p.speed = 0.8f;

  p.motion
      .add("constant_force_k_drift",
           control::ModLane{.wave = control::Waveform::Sawtooth,
                            .frequency_hz = 0.025f,
                            .amplitude = 1.0f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("stark_ladder_hue_tilt",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.06f,
                            .amplitude = 0.16f,
                            .offset = 0.5f},
           control::ModTarget::SpiralAngleCoeff)
      .add("bandwidth_breath",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.08f,
                            .amplitude = 0.06f,
                            .offset = 0.16f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
