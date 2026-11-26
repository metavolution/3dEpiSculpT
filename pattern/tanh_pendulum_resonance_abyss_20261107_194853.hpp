#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh pendulum resonance abyss preset.
 *
 * PHYSICS: GETRIEBENER GEDÄMPFTER HARMONISCHER OSZILLATOR (Pendelmodell)
 *
 * The PENDULUM is the archetype of the harmonic oscillator.  For small
 * angles: θ̈ + 2γθ̇ + (g/L)θ = τ₀cos(ωt).  For LARGE angles, the
 * restoring force is sin(θ), not θ — the oscillation becomes NONLINEAR.
 *
 * Tanh is the PERFECT function to visualize this transition:
 *   - For small z: tanh(z) ≈ z       (LINEAR/harmonic regime)
 *   - For large z: tanh(z) → ±1      (NONLINEAR/saturation regime)
 * Each tanh iteration shows BOTH regimes simultaneously: the center of
 * each fractal cell is linear (harmonic oscillation), while the edges
 * hit the ±1 walls (nonlinear turning points).  The fractal web IS the
 * phase space of the pendulum: the web's branching filaments trace out
 * the separatrix between oscillation and rotation in phase space.
 *
 * ENGINE MAPPING TO THE PENDULUM EQUATION:
 *
 *   THE PENDULUM = Tanh inner function
 *     tanh(z) maps small amplitudes linearly (harmonic: F = -kx) and
 *     bounds large amplitudes (anharmonic: the pendulum can't swing
 *     past the top).  Under iteration with |zn0| > 1.0, the system
 *     alternates between:
 *       - EXPANSION: zn0 pushes past the ±1 walls (the pendulum swings wide)
 *       - SATURATION: tanh folds back to [-1,1] (nonlinear restoring force)
 *       - CONTRACTION: zn2 shrinks (energy lost to damping)
 *     This expand-saturate-contract cycle IS one complete pendulum swing:
 *     accelerate → reach turning point → decelerate → return.
 *
 *   THE INFINITE TUNNEL = Torus reduction
 *     Torus provides seamless mirrored wrapping — no visible boundaries.
 *     Combined with tanh folding, this creates an infinite fractal tunnel:
 *     each depth layer is one complete oscillation cycle of the pendulum.
 *     Looking into the tunnel IS looking forward in time through the
 *     pendulum's oscillation history.
 *
 *   THE EXACT PERIOD = K(1/√2) pre-rotation ≈ 106.2°
 *     The period of a pendulum at amplitude θ₀ is:
 *
 *       T = 4K(sin(θ₀/2)) / √(g/L)
 *
 *     where K(k) = ∫₀^{π/2} dθ/√(1-k²sin²θ) is the COMPLETE ELLIPTIC
 *     INTEGRAL of the first kind.  At θ₀ = 90° (quarter-turn amplitude):
 *     sin(45°) = 1/√2, so the period factor is K(1/√2).
 *
 *     K(1/√2) = Γ(1/4)² / (4√π) ≈ 1.8541
 *
 *     This connects to the LEMNISCATE OF BERNOULLI (the figure-8 curve),
 *     the ARITHMETIC-GEOMETRIC MEAN (K(1/√2) = π/(2·AGM(1,1/√2))), and
 *     the theory of ELLIPTIC CURVES over ℚ.
 *
 *     Using K(1/√2) as the pre-rotation: each iteration rotates by the
 *     exact period constant of the 90° pendulum.  This makes each tunnel
 *     layer correspond to one pendulum half-swing.
 *
 *   THE LOGARITHMIC DECREMENT = π/5 post-rotation (−36°)
 *     The LOGARITHMIC DECREMENT δ measures damping by the log-ratio of
 *     successive amplitude peaks:
 *
 *       δ = ln(x_n/x_{n+1}) = πγ/ω_d ≈ π/Q  (for weak damping)
 *
 *     With Q = 5 (moderate quality factor): δ = π/5 ≈ 0.628.
 *     This means each oscillation cycle reduces amplitude by factor
 *     e^(-π/5) ≈ 0.534 — about half per cycle.  After 5 cycles, the
 *     amplitude is e^(-π) ≈ 0.043 — barely visible.  So Q = 5 means
 *     roughly 5 visible oscillation cycles before the pendulum effectively
 *     stops.  PERFECT for education: you can COUNT the damped cycles.
 *
 *     cos(π/5) = φ/2 = (1+√5)/4 — the GOLDEN RATIO appears in the
 *     damping!  The decagonal connection: π/5 = 36° is the central angle
 *     of a regular decagon, linking the pendulum's damping geometry to
 *     the golden ratio and Penrose tilings.
 *
 *   THE DRIVING FORCE = seed pulse
 *     Periodic energy injection (pulse_freq=0.12, pulse_amp=0.38) from
 *     the borders.  You can SEE the driving: bright pulses enter from
 *     the edges and propagate into the fractal tunnel.  Between pulses,
 *     the tunnel dims (damping).  When the next pulse arrives, the
 *     tunnel brightens again (re-excitation).  This is the pendulum
 *     being PUSHED at regular intervals — the DRIVEN in "driven damped."
 *
 *   THE BEAT PATTERN = detuning between drive and response
 *     breath_freq = 0.09 (response oscillation, near natural frequency)
 *     pulse_freq  = 0.12 (driving frequency)
 *     Beat: |0.12 − 0.09| = 0.03 → period ≈ 33 time units
 *     You SEE: brightness slowly builds (constructive: drive and response
 *     in phase), reaches maximum (RESONANCE PEAK), then decays
 *     (destructive: drive and response out of phase).  The slow envelope
 *     is the BEAT FREQUENCY made visible — the hallmark of near-resonance.
 *
 *   PHASE PORTRAIT = breathing spatial phase (3.5)
 *     The breathing varies with angle: at any moment, different angular
 *     sectors of the tunnel show different PHASES of the oscillation.
 *     One sector bright (pendulum at maximum displacement), adjacent
 *     sector dim (pendulum passing through equilibrium), next sector
 *     bright again (maximum displacement in other direction).  This
 *     angular pattern IS a frozen snapshot of the oscillator's phase
 *     portrait — the phase relationship made spatial.
 *
 * - |zn0| = 1.08: moderate tanh expansion.  Each iteration pushes past
 *   the ±1 walls — the pendulum swings into the nonlinear regime.
 *
 * - |zn2| = 0.85: strong contraction.  Net: 1.08 × 0.85 = 0.918.
 *   Near-critical: the pendulum barely loses energy per swing, creating
 *   many visible oscillation cycles before damping takes over.
 *
 * WHAT TO WATCH (the driven pendulum in action):
 *   1. DRIVING: bright pulse enters from borders → external torque τ₀cos(ωt)
 *   2. SWING: tanh folding at ±1 walls → pendulum reaching turning points
 *   3. DAMPING: tunnel dims between pulses → energy dissipation 2γθ̇
 *   4. RE-EXCITATION: next pulse → amplitude jumps → pushed pendulum
 *   5. BEAT ENVELOPE: brightness slowly waxes/wanes → near-resonance
 *   6. PHASE SECTORS: angular brightness variation → phase portrait
 *   7. COUNT THE CYCLES: ~5 visible oscillation layers → Q ≈ 5
 *   8. NONLINEAR TRANSITION: web center (linear) vs edges (saturated)
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern tanh_pendulum_resonance_abyss() {
  // K(1/√2) ≈ 1.8541 rad (106.2°): the exact period constant of the
  // 90° pendulum, via the complete elliptic integral
  // K(1/√2) = Γ(1/4)²/(4√π) — connects to lemniscate and AGM
  constexpr float CK   = -0.2794f;           // cos(K(1/√2))
  constexpr float SK   =  0.9602f;           // sin(K(1/√2))
  // −π/5 = −36° (logarithmic decrement at Q = 5)
  // cos(π/5) = φ/2 — the golden ratio governs the damping geometry
  constexpr float CD   =  0.8090f;           // cos(π/5) = φ/2
  constexpr float SDN  = -0.5878f;           // sin(−π/5)

  constexpr float PRE  = 1.5033f;  // moderate expansion: pendulum swings into nonlinear
  constexpr float POST = 1.3566f;  // strong contraction: near-critical Q

  Pattern p;
  p.name = "tanh_pendulum_resonance_abyss";
  p.inner = InnerFunction::Tanh;             // linear core + nonlinear walls = pendulum
  p.reduction = ReductionType::Torus;        // seamless tunnel: oscillation history

  p.zn.zn[0] = {CK * PRE, SK * PRE};
  p.zn.zn[1] = {0.1983f, -0.1681f};           // perturbation: excites multiple modes
  p.zn.zn[2] = {CD * POST, SDN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — oscillation amplitude envelope ──
  p.color.hue_rotation = 0.012f;             // moderate: oscillation visible in color
  p.color.sat_scale = 1.14f;                 // vivid: make oscillation phases distinct
  p.color.fade = 0.984f;                     // DAMPING: Q_eff ≈ 196, visible decay
  p.color.color_mode = as_int(ColorMode::Breathing);                    // Breathing: amplitude modulation
  p.color.spiral_angle_coeff = 0.95f;        // angular: oscillation node sectors
  p.color.spiral_radius_coeff = 0.42f;       // radial: depth rings = oscillation cycles
  // RESPONSE AMPLITUDE (the pendulum's swing envelope)
  p.color.chroma_breath_amp = 0.35f;         // STRONG: visible amplitude oscillation
  p.color.chroma_breath_freq = 0.09f;        // response frequency (near natural ω₀)
  p.color.chroma_breath_phase = 3.5f;        // STRONG spatial phase: phase portrait
  // THREE TIMESCALES: ω₀ (natural), ω (driving), Δω (beat)
  p.color.hue_velocity_base = 0.018f;        // FAST: pendulum natural frequency
  p.color.hue_velocity_breath = 0.012f;      // MEDIUM: driving frequency
  p.color.hue_velocity_drift = 0.004f;       // SLOW: beat/resonance envelope
  // Luminance: oscillation amplitude as brightness contrast
  p.color.luma_shape_depth = 0.26f;          // STRONG: amplitude directly visible
  p.color.luma_shape_center = 2.0f;

  // ── Seed: THE DRIVING FORCE τ₀cos(ωt) ──
  p.seed.border_width = 0.085f;              // WIDE: energy visibly enters from edges
  p.seed.hue_speed = 0.08f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.12f;                 // DRIVING FREQUENCY ω (detuned → beats!)
  p.seed.pulse_amp = 0.38f;                  // STRONG DRIVE: visible re-excitation

  p.speed = 0.55f;                           // moderate: educational observation tempo
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.190f;
  p.zn.zn0_anim_amp = 0.043f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.209f;
  p.zn.zn1_anim_amp = 0.039f;
  p.zn.zn1_anim_k = 5.0f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.069f,
                            .amplitude = 0.120f,
                            .offset = 0.420f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.098f,
                            .amplitude = 0.120f,
                            .offset = 0.350f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.050f,
                            .amplitude = 0.030f,
                            .offset = 0.080f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
