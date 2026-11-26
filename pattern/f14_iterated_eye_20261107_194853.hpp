#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * F14 Iterated Eye — showcase for GPU inner ID 34
 * (z + i·cos(i·sin(i·z))).
 *
 * MATHEMATICS: TRIPLE-NESTED COMPLEX FEEDBACK
 *
 * The map f(z) = z + i·cos(i·sin(i·z)) unwraps via the identity
 * cos(iu) = cosh(u), sin(iv) = i·sinh(v):
 *
 *   sin(iz)        = i·sinh(z)
 *   i·sin(iz)      = -sinh(z)
 *   cos(-sinh(z))  = cos(sinh(z))                    (cos is even)
 *   i·cos(sinh(z))                                    ← bounded-but-complex term
 *
 *   ⇒  f(z) = z + i·cos(sinh(z))
 *
 * The dynamics are AFFINE z + bounded-but-non-trivial deformation.  Where
 * sinh(z) is small (near origin), cos(sinh(z)) ≈ 1, and f(z) ≈ z + i — a
 * pure vertical drift.  Where sinh(z) grows (large |Re(z)|), cos oscillates
 * at exponential frequency in the imaginary part of cos(sinh(z)), creating
 * fine vertical fringe stratification.  The cumulative effect of iterating
 * this *nearly-affine* map under a contracting outer affine is a stack of
 * concentric eye-shaped lobes — the IMAGINARY DRIFT TERM defines the lid,
 * and the COSH-MODULATED FRINGE defines the iris striations.
 *
 * Triple nesting means every fixed point is a saddle: small perturbations
 * propagate through three trigonometric/hyperbolic compositions before
 * touching z again, producing extreme sensitivity to zn1/zn3 placement.
 *
 * COLOR MODE 0 — CLASSIC HSLA:
 *
 *   Straightforward HSL rotation with distance modulation.
 *
 * Mode 0 is the *least* opinionated color path; it lets the eye-stack
 * structure read crisply without competing spatial/temporal hue gradients.
 * The other landed presets use modes 1, 2, 4 — choosing 0 here gives this
 * preset its own visual signature: graphic, sharp, almost printmaking-like.
 *
 * AFFINE GEOMETRY:
 *
 * zn0 = (0.80, 0.10): moderate contraction tames the cosh-frequency
 * blow-up at large Re(z) without crushing the eye lobes; 7° tilt slants
 * the eye stack diagonally for graphic interest.
 *
 * zn1 = (0.0, -0.025): small downward bias resists the +i drift baked
 * into f(z) = z + i·…, balancing the iteration to a near-stationary
 * orbit — the eye stack becomes visible *because* zn1 cancels the
 * intrinsic vertical migration.
 *
 * zn2 = (0.94, 0.02): very gentle post-affine.
 *
 * zn3 = (0.0, 0.0): no post-translate — keep the eye stack centered.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode0) → seed → blend
 */
inline Pattern f14_iterated_eye() {
  Pattern p;
  p.name = "f14_iterated_eye";
  // Named-enum path: InnerFunction::F14 ↔ GPU id 34 (z + i·cos(sinh(z)))
  p.inner = InnerFunction::F14;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus;

  // Moderate contraction + 7° tilt: tames cosh-frequency, slants stack
  p.zn.zn[0] = {0.80f, 0.10f};
  // Small downward bias cancels f's intrinsic +i drift → stationary orbit
  p.zn.zn[1] = {0.0f, -0.025f};
  // Very gentle post-affine: don't disturb the eye geometry
  p.zn.zn[2] = {0.94f, 0.02f};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 0 (Classic HSLA) — crisp graphic readout of the structure ──
  p.color.hue_rotation      = 0.009f;   // brisker base hue: gives eye iris motion
  p.color.sat_scale         = 1.20f;    // high saturation: graphic, not painterly
  p.color.fade              = 0.988f;   // moderate fade: keep the iris sharp
  p.color.color_mode        = as_int(ColorMode::Classic);        // Classic HSLA
  p.color.distance_modulate = true;     // distance term emphasizes lobe boundaries
  // NOTE: distance modulation STRENGTH (formerly attempted as `distance_scale`
  //       / `distance_amount`) is a runtime concern, not a preset concern —
  //       those floats live on `VisualMode` and map to GPU `params.len_sc`
  //       and `params.len_sc_amt` via `mode_runtime_bridge.hpp`. The preset
  //       layer only owns the on/off toggle (the bool above). To set custom
  //       strength for this preset, drive it from the active `VisualMode`
  //       (e.g. via a mode persistence file or runtime command).
  p.color.spiral_angle_coeff  = 1.0f;   // not used heavily by mode 0; safe defaults
  p.color.spiral_radius_coeff = 0.50f;
  p.color.chroma_breath_amp   = 0.10f;  // minimal breathing — mode 0 is the star
  p.color.chroma_breath_freq  = 0.30f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base   = 0.10f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift  = 0.20f;
  p.color.luma_shape_depth    = 0.14f;
  p.color.luma_shape_center   = 1.5f;

  // ── Seed: medium-narrow border — refresh outer eyelid edges ──
  p.seed.border_width = 0.012f;
  p.seed.hue          = 0.05f;   // amber seed: warm iris against cool stack
  p.seed.hue_speed    = 0.020f;
  p.seed.alpha        = 0.82f;

  p.speed = 0.40f;  // brisker: feedback structure rewards visible motion

  // ── Motion: iris drift + focal wander — the eye opens, breathes, wanders ──
  // hue_drift:    iris colour shifts ±0.005 around 0.009 on a 32 s cycle.
  //               At mode 0 (classic HSLA), hue_rot is the dominant chromatic
  //               driver — a slow sine on it reads as a gradual iris colour shift.
  // zn1_wander:   The attractor centre (zn1) wanders via independent NoiseHash
  //               lanes: real (seed 17) and imaginary (seed 18) are statistically
  //               independent, so the focal point traces a non-repeating Brownian
  //               path through ℂ.  The im_centre = -0.025 matches the preset's
  //               design value (the downward bias that cancels f's intrinsic +i drift).
  using namespace epi::control;
  p.motion = PatternMotion::slow_color_drift(0.009f, 0.005f, 32.0f);
  p.motion.add("zn1_re_wander",
               ModLane{.wave         = Waveform::NoiseHash,
                       .frequency_hz = 1.0f / 22.0f,
                       .amplitude    = 0.007f,
                       .offset       = 0.0f,
                       .seed         = 17u},
               ModTarget::Zn1Re);
  p.motion.add("zn1_im_wander",
               ModLane{.wave         = Waveform::NoiseHash,
                       .frequency_hz = 1.0f / 22.0f,
                       .amplitude    = 0.007f,
                       .offset       = -0.025f,  // design resting value
                       .seed         = 18u},
               ModTarget::Zn1Im);
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.177f;
  p.zn.zn0_anim_amp = 0.035f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.175f;
  p.zn.zn1_anim_amp = 0.040f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.121f;
  p.seed.pulse_amp = 0.15f;


  return p;
}

} // namespace epi::presets
