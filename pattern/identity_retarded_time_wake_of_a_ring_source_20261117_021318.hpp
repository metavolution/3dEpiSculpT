#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Advanced 3.5D): the Feedback Loop as a Delay Line; the Wake of a
 * Moving Ring Source.
 *
 * Mathematics:
 * - Frame k is C(frame k-1 at T z) blended with the seed of frame k, so
 *   generation n carries the seed evaluated n frames ago: S(t - n Delta) at
 *   T^n z, with Delta = speed / fps of seed time per frame. The loop is a
 *   delay line, and each generation is a snapshot from the past.
 * - T(z) = z + tau on the Grid torus (translations commute with the wrap,
 *   so this is exact). Generation n is the ring wave of n frames ago,
 *   centred at -n tau: the source's position n frames ago, seen from a
 *   frame that rides with the source. With tau turning slowly the centres
 *   are the true past track, the sum of past steps, so the track curves.
 * - WaveRing crests move out at c = 5/20 = 0.25 units per second of seed
 *   time. Crest k of generation n is a circle of radius rho_k - n c Delta
 *   about -n tau. Circles whose centres step by |tau| and radii by c Delta
 *   have an envelope of two lines at half-angle beta, sin beta =
 *   c Delta / |tau|, with apex where the radius reaches 0. Each crest gives
 *   one chevron opening toward the source. New crests are born at the
 *   source every 2 pi / 5 s of seed time and their chevrons recede along
 *   the track: a wake. On the chevrons many generations' crests touch
 *   (stationary phase), so they are the brightest lines.
 * - Because snapshots are older, not propagated, the radii shrink with n:
 *   this is a Mach cone reversed in time, the apex behind the source.
 * - Grid wraps the wave to the nearest image of each source (the
 *   minimum-image convention); seams lie at distance >= 1 from a centre,
 *   where the ring intensity is below e^-2.
 *
 * Pipeline: Identity, zn0 = zn2 = 1, zn3 = tau; |tau| = 1/150 per frame.
 * At 60 fps and speed 0.8, sin beta = 0.25 x 0.8/60 x 150 = 1/2: a 30 degree
 * half-angle. Quadrature lanes turn tau once in 100 s. WaveRing at alpha
 * 0.1; fade 0.99 (about 100 generations, 0.67 units of track); OKLab step
 * 0.006, so hue along a chevron reads emission time.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Identity / Grid / WaveRing / OKLabColorRotation
 * Nearest:    identity_hexagonal_phased_array_colour_vortex (ring sources at
 *             fixed points of a rotation); here the sources are the past
 *             positions of one moving source.
 * Height:     age = passes = emission time of the snapshot.
 * Hue clock:  bounded; step 0.006.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. The angle depends on
 *             frame rate: sin beta = 0.2 / (fps |tau|); at 120 fps it is
 *             14.5 degrees, and below 30 fps sin beta >= 1 and the chevrons
 *             vanish. Not built, rendered or measured.
 * @assert p_N >= 0.95
 */
inline Pattern identity_retarded_time_wake_of_a_ring_source() {
  constexpr float TAU = 0.0066667f; // 1/150 per frame
  constexpr float TURN_HZ = 0.01f;
  Pattern p;
  p.name = "identity_retarded_time_wake_of_a_ring_source";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {TAU, 0.0f};

  p.color.hue_rotation = 0.006f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.99f;
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

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.5f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.1f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("heading_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = TAU,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("heading_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = TAU,
                            .offset = 0.0f,
                            .phase = 0.0f},
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
