#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Magnetic Dipole (Advanced 3.5D): the Mating of the Basilica and the
 * Rabbit.
 *
 * Mathematics:
 * - f(u) = (sqrt 3 / 2) (u^2 - 1) / (u^2 + 1) + i / 2, that is
 *   (P u^2 + Q) / (u^2 + 1) with P = e^{i pi / 6} and Q = -1 / P. Its two
 *   critical points are 0 and infinity, f(0) = Q and f(infinity) = P, and
 *       0 -> Q -> 0,        infinity -> P -> i -> infinity.
 *   Two disjoint superattracting cycles, of periods 2 and 3.
 * - Asking for exactly that in the family (P u^2 + Q) / (u^2 + 1) gives
 *   Q = -1 / P and P^2 - 1 = +-i P: four solutions with |P| = 1, which
 *   are two maps and their mirror images. The quadratic polynomials with
 *   a critical cycle of period 2 and 3 are the basilica and the rabbit,
 *   co-rabbit and airplane; the mating with the airplane is obstructed
 *   (same limb), so the two maps are the matings of the basilica with the
 *   rabbit and with the co-rabbit. Which sign is which was not determined.
 * - In a mating the two filled Julia sets are glued along their
 *   boundaries and together make the whole sphere. Here the basin of the
 *   3-cycle is the rabbit's interior and the basin of the 2-cycle is the
 *   basilica's; each is dense in the other's boundary, and the Julia set
 *   is the single curve they share.
 *
 * Pipeline: MagneticDipole with zn0 = 2.8, zn2 = (sqrt 3 / 2) / 2.8,
 * zn3 = (i / 2) / 2.8: T(z) = f(2.8 z) / 2.8. The frame |u| <= 2.8 holds
 * the Julia set (measured extent 2.57 by 2.28). Only the rabbit's cycle
 * passes through infinity, so only the rabbit's lakes are ever sent to
 * the Simplex border by Clamp: they are lit, in three hues 120 degrees
 * apart by OKLab rotation with step 1/3, and terraced by depth. The
 * basilica's lakes circle between 0 and Q, never reach the seed, and stay
 * black. The picture is the mating drawn in positive and negative.
 *
 * Fidelity:   identity
 * Cell:       MagneticDipole / Clamp / Simplex / OKLabColorRotation
 * Nearest:    magnetic_dipole_minimal_wada_three_lakes has one cycle that
 *             carries both critical points, so nothing is dark;
 *             douady_rabbit_three_phase_ring_trap is the rabbit alone,
 *             with the plane outside it. Here the outside is a basilica.
 * Height:     age = passes since first exit; terraces of three passes.
 * Hue clock:  bounded; step 1/3 +- 0.012 from the lane, which lets the
 *             terraces drift apart in hue and close again.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-16. Both cycles were
 *             confirmed in arithmetic (residuals 1e-16) and stayed
 *             attracting with periods 2 and 3 for a real shift of +-0.05
 *             in the constant term, more than twice the lane. On a grid, the
 *             rabbit basin was 60 % of a 2.3 frame, fully lit, with
 *             (k - 1) mod 3 matching the lake for 97 %; the 2.8 frame was
 *             chosen from the measured extent and was not itself gridded.
 *             Not built, rendered or measured.
 * @assert sing[0].period == 2 && p_X >= 0.5
 */
inline Pattern magnetic_dipole_mating_basilica_rabbit_obsidian_lakes() {
  Pattern p;
  p.name = "magnetic_dipole_mating_basilica_rabbit_obsidian_lakes";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.8f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.30929479f, 0.0f}; // (sqrt 3 / 2) / 2.8
  p.zn.zn[3] = {0.0f, 0.17857143f}; // (1 / 2) / 2.8

  p.color.hue_rotation = 0.33333333f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.93f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.06f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.55f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("rabbit_clock_detune",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.006f,
                            .amplitude = 0.012f,
                            .offset = 0.33333333f},
           control::ModTarget::HueRot)
      .add("flex_inside_hyperbolic_component", // +-0.022 in the constant
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.009f,
                            .amplitude = 0.008f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.03f,
                            .offset = 0.06f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
