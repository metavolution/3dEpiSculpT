#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Magnetic Dipole (Minimal 3.5D): Three Lakes of Wada.
 *
 * Mathematics:
 * - f(u) = 1 / (1 - u^2). Its two critical points are 0 and infinity, and
 *       0 -> 1 -> infinity -> 0.
 *   Both critical points lie on one superattracting cycle of period 3, so
 *   every Fatou component is an eventual preimage of the three components
 *   around 0, 1 and infinity.
 * - Under f^3 those are three attracting fixed points with three basins.
 *   The boundary of the full basin of an attracting cycle is always the
 *   whole Julia set, so the three basins have one common boundary: every
 *   point where two colours meet is a point where all three meet. These
 *   are lakes of Wada, drawn by one formula.
 * - f is real and even, so the picture has both mirror symmetries. The
 *   Julia set lies in |Re u| <= 1.79, |Im u| <= 1.11 (measured on a grid).
 *
 * Pipeline: MagneticDipole with zn0 = 1.95 i, zn2 = -1 / 3.9, zn3 = 1 / 3.9
 * gives T(z) = f(1.95 z) / 1.95 exactly; the frame is |u| <= 1.95. Clamp
 * sends every orbit that leaves the frame to the border, where the Simplex
 * seed is opaque, so a pixel takes the seed colour at its first exit, k.
 * OKLab rotation with a hue step of 1/3 turns that colour k - 1 times by
 * 120 degrees: the hue is the lake. One more lap of the cycle is three
 * more passes, the same hue and a factor fade^3 darker, so each lake is
 * terraced in depth toward its own centre.
 *
 * Fidelity:   identity (T is f up to the window)
 * Cell:       MagneticDipole / Clamp / Simplex / OKLabColorRotation
 * Nearest:    cayley_rational_superattracting_two_cycle has a 2-cycle and
 *             a second critical point that falls onto it; two colours by
 *             parity. Here both critical points are on one 3-cycle and it
 *             passes through infinity, so the trap is the frame itself.
 * Height:     age = passes since first exit; terraces of three passes.
 * Hue clock:  bounded; step 1/3 plus at most 0.01 turn from the len term.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-16. A separate arithmetic
 *             check on a 300 x 300 grid found every pixel lit and
 *             (k - 1) mod 3 equal to the basin for 99.95 % of pixels at
 *             frame 1.8 (99.6 % at 1.7); 1.95 was chosen from the measured
 *             extent of the Julia set. Not built, rendered or measured.
 * @assert sing[0].period == 3 && p_X >= 0.99
 */
inline Pattern magnetic_dipole_minimal_wada_three_lakes() {
  Pattern p;
  p.name = "magnetic_dipole_minimal_wada_three_lakes";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.0f, 1.95f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {-0.25641026f, 0.0f}; // -1 / (2 * 1.95)
  p.zn.zn[3] = {0.25641026f, 0.0f};

  p.color.hue_rotation = 0.33333333f; // one lake per third of a turn
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.92f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.06f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // bounded clock: the step stays at 1/3
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.07f;
  p.seed.hue = 0.02f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion.add("relief_depth",
               control::ModLane{.wave = control::Waveform::Triangle,
                                .frequency_hz = 0.02f,
                                .amplitude = 0.03f,
                                .offset = 0.06f},
               control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
