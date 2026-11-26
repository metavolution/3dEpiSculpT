# EpiC 3D dimensional preset atlas

These `.epimode` files are deliberately small, hand-editable scene recipes for
`SculptureSerializer::loadEpimodeFromFile`.  They use only parameters that the
renderer-neutral `SculptureProgramParams` contract currently understands.

The dimensional language is visual rather than a claim that the renderer is
literally displaying a higher-dimensional space:

- **2.5D** layers a field, advection, temporal feedback, and a volume into an
  object that reads as both surface and depth.
- **3.5D** uses a changing latent coordinate (`latentW`) as a controlled extra
  degree of freedom that deforms a three-dimensional projection.
- **4D projection** switches on the existing six-plane manifold path and uses
  the available hypersphere/cross-polytope/quaternion/octetion seed families.

Each file follows the legacy JSON `.epimode` grammar.  Enum values are their
public names from `SculptureProgram.h`; unspecified fields retain the stable
`SculptureProgramParams` defaults.  This keeps the files forward-compatible
with new renderer-neutral parameters while preserving intentionally selected
settings in version control.

Suggested first tour:

1. `dimensional/01_hopf_lantern.epimode`
2. `dimensional/02_tesseract_bloom.epimode`
3. `dimensional/03_quaternion_julia_chapel.epimode`
4. `kinetic/04_lorenz_silk_atrium.epimode`
5. `cosmological/08_calabi_yau_aurora.epimode`

The visual chain is always: **pre-affine -> inner map -> reduction -> seed ->
field/volume -> material colour -> presentation**.  Presets therefore combine
topology, temporal behavior, and material cues instead of treating colour as a
separate afterthought.
