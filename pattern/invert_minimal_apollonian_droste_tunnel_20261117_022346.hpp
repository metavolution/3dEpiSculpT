#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Minimal 3.5D): a Loxodromic Droste Tunnel of Apollonian Circles.
 *
 * Mathematics:
 * - T is the loxodromic Mobius map with fixed points q = 0.35 and 1/q and
 *   multiplier L = 1.3 e^(i theta) at q, theta = (pi/2)(0.38196601): in
 *   zeta = (z - q)/(z - 1/q) it is zeta -> L zeta. Because q and 1/q are
 *   mirror images in the unit circle, the circles |zeta| = const are the
 *   Apollonian circles of the pair, and the unit circle is |zeta| = q.
 * - T maps the disc D = {|zeta| < q} onto {|zeta| < 1.3 q}, which contains
 *   it, so T maps the outside of the unit circle into itself (checked:
 *   |T z| >= 1.14 for |z| >= 1).
 * - Exactness: the Chladni seed is zero for r >= 1, and Clamp only moves
 *   outside points onto the border (r >= 1), which T keeps outside. So
 *   generation n is exactly the plate carried by T^-n into the disc
 *   {|zeta| < q 1.3^-n}: a nest of non-concentric discs closing on q, each
 *   holding a copy of the plate turned by -n theta.
 * - Non-concentric nested circles are what a round tunnel looks like when
 *   seen at an angle: the picture is a tunnel drawn by a Mobius map, with
 *   no 3D projection anywhere. theta is the golden fraction of the plate's
 *   quarter-turn symmetry, so no two copies share an orientation and the
 *   orientations stay evenly spread (three-gap theorem).
 * - Turntable: zn0 e^(-i psi), zn2 e^(i psi), zn3 e^(i psi) gives the same
 *   map about e^(i psi) q; six quadrature lanes swing the tunnel's
 *   vanishing point round the circle |q| = 0.35 in 500 s.
 *
 * Pipeline: Invert with the slots below; Clamp; Chladni at alpha 0.6;
 * fade 0.93; OKLab step 1/10, so ten nested discs make one hue turn.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Invert / Clamp / ChladniCymatics / OKLabColorRotation
 * Nearest:    invert_minimal_poincare_pentagonal_rosette_orbit (an elliptic
 *             map that keeps the disc); here the map is loxodromic and only
 *             pushes the outside outward.
 * Height:     age = passes = depth in the tunnel.
 * Hue clock:  bounded; step 1/10.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; slots, the exterior
 *             bound and lane phases evaluated arithmetically (T(q) = q to
 *             5e-16, T'(q) = L). Every orbit but q leaves the frame toward
 *             the attracting point 1/q and is clamped; the assertion (no
 *             expanding class) is expected, not proved. Not built, rendered
 *             or measured.
 * @assert p_E <= 0.05
 */
inline Pattern invert_minimal_apollonian_droste_tunnel() {
  constexpr float ORBIT_HZ = 0.002f;
  Pattern p;
  p.name = "invert_minimal_apollonian_droste_tunnel";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {-0.07294305f, -0.73402535f}; // 1 - L
  p.zn.zn[1] = {-2.48161279f, 0.25690887f};  // L q - 1/q
  p.zn.zn[2] = {7.12842505f, -8.47967871f};
  p.zn.zn[3] = {3.19324646f, -3.38220803f};  // T(infinity)

  p.color.hue_rotation = 0.1f; // one hue turn per ten nested discs
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.93f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.04f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.6f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.6f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("turntable_zn0_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 0.73764077f,
                            .offset = 0.0f,
                            .phase = 0.51576411f},
           control::ModTarget::Zn0Re)
      .add("turntable_zn0_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 0.73764077f,
                            .offset = 0.0f,
                            .phase = 0.76576411f},
           control::ModTarget::Zn0Im)
      .add("turntable_zn2_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 11.07787862f,
                            .offset = 0.0f,
                            .phase = 0.11125561f},
           control::ModTarget::Zn2Re)
      .add("turntable_zn2_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 11.07787862f,
                            .offset = 0.0f,
                            .phase = 0.86125561f},
           control::ModTarget::Zn2Im)
      .add("turntable_zn3_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 4.65146795f,
                            .offset = 0.0f,
                            .phase = 0.12042756f},
           control::ModTarget::Zn3Re)
      .add("turntable_zn3_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 4.65146795f,
                            .offset = 0.0f,
                            .phase = 0.87042756f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.02f,
                            .offset = 0.04f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
