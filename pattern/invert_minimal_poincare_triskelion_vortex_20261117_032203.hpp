#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Minimal 3.5D): a Triskelion Vortex in the Poincare Disc.
 *
 * Mathematics:
 * - T is the elliptic Mobius map with fixed points a = 0.45 and 1/a and
 *   multiplier e^(2 pi i/3) at a: a hyperbolic third-turn of the disc about
 *   a, T^3 = id, keeping the disc and its outside (checked: T^3 = id to
 *   8e-16, the circle kept to 9e-16).
 * - Exact everywhere: the Chladni seed is zero for r >= 1 and nothing
 *   outside the disc ever re-enters it.
 * - Opponent with step 1/3: the chroma is sum_j e^(2 pi i j/3) P(T^j z).
 *   At a only the Taylor terms d^a conj(d)^b with a - b = -1 mod 3 survive,
 *   so the colour winds once backwards round a: a charge -1 vortex, all
 *   three hues meeting at one point, three plate copies swirled about it.
 *   Its partner (+1) sits at 1/a, outside the disc, where the seed is
 *   zero: within the disc the vortex is alone.
 * - Turntable lanes carry a round the circle |a| = 0.45 in 500 s; the
 *   vortex orbits the centre over the fixed plate.
 *
 * Pipeline: Invert with the slots below; Clamp; Chladni at alpha 0.12;
 * fade 0.99; Opponent step 1/3.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Invert / Clamp / ChladniCymatics / Opponent
 * Nearest:    invert_minimal_poincare_pentagonal_rosette_orbit (order 5,
 *             OKLab: a rosette, no vortex); here order 3 in Opponent.
 * Height:     age = passes; the three generations are the three group
 *             elements.
 * Hue clock:  bounded; step 1/3 +- 0.004.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; slots and lane phases
 *             evaluated arithmetically. With rho < 1 the core is displaced by
 *             order 1 - rho; a Chladni nodal kink crossing a leaves the charge
 *             momentarily undefined. Not built, rendered or measured.
 * @assert p_N >= 0.75
 */
inline Pattern invert_minimal_poincare_triskelion_vortex() {
  constexpr float ORBIT_HZ = 0.002f;
  Pattern p;
  p.name = "invert_minimal_poincare_triskelion_vortex";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.5f, -0.8660254f};        // 1 - e^(2 pi i/3)
  p.zn.zn[1] = {-2.44722222f, 0.38971143f};
  p.zn.zn[2] = {1.5703858f, -0.90666267f};
  p.zn.zn[3] = {1.33611111f, -0.51159649f};

  p.color.hue_rotation = 0.33333333f; // charge -1 at a
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.99f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.03f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.12f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("turntable_zn0_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 1.73205081f,
                            .offset = 0.0f,
                            .phase = 0.33333333f},
           control::ModTarget::Zn0Re)
      .add("turntable_zn0_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 1.73205081f,
                            .offset = 0.0f,
                            .phase = 0.58333333f},
           control::ModTarget::Zn0Im)
      .add("turntable_zn2_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 1.81332533f,
                            .offset = 0.0f,
                            .phase = 0.16666667f},
           control::ModTarget::Zn2Re)
      .add("turntable_zn2_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 1.81332533f,
                            .offset = 0.0f,
                            .phase = 0.91666667f},
           control::ModTarget::Zn2Im)
      .add("turntable_zn3_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 1.43070747f,
                            .offset = 0.0f,
                            .phase = 0.19180048f},
           control::ModTarget::Zn3Re)
      .add("turntable_zn3_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 1.43070747f,
                            .offset = 0.0f,
                            .phase = 0.94180048f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.015f,
                            .offset = 0.03f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
