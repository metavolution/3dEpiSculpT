#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential Hodgkin-Huxley action potential tunnel preset.
 *
 * BIOELECTRICITY: THE HODGKIN-HUXLEY ACTION POTENTIAL
 *
 * In 1952, Alan Hodgkin and Andrew Huxley published four papers that
 * revolutionized neuroscience.  They measured ionic currents in the giant
 * axon of the squid Loligo and derived a mathematical model that explains
 * how nerve impulses propagate — earning the 1963 Nobel Prize:
 *
 *   C_m dV/dt = -g_Na·m³h·(V-E_Na) - g_K·n⁴·(V-E_K) - g_L·(V-E_L) + I
 *
 * This equation IS the bridge between electrodynamics and cell biology:
 *   - LEFT SIDE: capacitive current through the lipid bilayer (electrodynamics)
 *   - RIGHT SIDE: ionic currents through protein channels (cell biology)
 *
 * The action potential is a REGENERATIVE WAVE of electrical depolarization:
 *   1. Stimulus opens Na⁺ channels (m³ gates): Na⁺ floods IN → depolarization
 *   2. Na⁺ channels inactivate (h gate): Na⁺ current stops
 *   3. K⁺ channels open (n⁴ gates): K⁺ flows OUT → repolarization
 *   4. Membrane returns to resting potential (-70 mV)
 *   Speed: ~1-100 m/s depending on axon diameter and myelination.
 *
 * The GATING EXPONENTS encode the number of independent subunit gates:
 *   m³: 3 activation gates for Na⁺ (each must open for current to flow)
 *   h:  1 inactivation gate for Na⁺ (closes slowly to stop the surge)
 *   n⁴: 4 activation gates for K⁺ (each must open for repolarization)
 *   Digits: 3, 1, 4 — the first three digits of π (coincidence or not?).
 *
 * ENGINE MAPPING TO THE HODGKIN-HUXLEY MODEL:
 *
 *   THE DEPOLARIZATION SURGE = Exp inner function
 *     The action potential's rising phase is an EXPONENTIAL surge:
 *     when Na⁺ channels open, the positive feedback between membrane
 *     voltage and Na⁺ conductance creates explosive depolarization.
 *     V(t) ∝ e^(t/τ_m) during the upstroke.  Exp's exponential mapping
 *     IS this explosive growth — each iteration amplifies the previous,
 *     creating an expanding wavefront of depolarization.
 *
 *   THE AXON = Torus reduction
 *     The axon is a CYLINDER — a tube of membrane enclosing cytoplasm.
 *     Torus reduction wraps the visual plane into a tube, creating the
 *     tunnel effect.  Looking into the tunnel = looking ALONG the axon,
 *     watching the action potential propagate toward you at ~100 m/s.
 *     The tunnel walls are the axon membrane, the channel-studded
 *     barrier between intracellular and extracellular space.
 *
 *   19 POTASSIUM CHANNELS = 2π/19 ≈ 18.95° pre-rotation
 *     19 is the ATOMIC NUMBER OF POTASSIUM (K⁺) — the ion responsible
 *     for the resting membrane potential (-70 mV) and repolarization.
 *     Potassium channels (Kv) are the most diverse ion channel family:
 *     over 80 genes in the human genome.  The K⁺ ion has 19 electrons
 *     in its neutral state, 18 when ionized (noble gas configuration).
 *
 *     cos(2π/19) ≈ 0.9455, sin(2π/19) ≈ 0.3256
 *     360°/18.95° = 19.0 → exact 19-fold symmetry.
 *     19 is prime, a centered hexagonal number (1+6+12), and the
 *     exponent in Mersenne prime M₁₉ = 2¹⁹-1 = 524287 (also prime).
 *     Each sector represents one K⁺ channel in the axonal membrane.
 *
 *   THE TRANSCENDENTAL RATIO = -π/e ≈ -66.22° post-rotation
 *     π/e ≈ 1.1557: the ratio of GEOMETRY (π = circle = membrane
 *     cross-section) to GROWTH (e = exponential = depolarization surge).
 *     In the action potential, the circular membrane (π) constrains
 *     the exponential voltage surge (e) — the ratio π/e captures this
 *     fundamental competition between containment and explosion.
 *
 *     cos(π/e) ≈ 0.4032, sin(-π/e) ≈ -0.9151
 *     π/e appears in the asymptotic expansion of Stirling's formula:
 *     n! ~ √(2πn)·(n/e)ⁿ — the combinatorics of channel gating
 *     states connected to the geometry of the membrane.
 *     360°/66.22° ≈ 5.44 → quasi-5.5-fold depth symmetry.
 *     Combined with 19-fold azimuthal: 19 × 5.4 ≈ 103 interference cells.
 *
 *   THREE HODGKIN-HUXLEY TIMESCALES = Mode 3 (Alive)
 *     base  = 0.026 → Na⁺ activation (m): FAST (~0.1-1 ms)
 *     breath = 0.014 → K⁺ activation (n): MEDIUM (~1-5 ms)
 *     drift  = 0.004 → Na⁺ inactivation (h): SLOW (~5-10 ms)
 *     These three timescales are the EXACT dynamics of the HH model:
 *     fast depolarization, medium repolarization, slow recovery.
 *
 * WHAT TO WATCH:
 *   1. 19-FOLD LATTICE: K⁺ channel array in the axonal membrane
 *   2. EXPONENTIAL SURGE: the Na⁺ depolarization wavefront expanding
 *   3. TRIPLE SHIMMER: m/n/h gating dynamics as nested flickering
 *   4. TUNNEL: looking along the axon, action potential approaching
 *   5. TRANSCENDENTAL DEPTH: π/e creates geometry/growth interference
 *   6. REFRACTORY TRAIL: fade persistence = the absolute refractory period
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern exp_hodgkin_huxley_tunnel() {
  // 2π/19 ≈ 0.3307 rad (18.95°): atomic number of potassium K⁺
  // 19 = the resting-potential ion; Kv channels gate repolarization
  // Exact 19-fold symmetry → K⁺ channel lattice in axon membrane
  constexpr float CK   =  0.9455f;           // cos(2π/19)
  constexpr float SK   =  0.3256f;           // sin(2π/19)
  // -π/e ≈ -1.1557 rad (-66.22°): geometry/growth ratio
  // π = membrane circle; e = depolarization exponential
  // The transcendental interface between containment and explosion
  constexpr float CPE  =  0.4032f;           // cos(π/e)
  constexpr float SPEN = -0.9151f;           // sin(-π/e)

  constexpr float PRE  = 0.84f;  // moderate: safe for Exp exponential growth
  constexpr float POST = 0.92f;  // gentle contraction: product 0.773

  Pattern p;
  p.name = "exp_hodgkin_huxley_tunnel";
  p.inner = InnerFunction::Exp;              // exponential depolarization surge
  p.reduction = ReductionType::Torus;        // axon cylinder: seamless membrane tube

  p.zn.zn[0] = {CK * PRE, SK * PRE};
  p.zn.zn[1] = {0.004f, -0.003f};           // perturbation: membrane noise (thermal)
  p.zn.zn[2] = {CPE * POST, SPEN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 3 (Alive) — three Hodgkin-Huxley gating timescales ──
  p.color.hue_rotation = 0.012f;             // moderate: voltage color evolution
  p.color.sat_scale = 1.14f;                 // vivid: bioluminescent nerve glow
  p.color.fade = 0.989f;                     // deep: ~91 frame persistence (refractory)
  p.color.color_mode = as_int(ColorMode::Alive);                    // Alive: m/n/h gating triple dynamics
  p.color.spiral_angle_coeff = 0.95f;        // STRONG: shows 19 K⁺ channel sectors
  p.color.spiral_radius_coeff = 0.50f;       // strong: axonal depth rings
  // Membrane oscillation (voltage fluctuations at rest)
  p.color.chroma_breath_amp = 0.20f;         // moderate: subthreshold membrane noise
  p.color.chroma_breath_freq = 0.10f;        // membrane oscillation rate
  p.color.chroma_breath_phase = 0.3f;        // voltage phase
  // THREE HODGKIN-HUXLEY TIMESCALES
  p.color.hue_velocity_base = 0.026f;        // FAST: Na⁺ activation (m gate)
  p.color.hue_velocity_breath = 0.014f;      // MEDIUM: K⁺ activation (n gate)
  p.color.hue_velocity_drift = 0.004f;       // SLOW: Na⁺ inactivation (h gate)
  // Luminance: strong wavefront contrast
  p.color.luma_shape_depth = 0.24f;          // strong: crisp depolarization wavefront
  p.color.luma_shape_center = 2.0f;

  // ── Seed: moderate — injects stimulus current ──
  p.seed.border_width = 0.010f;              // moderate: membrane boundary visible
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.86f;

  p.speed = 0.48f;                           // moderate-fast: action potential propagation
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.118f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.147f;
  p.zn.zn1_anim_amp = 0.042f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.103f;
  p.seed.pulse_amp = 0.17f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.059f,
                            .amplitude = 0.140f,
                            .offset = 0.500f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.094f,
                            .amplitude = 0.110f,
                            .offset = 0.200f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.044f,
                            .amplitude = 0.030f,
                            .offset = 0.050f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
