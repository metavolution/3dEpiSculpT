#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Cube gluon flux mandala preset.
 *
 * QUANTUM CHROMODYNAMICS: COLOR CONFINEMENT & THE BARYON FLUX JUNCTION
 *
 * In 1973, Gross, Wilczek, and Politzer discovered ASYMPTOTIC FREEDOM
 * (Nobel Prize 2004): the strong force becomes WEAKER at short distances
 * and STRONGER at long distances — the opposite of electromagnetism.
 * This leads to COLOR CONFINEMENT: quarks can never be isolated.
 *
 * SU(3) COLOR SYMMETRY:
 *   - 3 color charges: red, green, blue
 *   - 8 gluons (3²-1 = 8 generators of SU(3))
 *   - Gluons themselves carry color → self-interaction (unlike photons)
 *   - This self-interaction creates CONFINEMENT: the Clay Millennium
 *     Prize problem (prove Yang-Mills mass gap, $1M, still unsolved)
 *
 * A BARYON (proton, neutron) consists of three quarks connected by a
 * Y-SHAPED GLUON FLUX TUBE — a Mercedes-Benz junction of chromodynamic
 * field energy.  Three flux tubes meet at a STRING JUNCTION, the center
 * of the baryon.  The total color at the junction sums to white:
 *
 *   red + green + blue = white (color singlet)
 *
 * If you pull quarks apart, the flux tube STRETCHES, storing ~1 GeV/fm
 * of energy.  Beyond ~1 fm, the string breaks and creates a NEW quark-
 * antiquark pair — you get more hadrons, never a free quark.
 *
 * THE QCD VACUUM is not empty.  It contains a fluctuating GLUON
 * CONDENSATE: <(alpha_s/pi) G_mu_nu G^mu_nu> ~ 0.012 GeV^4.
 * Virtual gluon pairs are continuously created and annihilated,
 * and it is this vacuum energy that generates ~99% of the proton's
 * mass (the Higgs mechanism accounts for only ~1%).
 *
 * ENGINE MAPPING TO QCD COLOR CONFINEMENT:
 *
 *   THE COLOR TRIPLET = Cube inner function (z^3)
 *     z^3 has EXACT 3-fold rotational symmetry: z -> r^3 * e^{i*3*theta},
 *     tripling every angle.  This IS the fundamental representation of
 *     SU(3): three equivalent color states related by 120-degree rotations.
 *
 *     The Y-junction appears at z=0, where z^3 creates a 3-fold symmetric
 *     sink — the string junction where three flux tubes meet.  Under
 *     iteration, z^3 creates kaleidoscopic 3-fold structure that, combined
 *     with Torus mirror-folding, produces 6-fold MANDALA geometry: the
 *     baryon's internal color field made visible as fractal symmetry.
 *
 *     CRITICAL: z^3 TRIPLES the pre-rotation angle.  With the golden-root
 *     pre-rotation at 51.83 degrees, z^3 maps this to 155.5 degrees —
 *     deep into the second quadrant, creating near-bilateral patterns
 *     with 3-fold modulation: a BREATHING TREFOIL.
 *
 *   THE CONFINEMENT = Torus reduction
 *     Torus provides SEAMLESS wrapping — no escape from the color field.
 *     Quarks that try to leave are folded back into the confinement volume,
 *     exactly as the QCD string tension prevents isolation.  The smooth
 *     Torus wrapping (no tile edges) creates the mandala's unbroken symmetry.
 *
 *   THE GOLDEN ROOT = arctan(sqrt(phi)) ~ 51.83 degree pre-rotation
 *     cos(arctan(sqrt(phi))) = 1/phi EXACT
 *     sin(arctan(sqrt(phi))) = 1/sqrt(phi) EXACT
 *
 *     Proof: 1 + tan^2(theta) = 1 + phi = phi^2  (golden identity phi^2 = phi+1)
 *            => sec(theta) = phi, cos(theta) = 1/phi
 *            sin = sqrt(phi)/phi = 1/sqrt(phi)
 *     Check: (1/phi)^2 + (1/sqrt(phi))^2 = 1/phi^2 + 1/phi = (1+phi)/phi^2
 *            = phi^2/phi^2 = 1  EXACT
 *
 *     phi is the MOST IRRATIONAL number — its continued fraction [1;1,1,1,...]
 *     converges the slowest.  The golden-root rotation produces the DENSEST
 *     possible quasiperiodic pattern: maximum visual complexity, minimum
 *     repetition.  360 deg / 51.83 deg ~ 6.95 -> quasi-7-fold azimuthal symmetry.
 *
 *   THE PENTAGONAL DEPTH = -arctan(2) ~ -63.43 degree post-rotation
 *     cos(arctan(2)) = 1/sqrt(5) EXACT  (Pythagorean: 1^2 + 2^2 = 5)
 *     sin(-arctan(2)) = -2/sqrt(5) EXACT
 *
 *     sqrt(5) = 2*phi - 1: the golden ratio's DISCRIMINANT.  This connects
 *     the pre-rotation (phi itself) to the post-rotation (sqrt(5) = phi's
 *     algebraic core) through one fundamental identity:  phi = (1+sqrt(5))/2.
 *
 *     360 deg / 63.43 deg ~ 5.67 -> quasi-5.7-fold depth symmetry.
 *     Combined: 7-fold x 5.7-fold ~ 40 interference cells.
 *     The phi-sqrt(5) pair creates MAXIMALLY INCOMMENSURATE depth layering:
 *     the pattern at each depth level has a different relationship to its
 *     neighbors, preventing any periodic repetition.
 *
 *   QCD VACUUM OSCILLATION = Mode 2 (Breathing)
 *     The breathing IS the gluon condensate fluctuation: chromodynamic
 *     field energy pulsing as virtual gluon pairs are created and
 *     annihilated.  The spatial phase gradient (chroma_breath_phase = 2.5)
 *     makes the breathing ROTATE around the Y-junction — the 3 color
 *     charges cycling as the vacuum condensate shifts between sectors.
 *
 * WHAT TO WATCH:
 *   1. Y-JUNCTION: 3-fold gluon flux tube meeting point at center
 *   2. MANDALA: Cube + Torus -> 6-fold kaleidoscopic structure
 *   3. GOLDEN QUASIPERIODIC: phi rotation -> never-repeating richness
 *   4. BREATHING: QCD vacuum condensate oscillation (gluon pairs)
 *   5. CONFINEMENT: all structure folds back, nothing escapes (Torus)
 *   6. PENTAGONAL DEPTH: 1/sqrt(5) creates golden-ratio layer stacking
 *   7. TREFOIL: near-bilateral from z^3 tripling the golden-root angle
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern cube_gluon_flux_mandala() {
  // arctan(sqrt(phi)) ~ 0.9046 rad (51.83 deg): the golden-root angle
  // cos = 1/phi EXACT, sin = 1/sqrt(phi) EXACT (from phi^2 = phi+1)
  // phi = most irrational -> densest quasiperiodic mandala structure
  // z^3 triples to 155.5 deg -> near-bilateral breathing trefoil
  constexpr float CGR  =  0.6180f;           // cos(arctan(sqrt(phi))) = 1/phi
  constexpr float SGR  =  0.7862f;           // sin(arctan(sqrt(phi))) = 1/sqrt(phi)
  // -arctan(2) ~ -1.1071 rad (-63.43 deg): the sqrt(5) depth angle
  // cos = 1/sqrt(5) EXACT, sin = -2/sqrt(5) EXACT (1^2 + 2^2 = 5)
  // sqrt(5) = 2*phi - 1: connects pre(phi) to post(sqrt(5)) algebraically
  constexpr float C5   =  0.4472f;           // cos(arctan(2)) = 1/sqrt(5)
  constexpr float S5N  = -0.8944f;           // sin(-arctan(2)) = -2/sqrt(5)

  constexpr float PRE  = 0.90f;  // moderate: z^3 amplifies (|z^3| = |z|^3)
  constexpr float POST = 0.92f;  // gentle contraction: product 0.828

  Pattern p;
  p.name = "cube_gluon_flux_mandala";
  p.inner = InnerFunction::Cube;            // 3-fold: SU(3) color triplet z^3
  p.reduction = ReductionType::Torus;       // confinement: quarks cannot escape

  p.zn.zn[0] = {CGR * PRE, SGR * PRE};
  p.zn.zn[1] = {0.005f, -0.003f};          // perturbation: QCD vacuum noise
  p.zn.zn[2] = {C5 * POST, S5N * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // -- Color: MODE 2 (Breathing) -- QCD vacuum condensate fluctuation --
  p.color.hue_rotation = 0.012f;            // moderate: color field evolution
  p.color.sat_scale = 1.10f;                // moderate: clean gluon field glow
  p.color.fade = 0.986f;                    // deep: ~71 frame persistence
  p.color.color_mode = as_int(ColorMode::Breathing);                   // Breathing: vacuum condensate oscillation
  p.color.spiral_angle_coeff = 1.10f;       // STRONG: shows 3-fold/6-fold structure
  p.color.spiral_radius_coeff = 0.45f;      // moderate: flux tube depth rings
  // QCD VACUUM BREATHING: gluon condensate fluctuation
  p.color.chroma_breath_amp = 0.28f;        // moderate-strong: visible vacuum pulsation
  p.color.chroma_breath_freq = 0.09f;       // chromodynamic oscillation rate
  p.color.chroma_breath_phase = 2.5f;       // strong: rotating color field gradient
  // Mode 2 does not use hue velocity params
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: moderate flux tube contrast
  p.color.luma_shape_depth = 0.22f;         // moderate: Y-junction visibility
  p.color.luma_shape_center = 1.8f;

  // -- Seed: narrow -- flux junction structure dominates --
  p.seed.border_width = 0.015f;             // narrow: confinement field dominates
  p.seed.hue_speed = 0.06f;                 // moderate: color charge cycling
  p.seed.alpha = 0.90f;

  p.speed = 0.55f;                          // moderate: watch confinement mandala form
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.110f;
  p.zn.zn0_anim_amp = 0.044f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.189f;
  p.zn.zn1_anim_amp = 0.037f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.081f;
  p.seed.pulse_amp = 0.20f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.070f,
                            .amplitude = 0.126f,
                            .offset = 0.450f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.118f,
                            .amplitude = 0.120f,
                            .offset = 0.280f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.084f,
                            .amplitude = 0.030f,
                            .offset = 0.060f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
