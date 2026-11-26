#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * MagneticDipole (Advanced 3.5D): a Blaschke Product whose Rim Shatters into
 * Cantor Dust and Heals Again.
 *
 * Mathematics:
 * - The dipole inner (w^2 - 1)/(w^2 + 1) composed with affine maps reaches
 *   every quadratic rational map with critical points 0 and infinity. With
 *   zn0^2 = k, zn2 = e^(i theta) (1/k - k)/2, zn3 = e^(i theta) (1/k + k)/2
 *   it is the degree-2 Blaschke product
 *     B(z) = e^(i theta) (z^2 + k) / (1 + k z^2),  k = 0.6,
 *   zeros +-i sqrt k. B maps the disc, the circle and the outside each onto
 *   itself, so with the disc-masked Chladni seed and Clamp the picture is
 *   exact (as for the disc isometries).
 * - On the circle |B'| = sum (1 - k)/|z - a_j|^2 has minimum
 *   2 (1 - k)/(1 + k) = 1/2 at z = +-1. By Denjoy-Wolff, if a fixed point on
 *   the circle has |B'| < 1 it attracts the whole disc and the Julia set is
 *   a Cantor set on the circle; otherwise the attracting point is inside
 *   and the Julia set is the whole circle. Solving gives: Cantor phase for
 *   |theta| < 16.98 degrees and |theta - 180| < 16.98 degrees (18.9 % of the
 *   walk), circle phase elsewhere; parabolic at the four edges.
 * - Generation n is the plate pulled back by B^n: 2^n copies, crowding onto
 *   the Julia set. In the circle phase they line the whole rim; in the
 *   Cantor phase they gather on the dust and leave gaps, the arcs through
 *   which inside and outside basins join. The walk crosses this change of
 *   topology four times per turn.
 *
 * Pipeline: MagneticDipole, zn0 = sqrt 0.6, zn1 = 0; quadrature lanes turn
 * zn2 and zn3 by theta at 0.0025 Hz (400 s a turn; each Cantor phase lasts
 * 38 s). Rest state theta = 0: the Cantor phase, fixed point 1 with
 * multiplier 1/2. Clamp; Chladni at alpha 0.6; fade 0.95; OKLab step 1/12.
 *
 * Fidelity:   identity for every lane value
 * Cell:       MagneticDipole / Clamp / ChladniCymatics / OKLabColorRotation
 * Nearest:    magnetic_dipole_* rows use the dipole on Torus or
 *             AmmannBeenker; here it is tuned to a Blaschke product.
 *             square_minimal_fresnel_zone_plate_from_a_ring_wave is the
 *             case k = 0 (z^2).
 * Height:     age = passes; generation n has 2^n copies.
 * Hue clock:  bounded; step 1/12.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; slots, |B'| minimum and
 *             the Cantor arcs evaluated arithmetically. Near the rim the
 *             copies of deep generations fall below a pixel. Not built,
 *             rendered or measured.
 * @assert sing[0].escapes == 0 && p_C >= 0.5
 */
inline Pattern magnetic_dipole_blaschke_rim_shatters_into_cantor_dust() {
  constexpr float WALK_HZ = 0.0025f;
  Pattern p;
  p.name = "magnetic_dipole_blaschke_rim_shatters_into_cantor_dust";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.77459667f, 0.0f}; // sqrt k
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.53333333f, 0.0f}; // (1/k - k)/2
  p.zn.zn[3] = {1.13333333f, 0.0f}; // (1/k + k)/2

  p.color.hue_rotation = 0.08333333f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.72f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.6f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("rim_walk_zn2_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = 0.53333333f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Re)
      .add("rim_walk_zn2_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = 0.53333333f,
                            .offset = 0.0f,
                            .phase = 0.0f},
           control::ModTarget::Zn2Im)
      .add("rim_walk_zn3_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = 1.13333333f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("rim_walk_zn3_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = 1.13333333f,
                            .offset = 0.0f,
                            .phase = 0.0f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.025f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
