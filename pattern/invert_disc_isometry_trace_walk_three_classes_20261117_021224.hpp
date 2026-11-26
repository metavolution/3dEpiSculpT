#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Advanced 3.5D): a Trace Walk through the Three Kinds of Isometry
 * of the Hyperbolic Disc.
 *
 * Mathematics:
 * - T(z) = e^(i alpha) (z + t)/(1 + t z) with t = 1/2: a hyperbolic
 *   translation by d = 2 artanh t = ln 3 along the real diameter, then a
 *   rotation by alpha. Every such T maps the unit disc onto itself. As a
 *   matrix in SU(1,1) its trace is 2 cos(alpha/2) / sech(d/2), so:
 *   |cos(alpha/2)| > sqrt(1 - t^2) = 0.866: hyperbolic (plates march along
 *   a geodesic between two rim points); equality: parabolic (they creep
 *   along horocycles into one rim point); smaller: elliptic, a rotation by
 *   theta with cos(theta/2) = cos(alpha/2) / 0.866 about an inside point.
 *   With t = 1/2, alpha in (60, 300) degrees is elliptic, the rest
 *   hyperbolic, and the two parabolic moments sit at 60 and 300.
 * - Across the elliptic arc theta sweeps 0 to 2 pi, so the walk crosses
 *   every rotation number: at theta = 2 pi p/q the plate copies close up
 *   into an exact q-fold hyperbolic rosette (order 5 at alpha = 91.04
 *   degrees, order 2 at alpha = 180); between rationals they never close.
 * - Exactness as in invert_minimal_poincare_pentagonal_rosette_orbit:
 *   Chladni is zero for r >= 1, T keeps the disc and its outside, and
 *   Clamp only moves outside points to the border.
 * - Two exact lane families: post-rotation alpha multiplies zn2, zn3 by
 *   e^(i alpha); the turntable psi multiplies zn0 by e^(-i psi) and zn2,
 *   zn3 by e^(i psi). So zn2, zn3 turn at f_alpha + f_psi = 0.005 Hz and
 *   zn0 counter-turns at f_psi = 0.003 Hz. The class depends on alpha only;
 *   psi turns the geometry against the fixed plate. alpha makes 2 turns
 *   while psi makes 3: the path in the (alpha, psi) torus is the (2, 3)
 *   curve, a trefoil when the torus is drawn in space, closing in 1000 s.
 *
 * Pipeline: Invert, zn1 = 1 fixed; base slots are the hyperbolic rest state
 * (alpha = psi = 0): zn0 = 1/2, zn2 = -3/2, zn3 = 2. Clamp; Chladni at
 * alpha 0.55; fade 0.95; OKLab step 1/8 (an order-8 rosette locks colour to
 * group element).
 *
 * Fidelity:   identity for every lane value
 * Cell:       Invert / Clamp / ChladniCymatics / OKLabColorRotation
 * Nearest:    invert_minimal_poincare_pentagonal_rosette_orbit (one
 *             elliptic point of this walk); invert_minimal_elliptic_mobius_
 *             six_meridians (an elliptic map of the sphere, not the disc).
 * Height:     age = passes; copy n is the plate moved by T^-n.
 * Hue clock:  bounded; step 1/8.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; thresholds and lane
 *             phases evaluated arithmetically. In the elliptic arc the copies
 *             are rotated by theta per pass, so a copy of generation n turns
 *             n times as fast as theta; check the deep generations against
 *             the photosensitivity gate once rendered. Not built, rendered or
 *             measured.
 * @assert p_C >= 0.5
 */
inline Pattern invert_disc_isometry_trace_walk_three_classes() {
  constexpr float SPIN_HZ = 0.005f; // f_alpha + f_psi
  constexpr float TURN_HZ = 0.003f; // f_psi
  Pattern p;
  p.name = "invert_disc_isometry_trace_walk_three_classes";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.5f, 0.0f};  // t
  p.zn.zn[1] = {1.0f, 0.0f};
  p.zn.zn[2] = {-1.5f, 0.0f}; // t - 1/t
  p.zn.zn[3] = {2.0f, 0.0f};  // 1/t

  p.color.hue_rotation = 0.125f;
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
  p.seed.hue = 0.48f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.55f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("turntable_zn0_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 0.5f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn0Re)
      .add("turntable_zn0_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 0.5f,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn0Im)
      .add("trace_zn2_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SPIN_HZ,
                            .amplitude = 1.5f,
                            .offset = 0.0f,
                            .phase = 0.75f},
           control::ModTarget::Zn2Re)
      .add("trace_zn2_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SPIN_HZ,
                            .amplitude = 1.5f,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn2Im)
      .add("trace_zn3_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SPIN_HZ,
                            .amplitude = 2.0f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("trace_zn3_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SPIN_HZ,
                            .amplitude = 2.0f,
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
