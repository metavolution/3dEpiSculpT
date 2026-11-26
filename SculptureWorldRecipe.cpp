#include "SculptureWorldRecipe.h"
#include "SculptureModeProjection.h"

namespace epi_sculpt {

namespace {

constexpr std::array kWorldCatalog{
    WorldCatalogEntry{
        .canonicalPresetId = "authored/cosmological/93_schwarzschild_black_hole",
        .title = "Schwarzschild black-hole lens",
        .aliases = {"schwarzschild_black_hole", "black_hole", "gravitational_lens"},
        .availability = WorldRouteAvailability::Implemented,
        // The optical program is produced by the established implicit route;
        // OrbitSamples is retained only as the neutral mathematical default
        // because no instanced generator participates in this presentation.
        .generator = WorldGeneratorKind::OrbitSamples,
        .primitive = WorldPrimitiveKind::Sphere,
        .delivery = WorldRecipeDelivery::ExistingCompactPath,
        .presentation = WorldPresentationKind::ImplicitField,
        .instanceBudget = 0U,
        .generatorHz = 20U,
        .materialBandCount = 1U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/kinetic/81_kuramoto_lattice_synchronization",
        .title = "Kuramoto q-bit synchronization lattice",
        .aliases = {"authored/kinetic/68_kuramoto_phase_lattice_sync",
                    "kuramoto_synchronization_manifold",
                    "quantum_wave_interference",
                    "spatial_wave_minimal_lattice",
                    "authored/dimensional/201_kuramoto_lattice"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::KuramotoLattice,
        .primitive = WorldPrimitiveKind::TorusKnot,
        .instanceBudget = 21U,
        // Three semantic roles: Form (sync core), Structure (coupling grid), Accent (phase slips).
        .materialBandCount = 3U,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/80_clifford_so4_isoclinic_fibration",
        .title = "Clifford-Hopf fiber constellation",
        .aliases = {"authored/dimensional/67_hopf_clifford_fibers",
                    "authored/dimensional/93_hopf_clifford_fibration_superstructure",
                    "hopf_fibration_drift",
                    "clifford_hopf_fibration_continuum",
                    "authored/dimensional/202_clifford_hopf"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::HopfFibers,
        .primitive = WorldPrimitiveKind::TorusKnot,
        .instanceBudget = 32U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/cosmological/83_golden_phyllotaxis_cathedral",
        .title = "Golden phyllotaxis cathedral",
        .aliases = {"exp_golden_phyllotaxis_cathedral", "golden_ratio",
                    "authored/dimensional/203_golden_phyllotaxis"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::GoldenPhyllotaxis,
        .primitive = WorldPrimitiveKind::Cylinder,
        .instanceBudget = 1024U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/kinetic/84_rossler_lorenz_strange_attractor",
        .title = "Rossler-Lorenz lantern garden",
        .aliases = {"sinh_rossler_spiral_ribbon", "strange_attractor",
                    "silent_lorenz_lanterns", "rossler_lantern_garden",
                    "authored/dimensional/204_rossler_lanterns"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::StrangeAttractorLanterns,
        .primitive = WorldPrimitiveKind::Cylinder,
        .instanceBudget = 256U,
        .materialBandCount = 3U,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/86_120_cell_quantum_hologram",
        .title = "120-cell quantum hologram",
        .aliases = {"120_cell_quantum_hologram", "quantum_spin_liquid_120cell",
                    "authored/dimensional/205_cell120_hologram"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::Cell120Hologram,
        .primitive = WorldPrimitiveKind::TorusKnot,
        .instanceBudget = 600U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    // A compact performance composition with its own bounded OpenCL geometry
    // branch. It emits a faceted core, mirror/truss shell, and outer field via
    // the existing ABI and bridge rather than adding another runtime path.
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/200_neon_bubblesphere",
        .title = "Neon epimorphic bubblesphere",
        .aliases = {"neon_bubblesphere", "bubblesphere", "neon_hopf_orb"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::NeonBubblesphere,
        .primitive = WorldPrimitiveKind::TorusKnot,
        .instanceBudget = 2048U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
        .mirrorTopology = MirrorCellTopology::Hexagonal,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/85_ammann_beenker_quasicrystal_lattice",
        .title = "Ammann-Beenker quasicrystal field",
        .aliases = {"ammann_beenker_quasicrystal", "hofstadter_butterfly_spectrum",
                    "authored/dimensional/206_ammann_beenker"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::AmmannBeenkerField,
        .primitive = WorldPrimitiveKind::Tile,
        .instanceBudget = 768U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/kinetic/87_chladni_quantum_membrane",
        .title = "Chladni quantum membrane",
        .aliases = {"faraday_chladni_resonance", "acoustic_cymatic",
                    "chladni_quantum_membrane",
                    "authored/dimensional/207_chladni_membrane"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::ChladniMembrane,
        .primitive = WorldPrimitiveKind::Pillar,
        .instanceBudget = 64U * 36U,
        .materialBandCount = 3U,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/cosmological/88_lichtenberg_discharge_forest",
        .title = "Lichtenberg discharge forest",
        .aliases = {"lichtenberg_discharge", "lichtenberg_forest",
                    "authored/dimensional/208_lichtenberg_forest"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::LichtenbergForest,
        .primitive = WorldPrimitiveKind::BranchSegment,
        .instanceBudget = 2048U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/cosmological/89_epimorphic_gyroid_maze",
        .title = "Epimorphic gyroid maze",
        .aliases = {"epimorphic_gyroid", "gyroid_maze", "gyroid_minimal_surface",
                    "authored/dimensional/209_gyroid_maze"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::EpimorphicGyroidMaze,
        .primitive = WorldPrimitiveKind::Cube,
        .instanceBudget = 2400U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/neural_synapse_lattice",
        .title = "Neural Synapse Lattice",
        .aliases = {"neural", "synapse_lattice"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::NeuralSynapseLattice,
        .primitive = WorldPrimitiveKind::Sphere,
        .instanceBudget = 15000U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/bioluminescent_spore_swarm",
        .title = "Bioluminescent spore swarm",
        .aliases = {"spores", "bioluminescent_spore"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::BioluminescentSporeSwarm,
        .primitive = WorldPrimitiveKind::Sphere,
        .instanceBudget = 10000U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/procedural_woodland_world",
        .title = "Procedural woodland world",
        .aliases = {"woodland", "procedural_woodland"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::ProceduralWoodland,
        // The generator dynamically overrides this to Sphere (canopy) or Cylinder (trunk).
        // We set the baseline budget.
        .primitive = WorldPrimitiveKind::Sphere,
        .instanceBudget = 4000U,
        .uses4DProjection = false,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/cosmological/92_mandelbulb_fractal",
        .title = "Mandelbulb Quantum Foam",
        .aliases = {"mandelbulb", "quantum_foam", "mandelbulb_fractal"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::MandelbulbFractal,
        .primitive = WorldPrimitiveKind::Sphere,
        .instanceBudget = 4000U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/cosmological/91_kali_set_orbits",
        .title = "Kali set fractal orbits",
        .aliases = {"kali_set", "fractal_orbits", "kali_fractal"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::KaliSetOrbits,
        .primitive = WorldPrimitiveKind::Cube, // Cubes rotating in fractal space
        .instanceBudget = 4096U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/125_chirikov_kam_island_chain_35d",
        .title = "Chirikov KAM island chain",
        .aliases = {"chirikov_kam", "hamiltonian_chaos", "kam_tori"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::KuramotoLattice,
        .primitive = WorldPrimitiveKind::TorusKnot,
        .instanceBudget = 2048U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/126_allen_cahn_domain_coarsening_25d",
        .title = "Allen-Cahn domain coarsening",
        .aliases = {"allen_cahn", "domain_coarsening", "phase_separation"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::ChladniMembrane,
        .primitive = WorldPrimitiveKind::Tile,
        .instanceBudget = 2304U,
        .uses4DProjection = false,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/127_cube_gluon_flux_mandala_4d",
        .title = "Cube gluon flux mandala & 4D tesseract",
        .aliases = {"gluon_flux", "qcd_mandala", "flux_tube", "hypercube_tesseract",
                    "tesseract", "authored/dimensional/210_hypercube_tesseract"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::HypercubeTesseract,
        .primitive = WorldPrimitiveKind::Cube,
        .instanceBudget = 1024U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/130_calabi_yau_manifold",
        .title = "Calabi-Yau quintic manifold",
        .aliases = {"calabi_yau", "string_theory_compactification", "quintic_manifold"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::CalabiYauManifold,
        .primitive = WorldPrimitiveKind::TorusKnot,
        .instanceBudget = 1000U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/155_deep_sea_siphonophore_ecology",
        .title = "Deep Sea Siphonophore Ecology",
        .aliases = {"deep_sea", "siphonophore", "siphonophore_ecology"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::DeepSeaSiphonophores,
        .primitive = WorldPrimitiveKind::Cylinder, // The stem/zooids are drawn as cylinders or tubes
        .instanceBudget = 8192U, // 256 * 32 = 8192
        .uses4DProjection = false,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/160_lsystem_fractal_tree",
        .title = "L-System Fractal Tree",
        .aliases = {"lsystem", "fractal_tree", "recursive_tree"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::LSystemFractal,
        .primitive = WorldPrimitiveKind::Cylinder,
        .instanceBudget = 16383U,
        .uses4DProjection = false,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/quantum/170_hydrogen_atom_orbitals",
        .title = "Hydrogen Atom Wavefunction",
        .aliases = {"hydrogen", "wavefunction", "quantum_orbitals"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::HydrogenAtomOrbitals,
        .primitive = WorldPrimitiveKind::Sphere,
        .instanceBudget = 32768U, // 32x32x32 lattice
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/180_clifford_hopf_multi_soliton_lattice_4d",
        .title = "Clifford-Hopf Multi-Soliton Lattice",
        .aliases = {"clifford_hopf_soliton", "clifford_hopf_multi_soliton_lattice_4d", "twistor_foliation_lattice", "soliton_lattice_4d"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::CliffordHopfSolitonLattice,
        .primitive = WorldPrimitiveKind::TorusKnot,
        .instanceBudget = 1024U,
        .generatorHz = 30U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/189_so4_isoclinic_penrose_twistor_foliation_4d",
        .title = "Penrose Twistor Foliation & SO(4) Isoclinic World",
        .aliases = {"penrose_twistor_foliation", "penrose_twistor", "so4_twistor_foliation", "robinson_congruence"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::PenroseTwistorFoliation,
        .primitive = WorldPrimitiveKind::TorusKnot,
        .instanceBudget = 1280U,
        .generatorHz = 30U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/181_hecatonicosachoron_120cell_coxeter_4d",
        .title = "120-Cell Hecatonicosachoron Coxeter 4D",
        .aliases = {"120_cell_coxeter", "hecatonicosachoron", "120cell_4d", "dodecaplex"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::Hecatonicosachoron120Cell4D,
        .primitive = WorldPrimitiveKind::Sphere,
        .instanceBudget = 600U,
        .generatorHz = 30U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/182_hexacosichoron_600cell_h4_polytope_4d",
        .title = "600-Cell Hexacosichoron Polytope",
        .aliases = {"hexacosichoron", "600_cell", "hexacosichoron_600cell", "hexacosichoron_4d", "h4_polytope"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::LieE8GossetLattice,
        .primitive = WorldPrimitiveKind::TorusKnot,
        .instanceBudget = 720U,
        .generatorHz = 30U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/183_hyperbolic_icosahedral_honeycomb_horosphere_4d",
        .title = "Hyperbolic {3,5,3} Icosahedral Honeycomb Horosphere 4D",
        .aliases = {"hyperbolic_honeycomb_4d", "icosahedral_honeycomb", "horosphere_4d", "hyperbolic_353"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::HyperbolicHoneycombHorosphere4D,
        .primitive = WorldPrimitiveKind::TorusKnot,
        .instanceBudget = 1024U,
        .generatorHz = 30U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/184_lie_e8_root_lattice_coxeter_projection_4d",
        .title = "Lie E8 Gosset Root Lattice Projection",
        .aliases = {"lie_e8_gosset", "e8_root_lattice", "gosset_421", "e8_coxeter_projection", "184_lie_e8_root_lattice_4d"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::LieE8GossetLattice,
        .primitive = WorldPrimitiveKind::Sphere,
        .instanceBudget = 960U,
        .generatorHz = 30U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/185_conformal_hyperbolic_rose_petal_relief_25d",
        .title = "Conformal Hyperbolic Rose Petal Relief",
        .aliases = {"conformal_relief_25d", "rose_petal_relief", "hyperbolic_relief_25d"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::ConformalRelief25D,
        .primitive = WorldPrimitiveKind::Tile,
        .instanceBudget = 1024U,
        .generatorHz = 30U,
        .materialBandCount = 3U,
        .uses4DProjection = false,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/186_clifford_vortex_kaleidoscope_mandala_35d",
        .title = "Clifford Vortex Kaleidoscope Mandala",
        .aliases = {"toroidal_foliation_35d", "clifford_vortex_kaleidoscope", "mandala_foliation_35d"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::ToroidalFoliation35D,
        .primitive = WorldPrimitiveKind::TorusKnot,
        .instanceBudget = 1024U,
        .generatorHz = 30U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/187_dedekind_eta_quartic_soliton_lattice_4d",
        .title = "Dedekind Eta Quartic Soliton Lattice 4D",
        .aliases = {"dedekind_eta_quartic_soliton_lattice_4d", "dedekind_eta", "quartic_soliton_lattice", "calabi_yau_k3_quartic"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::DedekindEtaQuarticLattice4D,
        .primitive = WorldPrimitiveKind::Sphere,
        .instanceBudget = 1024U,
        .generatorHz = 30U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/188_kuramoto_hyperbolic_synchronization_canopy_35d",
        .title = "Kuramoto Hyperbolic Synchronization Canopy 3.5D",
        .aliases = {"kuramoto_hyperbolic_synchronization_canopy_35d", "kuramoto_canopy_35d", "hyperbolic_synchronization_canopy", "kuramoto_poincare_canopy"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::KuramotoHyperbolicCanopy35D,
        .primitive = WorldPrimitiveKind::Pillar,
        .instanceBudget = 1024U,
        .generatorHz = 30U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/190_dirac_monopole_quantum_tunnel",
        .title = "Dirac Magnetic Monopole & Quantum Flux Tunnel",
        .aliases = {"exp_dirac_monopole_tunnel", "dirac_monopole", "monopole", "dirac_string", "qed_flux_tunnel"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::DiracMonopoleQuantumTunnel,
        .primitive = WorldPrimitiveKind::Sphere,
        .instanceBudget = 2048U,
        .generatorHz = 30U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
    WorldCatalogEntry{
        .canonicalPresetId = "authored/dimensional/191_su2_yang_mills_instanton",
        .title = "SU(2) Yang-Mills BPST Instanton & Skyrmion Soliton Lattice",
        .aliases = {"su2_yang_mills_instanton", "yang_mills_instanton", "bpst_instanton", "skyrmion_soliton", "non_abelian_gauge"},
        .availability = WorldRouteAvailability::Implemented,
        .generator = WorldGeneratorKind::Su2YangMillsInstanton,
        .primitive = WorldPrimitiveKind::TorusKnot,
        .instanceBudget = 2048U,
        .generatorHz = 30U,
        .materialBandCount = 3U,
        .uses4DProjection = true,
    },
};

// These intentionally cover patterns whose visual intent is stronger than a
// generic orbit-sample fallback and which do not already resolve through a
// direct world-catalogue alias.  Each binding preserves the root Pattern's
// own epimorphism parameters at selection time.
constexpr std::array kPatternWorldBindings{
    // A triangular reflection chamber is the compact fundamental-domain proxy
    // for the kaleidoscope's mirror planes.
    PatternWorldBinding{"hyperbolic_mirror_kaleidoscope", "neon_bubblesphere",
                        MirrorCellTopology::Triangular},
    PatternWorldBinding{"tanh_infinity_mirror", "neon_bubblesphere",
                        MirrorCellTopology::Quadrilateral},
    PatternWorldBinding{"tanh_honeycomb_tunnel", "neon_bubblesphere",
                        MirrorCellTopology::Hexagonal},
    PatternWorldBinding{"tanh_pentagonal_kaleidoscope", "neon_bubblesphere",
                        MirrorCellTopology::Pentagonal},
    // The fixed world ABI deliberately bounds local cells at six sides. Dense
    // hexagons are the nearest compact proxy for 9/12-fold mirror halls.
    PatternWorldBinding{"tanh_nonagonal_mirror_hall", "neon_bubblesphere",
                        MirrorCellTopology::Hexagonal},
    PatternWorldBinding{"exp_dodecagonal_breathing_vault", "neon_bubblesphere",
                        MirrorCellTopology::Hexagonal},
    PatternWorldBinding{"cube_viral_capsid_bloom", "120_cell_quantum_hologram", std::nullopt},
    PatternWorldBinding{"weierstrass_fractal_foam", "mandelbulb", std::nullopt},
    PatternWorldBinding{"schwarzschild_gravitational_lens", "gravitational_lens", std::nullopt},
};

} // namespace

std::span<const WorldCatalogEntry> worldCatalog() noexcept {
    return kWorldCatalog;
}

const WorldCatalogEntry* findWorldCatalogEntry(
    std::string_view presetId) noexcept {
    for (const WorldCatalogEntry& entry : kWorldCatalog) {
        if (entry.matches(presetId)) return &entry;
    }
    return nullptr;
}

const WorldCatalogEntry* findAvailableWorldCatalogEntry(
    std::string_view presetId) noexcept {
    const WorldCatalogEntry* const entry = findWorldCatalogEntry(presetId);
    return entry != nullptr && entry->hasCompleteRoute() ? entry : nullptr;
}

std::span<const PatternWorldBinding> patternWorldBindings() noexcept {
    return kPatternWorldBindings;
}

const PatternWorldBinding* findPatternWorldBinding(
    std::string_view patternId) noexcept {
    for (const PatternWorldBinding& binding : kPatternWorldBindings) {
        if (binding.matches(patternId)) return &binding;
    }
    return nullptr;
}

const WorldCatalogEntry* findPreferredWorldCatalogEntryForPattern(
    std::string_view patternId) noexcept {
    const PatternWorldBinding* const binding = findPatternWorldBinding(patternId);
    return binding == nullptr
        ? nullptr
        : findAvailableWorldCatalogEntry(binding->preferredWorldAlias);
}

WorldRecipe makeWorldRecipe(
    std::string_view presetId,
    const SculptureProgramParams& params) noexcept {
    // The default preserves the already-connected producer -> bridge path.
    WorldRecipe recipe{};
    recipe.instanceBudget = static_cast<std::uint32_t>(kSampleCount);
    recipe.uses4DProjection = params.enable4D != 0U;
    recipe.primitive = params.instanceMeshType == 1U ? WorldPrimitiveKind::TorusKnot
        : params.instanceMeshType == 2U ? WorldPrimitiveKind::Cube
        : params.instanceMeshType == 3U ? WorldPrimitiveKind::Cylinder
                                        : WorldPrimitiveKind::Sphere;

    if (const WorldCatalogEntry* entry = findWorldCatalogEntry(presetId)) {
        return entry->recipe();
    }
    return recipe;
}

std::string_view worldGeneratorName(WorldGeneratorKind kind) noexcept {
    switch (kind) {
    case WorldGeneratorKind::OrbitSamples: return "compact orbit samples";
    case WorldGeneratorKind::KuramotoLattice: return "Kuramoto lattice";
    case WorldGeneratorKind::HopfFibers: return "Hopf fibers";
    case WorldGeneratorKind::GoldenPhyllotaxis: return "golden phyllotaxis";
    case WorldGeneratorKind::StrangeAttractorLanterns: return "strange-attractor lanterns";
    case WorldGeneratorKind::Cell120Hologram: return "120-cell hologram";
    case WorldGeneratorKind::AmmannBeenkerField: return "Ammann-Beenker field";
    case WorldGeneratorKind::ChladniMembrane: return "Chladni membrane";
    case WorldGeneratorKind::LichtenbergForest: return "Lichtenberg forest";
    case WorldGeneratorKind::EpimorphicGyroidMaze: return "epimorphic gyroid maze";
    case WorldGeneratorKind::HypercubeTesseract: return "tesseract hypercube projection";
    case WorldGeneratorKind::ProceduralWoodland: return "procedural woodland world";
    case WorldGeneratorKind::BioluminescentSporeSwarm: return "bioluminescent spore swarm";
    case WorldGeneratorKind::NeuralSynapseLattice: return "neural synapse lattice";
    case WorldGeneratorKind::KaliSetOrbits: return "kali set fractal orbits";
    case WorldGeneratorKind::MandelbulbFractal: return "MandelbulbFractal";
    case WorldGeneratorKind::DeepSeaSiphonophores: return "deep sea siphonophores";
    case WorldGeneratorKind::CalabiYauManifold: return "Calabi-Yau manifold";
    case WorldGeneratorKind::LSystemFractal: return "L-system fractal tree";
    case WorldGeneratorKind::HydrogenAtomOrbitals: return "hydrogen atom wavefunction";
    case WorldGeneratorKind::CliffordHopfSolitonLattice: return "Clifford-Hopf soliton lattice";
    case WorldGeneratorKind::PenroseTwistorFoliation: return "Penrose twistor foliation";
    case WorldGeneratorKind::LieE8GossetLattice: return "Lie E8 Gosset root lattice & 600-cell";
    case WorldGeneratorKind::ConformalRelief25D: return "conformal hyperbolic rose petal relief";
    case WorldGeneratorKind::ToroidalFoliation35D: return "Clifford vortex kaleidoscope mandala foliation";
    case WorldGeneratorKind::DedekindEtaQuarticLattice4D: return "Dedekind eta quartic soliton lattice 4D";
    case WorldGeneratorKind::KuramotoHyperbolicCanopy35D: return "Kuramoto hyperbolic synchronization canopy 3.5D";
    case WorldGeneratorKind::HyperbolicHoneycombHorosphere4D: return "hyperbolic {3,5,3} icosahedral honeycomb horosphere 4D";
    case WorldGeneratorKind::Hecatonicosachoron120Cell4D: return "120-cell Hecatonicosachoron Coxeter 4D";
    case WorldGeneratorKind::DiracMonopoleQuantumTunnel: return "Dirac magnetic monopole & quantum flux tunnel";
    case WorldGeneratorKind::Su2YangMillsInstanton: return "SU(2) Yang-Mills BPST instanton & skyrmion soliton lattice";
    case WorldGeneratorKind::NeonBubblesphere: return "neon epimorphic mirror-chamber bubblesphere";
    }
    return "unknown";
}

std::string_view worldPrimitiveName(WorldPrimitiveKind kind) noexcept {
    switch (kind) {
    case WorldPrimitiveKind::Sphere: return "sphere";
    case WorldPrimitiveKind::TorusKnot: return "torus knot";
    case WorldPrimitiveKind::Cylinder: return "cylinder";
    case WorldPrimitiveKind::Tile: return "tile";
    case WorldPrimitiveKind::Cube: return "cube";
    case WorldPrimitiveKind::Pillar: return "pillar";
    case WorldPrimitiveKind::BranchSegment: return "branch segment";
    case WorldPrimitiveKind::TrussStrut: return "truss strut";
    }
    return "unknown";
}

std::string_view mirrorCellTopologyName(MirrorCellTopology topology) noexcept {
    switch (topology) {
    case MirrorCellTopology::Triangular: return "triangular";
    case MirrorCellTopology::Quadrilateral: return "quadrilateral";
    case MirrorCellTopology::Pentagonal: return "pentagonal";
    case MirrorCellTopology::Hexagonal: return "hexagonal";
    }
    return "unknown";
}

WorldMaterialPalette lookupWorldMaterialPalette(WorldGeneratorKind kind) noexcept {
    static constexpr std::array<WorldMaterialPalette, kWorldGeneratorCount> kPalettes{{
        // 0: OrbitSamples
        {{{{0.10f, 0.82f, 1.00f, 1.0f}, 0.72f, 0.18f, {0.03f, 0.22f, 0.35f}},
          {{0.92f, 0.14f, 0.72f, 1.0f}, 0.35f, 0.24f, {0.24f, 0.02f, 0.18f}},
          {{1.00f, 0.63f, 0.13f, 1.0f}, 0.92f, 0.20f, {0.18f, 0.055f, 0.008f}}}},
        // 1: KuramotoLattice — blue-cyan sync core (Form), violet-amethyst
        // coupling lattice (Structure), and gold phase-slip crests (Accent).
        {makeWorldMaterialPalette(
            {{0.08f, 0.60f, 0.95f, 1.0f}, 0.58f, 0.22f, {0.02f, 0.12f, 0.30f}},
            {{0.46f, 0.16f, 0.96f, 1.0f}, 0.42f, 0.20f, {0.10f, 0.015f, 0.34f}},
            {{0.98f, 0.91f, 0.40f, 1.0f}, 0.70f, 0.17f, {0.30f, 0.22f, 0.025f}})},
        // 2: HopfFibers — deep cobalt core bundle (Form), coral-orange torus
        // boundary weave (Structure), and amber SO(4) rotor crests (Accent).
        {makeWorldMaterialPalette(
            {{0.06f, 0.22f, 0.58f, 1.0f}, 0.78f, 0.19f, {0.008f, 0.025f, 0.15f}},
            {{0.88f, 0.30f, 0.12f, 1.0f}, 0.55f, 0.25f, {0.15f, 0.03f, 0.008f}},
            {{0.96f, 0.82f, 0.25f, 1.0f}, 0.62f, 0.22f, {0.20f, 0.14f, 0.015f}})},
        // 3: GoldenPhyllotaxis — burnished gold/bronze spire (Form),
        // emerald cathedral patina (Structure), and radiant lantern crests (Accent).
        {makeWorldMaterialPalette(
            {{0.98f, 0.82f, 0.28f, 1.0f}, 0.92f, 0.14f, {0.25f, 0.16f, 0.02f}},
            {{0.14f, 0.72f, 0.45f, 1.0f}, 0.45f, 0.28f, {0.02f, 0.14f, 0.06f}},
            {{1.00f, 0.94f, 0.65f, 1.0f}, 0.85f, 0.10f, {0.95f, 0.78f, 0.25f}})},
        // 4: StrangeAttractorLanterns — glowing amber lantern core (Form),
        // emerald-cyan crystalline bridge frame (Structure), and radiant coronal
        // flare ignition at chaotic escape apexes (Accent).
        {makeWorldMaterialPalette(
            {{0.98f, 0.52f, 0.12f, 1.0f}, 0.85f, 0.18f, {0.25f, 0.08f, 0.015f}},
            {{0.12f, 0.82f, 0.90f, 1.0f}, 0.40f, 0.22f, {0.02f, 0.20f, 0.28f}},
            {{1.00f, 0.95f, 0.45f, 1.0f}, 0.92f, 0.10f, {1.20f, 0.85f, 0.15f}})},
        // 5: Cell120Hologram — quantum superfluid core (Form),
        // hyper-shell lattice (Structure), and radiant holographic horizon nodes (Accent).
        {makeWorldMaterialPalette(
            {{0.10f, 0.65f, 0.98f, 1.0f}, 0.88f, 0.12f, {0.04f, 0.22f, 0.45f}},
            {{0.82f, 0.25f, 0.92f, 1.0f}, 0.65f, 0.18f, {0.25f, 0.04f, 0.28f}},
            {{0.98f, 0.92f, 0.42f, 1.0f}, 0.95f, 0.08f, {0.95f, 0.82f, 0.20f}})},
        // 6: AmmannBeenkerField — platinum silver core star (Form),
        // imperial jade aperiodic tiling weave (Structure), and radiant gold Hofstadter energy peaks (Accent).
        {makeWorldMaterialPalette(
            {{0.92f, 0.94f, 0.98f, 1.0f}, 0.95f, 0.12f, {0.05f, 0.08f, 0.12f}},
            {{0.18f, 0.85f, 0.62f, 1.0f}, 0.35f, 0.25f, {0.04f, 0.18f, 0.10f}},
            {{1.00f, 0.85f, 0.30f, 1.0f}, 0.90f, 0.10f, {1.20f, 0.85f, 0.15f}})},
        // 7: ChladniMembrane — matte basalt nodal lines of rest (Form),
        // electric azure acoustic slopes (Structure), and luminescent sonic antinode flares (Accent).
        {makeWorldMaterialPalette(
            {{0.12f, 0.14f, 0.18f, 1.0f}, 0.15f, 0.85f, {0.005f, 0.005f, 0.01f}},
            {{0.15f, 0.65f, 0.98f, 1.0f}, 0.65f, 0.25f, {0.05f, 0.18f, 0.35f}},
            {{1.00f, 0.95f, 0.85f, 1.0f}, 0.95f, 0.08f, {1.35f, 1.10f, 0.65f}})},
        // 8: LichtenbergForest — obsidian/charcoal trunk core (Form),
        // electric azure plasma streamers (Structure), and incandescent
        // gold-white leader breakdown sparks (Accent).
        {makeWorldMaterialPalette(
            {{0.12f, 0.14f, 0.20f, 1.0f}, 0.85f, 0.20f, {0.05f, 0.08f, 0.25f}},
            {{0.18f, 0.75f, 0.98f, 1.0f}, 0.45f, 0.22f, {0.25f, 0.85f, 1.20f}},
            {{1.00f, 0.98f, 0.85f, 1.0f}, 0.95f, 0.08f, {1.50f, 1.35f, 0.85f}})},
        // 9: EpimorphicGyroidMaze — polished platinum-titanium skeleton (Form),
        // opalescent bismuth jade minimal surface membrane (Structure),
        // and radiant chiral amber/gold wave crest flare (Accent).
        {makeWorldMaterialPalette(
            {{0.88f, 0.90f, 0.96f, 1.0f}, 0.92f, 0.12f, {0.08f, 0.10f, 0.16f}},
            {{0.22f, 0.82f, 0.65f, 1.0f}, 0.35f, 0.28f, {0.05f, 0.22f, 0.18f}},
            {{1.00f, 0.78f, 0.25f, 1.0f}, 0.90f, 0.08f, {1.25f, 0.95f, 0.20f}})},
        // 10: HypercubeTesseract — polished hyper-gold frame (Form),
        // electric cyan / cobalt gluon flux tubes (Structure),
        // and super-specular magenta / violet QCD vacuum node flare (Accent).
        {makeWorldMaterialPalette(
            {{0.98f, 0.72f, 0.28f, 1.0f}, 0.95f, 0.12f, {0.20f, 0.12f, 0.04f}},
            {{0.15f, 0.72f, 0.98f, 1.0f}, 0.55f, 0.20f, {0.08f, 0.25f, 0.45f}},
            {{0.95f, 0.22f, 0.88f, 1.0f}, 0.90f, 0.06f, {1.40f, 0.15f, 0.95f}})},
        // 11: ProceduralWoodland
        {{{{0.45f, 0.30f, 0.15f, 1.0f}, 0.15f, 0.85f, {0.01f, 0.005f, 0.0f}},
          {{0.18f, 0.55f, 0.22f, 1.0f}, 0.10f, 0.65f, {0.02f, 0.08f, 0.03f}},
          {{0.82f, 0.75f, 0.35f, 1.0f}, 0.20f, 0.45f, {0.15f, 0.12f, 0.02f}}}},
        // 12: BioluminescentSporeSwarm
        {{{{0.12f, 0.95f, 0.85f, 1.0f}, 0.45f, 0.15f, {0.25f, 0.85f, 0.75f}},
          {{0.85f, 0.25f, 0.95f, 1.0f}, 0.50f, 0.18f, {0.75f, 0.15f, 0.85f}},
          {{0.95f, 0.85f, 0.15f, 1.0f}, 0.60f, 0.12f, {0.85f, 0.75f, 0.10f}}}},
        // 13: NeuralSynapseLattice
        {{{{0.95f, 0.95f, 0.98f, 1.0f}, 0.75f, 0.20f, {0.65f, 0.65f, 0.95f}},
          {{0.25f, 0.65f, 0.95f, 1.0f}, 0.80f, 0.15f, {0.15f, 0.45f, 0.85f}},
          {{0.95f, 0.35f, 0.15f, 1.0f}, 0.85f, 0.12f, {0.85f, 0.25f, 0.05f}}}},
        // 14: KaliSetOrbits
        {{{{0.95f, 0.25f, 0.15f, 1.0f}, 0.85f, 0.18f, {0.65f, 0.10f, 0.05f}},
          {{0.15f, 0.85f, 0.45f, 1.0f}, 0.75f, 0.22f, {0.05f, 0.65f, 0.25f}},
          {{0.95f, 0.75f, 0.15f, 1.0f}, 0.90f, 0.15f, {0.75f, 0.55f, 0.05f}}}},
        // 15: MandelbulbFractal
        {{{{0.85f, 0.45f, 0.15f, 1.0f}, 0.65f, 0.25f, {0.45f, 0.15f, 0.05f}},
          {{0.15f, 0.45f, 0.85f, 1.0f}, 0.75f, 0.20f, {0.05f, 0.25f, 0.65f}},
          {{0.95f, 0.85f, 0.35f, 1.0f}, 0.85f, 0.15f, {0.65f, 0.55f, 0.15f}}}},
        // 16: DeepSeaSiphonophores
        {{{{0.15f, 0.65f, 0.95f, 1.0f}, 0.25f, 0.15f, {0.15f, 0.65f, 0.95f}},
          {{0.95f, 0.35f, 0.75f, 1.0f}, 0.30f, 0.18f, {0.85f, 0.25f, 0.65f}},
          {{0.35f, 0.95f, 0.65f, 1.0f}, 0.20f, 0.12f, {0.25f, 0.85f, 0.55f}}}},
        // 17: CalabiYauManifold
        {{{{0.45f, 0.25f, 0.85f, 1.0f}, 0.85f, 0.15f, {0.35f, 0.15f, 0.75f}},
          {{0.15f, 0.75f, 0.85f, 1.0f}, 0.80f, 0.18f, {0.05f, 0.55f, 0.75f}},
          {{0.95f, 0.65f, 0.25f, 1.0f}, 0.90f, 0.12f, {0.75f, 0.45f, 0.15f}}}},
        // 18: LSystemFractal
        {{{{0.45f, 0.75f, 0.25f, 1.0f}, 0.35f, 0.45f, {0.15f, 0.45f, 0.05f}},
          {{0.75f, 0.45f, 0.15f, 1.0f}, 0.25f, 0.55f, {0.35f, 0.15f, 0.05f}},
          {{0.95f, 0.85f, 0.45f, 1.0f}, 0.45f, 0.35f, {0.55f, 0.45f, 0.15f}}}},
        // 19: HydrogenAtomOrbitals
        {{{{0.95f, 0.35f, 0.15f, 1.0f}, 0.40f, 0.15f, {0.85f, 0.25f, 0.05f}},
          {{0.15f, 0.55f, 0.95f, 1.0f}, 0.50f, 0.18f, {0.05f, 0.35f, 0.85f}},
          {{0.45f, 0.82f, 1.00f, 1.0f}, 0.30f, 0.12f, {0.35f, 0.65f, 0.95f}}}},
        // 20: CliffordHopfSolitonLattice
        {{{{0.08f, 0.25f, 0.85f, 1.0f}, 0.78f, 0.15f, {0.04f, 0.12f, 0.45f}},
          {{0.95f, 0.76f, 0.25f, 1.0f}, 0.85f, 0.18f, {0.45f, 0.32f, 0.06f}},
          {{0.82f, 0.22f, 0.98f, 1.0f}, 0.52f, 0.14f, {0.55f, 0.10f, 0.65f}}}},
        // 21: PenroseTwistorFoliation
        {{{{0.12f, 0.88f, 0.98f, 1.0f}, 0.82f, 0.12f, {0.15f, 0.75f, 0.95f}},
          {{0.98f, 0.82f, 0.30f, 1.0f}, 0.88f, 0.15f, {0.60f, 0.45f, 0.08f}},
          {{0.75f, 0.18f, 0.96f, 1.0f}, 0.58f, 0.16f, {0.45f, 0.08f, 0.70f}}}},
        // 22: LieE8GossetLattice
        {{{{0.96f, 0.72f, 0.18f, 1.0f}, 0.90f, 0.12f, {0.65f, 0.40f, 0.05f}},
          {{0.15f, 0.85f, 0.95f, 1.0f}, 0.65f, 0.16f, {0.10f, 0.60f, 0.85f}},
          {{0.85f, 0.15f, 0.65f, 1.0f}, 0.75f, 0.14f, {0.50f, 0.05f, 0.45f}}}},
        // 23: ConformalRelief25D
        {{{{0.12f, 0.78f, 0.55f, 1.0f}, 0.65f, 0.22f, {0.08f, 0.45f, 0.25f}},
          {{0.98f, 0.75f, 0.22f, 1.0f}, 0.88f, 0.14f, {0.55f, 0.35f, 0.05f}},
          {{0.18f, 0.88f, 0.96f, 1.0f}, 0.78f, 0.15f, {0.12f, 0.65f, 0.85f}}}},
        // 24: ToroidalFoliation35D
        {{{{0.78f, 0.18f, 0.95f, 1.0f}, 0.72f, 0.14f, {0.55f, 0.10f, 0.80f}},
          {{0.98f, 0.65f, 0.15f, 1.0f}, 0.90f, 0.12f, {0.70f, 0.38f, 0.05f}},
          {{0.15f, 0.92f, 0.88f, 1.0f}, 0.82f, 0.12f, {0.10f, 0.75f, 0.70f}}}},
        // 25: DedekindEtaQuarticLattice4D
        {{{{0.06f, 0.22f, 0.72f, 1.0f}, 0.75f, 0.16f, {0.05f, 0.15f, 0.55f}},
          {{0.98f, 0.82f, 0.24f, 1.0f}, 0.88f, 0.12f, {0.60f, 0.42f, 0.08f}},
          {{0.72f, 0.16f, 0.92f, 1.0f}, 0.65f, 0.18f, {0.45f, 0.08f, 0.65f}}}},
        // 26: KuramotoHyperbolicCanopy35D
        {{{{0.10f, 0.82f, 0.45f, 1.0f}, 0.60f, 0.20f, {0.08f, 0.55f, 0.25f}},
          {{0.98f, 0.68f, 0.12f, 1.0f}, 0.85f, 0.14f, {0.65f, 0.35f, 0.04f}},
          {{0.14f, 0.88f, 0.92f, 1.0f}, 0.75f, 0.15f, {0.10f, 0.65f, 0.75f}}}},
        // 27: HyperbolicHoneycombHorosphere4D
        {{{{0.12f, 0.75f, 0.95f, 1.0f}, 0.85f, 0.12f, {0.10f, 0.55f, 0.85f}},
          {{0.98f, 0.72f, 0.18f, 1.0f}, 0.92f, 0.14f, {0.65f, 0.45f, 0.06f}},
          {{0.78f, 0.15f, 0.92f, 1.0f}, 0.65f, 0.16f, {0.50f, 0.08f, 0.75f}}}},
        // 28: Hecatonicosachoron120Cell4D
        {{{{0.92f, 0.94f, 0.98f, 1.0f}, 0.92f, 0.10f, {0.25f, 0.35f, 0.60f}},
          {{0.98f, 0.55f, 0.45f, 1.0f}, 0.88f, 0.14f, {0.65f, 0.25f, 0.18f}},
          {{0.15f, 0.92f, 0.65f, 1.0f}, 0.78f, 0.15f, {0.12f, 0.70f, 0.45f}}}},
        // 29: DiracMonopoleQuantumTunnel
        {{{{0.95f, 0.65f, 0.35f, 1.0f}, 0.90f, 0.15f, {0.05f, 0.02f, 0.01f}},
          {{0.10f, 0.85f, 0.95f, 1.0f}, 0.40f, 0.20f, {0.40f, 0.90f, 1.0f}},
          {{0.95f, 0.15f, 0.70f, 1.0f}, 0.10f, 0.05f, {1.50f, 0.20f, 0.90f}}}},
        // 30: Su2YangMillsInstanton
        {{{{0.28f, 0.15f, 0.88f, 1.0f}, 0.75f, 0.25f, {0.18f, 0.10f, 0.75f}},
          {{0.12f, 0.95f, 0.55f, 1.0f}, 0.35f, 0.18f, {0.20f, 0.85f, 0.45f}},
          {{0.98f, 0.82f, 0.15f, 1.0f}, 0.15f, 0.08f, {1.20f, 0.90f, 0.20f}}}},
        // 31: NeonBubblesphere — cyan core, green-gold truss shell, and
        // magenta-blue recursive energy field. Three material bands retain
        // the existing renderer palette contract.
        {makeWorldMaterialPalette(
            {{0.10f, 0.78f, 0.98f, 1.0f}, 0.70f, 0.18f, {0.04f, 0.36f, 0.65f}},
            {{0.26f, 0.96f, 0.48f, 1.0f}, 0.52f, 0.22f, {0.08f, 0.62f, 0.18f}},
            {{0.92f, 0.18f, 0.92f, 1.0f}, 0.38f, 0.20f, {0.58f, 0.06f, 0.62f}})},
    }};

    const std::size_t idx = static_cast<std::size_t>(kind);
    return idx < kPalettes.size() ? kPalettes[idx] : kPalettes[0];
}

WorldPresentationPreset lookupWorldPresentationPreset(
    WorldGeneratorKind kind,
    MirrorCellTopology topology) noexcept {
    WorldPresentationPreset preset{};
    preset.materials = lookupWorldMaterialPalette(kind);

    // Kuramoto phase synchronization lattice: high contrast between
    // synchronized core form and outer coupling structure; subtle atmospheric haze.
    if (kind == WorldGeneratorKind::KuramotoLattice) {
        preset.post = {false, 0.75f, 0.85f, 0.80f, 0.88f, {0.10f, 0.35f, 0.55f}};
        return preset;
    }

    // Clifford-Hopf fiber constellation: deep cobalt core and luminescent
    // SO(4) rotor crests benefit from clear atmospheric depth and mild vignette.
    if (kind == WorldGeneratorKind::HopfFibers) {
        preset.post = {false, 0.68f, 0.78f, 0.72f, 0.85f, {0.08f, 0.22f, 0.48f}};
        return preset;
    }

    // Golden phyllotaxis cathedral: warm incense amber in-scatter, balanced
    // fog distance density, and soft vignette framing the vertical spire.
    if (kind == WorldGeneratorKind::GoldenPhyllotaxis) {
        preset.post = {false, 0.72f, 0.80f, 0.76f, 0.85f, {0.35f, 0.28f, 0.16f}};
        return preset;
    }

    // Rössler/Lorenz lantern garden: deep dark cosmos with high localized bloom,
    // clear atmospheric depth, and subtle framing vignette around the chaotic swarm.
    if (kind == WorldGeneratorKind::StrangeAttractorLanterns) {
        preset.post = {false, 0.65f, 0.75f, 0.70f, 0.85f, {0.04f, 0.02f, 0.08f}};
        return preset;
    }

    // 120-cell quantum hologram: deep indigo cosmic atmosphere with soft optical
    // in-scatter and balanced exposure for multi-shell quantum interference rings.
    if (kind == WorldGeneratorKind::Cell120Hologram) {
        preset.post = {false, 0.70f, 0.82f, 0.75f, 0.88f, {0.05f, 0.04f, 0.14f}};
        return preset;
    }

    // Ammann-Beenker quasicrystal field: clean crystalline air with subtle cool
    // specular rim illumination and balanced depth framing for aperiodic tilings.
    if (kind == WorldGeneratorKind::AmmannBeenkerField) {
        preset.post = {false, 0.60f, 0.70f, 0.65f, 0.82f, {0.10f, 0.18f, 0.28f}};
        return preset;
    }

    // Chladni quantum membrane: dark acoustic chamber with electric azure bloom
    // and concentrated antinode luminescence.
    if (kind == WorldGeneratorKind::ChladniMembrane) {
        preset.post = {false, 0.80f, 0.90f, 0.85f, 0.92f, {0.04f, 0.08f, 0.16f}};
        return preset;
    }

    // Lichtenberg discharge forest: dark void atmosphere with sharp dielectric
    // breakdown glow and tight high-voltage plasma flare in-scatter.
    if (kind == WorldGeneratorKind::LichtenbergForest) {
        preset.post = {false, 0.55f, 0.65f, 0.60f, 0.85f, {0.05f, 0.12f, 0.24f}};
        return preset;
    }

    // Epimorphic gyroid maze: minimal surface labyrinth with soft opalescent
    // subsurface depth and balanced atmospheric in-scatter.
    if (kind == WorldGeneratorKind::EpimorphicGyroidMaze) {
        preset.post = {false, 0.65f, 0.75f, 0.70f, 0.88f, {0.12f, 0.28f, 0.32f}};
        return preset;
    }

    // Hypercube tesseract & 4D gluon flux mandala: deep cosmic void with sharp
    // specular rim, electric violet in-scatter, and framing vignette.
    if (kind == WorldGeneratorKind::HypercubeTesseract) {
        preset.post = {false, 0.58f, 0.68f, 0.62f, 0.82f, {0.18f, 0.08f, 0.35f}};
        return preset;
    }

    // Mirror topology is a Bubblesphere-specific OpenCL geometry grammar.
    // Other worlds keep their canonical palette and the application's post
    // baseline rather than accidentally inheriting a triangle treatment.
    if (kind != WorldGeneratorKind::NeonBubblesphere) return preset;

    preset.post.topologyAware = true;
    float emissionScale = 1.0f;
    switch (topology) {
    case MirrorCellTopology::Triangular:
        // Sparse sharp cells benefit from brighter edges and clear air.
        emissionScale = 1.32f;
        preset.post = {true, 0.62f, 0.72f, 0.68f, 0.80f, {0.16f, 0.48f, 0.56f}};
        break;
    case MirrorCellTopology::Quadrilateral:
        emissionScale = 1.12f;
        preset.post = {true, 0.84f, 0.88f, 0.86f, 0.92f, {0.16f, 0.52f, 0.47f}};
        break;
    case MirrorCellTopology::Pentagonal:
        emissionScale = 0.95f;
        preset.post = {true, 1.04f, 1.06f, 1.02f, 1.06f, {0.20f, 0.48f, 0.38f}};
        break;
    case MirrorCellTopology::Hexagonal:
        // Dense cells need lower emission and more depth separation so their
        // recursive lattice reads as volume rather than a flat neon wall.
        emissionScale = 0.80f;
        preset.post = {true, 1.24f, 1.30f, 1.18f, 1.23f, {0.18f, 0.42f, 0.30f}};
        break;
    }

    for (auto& band : preset.materials) {
        for (float& channel : band.emission) channel *= emissionScale;
    }
    return preset;
}

namespace {

using namespace world_capability;

// Authoritative 32-generator mathematical affinity registry (indexed by WorldGeneratorKind).
// Every procedural world generator maps deterministically to its 6-stage epimorphic
// operators (InnerMap, ReductionMode, SeedType, ColorEffectType), non-Euclidean relief
// metric projection, 4D Coxeter polychoron companion, SO(4) rotor defaults, and
// Living Textbook formula summary.
constexpr std::array<WorldMathematicalAffinity, kWorldGeneratorCount> kWorldMathematicalAffinities{{
    // 0: OrbitSamples (Schwarzschild / compact feedback orbits)
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::Sinh,
        .secondaryInner = InnerMap::Mobius,
        .primaryReduction = ReductionMode::Torus,
        .primarySeed = SeedType::Border,
        .primaryColorEffect = ColorEffectType::StandardHSV,
        .domain = WorldTopologicalDomain::ImplicitRelativistic,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kImplicitFieldRaymarch,
        .preferredReliefProjection = 3U, // RiemannSphere
        .preferredPolychoron = 5U,       // CliffordTorus
        .defaultEnable4D = 1U,
        .defaultOutputMode = 0U,
        .defaultInstanceMeshType = 0U,
        .canonicalEquation = "ds^2 = -(1 - r_s/r)c^2 dt^2 + (1 - r_s/r)^{-1} dr^2 + r^2 dOmega^2",
    },
    // 1: KuramotoLattice
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::KuramotoLattice,
        .secondaryInner = InnerMap::Sin,
        .primaryReduction = ReductionMode::Torus,
        .primarySeed = SeedType::KuramotoSynchronization,
        .primaryColorEffect = ColorEffectType::QuantumInterference,
        .domain = WorldTopologicalDomain::Euclidean3D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kHybridReliefCompanion,
        .preferredReliefProjection = 4U, // ToroidalChamber
        .preferredPolychoron = 5U,
        .defaultEnable4D = 0U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 1U,   // TorusKnot
        .canonicalEquation = "dtheta_i/dt = omega_i + (K/N) sum_j sin(theta_j - theta_i)",
    },
    // 2: HopfFibers
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::CliffordHopf,
        .secondaryInner = InnerMap::QuaternionJulia,
        .primaryReduction = ReductionMode::CliffordFoliation,
        .primarySeed = SeedType::Hopf,
        .primaryColorEffect = ColorEffectType::HopfFibration,
        .domain = WorldTopologicalDomain::HopfTwistorFibration4D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy | kHybridReliefCompanion,
        .preferredReliefProjection = 3U, // RiemannSphere
        .preferredPolychoron = 5U,       // CliffordTorus
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 1U,   // TorusKnot
        .defaultRotorAlpha = 0.50f,
        .defaultRotorBeta = 0.50f,
        .canonicalEquation = "pi_Hopf: S^3 -> S^2,  (z_1, z_2) |-> (2 z_1 conj(z_2), |z_1|^2 - |z_2|^2)",
    },
    // 3: GoldenPhyllotaxis
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::Exp,
        .secondaryInner = InnerMap::Log,
        .primaryReduction = ReductionMode::PareidoliaCathedral,
        .primarySeed = SeedType::FibonacciPhyllotaxis,
        .primaryColorEffect = ColorEffectType::GoldenRatio,
        .domain = WorldTopologicalDomain::Euclidean3D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kHybridReliefCompanion,
        .preferredReliefProjection = 0U, // Planar
        .preferredPolychoron = 4U,       // Hecatonicosachoron120
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 3U,   // Cylinder
        .canonicalEquation = "theta_n = 2*pi*n / phi^2,  r_n = c * sqrt(n),  phi = (1 + sqrt(5))/2",
    },
    // 4: StrangeAttractorLanterns
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::RosslerAttractor,
        .secondaryInner = InnerMap::LorenzAttractor,
        .primaryReduction = ReductionMode::Clamp,
        .primarySeed = SeedType::Attractor,
        .primaryColorEffect = ColorEffectType::KineticVelocity,
        .domain = WorldTopologicalDomain::Euclidean3D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling,
        .preferredReliefProjection = 0U,
        .preferredPolychoron = 2U,
        .defaultEnable4D = 0U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 0U,
        .defaultAttractorMode = 2U,
        .defaultAttractorStrength = 1.0f,
        .defaultLatentW = 0.5f,
        .canonicalEquation = "dx/dt = -y - z,  dy/dt = x + a*y,  dz/dt = b + z*(x - c)",
    },
    // 5: Cell120Hologram
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::HyperbolicHoneycomb,
        .secondaryInner = InnerMap::QuaternionMandelbrot,
        .primaryReduction = ReductionMode::PoincareHyperbolic,
        .primarySeed = SeedType::HyperSphere4D,
        .primaryColorEffect = ColorEffectType::ThermalHologram,
        .domain = WorldTopologicalDomain::CoxeterPolytope4D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy,
        .preferredReliefProjection = 1U, // PoincareDisk
        .preferredPolychoron = 4U,       // Hecatonicosachoron120
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 2U,   // Cube / polytope cell
        .defaultRotorAlpha = 0.50f,
        .defaultRotorBeta = 0.50f,
        .canonicalEquation = "H_4 Coxeter {5,3,3}: |V| = 600, |E| = 1200, |F| = 720, |C| = 120",
    },
    // 6: AmmannBeenkerField
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::PenroseTiling4D,
        .secondaryInner = InnerMap::Kaleidoscope,
        .primaryReduction = ReductionMode::AmmannBeenker,
        .primarySeed = SeedType::CrossPolytope4D,
        .primaryColorEffect = ColorEffectType::OpArtIsoluminance,
        .domain = WorldTopologicalDomain::QuasicrystalAperiodic,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kHybridReliefCompanion,
        .preferredReliefProjection = 0U,
        .preferredPolychoron = 0U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 2U,
        .canonicalEquation = "Lambda_AB = pi_par(Z^4),  D_8 octagonal cut-and-project quasicrystal",
    },
    // 7: ChladniMembrane
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::ReactionDiffusionTuring,
        .secondaryInner = InnerMap::Cos,
        .primaryReduction = ReductionMode::Grid,
        .primarySeed = SeedType::ChladniCymatics,
        .primaryColorEffect = ColorEffectType::ChladniAcoustic,
        .domain = WorldTopologicalDomain::ConformalRelief25D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kHybridReliefCompanion,
        .preferredReliefProjection = 0U,
        .preferredPolychoron = 1U,
        .defaultEnable4D = 0U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 3U,
        .canonicalEquation = "u(x,z) = A cos(n*pi*x/L) cos(m*pi*z/L) - B cos(m*pi*x/L) cos(n*pi*z/L)",
    },
    // 8: LichtenbergForest
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::HyperbolicExponentialHomotopy,
        .secondaryInner = InnerMap::Invert,
        .primaryReduction = ReductionMode::Clamp,
        .primarySeed = SeedType::Dipole,
        .primaryColorEffect = ColorEffectType::ChromaticDispersion,
        .domain = WorldTopologicalDomain::Euclidean3D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling,
        .preferredReliefProjection = 0U,
        .preferredPolychoron = 1U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 3U,
        .canonicalEquation = "nabla^2 phi = 0,  P(growth) propto |E|^eta  (Dielectric Breakdown Model)",
    },
    // 9: EpimorphicGyroidMaze
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::Gyroid,
        .secondaryInner = InnerMap::Tanh,
        .primaryReduction = ReductionMode::Torus,
        .primarySeed = SeedType::TuringPerturbation,
        .primaryColorEffect = ColorEffectType::ThinFilmIridescence,
        .domain = WorldTopologicalDomain::FoliatedManifold35D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kHybridReliefCompanion,
        .preferredReliefProjection = 4U,
        .preferredPolychoron = 2U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 2U,
        .canonicalEquation = "sin(x)cos(y) + sin(y)cos(z) + sin(z)cos(x) = 0  (Schoen G Minimal Surface)",
    },
    // 10: HypercubeTesseract
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::Cube,
        .secondaryInner = InnerMap::HyperbolicMandelboxFold,
        .primaryReduction = ReductionMode::Grid,
        .primarySeed = SeedType::CrossPolytope4D,
        .primaryColorEffect = ColorEffectType::QuaternionCurvature,
        .domain = WorldTopologicalDomain::CoxeterPolytope4D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy,
        .preferredReliefProjection = 2U,
        .preferredPolychoron = 0U,       // Tesseract8Cell
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 2U,
        .defaultRotorAlpha = 0.50f,
        .defaultRotorBeta = 0.50f,
        .canonicalEquation = "B_4 Coxeter {4,3,3}: (-1,+1)^4 subset R^4,  pi(v) = v_xyz / (R_pole - v_w)",
    },
    // 11: ProceduralWoodland
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::Sinh,
        .secondaryInner = InnerMap::Cos,
        .primaryReduction = ReductionMode::Grid,
        .primarySeed = SeedType::FibonacciPhyllotaxis,
        .primaryColorEffect = ColorEffectType::PhenomenologicalPalette,
        .domain = WorldTopologicalDomain::Euclidean3D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling,
        .preferredReliefProjection = 0U,
        .preferredPolychoron = 0U,
        .defaultEnable4D = 0U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 0U,
        .canonicalEquation = "F_canopy(x,z) = sum_k a_k fBM(2^k x) + L-system phyllotaxis branching",
    },
    // 12: BioluminescentSporeSwarm
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::AizawaAttractor,
        .secondaryInner = InnerMap::Lissajous,
        .primaryReduction = ReductionMode::Torus,
        .primarySeed = SeedType::NavierStokesVortex,
        .primaryColorEffect = ColorEffectType::Bioluminescence,
        .domain = WorldTopologicalDomain::Euclidean3D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling,
        .preferredReliefProjection = 3U,
        .preferredPolychoron = 5U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 0U,
        .canonicalEquation = "dx/dt = u_curl(x,t) + D nabla^2 rho_spore,  I_bio = I_0 sin^2(omega t + phi)",
    },
    // 13: NeuralSynapseLattice
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::KuramotoLattice,
        .secondaryInner = InnerMap::Tanh,
        .primaryReduction = ReductionMode::Grid,
        .primarySeed = SeedType::KuramotoSynchronization,
        .primaryColorEffect = ColorEffectType::KineticVelocity,
        .domain = WorldTopologicalDomain::Euclidean3D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling,
        .preferredReliefProjection = 3U,
        .preferredPolychoron = 2U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 0U,
        .canonicalEquation = "tau dV_i/dt = -V_i + sum_j W_ij tanh(g V_j + I_ext)",
    },
    // 14: KaliSetOrbits
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::KaliSetOrbit,
        .secondaryInner = InnerMap::Mandelbox,
        .primaryReduction = ReductionMode::Clamp,
        .primarySeed = SeedType::JuliaQuaternionicSlice,
        .primaryColorEffect = ColorEffectType::DistanceHSV,
        .domain = WorldTopologicalDomain::CoxeterPolytope4D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy,
        .preferredReliefProjection = 1U,
        .preferredPolychoron = 2U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 2U,
        .canonicalEquation = "p_{n+1} = |p_n| / (p_n . p_n) - c_Kali",
    },
    // 15: MandelbulbFractal
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::QuaternionMandelbrot,
        .secondaryInner = InnerMap::HyperbolicMandelboxFold,
        .primaryReduction = ReductionMode::Clamp,
        .primarySeed = SeedType::JuliaQuaternionicSlice,
        .primaryColorEffect = ColorEffectType::DistanceHSV,
        .domain = WorldTopologicalDomain::Euclidean3D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling,
        .preferredReliefProjection = 3U,
        .preferredPolychoron = 3U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 0U,
        .canonicalEquation = "v_{n+1} = r^8 <sin(8 theta)cos(8 phi), sin(8 theta)sin(8 phi), cos(8 theta)> + c",
    },
    // 16: DeepSeaSiphonophores
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::SiphonophoreAttractor,
        .secondaryInner = InnerMap::Sinh,
        .primaryReduction = ReductionMode::Torus,
        .primarySeed = SeedType::Lissajous,
        .primaryColorEffect = ColorEffectType::Bioluminescence,
        .domain = WorldTopologicalDomain::Euclidean3D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling,
        .preferredReliefProjection = 5U,
        .preferredPolychoron = 5U,
        .defaultEnable4D = 0U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 3U,
        .canonicalEquation = "gamma(s,t) = R_helix(s) + A_wave sin(k*s - omega*t) N_Frenet(s)",
    },
    // 17: CalabiYauManifold
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::CalabiYauQuintic,
        .secondaryInner = InnerMap::HolomorphicPotential,
        .primaryReduction = ReductionMode::CliffordFoliation,
        .primarySeed = SeedType::CalabiYauManifold,
        .primaryColorEffect = ColorEffectType::ThinFilmIridescence,
        .domain = WorldTopologicalDomain::FoliatedManifold35D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy | kHybridReliefCompanion,
        .preferredReliefProjection = 3U,
        .preferredPolychoron = 2U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 1U,
        .defaultRotorAlpha = 0.50f,
        .defaultRotorBeta = 0.50f,
        .canonicalEquation = "z_1^n + z_2^n + z_3^n + z_4^n + z_5^n = 0  subset CP^4  (Ricci-Flat Kahler)",
    },
    // 18: LSystemFractal
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::Exp,
        .secondaryInner = InnerMap::ChebyshevPolynomial,
        .primaryReduction = ReductionMode::Clamp,
        .primarySeed = SeedType::FibonacciPhyllotaxis,
        .primaryColorEffect = ColorEffectType::GoldenRatio,
        .domain = WorldTopologicalDomain::Euclidean3D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling,
        .preferredReliefProjection = 0U,
        .preferredPolychoron = 0U,
        .defaultEnable4D = 0U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 3U,
        .canonicalEquation = "omega_{n+1} = P_tropism(R_yaw(alpha) R_pitch(beta) omega_n),  r_{k+1} = lambda r_k",
    },
    // 19: HydrogenAtomOrbitals
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::QuantumWave,
        .secondaryInner = InnerMap::ChebyshevPolynomial,
        .primaryReduction = ReductionMode::Clamp,
        .primarySeed = SeedType::QuantumProbabilityCloud,
        .primaryColorEffect = ColorEffectType::QuantumInterference,
        .domain = WorldTopologicalDomain::Euclidean3D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling,
        .preferredReliefProjection = 3U,
        .preferredPolychoron = 3U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 0U,
        .canonicalEquation = "psi_{nlm}(r,theta,phi) = R_{nl}(r) Y_l^m(theta,phi),  P = |psi_{nlm}|^2",
    },
    // 20: CliffordHopfSolitonLattice
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::CliffordHopf,
        .secondaryInner = InnerMap::OctonionFeedback,
        .primaryReduction = ReductionMode::CliffordFoliation,
        .primarySeed = SeedType::Hopf,
        .primaryColorEffect = ColorEffectType::HopfFibration,
        .domain = WorldTopologicalDomain::HopfTwistorFibration4D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy | kHybridReliefCompanion,
        .preferredReliefProjection = 4U,
        .preferredPolychoron = 5U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 1U,
        .defaultRotorAlpha = 0.50f,
        .defaultRotorBeta = 0.50f,
        .canonicalEquation = "R_SO(4)(alpha,beta) = L_q(alpha) R_p(beta),  Q_Hopf = (1/4pi^2) int A ^ dA",
    },
    // 21: PenroseTwistorFoliation
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::CliffordHopf,
        .secondaryInner = InnerMap::Mobius,
        .primaryReduction = ReductionMode::CliffordFoliation,
        .primarySeed = SeedType::TorusKnotPQ,
        .primaryColorEffect = ColorEffectType::ChromaticDispersion,
        .domain = WorldTopologicalDomain::HopfTwistorFibration4D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy | kHybridReliefCompanion,
        .preferredReliefProjection = 3U,
        .preferredPolychoron = 5U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 1U,
        .defaultRotorAlpha = 0.50f,
        .defaultRotorBeta = 0.50f,
        .canonicalEquation = "Z^alpha = (omega^A, pi_{A'}),  omega^A = i x^{AA'} pi_{A'}  (Robinson Congruence)",
    },
    // 22: LieE8GossetLattice
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::OctonionCayley,
        .secondaryInner = InnerMap::OctonionAssociatorManifold,
        .primaryReduction = ReductionMode::AmmannBeenker,
        .primarySeed = SeedType::OctonionFibrationS7,
        .primaryColorEffect = ColorEffectType::QuaternionCurvature,
        .domain = WorldTopologicalDomain::CoxeterPolytope4D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy,
        .preferredReliefProjection = 1U,
        .preferredPolychoron = 3U,       // Hexacosichoron600
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 0U,
        .defaultRotorAlpha = 0.50f,
        .defaultRotorBeta = 0.50f,
        .canonicalEquation = "E_8 Gosset 4_21: 240 roots in R^8 -> H_4 Coxeter plane projection",
    },
    // 23: ConformalRelief25D
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::PoincareHyperbolic,
        .secondaryInner = InnerMap::HolomorphicPotential,
        .primaryReduction = ReductionMode::PoincareHyperbolic,
        .primarySeed = SeedType::HyperbolicTessellation,
        .primaryColorEffect = ColorEffectType::PhenomenologicalPalette,
        .domain = WorldTopologicalDomain::ConformalRelief25D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kHybridReliefCompanion,
        .preferredReliefProjection = 1U, // PoincareDisk
        .hasDedicatedReliefProjection = true,
        .autoPromoteHybridVolumetric = true,
        .preferredPolychoron = 4U,
        .defaultEnable4D = 0U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 2U,
        .canonicalEquation = "ds^2 = 4(dx^2 + dz^2)/(1 - r^2)^2,  K = -1  (Poincare Conformal Disk)",
    },
    // 24: ToroidalFoliation35D
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::Clifford,
        .secondaryInner = InnerMap::Kaleidoscope,
        .primaryReduction = ReductionMode::CliffordFoliation,
        .primarySeed = SeedType::TorusKnotPQ,
        .primaryColorEffect = ColorEffectType::HopfFibration,
        .domain = WorldTopologicalDomain::FoliatedManifold35D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy | kHybridReliefCompanion,
        .preferredReliefProjection = 4U, // ToroidalChamber
        .hasDedicatedReliefProjection = true,
        .autoPromoteHybridVolumetric = true,
        .preferredPolychoron = 5U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 1U,
        .defaultRotorAlpha = 0.50f,
        .defaultRotorBeta = 0.50f,
        .canonicalEquation = "F_r(u,v) = ((R + r cos u)cos v, (R + r cos u)sin v, r sin u)  (Reeb Foliation)",
    },
    // 25: DedekindEtaQuarticLattice4D
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::JacobiEllipticSnCn,
        .secondaryInner = InnerMap::CalabiYauQuintic,
        .primaryReduction = ReductionMode::PoincareHyperbolic,
        .primarySeed = SeedType::CalabiYauManifold,
        .primaryColorEffect = ColorEffectType::ThinFilmIridescence,
        .domain = WorldTopologicalDomain::CoxeterPolytope4D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy,
        .preferredReliefProjection = 1U,
        .preferredPolychoron = 2U,       // Icositetrachoron24
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 0U,
        .defaultRotorAlpha = 0.50f,
        .defaultRotorBeta = 0.50f,
        .canonicalEquation = "eta(tau) = q^{1/24} prod_{n>=1}(1 - q^n),  z_0^4 + z_1^4 + z_2^4 + z_3^4 = 0",
    },
    // 26: KuramotoHyperbolicCanopy35D
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::KuramotoLattice,
        .secondaryInner = InnerMap::KleinProjective,
        .primaryReduction = ReductionMode::PoincareHyperbolic,
        .primarySeed = SeedType::KuramotoSynchronization,
        .primaryColorEffect = ColorEffectType::QuantumInterference,
        .domain = WorldTopologicalDomain::FoliatedManifold35D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kHybridReliefCompanion,
        .preferredReliefProjection = 2U, // KleinBeltrami
        .hasDedicatedReliefProjection = true,
        .autoPromoteHybridVolumetric = true,
        .preferredPolychoron = 4U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 3U,
        .canonicalEquation = "ds^2_Klein = dr^2/(1-r^2)^2 + r^2 dtheta^2/(1-r^2),  r*e^{i Psi} = <e^{i theta_j}>",
    },
    // 27: HyperbolicHoneycombHorosphere4D
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::HyperbolicHoneycomb,
        .secondaryInner = InnerMap::PoincareHyperbolic,
        .primaryReduction = ReductionMode::PoincareHyperbolic,
        .primarySeed = SeedType::HyperbolicTessellation,
        .primaryColorEffect = ColorEffectType::QuaternionCurvature,
        .domain = WorldTopologicalDomain::CoxeterPolytope4D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy | kHybridReliefCompanion,
        .preferredReliefProjection = 5U, // HyperbolicPseudosphere
        .hasDedicatedReliefProjection = true,
        .autoPromoteHybridVolumetric = false,
        .preferredPolychoron = 3U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 1U,
        .defaultRotorAlpha = 0.50f,
        .defaultRotorBeta = 0.50f,
        .canonicalEquation = "H^3 {3,5,3} Honeycomb: -X_0^2 + X_1^2 + X_2^2 + X_3^2 = -1,  ds^2 = du^2 + e^{2u} dv^2",
    },
    // 28: Hecatonicosachoron120Cell4D
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::HyperbolicHoneycomb,
        .secondaryInner = InnerMap::OctonionCayley,
        .primaryReduction = ReductionMode::PoincareHyperbolic,
        .primarySeed = SeedType::HyperSphere4D,
        .primaryColorEffect = ColorEffectType::QuaternionCurvature,
        .domain = WorldTopologicalDomain::CoxeterPolytope4D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy,
        .preferredReliefProjection = 1U,
        .preferredPolychoron = 4U,       // Hecatonicosachoron120
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 0U,
        .defaultRotorAlpha = 0.50f,
        .defaultRotorBeta = 0.50f,
        .canonicalEquation = "{5,3,3} Hecatonicosachoron in S^3: 120 dodecahedral cells, H_4 golden-ratio rings",
    },
    // 29: DiracMonopoleQuantumTunnel
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::Exp,
        .secondaryInner = InnerMap::CliffordHopf,
        .primaryReduction = ReductionMode::CliffordFoliation,
        .primarySeed = SeedType::Hopf,
        .primaryColorEffect = ColorEffectType::QuantumInterference,
        .domain = WorldTopologicalDomain::GaugeFieldSoliton4D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy,
        .preferredReliefProjection = 3U,
        .preferredPolychoron = 5U,
        .defaultEnable4D = 15U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 0U,
        .defaultRotorAlpha = 0.785398f,
        .defaultRotorBeta = 0.436332f,
        .defaultRotorTheta4D = 0.618034f,
        .defaultLatentW = 0.65f,
        .canonicalEquation = "A_pm = g(1 +- cos theta) dphi,  B = g r_hat / r^2,  2*e*g = n*hbar*c",
    },
    // 30: Su2YangMillsInstanton
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::OctonionAssociatorManifold,
        .secondaryInner = InnerMap::CliffordHopf,
        .primaryReduction = ReductionMode::CliffordFoliation,
        .primarySeed = SeedType::OctonionFibrationS7,
        .primaryColorEffect = ColorEffectType::ChromaticDispersion,
        .domain = WorldTopologicalDomain::GaugeFieldSoliton4D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy,
        .preferredReliefProjection = 3U,
        .preferredPolychoron = 2U,
        .defaultEnable4D = 15U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 1U,
        .defaultRotorAlpha = 0.654498f,
        .defaultRotorBeta = 0.523599f,
        .defaultRotorTheta4D = 0.450000f,
        .defaultLatentW = 0.72f,
        .defaultTopologyBlend = 0.78f,
        .canonicalEquation = "A_mu^a = 2 eta^a_{mu nu} x_nu / (x^2 + rho^2),  F_{mu nu} = *F_{mu nu}  (BPST Q=1)",
    },
    // 31: NeonBubblesphere
    WorldMathematicalAffinity{
        .primaryInner = InnerMap::Kaleidoscope,
        .secondaryInner = InnerMap::Tanh,
        .primaryReduction = ReductionMode::PoincareHyperbolic,
        .primarySeed = SeedType::HyperSphere4D,
        .primaryColorEffect = ColorEffectType::ThinFilmIridescence,
        .domain = WorldTopologicalDomain::CoxeterPolytope4D,
        .capabilityFlags = kOpenClDedicatedKernel | kGpuVolumetricCoupling | kGpu4DHomotopy,
        .preferredReliefProjection = 3U,
        .preferredPolychoron = 2U,
        .defaultEnable4D = 1U,
        .defaultOutputMode = 1U,
        .defaultInstanceMeshType = 1U,
        .defaultRotorAlpha = 0.50f,
        .defaultRotorBeta = 0.50f,
        .canonicalEquation = "S^3 Mirror-Cell Bubblesphere: Faceted Core + Geodesic Truss + Orbital Swarm",
    },
}};

} // namespace

const WorldMathematicalAffinity& lookupWorldMathematicalAffinity(
    WorldGeneratorKind kind) noexcept {
    const std::size_t idx = static_cast<std::size_t>(kind);
    return idx < kWorldMathematicalAffinities.size()
        ? kWorldMathematicalAffinities[idx]
        : kWorldMathematicalAffinities[0];
}

void applyWorldMathematicalAffinity(
    WorldGeneratorKind kind,
    SculptureProgramParams& params) noexcept {
    const WorldMathematicalAffinity& aff = lookupWorldMathematicalAffinity(kind);
    params.inner = aff.primaryInner;
    params.innerB = aff.secondaryInner;
    params.reduction = aff.primaryReduction;
    params.seedType = aff.primarySeed;
    params.colorEffect = aff.primaryColorEffect;
    params.enable4D = aff.defaultEnable4D;
    params.outputMode = aff.defaultOutputMode;
    params.instanceMeshType = aff.defaultInstanceMeshType;
    params.attractorMode = aff.defaultAttractorMode;
    params.attractorStrength = aff.defaultAttractorStrength;
    params.rotorAlpha = aff.defaultRotorAlpha;
    params.rotorBeta = aff.defaultRotorBeta;
    params.rotorTheta4D = aff.defaultRotorTheta4D;
    params.latentW = aff.defaultLatentW;
    params.topologyBlend = aff.defaultTopologyBlend;
    params.patternSpeed = aff.defaultPatternSpeed;
}

SculptureProgramParams resolveWorldProgramParams(
    std::string_view presetId) noexcept {
    const WorldCatalogEntry* entry = findWorldCatalogEntry(presetId);
    if (entry == nullptr) {
        entry = findPreferredWorldCatalogEntryForPattern(presetId);
    }

    // Special legacy alias for Rossler lantern garden explicit attractor configuration
    if (presetId == "rossler_lantern_garden") {
        SculptureProgramParams p{};
        applyWorldMathematicalAffinity(WorldGeneratorKind::StrangeAttractorLanterns, p);
        return p;
    }

    // Attempt deterministic root 2D pattern projection first (either by requested
    // presetId, or for Dirac monopole worlds via its canonical root pattern alias).
    std::optional<epi::VisualMode> rootMode = mode_projection::fetchRootPatternMode(presetId);
    if (!rootMode && entry != nullptr && entry->generator == WorldGeneratorKind::DiracMonopoleQuantumTunnel) {
        rootMode = mode_projection::fetchRootPatternMode("exp_dirac_monopole_tunnel");
    }
    if (!rootMode && entry != nullptr) {
        rootMode = mode_projection::fetchRootPatternMode(entry->canonicalPresetId);
    }

    if (rootMode) {
        SculptureProgramParams p{};
        mode_projection::visualModeToSculptureParams(*rootMode, p);
        if (entry != nullptr) {
            const WorldMathematicalAffinity& aff = lookupWorldMathematicalAffinity(entry->generator);
            p.instanceMeshType = aff.defaultInstanceMeshType;
            if (entry->generator == WorldGeneratorKind::DiracMonopoleQuantumTunnel ||
                entry->generator == WorldGeneratorKind::Su2YangMillsInstanton) {
                p.outputMode = aff.defaultOutputMode;
                p.enable4D = aff.defaultEnable4D;
                p.rotorAlpha = aff.defaultRotorAlpha;
                p.rotorBeta = aff.defaultRotorBeta;
                p.rotorTheta4D = aff.defaultRotorTheta4D;
                p.latentW = aff.defaultLatentW;
            }
        }
        return p;
    }

    // When no 2D root pattern matches (e.g., native 3D/4D catalog worlds),
    // synthesize the complete 6-stage mathematical parameters from the registry.
    SculptureProgramParams p{};
    if (entry != nullptr) {
        applyWorldMathematicalAffinity(entry->generator, p);
    }
    return p;
}

} // namespace epi_sculpt

