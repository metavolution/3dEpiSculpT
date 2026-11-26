#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Advanced 3.5D): a Vortex-Antivortex Pair of Colour at the Poles
 * of a Turning Sphere, Climbing from Charge 1 to Charge 2.
 *
 * Mathematics:
 * - M is the sixth turn of the sphere of invert_minimal_elliptic_mobius_six_
 *   meridians: poles +-p, p = 0.85 i, M^6 = id. Near p it acts as the
 *   rotation d -> L d with L = e^(i pi / 3) (d = z - p), near -p as d -> d / L.
 * - Opponent colour is linear in RGB. With a hue step h and weights near 1,
 *   the chroma at z is C(z) = sum_j e^(2 pi i j h) S(M^j z), a sum over
 *   the six-point orbit (S is the seed's chroma, a smooth function). Expand
 *   S about p in d and conj(d): the term d^a conj(d)^b picks up
 *   sum_j e^(2 pi i j (h + (a - b) / 6)), which vanishes unless
 *   a - b = -6 h mod 6.
 * - At h = m / 6 every term below order m cancels, the first survivor is
 *   conj(d)^m, and the colour winds -m times round p: a phase singularity of
 *   charge -m. At -p the rotation runs backwards and the survivor is d^m:
 *   charge +m. Total charge 0, as it must be on a sphere. m = 1 and m = 2
 *   give clean vortices; at m = 3 the terms d^3 and conj(d)^3 tie and the
 *   pole becomes a threefold saddle.
 * - Between rungs (6 h not an integer) the constant term no longer cancels,
 *   and the vortices are no longer pinned to the poles.
 *
 * Pipeline: the slots of the meridian preset; Clamp; WaveRing seed (smooth,
 * with a hue that varies with radius, so its gradient at the poles is not
 * zero in general); alpha 0.12 and fade 0.99 to keep the weights near 1;
 * Opponent with a HueRot lane between 1/6 and 1/3 at 0.002 Hz, lingering
 * at charge 1 and at charge 2.
 *
 * Fidelity:   identity
 * Cell:       Invert / Clamp / WaveRing / Opponent
 * Nearest:    identity_hexagonal_phased_array_colour_vortex is the flat,
 *             one-pole version at charge 1; invert_minimal_elliptic_mobius_
 *             six_meridians draws the same rotation with lines.
 * Height:     age = passes; the winding is a property of the sum over six
 *             generations.
 * Hue clock:  bounded; step 0.25 +- 0.0833.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. The cancellation is
 *             exact for equal weights; with rho = fade (1 - seed alpha) < 1
 *             a residue of order 1 - rho remains, which moves the vortex
 *             core off the pole by a small distance without changing its
 *             charge. When the ring wave puts a crest exactly on a pole the
 *             charge-1 core momentarily vanishes. Not built, rendered or
 *             measured.
 * @assert p_C <= 0.05
 */
inline Pattern invert_mobius_colour_vortex_charge_ladder() {
  Pattern p;
  p.name = "invert_mobius_colour_vortex_charge_ladder";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.5f, -0.86602540f};
  p.zn.zn[1] = {-0.73612159f, 1.275f};
  p.zn.zn[2] = {-1.445f, 2.50281342f};
  p.zn.zn[3] = {-1.47224319f, 0.0f};

  p.color.hue_rotation = 0.25f;
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
      .add("topological_charge_ladder", // h from 1/6 (charge 1) to 1/3 (charge 2)
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.002f,
                            .amplitude = 0.08333333f,
                            .offset = 0.25f},
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
