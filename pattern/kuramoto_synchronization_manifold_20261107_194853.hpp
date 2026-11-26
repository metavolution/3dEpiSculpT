#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Kuramoto synchronization manifold preset.
 *
 * Scientific foundation:
 * - Yoshiki Kuramoto's 1975 model of N globally coupled phase oscillators:
 *
 *       d(theta_i)/dt = omega_i + (K/N) * sum_j sin(theta_j - theta_i)
 *
 *   Each oscillator has a natural frequency omega_i drawn from a
 *   distribution g(omega); K is the global coupling strength. Despite
 *   its minimal form, this equation is the universal description of
 *   phase synchronisation for everything from firefly flashing and
 *   pacemaker cells to power-grid rotors and Josephson-junction arrays.
 *
 * - Mean-field order parameter:
 *       r * exp(i*psi) = (1/N) * sum_k exp(i*theta_k)
 *   r ∈ [0,1] measures phase coherence (0 = incoherent, 1 = fully
 *   synchronised). Kuramoto showed that in the N → infinity limit
 *   there is a SECOND-ORDER phase transition at a critical coupling
 *       K_c = 2 / (pi * g(0))
 *   with mean-field exponent r ~ (K - K_c)^(1/2) just above the
 *   transition. This is one of the cleanest mathematical analogues of
 *   a ferromagnetic Curie point in all of non-equilibrium physics.
 *
 * - For a uniform frequency distribution on [-1/2, +1/2], g(0) = 1,
 *   so K_c = 2/pi = 0.6366197724..., a classical mathematical
 *   constant that encodes the precise coupling strength above which
 *   global order spontaneously emerges from disordered rotation.
 *
 * - Arnold-tongue theory (Arnold 1961 / Herman 1979): for two coupled
 *   oscillators with a golden-mean winding ratio, the last frequency-
 *   lock resonance to give way to global sync sits at rotation number
 *   1/phi. The golden-mean winding is thus the most robustly
 *   partially-synchronised configuration and the natural companion
 *   to the mean-field coupling constant 2/pi.
 *
 * Mapping to this engine:
 * - Inner = Sin. This IS the Kuramoto coupling kernel — sin of phase
 *   differences — and makes the transform stage a direct iterate of
 *   the discrete-time Kuramoto map, with one iteration per pixel.
 * - Pre-affine magnitude K_c = 2/pi places the feedback right at the
 *   partial-synchronisation threshold: below fully locked, above
 *   pure incoherence, in the critical regime where order and chaos
 *   coexist most richly.
 * - Pre-rotation at omega_0 = 1 rad/frame enacts the rotating-frame
 *   transformation (standard step in the Kuramoto analysis).
 * - Post-rotation at sqrt(3) rad is incommensurate with pi and with
 *   every golden-mean angle already used in the catalog, encoding an
 *   irrational frequency spread that breaks any trivial resonance.
 * - Torus reduction realises the T^2 phase manifold of two
 *   representative oscillators (projection of the full T^N phase space
 *   onto the dominant two-mode plane).
 * - Color mode 4 (spatial wave) uses two incommensurate hue waves
 *   across clip-space — directly encoding the partial-sync order
 *   parameter field r(x): synchronised domains appear as coherent
 *   hue plateaus, incoherent domains as mixed-hue interference.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode4) -> seed -> blend
 */
inline Pattern kuramoto_synchronization_manifold() {
  // Kuramoto critical coupling for unit-width uniform frequency
  // distribution: K_c = 2/pi. Placing |zn0| at exactly this value
  // puts the feedback at the onset of partial synchronisation — the
  // regime where r(K) is finite but not yet saturated.
  constexpr float KC = 1.4277f;  // 2/pi

  // Pre-rotation: omega_0 = 1 rad (rotating-frame ansatz).
  // Classic Kuramoto reduction uses a rotating frame at the mean
  // natural frequency; here we set that frequency to 1 rad/step.
  constexpr float CW = 0.54030230587f;  // cos(1)
  constexpr float SW = 0.84147098481f;  // sin(1)

  // Post-rotation: sqrt(3) rad. Irrational, incommensurate with pi
  // and with the golden/silver rotations used by the other physics
  // presets — ensures the phase-manifold projection explores all of
  // T^2 rather than collapsing to a resonant periodic orbit.
  constexpr float CR = -0.16055965800f; // cos(sqrt(3))
  constexpr float SR = 0.98702906322f;  // sin(sqrt(3))

  // Post-scale: small contraction so the pulsing stays visible
  // without saturating the feedback texture.
  constexpr float POST = 1.2304f;

  Pattern p;
  p.name = "kuramoto_synchronization_manifold";
  p.inner = InnerFunction::Sin;          // Kuramoto coupling kernel
  p.reduction = ReductionType::Torus;    // T^N phase manifold (projection)

  p.zn.zn[0] = {CW * KC, SW * KC};
  // Small irrational detune: encodes the finite-N frequency-spread
  // residual after the N → infinity mean-field limit. Breaks exact
  // rotational symmetry so the critical-fluctuation texture is visible.
  p.zn.zn[1] = {0.017f, -0.012f};
  p.zn.zn[2] = {CR * POST, SR * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // Color mode 4: spatial-wave hue field. The two incommensurate hue
  // waves directly encode the order-parameter gradient r(x), making
  // synchronised domains visible as coherent hue plateaus.
  p.color.hue_rotation = 0.012f;
  p.color.sat_scale = 1.15f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);               // Spatial Wave (mode 4)
  // spiral_a: leftward hue slope across clip-space — analogous to the
  // global mean-phase psi(t) drifting uniformly in the rotating frame.
  p.color.spiral_angle_coeff = 0.52f;
  // spiral_r: wave1 frequency ~ natural-frequency spread sigma_omega.
  p.color.spiral_radius_coeff = 0.31f;
  // breath_amp: wave1 amplitude — partial-sync "coherence depth".
  p.color.chroma_breath_amp = 0.17f;
  // breath_freq: wave2 frequency — detuned from wave1 to create the
  // beat pattern characteristic of the frequency-spread residual.
  p.color.chroma_breath_freq = 0.11f;
  // breath_phase: wave2 amplitude — secondary-mode contribution. In
  // mode 4 this term adds a second incommensurate hue wave whose beat
  // with wave1 visually represents the Kuramoto order parameter r.
  p.color.chroma_breath_phase = 0.24f;
  // hue_velocity_base: time coefficient in wave1. This is omega_bar —
  // the mean natural frequency (set to 1 rad via pre-rotation above).
  p.color.hue_velocity_base = 0.018f;
  // hue_velocity_breath: time coefficient in wave2. Detuned slightly
  // to produce slow beat envelopes across spatially extended regions
  // — visually, these are the "synchronisation clusters" growing and
  // merging as K crosses K_c.
  p.color.hue_velocity_breath = 0.013f;
  p.color.hue_velocity_drift = 0.004f;
  // Spatial saturation wave (color_mode 4 path): slow sinusoid across
  // the u axis produces "breathing" of chroma amplitude — the visible
  // analogue of the order-parameter oscillation during partial sync.
  p.color.luma_shape_depth = 0.21f;
  p.color.luma_shape_center = 1.6f;

  p.seed.border_width = 0.085f;
  p.seed.hue_speed = 0.085f;
  p.seed.alpha = 0.92f;
  // Periodic energy injection: small pulse simulates the mean-field
  // coupling term driving the oscillator ensemble. The pulse beats
  // slowly against the hue_velocity_breath wave, producing the
  // order-parameter envelope.
  p.seed.pulse_freq = 0.098f;
  p.seed.pulse_amp = 0.22f;
  p.speed = 0.82f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.175f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.169f;
  p.zn.zn1_anim_amp = 0.037f;
  p.zn.zn1_anim_k = 7.0f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.081f,
                            .amplitude = 0.120f,
                            .offset = 0.310f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.092f,
                            .amplitude = 0.100f,
                            .offset = 0.170f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.061f,
                            .amplitude = 0.030f,
                            .offset = 0.085f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
