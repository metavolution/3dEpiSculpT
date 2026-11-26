#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Advanced 3.5D): a Vortex and an Antivortex Dance a Double Helix.
 *
 * Mathematics:
 * - T is the elliptic Mobius map of order 6 with fixed points +-p,
 *   p = 0.85i, multiplier e^(i pi/3) at p (the slots of
 *   invert_minimal_elliptic_mobius_six_meridians): a sixth turn of the
 *   Riemann sphere about the axis through +-p.
 * - Opponent with step 1/6 over this 6-periodic map: at p, where the map
 *   turns by +1/6, only Taylor terms with a - b = -1 mod 6 survive, so the
 *   colour winds once backwards (charge -1); at -p, where it turns by
 *   -1/6, it winds once forwards (charge +1). A vortex-antivortex pair,
 *   1.7 apart, made by the sum over generations.
 * - Turntable: zn0 e^(-i psi), zn2 e^(i psi), zn3 e^(i psi) gives the same
 *   rotation about +-p e^(i psi). The pair turns about its midpoint at
 *   0.003 Hz. In space-time (x, y, t) the two cores trace a double helix.
 *   Over one turn the braid is a full twist, sigma_1^2, and since the
 *   motion is periodic the two closed world-lines are linked once: the
 *   linking number is a topological invariant of the animation.
 *
 * Pipeline: Invert with the slots below; Clamp; WaveRing at alpha 0.12
 * (centred at the midpoint, so never symmetric under T); fade 0.99;
 * Opponent step 1/6; turntable lanes, one turn in 333 s.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Invert / Clamp / WaveRing / Opponent
 * Nearest:    invert_mobius_colour_vortex_charge_ladder (the same pair, held
 *             still while the charge steps); here the charge is held and the
 *             pair moves.
 * Height:     age = passes; the six generations are the six group
 *             elements.
 * Hue clock:  bounded; step 1/6 +- 0.004.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; turntable fixed points
 *             (+-p e^(i psi), to 4e-9 from the float slots) and M^6 = id
 *             checked arithmetically. As in E7h: with rho < 1 the cores are
 *             displaced by order 1 - rho, a core vanishes for a moment when a
 *             ring crest (a clamp kink) sits on it, and points the sphere
 *             turn throws past the square are clamped to its border. Not
 *             built, rendered or measured.
 * @assert p_E <= 0.05
 */
inline Pattern invert_vortex_antivortex_pair_double_helix() {
  constexpr float TURN_HZ = 0.003f;
  Pattern p;
  p.name = "invert_vortex_antivortex_pair_double_helix";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.5f, -0.86602540f};
  p.zn.zn[1] = {-0.73612159f, 1.275f};
  p.zn.zn[2] = {-1.445f, 2.50281342f};
  p.zn.zn[3] = {-1.47224319f, 0.0f};

  p.color.hue_rotation = 0.16666667f; // charges -1 at p, +1 at -p
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

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.12f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("helix_zn0_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.0f,
                            .offset = 0.0f,
                            .phase = 0.41666667f},
           control::ModTarget::Zn0Re)
      .add("helix_zn0_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.0f,
                            .offset = 0.0f,
                            .phase = 0.66666667f},
           control::ModTarget::Zn0Im)
      .add("helix_zn2_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 2.89f,
                            .offset = 0.0f,
                            .phase = 0.58333333f},
           control::ModTarget::Zn2Re)
      .add("helix_zn2_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 2.89f,
                            .offset = 0.0f,
                            .phase = 0.33333333f},
           control::ModTarget::Zn2Im)
      .add("helix_zn3_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.47224319f,
                            .offset = 0.0f,
                            .phase = 0.75f},
           control::ModTarget::Zn3Re)
      .add("helix_zn3_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.47224319f,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn3Im)
      .add("vortex_tuning",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.004f,
                            .offset = 0.16666667f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.015f,
                            .offset = 0.03f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
