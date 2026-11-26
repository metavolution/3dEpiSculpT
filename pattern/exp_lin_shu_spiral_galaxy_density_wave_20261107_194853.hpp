#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Lin-Shu Spiral Galaxy Density Wave
 * ================================================================
 *
 * PHYSICS / MATH  (C. C. Lin & Frank Shu 1964, "On the spiral
 * structure of disk galaxies", Astrophysical Journal 140, 646-655):
 *
 * Spiral arms in galaxies like the Milky Way, M51, NGC 1232 do NOT
 * rotate with the stars. They are LONG-LIVED DENSITY WAVES — regions
 * of enhanced stellar and gas density that propagate through the
 * differentially rotating stellar disk at a fixed pattern speed Ω_p.
 *
 *   The differential rotation curve Ω(r) is nearly Keplerian in the
 *   outer disk, so without a persistent wave the arms would "wind up"
 *   catastrophically within a few rotations — a classic puzzle of
 *   galactic astronomy until Lin & Shu resolved it via the QSSS
 *   (quasi-stationary spiral structure) hypothesis: the arms are
 *   EIGENMODES of the stellar disk, persisting as density waves
 *   driven by self-gravity while stars stream through them.
 *
 * Each arm is a LOGARITHMIC SPIRAL:
 *
 *     r(θ) = r₀ · exp(θ · cot(α))
 *
 * where α is the "pitch angle" — the angle between a tangent to the
 * arm and a circle of constant radius. Smaller α → tighter winding.
 * Observationally:
 *   • Sa galaxies (M81, M104):      α ≈ 10–15°  (tightly wound)
 *   • Sb galaxies (Milky Way, M31): α ≈ 12–18°  (moderate)
 *   • Sc galaxies (M51, NGC 1232):  α ≈ 20–25°  (loose arms)
 *
 * This preset uses α = 15° — the Milky Way's spiral pitch — for
 * the canonical "two-armed barred spiral" structure.
 *
 * At the COROTATION RADIUS r_CR the pattern moves with the stars
 * (Ω(r_CR) = Ω_p); inside r_CR stars overtake the arms, outside
 * they lag. At the INNER and OUTER LINDBLAD RESONANCES, the stellar
 * epicyclic frequency κ matches (m(Ω − Ω_p)) — creating density
 * enhancements that propagate along the arms. The result: the visual
 * impression of fixed arms while the underlying stellar content
 * continuously cycles through.
 *
 *
 * ENGINE MAPPING — iterated complex map AS density-wave orbit
 *
 *   LOG-SPIRAL GENERATOR = Exp inner function
 *     The complex map z → exp(α·z + β) IS the canonical parameter-
 *     ization of a logarithmic spiral. Exp maps vertical lines into
 *     circles and horizontal lines into rays, so a line through
 *     origin at slope cot(α) becomes a log-spiral with pitch α.
 *     Under iterated feedback, orbits tile the plane with the
 *     galaxy's exact arm geometry.
 *
 *   PITCH ANGLE ENCODING = zn[0] complex phase = (π/2 − α)
 *     The pre-multiply zn[0] = 0.74 · (cos(75°) + i·sin(75°)) rotates
 *     the input z by 75° before Exp — this 75° is exactly the angle
 *     from the VERTICAL (imaginary axis), equivalent to a 15° pitch
 *     from CIRCLES (the observational definition). The resulting
 *     log-spiral has arms at pitch α = 15°.
 *
 *   TWO-ARM SYMMETRY (m = 2) = torus reduction
 *     Barred spirals like the Milky Way have dominant m = 2 density
 *     waves — two opposing arms. Torus wrapping on the unit cell
 *     [-1,1]² automatically produces the 2-fold mirror symmetry, as
 *     opposed to Grid (which would inject artificial tile seams into
 *     the smooth galactic disk).
 *
 *   COUNTER-ROTATION FOR PATTERN SPEED = zn[2] post-rotation −π/12
 *     Density waves rotate at Ω_p, but stars rotate at Ω(r) ≠ Ω_p
 *     almost everywhere. The post-multiply counter-rotation by
 *     −π/12 (−15°, matching the pitch angle) implements this
 *     differential: the spiral pattern stays visually fixed while
 *     iteration cycles make stellar content flow through the arms.
 *
 *   STABILITY PRODUCT = 0.74 · 0.92 = 0.681
 *     Well-bounded. Galaxies are NOT chaotic systems — they are
 *     stable dynamical equilibria. A moderate contracting product
 *     (not near-critical) keeps the spiral pattern calm and
 *     long-lived, matching real galactic arm lifetimes of ≥ 10⁹ yr.
 *
 *   LINDBLAD RESONANCE PHASE = zn[3] = (0, π/6)
 *     Pure imaginary post-translate rotates the radial structure by
 *     π/6 (30°) each iteration. This encodes the inner/outer
 *     Lindblad resonance offset — the characteristic angle between
 *     the resonant spiral density wave peaks and the star field
 *     streaming through them.
 *
 *   RADIAL DEPTH STRUCTURE = color mode 1 (Spiral)
 *     Mode 1 maps log_r = log|t_z| directly into hue, so each
 *     concentric "density ring" of the galaxy gets a distinct hue —
 *     making the radial ring structure (bulge → arms → halo)
 *     visually readable. Luma depth 0.24 with center 2.2 creates
 *     visible bright/dark radial banding, mirroring the alternating
 *     arm/interarm brightness contrast seen in real galaxies.
 *
 *   LONG-TRAIL PERSISTENCE = fade 0.988 (Q ≈ 260)
 *     Real density waves persist for gigayears. fade ≈ 0.988 gives
 *     an effective lifetime of ~83 frames — long enough for the
 *     eye to see arm persistence while iteration continues flowing
 *     new stars through them.
 *
 *
 * WHAT TO WATCH:
 *   1. TWO ARMS winding from a bright central bulge outward in a
 *      tight, uniform 15° log-spiral — the signature Sa/Sb morphology.
 *   2. RADIAL RING STRUCTURE: hue banding from inner disk (near-red)
 *      to outer halo (near-blue) as log_r grows. The luma_shape_depth
 *      highlights alternating bright arms vs dark interarm regions.
 *   3. SLOW EVOLUTION: the arm pattern itself is nearly fixed, but
 *      the luminosity/color within each arm sweeps slowly — matching
 *      the density wave theory's "stars flow, arms stay" prediction.
 *   4. CENTRAL BAR-LIKE DENSITY: the narrow seed (0.012) keeps the
 *      interior feedback-dominated, producing a bright bar-like
 *      inner region reminiscent of the Milky Way's central bar.
 *   5. COROTATION BOUNDARY visible as a subtle hue transition at
 *      mid-radius where Ω(r) ≈ Ω_p — the log_r hue coupling makes
 *      this natural resonance radius show up as a color ring.
 *
 * Complementary to `exp_decagonal_spiral_abyss` (10-fold mandala,
 * not m=2) and `exp_golden_phyllotaxis_cathedral` (phyllotaxis rather
 * than galaxy). This preset is specifically astronomical/galactic.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern exp_lin_shu_spiral_galaxy_density_wave() {
  // Pitch angle α = 15° (Milky Way / Sa-Sb type).
  // Rotation in the complex plane is measured from the REAL axis,
  // but galactic pitch is measured from CIRCLES (the imaginary axis
  // direction in our parameterization). So φ = π/2 − α = 75°.
  constexpr float COS_75 = 0.2588190451f;  // cos(75°) = sin(15°)
  constexpr float SIN_75 = 0.9659258263f;  // sin(75°) = cos(15°)

  // Post: counter-rotation by −π/12 (−15°) — matches pitch angle,
  // giving pattern-speed differential that keeps the arms visually
  // fixed while stars flow through.
  constexpr float COS_N15 = 0.9659258263f;   // cos(−15°) = cos(15°)
  constexpr float SIN_N15 = -0.2588190451f;  // sin(−15°) = −sin(15°)

  // Exp inner: design guide §3 recommends |zn[0]| in 0.39–0.98
  // (keep moderate so e^z stays bounded). 0.74 places us in the
  // "log spiral tunnels" regime — rich enough for dense arm
  // structure, gentle enough to remain stable long-term.
  constexpr float PRE  = 0.74f;
  constexpr float POST = 0.92f;
  // Stability product: 0.74 · 0.92 = 0.681. Comfortably contracting.

  // π/6 ≈ 0.5236 — Lindblad resonance phase offset.
  // Pure imaginary zn[3] rotates the radial exp structure by this
  // angle, giving the classic "twist" where density-wave peaks
  // don't coincide with the star field's phase extrema.
  constexpr float LINDBLAD_PHASE = 0.5235987756f;

  Pattern p;
  p.name = "exp_lin_shu_spiral_galaxy_density_wave";
  p.inner = InnerFunction::Exp;          // log-spiral generator
  p.reduction = ReductionType::Torus;    // smooth disk, m=2 symmetry

  // Pre-affine: 0.74 magnitude at 75° phase → pitch α = 15°
  p.zn.zn[0] = {COS_75 * PRE, SIN_75 * PRE};
  p.zn.zn[1] = {0.0f, 0.0f};              // no pre-translate: galaxy centered

  // Post-affine: 0.92 magnitude at −15° phase → pattern speed offset
  p.zn.zn[2] = {COS_N15 * POST, SIN_N15 * POST};

  // Post-translate: pure imaginary Lindblad resonance phase
  p.zn.zn[3] = {0.0f, LINDBLAD_PHASE};

  // ── Color: MODE 1 (Spiral) — radial-ring + angular-sector hue ──
  p.color.hue_rotation = 0.011f;
  p.color.sat_scale = 1.06f;
  p.color.fade = 0.988f;                  // Q ≈ 260: gigayear pattern persistence
  p.color.color_mode = as_int(ColorMode::Spiral);
  p.color.spiral_angle_coeff = 0.90f;     // angular sectors: 2 arms visible as hues
  p.color.spiral_radius_coeff = 0.52f;    // strong radial depth banding
  // Mode 1 uses the mode-1/2/3 breathing + velocity infrastructure
  // lightly — keep these near defaults so Mode 1's pure spiral
  // behavior dominates the visualization.
  p.color.chroma_breath_amp = 0.06f;
  p.color.chroma_breath_freq = 0.07f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.010f;
  p.color.hue_velocity_breath = 0.004f;
  p.color.hue_velocity_drift = 0.002f;
  // Luma shaping: visible bright arms vs dark interarm
  p.color.luma_shape_depth = 0.24f;
  p.color.luma_shape_center = 2.2f;

  // ── Seed: NARROW — feedback-dominated interior (bright bulge) ──
  p.seed.border_width = 0.012f;           // thin: ~1.2% of unit cell
  p.seed.hue_speed = 0.05f;               // slow hue drift
  p.seed.alpha = 0.92f;

  // Speed: CONTEMPLATIVE — galaxies rotate on 10⁸-year timescales.
  // 0.45 matches the "slow" cluster used by deep-structure presets
  // (tanh_infinity_mirror, breathing_cosmos).
  p.speed = 0.45f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.113f;
  p.zn.zn0_anim_amp = 0.049f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.221f;
  p.zn.zn1_anim_amp = 0.052f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.113f;
  p.seed.pulse_amp = 0.16f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.081f,
                            .amplitude = 0.146f,
                            .offset = 0.520f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.073f,
                            .amplitude = 0.061f,
                            .offset = 0.060f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.062f,
                            .amplitude = 0.030f,
                            .offset = 0.050f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
