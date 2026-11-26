#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Advanced 3.5D): a Ring Wave Interferes with its own Image under
 * the Modular Group.
 *
 * Mathematics:
 * - The map of invert_minimal_modular_triangle_mirror_box: 4 / (z - 1) + 1
 *   with the Torus fold, a product of reflections whose fundamental pieces
 *   are (2, 3, infinity) triangles. On a piece every point a has period 2:
 *   a -> b -> a with b = 4 / (a - 1) + 1 inside the box.
 * - Opponent colour is linear in RGB; with a step of exactly 1/2 it
 *   reverses chroma on every pass. Summing the geometric series of the
 *   update on the cycle, with rho = fade (1 - seed alpha), the settled
 *   chroma at a is proportional to rho s(a) - s(b) and the light to
 *   s(b) + rho s(a): the colour is the difference between the seed at a
 *   point and at its image, the light is their sum (up to the [0, 1]
 *   clamp).
 * - The WaveRing seed is a travelling ring wave, sin(20 r - 5 t) e^(-2 r),
 *   cut to its positive half. At a and at b it has the phases 20 |a| - 5 t
 *   and 20 |b| - 5 t: the same frequency and a fixed phase difference
 *   20 (|a| - |b|). Their difference is a two-beam interference pattern:
 *   standing fringes along the curves |a| - |b| = const, where the colour
 *   cancels or doubles, with the carrier running through them in time.
 *   The partner b comes from a Mobius map, so the fringes are a property
 *   of the group, not of the seed.
 *
 * Pipeline: Invert (zn0 = 1, zn1 = -1, zn2 = 4, zn3 = 1), Torus, WaveRing at
 * alpha 0.25 (rho between 0.73 and 0.97, close to 1 so the difference is
 * nearly balanced), fade 0.97, Opponent with step 1/2.
 *
 * Fidelity:   identity
 * Cell:       Invert / Torus / WaveRing / Opponent
 * Nearest:    invert_minimal_modular_triangle_mirror_box (same map, Cross
 *             seed, hue by depth); magnetic_dipole_octagonal_complementary_
 *             lakes (same chroma reversal, static seed). Here the seed moves
 *             and the reversal turns it into interference.
 * Height:     age = passes; the fringe order is |a| - |b| in units of pi/10.
 * Hue clock:  bounded; step 1/2 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. The linear analysis
 *             ignores the dependence of seed alpha on the ring intensity and
 *             the clamp; the carrier moves at the WaveRing's own rate, as in
 *             every WaveRing preset. Not built, rendered or measured.
 * @assert p_X == 0 && p_C <= 0.05
 */
inline Pattern invert_modular_wave_meets_its_mirror_image() {
  Pattern p;
  p.name = "invert_modular_wave_meets_its_mirror_image";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {-1.0f, 0.0f};
  p.zn.zn[2] = {4.0f, 0.0f};
  p.zn.zn[3] = {1.0f, 0.0f};

  p.color.hue_rotation = 0.5f; // chroma = seed here minus seed at the image
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.97f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.04f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0004f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.55f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.25f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("fringe_balance",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.01f,
                            .offset = 0.5f},
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
