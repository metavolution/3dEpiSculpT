#include <cmath>
#include <chrono>
#include <cstdio>
#include <vector>
#include <stdexcept>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "mgfx/vk/Mesh.h"
#include "mgfx/Engine.h"
#include "SculptureProgram.h"
#include "SculpturePresetLibrary.h"
#include "SculptureFrameProducer.h"
#include "SculptureWorldExternalTransport.h"
#include "SculptureWorldRenderBridge.h"
#include "SculptureWorldRecipe.h"
#include "SculptureProducerCadence.h"
#include "SculpturePipelineCoordinator.h"
#include "SculptureAutoDirector.h"
#include "SculptureResourcePaths.h"
#include "SculptureVulkanDeviceIdentity.h"
#include "SculptureFluidAtmosphereManager.h"
#include "SculptureWorldFraming.h"
#include "SculptureStateRecorder.h"
#include "SculptureWorldNavigator.h"
#include "SculptureEventBus.h"
#include "mgfx/vk/FrameLoop.h"

#include "SculptureKeyActionMapper.h"
#include "SculptureLiveControls.hpp"
#include "SculptureWorldBenchmarkProfile.h"
#include "SculptureWorldRecipe.h"

#include <iostream>
#include <iomanip>

using namespace mgfx;
using namespace epi_sculpt;

struct WorldDemoCliArgs {
    std::string worldName;
    std::string tourId;
    std::string categoryName;
    std::string snapshotPath;
    std::string timelinePath;
    std::string profileWorldId;
    float dwellSeconds = 12.0f;
    float flightSeconds = 3.5f;
    float speedMultiplier = 1.0f;
    std::string presentationMode; // "pointcloud", "relief", "hybrid"
    bool autoDirector = false;
    bool stereoSBS = false;
    float ipdMeters = 0.064f;
    bool listWorlds = false;
    bool listTours = false;
    bool listCategories = false;
    bool profileAll = false;
    bool profileSingle = false;
    bool showHelp = false;
    int width = 2560;
    int height = 1440;
};

inline WorldDemoCliArgs parseDemoArgs(int argc, char** argv) {
    WorldDemoCliArgs args{};
#ifdef MGFX_EPI_SCULPT_DEFAULT_WORLD
    args.worldName = MGFX_EPI_SCULPT_DEFAULT_WORLD;
#else
    args.worldName = "exp_golden_phyllotaxis_cathedral";
#endif

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            args.showHelp = true;
        } else if (arg == "-l" || arg == "--list" || arg == "--list-worlds") {
            args.listWorlds = true;
        } else if (arg == "--list-tours") {
            args.listTours = true;
        } else if (arg == "--list-categories") {
            args.listCategories = true;
        } else if (arg == "--profile-all") {
            args.profileAll = true;
        } else if (arg == "--profile") {
            args.profileSingle = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                args.profileWorldId = argv[++i];
            } else {
                args.profileWorldId = args.worldName;
            }
        } else if ((arg == "-w" || arg == "--world") && i + 1 < argc) {
            args.worldName = argv[++i];
        } else if ((arg == "-t" || arg == "--tour") && i + 1 < argc) {
            args.tourId = argv[++i];
        } else if ((arg == "-c" || arg == "--category") && i + 1 < argc) {
            args.categoryName = argv[++i];
        } else if ((arg == "-d" || arg == "--dwell") && i + 1 < argc) {
            args.dwellSeconds = std::stof(argv[++i]);
        } else if ((arg == "-f" || arg == "--flight") && i + 1 < argc) {
            args.flightSeconds = std::stof(argv[++i]);
        } else if ((arg == "-m" || arg == "--mode") && i + 1 < argc) {
            args.presentationMode = argv[++i];
        } else if (arg == "--sbs" || arg == "--stereo") {
            args.stereoSBS = true;
        } else if (arg == "--ipd" && i + 1 < argc) {
            args.ipdMeters = std::stof(argv[++i]);
        } else if (arg == "-a" || arg == "--autodirector") {
            args.autoDirector = true;
        } else if (arg == "--speed" && i + 1 < argc) {
            args.speedMultiplier = std::stof(argv[++i]);
        } else if ((arg == "-s" || arg == "--snapshot") && i + 1 < argc) {
            args.snapshotPath = argv[++i];
        } else if (arg == "--timeline" && i + 1 < argc) {
            args.timelinePath = argv[++i];
        } else if (arg == "--1440p") {
            args.width = 2560;
            args.height = 1440;
        } else if (arg == "--1080p") {
            args.width = 1920;
            args.height = 1080;
        } else if (arg == "--4k") {
            args.width = 3840;
            args.height = 2160;
        } else if ((arg == "-W" || arg == "--width") && i + 1 < argc) {
            args.width = std::stoi(argv[++i]);
        } else if ((arg == "-H" || arg == "--height") && i + 1 < argc) {
            args.height = std::stoi(argv[++i]);
        } else if (!arg.starts_with("-")) {
            if (arg.ends_with(".timeline.json")) {
                args.timelinePath = std::string(arg);
            } else if (arg.ends_with(".world") || arg.ends_with(".epimode") || arg.ends_with(".json")) {
                args.snapshotPath = std::string(arg);
            } else {
                args.worldName = std::string(arg);
            }
        }
    }
    return args;
}

inline void printHelp(const char* progName) {
    std::printf("Usage: %s [OPTIONS] [WORLD_ALIAS_OR_SNAPSHOT]\n\n", progName);
    std::printf("epiC Procedural Dedicated World Showcase & Architectural Explorer\n\n");
    std::printf("Options:\n");
    std::printf("  -h, --help                Show this help message and exit\n");
    std::printf("  -l, --list, --list-worlds List implemented catalog worlds\n");
    std::printf("      --list-tours          List all available curated thematic tours\n");
    std::printf("      --list-categories     List all preset library categories\n");
    std::printf("      --profile-all         Run canonical CPU-oracle profiles (not GPU timing)\n");
    std::printf("      --profile [WORLD]     CPU-oracle profile of a world or the default\n");
    std::printf("  -w, --world WORLD         Catalog alias, library ID (pattern/...), or catalog index\n");
    std::printf("  -t, --tour TOUR_ID        Activate curated thematic tour on startup\n");
    std::printf("  -c, --category CAT        Activate category tour on startup\n");
    std::printf("  -d, --dwell SECONDS       Set tour waypoint dwell duration (default: 12.0s)\n");
    std::printf("  -f, --flight SECONDS      Set tour transition flight duration (default: 3.5s)\n");
    std::printf("  -m, --mode MODE           Set presentation mode: pointcloud, relief, hybrid\n");
    std::printf("      --sbs, --stereo       Enable Side-By-Side (SBS) stereoscopic 3D presentation\n");
    std::printf("      --ipd METERS          Set stereoscopic interpupillary distance (default: 0.064m)\n");
    std::printf("  -a, --autodirector        Enable autonomous exhibition director on launch\n");
    std::printf("      --speed MULTIPLIER    Set simulation playback speed multiplier\n");
    std::printf("  -s, --snapshot PATH       Load startup snapshot bundle (.world/.epimode/.manifest)\n");
    std::printf("      --timeline PATH       Load and start timeline script (.timeline.json)\n");
    std::printf("      --1440p               Start in 1440p resolution (2560x1440, default)\n");
    std::printf("      --1080p               Start in 1080p resolution (1920x1080)\n");
    std::printf("      --4k                  Start in 4K UHD resolution (3840x2160)\n");
    std::printf("  -W, --width PIXELS        Set custom window width (default: 2560)\n");
    std::printf("  -H, --height PIXELS       Set custom window height (default: 1440)\n\n");
    std::printf("Interactive Keyboard Controls:\n");
    std::printf("  Right / N, Left / Shift+N Next / Previous World Preset (1.5s flight)\n");
    std::printf("  P                         Toggle Side-By-Side (SBS) Stereoscopic 3D Mode\n");
    std::printf("  T / O                     Toggle Autonomous Flight Tour Autopilot\n");
    std::printf("  Y / Shift+Y               Cycle Curated Thematic Tour / Category Tour\n");
    std::printf("  M, C                      Geodesic Parameter Mutation / Palette Drift\n");
    std::printf("  V / Alt+V / Ctrl+Alt+V    Presentation Mode / Volumetric Advection / Field Coupling\n");
    std::printf("  B / Shift+W               Non-Euclidean Relief / Torus Domain Wrap\n");
    std::printf("  A, G                      Toggle Fluid Atmosphere / Cycle Governor Scale\n");
    std::printf("  4, X                      Toggle 4D Spacetime Wave / Cycle Manifold Slicing\n");
    std::printf("  Z                         Toggle Living OKLab Palette Agent\n");
    std::printf("  [ / ], ; / '              Scrub W Slicing Plane / Left SO(4) Rotor Alpha\n");
    std::printf("  L / Shift+L               Cycle Solo Learn Stage / Reset Pipeline\n");
    std::printf("  Space / F8                Capture Snapshot / Reload Latest Snapshot Bundle\n");
    std::printf("  F9, F10                   Toggle Sequence Recording / Timeline Playback\n");
    std::printf("  F11 / Shift+F11           Cycle Target FPS Tier / Toggle Performance Monitor\n");
    std::printf("  KP +/-                    Increase / Decrease Simulation Speed\n");
    std::printf("  Q / ESC                   Quit\n\n");
}

int main(int argc, char** argv) try {
    const SculptureResourcePaths resources = SculptureResourceLocator::resolve(argv[0]);
    const auto cli = parseDemoArgs(argc, argv);

    if (cli.showHelp) {
        printHelp(argv[0]);
        return 0;
    }

    if (cli.listWorlds || cli.listTours || cli.listCategories || cli.profileAll || cli.profileSingle) {
        SculpturePresetLibrary presetLib;
        if (!resources.presetDirectory.empty())
            static_cast<void>(presetLib.reload(resources.presetDirectory));
        SculpturePipelineCoordinatorConfig queryCfg{};
        queryCfg.enableAutoDirector = false;
        SculpturePipelineCoordinator queryCoord(queryCfg, &presetLib);
        SculptureWorldNavigator queryNav(
            nullptr, queryCoord, &presetLib, cli.worldName, resources.presetDirectory);

        if (cli.listWorlds) {
            std::printf("========================================================================================================\n");
            std::printf("  epiC World Catalog (%zu Worlds Indexed)\n", queryNav.count());
            std::printf("========================================================================================================\n");
            std::printf("%-5s | %-36s | %-40s | %-12s | %-6s\n", "Idx", "ID / Alias", "Title", "Primitive", "Budget");
            std::printf("--------------------------------------------------------------------------------------------------------\n");
            for (std::size_t i = 0; i < queryNav.count(); ++i) {
                queryNav.selectWorld(i, 0.0f);
                const auto& entry = queryNav.activeEntry();
                const auto& recipe = queryNav.activeRecipe();
                std::printf("%-5zu | %-36s | %-40s | %-12s | %-6u\n",
                    i, entry.canonicalPresetId.data(), entry.title.data(),
                    worldPrimitiveName(recipe.primitive).data(), recipe.instanceBudget);
            }
            std::printf("========================================================================================================\n");
            return 0;
        }

        if (cli.listTours) {
            const auto tours = queryNav.availableCuratedTours();
            std::printf("========================================================================================================\n");
            std::printf("  epiC Curated Thematic Tours (%zu Tours Available)\n", tours.size());
            std::printf("========================================================================================================\n");
            std::printf("%-5s | %-36s | %-42s | %-10s\n", "Idx", "Tour ID", "Title", "Waypoints");
            std::printf("--------------------------------------------------------------------------------------------------------\n");
            for (std::size_t i = 0; i < tours.size(); ++i) {
                const auto& t = tours[i];
                std::printf("%-5zu | %-36s | %-42s | %-10zu\n",
                    i, t.id.c_str(), t.title.c_str(), t.presetIds.size());
            }
            std::printf("========================================================================================================\n");
            return 0;
        }

        if (cli.listCategories) {
            const auto categories = queryNav.availableTourCategories();
            std::printf("========================================================================================================\n");
            std::printf("  epiC Tour Categories (%zu Categories Available)\n", categories.size());
            std::printf("========================================================================================================\n");
            for (const auto& cat : categories) {
                std::printf("  - %s\n", cat.c_str());
            }
            std::printf("========================================================================================================\n");
            return 0;
        }

        if (cli.profileAll) {
            const auto results = SculptureWorldBenchmarkProfile::profileCanonicalBenchmarks(queryNav, 8);
            SculptureWorldBenchmarkProfile::printProfileTable(results);
            return 0;
        }

        if (cli.profileSingle) {
            const auto res = SculptureWorldBenchmarkProfile::profileWorld(queryNav, cli.profileWorldId, 8);
            SculptureWorldBenchmarkProfile::printProfileTable({res});
            return 0;
        }
    }

    vk::Context ctx(vk::Context::CreateInfo{
        // The catalog demo is a deterministic performance/artifact harness.
        // Do not inherit a workstation's native fullscreen mode: it makes a
        // selected world silently vary from a laptop-sized preview to a
        // multi-monitor-sized workload.
        .width = cli.width,
        .height = cli.height,
        .title = "mGFX - epiC World Catalog Demo (1440p)",
        .enableValidation = false,
        .desktopFullscreenWindowed = false,
        .preferX11CurrentMode = false,
        .shaderBinaryDirectory = resources.shaderBinaryDirectory,
    });
    vk::Swapchain swap(ctx);
    vk::FrameLoop loop(ctx, swap);

    Engine::Config cfg{};
    cfg.validateShaderContract = true;
    cfg.quality = RenderQuality::basic();
    const std::string shaderSourceDirectory = resources.shaderSourceDirectory.string();
    const std::string fontPath = resources.fontPath.string();
    cfg.shaderSourceDir = shaderSourceDirectory.empty() ? nullptr : shaderSourceDirectory.c_str();
    cfg.text.fontPath = fontPath.empty() ? nullptr : fontPath.c_str();
    if (!fontPath.empty()) cfg.text.fontPixelHeight = 24.0f;
    Engine engine(ctx, loop, cfg);

    SculptureFrameProducer::Config prodCfg{};
    prodCfg.kernelPath = resources.sculptureKernel.string();
    prodCfg.worldKernelPath = resources.worldKernel.string();
    prodCfg.preferOpenCl = true;
    prodCfg.enableCpuFailureRecovery = false;
    matchOpenClToVulkanDevice(prodCfg, ctx.device().physical());
    // Dedicated worlds are OpenCL-authored and directly consumed by Vulkan.
    // Advertise the actual Vulkan capabilities before creating the producer so
    // it can accept only the external-memory route, never a CPU-staged copy.
    prodCfg.vulkanExternalMemoryFd = ctx.device().supportsExternalMemoryFd();
    prodCfg.vulkanExternalSemaphoreFd = ctx.device().supportsExternalSemaphoreFd();
    
    // Reverse destruction order preserves exported world allocations through
    // the producer's queue drain, including early exits during import/startup.
    std::unique_ptr<SculptureWorldExternalTransport> worldTransport;
    SculptureFrameProducer producer(prodCfg);

    std::string presetName = cli.worldName;

    if (!producer.usingOpenCl()) {
        throw std::runtime_error(
            "OpenCL is required for the world catalog: "
            + std::string(producer.diagnostic()));
    }
    if (!producer.worldKernelReady()) {
        throw std::runtime_error(
            "dedicated world kernel unavailable: "
            + std::string(producer.worldDiagnostic()));
    }
    if (!prodCfg.vulkanExternalMemoryFd || !prodCfg.vulkanExternalSemaphoreFd) {
        throw std::runtime_error(
            "The selected Vulkan device lacks required external-memory/semaphore FD support; "
            "the dedicated-world demo cannot use a CPU-staged fallback.");
    }

    // This is the single OpenCL -> Vulkan ownership bridge for the standalone
    // demos. Each duplicated FD belongs to one ring slot and is imported by
    // OpenCL before the first world dispatch; the render bridge later signals
    // when Vulkan has finished consuming that immutable generation.
    worldTransport = std::make_unique<SculptureWorldExternalTransport>(ctx,
        SculptureWorldExternalTransport::Config{});
    for (std::uint32_t slot = 0; slot < SculptureWorldExternalTransport::kSlotCount; ++slot) {
        auto openClSlot = worldTransport->exportOpenClImport(slot);
        if (!openClSlot || !producer.importWorldExternalOutput(slot, *openClSlot)
            || !worldTransport->markOpenClImported(slot)) {
            worldTransport->quarantine(slot, VulkanOpenClRingFault::ImportFailed);
            throw std::runtime_error(
                "Could not bind OpenCL/Vulkan external world slot " + std::to_string(slot));
        }
    }

    SculpturePipelineCoordinatorConfig coordCfg{};
    coordCfg.cadence.updateHz = 30.0f;
    coordCfg.cadence.dispatchOnParameterChange = true;
    coordCfg.cadence.enableAdaptiveThrottling = true;
    coordCfg.cadence.targetLatencyMilliseconds = 20.0f;
    // A named demo target must hold its requested world until an explicit
    // input action changes it. Tours and palette drift remain available from
    // the controls, but are not hidden startup side effects.
    coordCfg.enableAutoDirector = false;
    coordCfg.autoDirectorConfig.enableAutonomousNavigation = false;
    coordCfg.autoDirectorConfig.enablePaletteDrift = false;
    coordCfg.autoDirectorConfig.enableStagnationIntervention = false;
    // Audio remains an opt-in future control source. Keep the normal world
    // path focused on deterministic OpenCL simulation and Vulkan presentation.
    coordCfg.enableAudioResonance = false;
    
    SculpturePresetLibrary presetLibrary;
    const bool presetLibraryAvailable = !resources.presetDirectory.empty()
        && presetLibrary.reload(resources.presetDirectory);
    SculpturePipelineCoordinator coordinator(
        coordCfg, presetLibraryAvailable ? &presetLibrary : nullptr);
    SculptureWorldNavigator navigator(
        engine, coordinator, presetLibraryAvailable ? &presetLibrary : nullptr, presetName,
        resources.presetDirectory);
    navigator.setExternalTransport(worldTransport.get());
    // Unified EventBus messaging foundation
    SculptureEventBus eventBus;
    coordinator.bindEventBus(eventBus);
    navigator.bindEventBus(eventBus);

    // Attach reactive logging observer for terminal telemetry
    auto demoObserver = std::make_shared<NavigatorLoggingObserver>("[WorldDemo]", true);
    navigator.addObserver(demoObserver);

    // Bind declarative platform input mapper
    SculptureKeyActionMapper::attachGlfwKeyCallback(ctx.window().glfw(), &navigator);
    SculptureLiveControls cameraControls(ctx.window().glfw(), true);
    navigator.setCameraController([&](const Camera& camera, float dt) {
        cameraControls.beginFrame(dt);
        return cameraControls.applyCamera(camera, dt);
    });

    if (!cli.tourId.empty()) {
        navigator.selectCuratedTour(cli.tourId);
    } else if (!cli.categoryName.empty()) {
        navigator.selectCategoryTour(cli.categoryName, cli.flightSeconds, cli.dwellSeconds);
    }

    if (auto* ch = navigator.tourChoreographer()) {
        auto chCfg = ch->config();
        if (cli.dwellSeconds > 0.0f) chCfg.dwellSeconds = cli.dwellSeconds;
        if (cli.flightSeconds > 0.0f) chCfg.transitionSeconds = cli.flightSeconds;
        ch->setConfig(chCfg);
    }

    if (!cli.presentationMode.empty()) {
        if (cli.presentationMode == "relief") {
            navigator.setPresentationMode(WorldPresentationMode::ReliefManifold);
        } else if (cli.presentationMode == "hybrid") {
            navigator.setPresentationMode(WorldPresentationMode::HybridVolumetric);
        } else {
            navigator.setPresentationMode(WorldPresentationMode::PointCloud);
        }
    }

    if (cli.stereoSBS) {
        navigator.setStereoEnabled(true);
        std::printf("[WorldDemo] Stereoscopic 3D Side-By-Side (SBS) mode active (IPD: %.1f mm)\n",
                    cli.ipdMeters * 1000.0f);
    }
    if (cli.ipdMeters > 0.0f) {
        navigator.setInterpupillaryDistance(cli.ipdMeters);
    }

    if (cli.autoDirector) {
        coordinator.enableAutoDirector(true);
    }

    if (cli.speedMultiplier != 1.0f && cli.speedMultiplier > 0.0f) {
        coordinator.setSpeedMultiplier(cli.speedMultiplier);
    }

    if (!cli.snapshotPath.empty()) {
        if (navigator.loadSnapshot(cli.snapshotPath)) {
            std::printf("[WorldDemo] Loaded startup snapshot bundle: %s\n", cli.snapshotPath.c_str());
        }
    }

    if (!cli.timelinePath.empty()) {
        if (navigator.loadTimelineScript(cli.timelinePath)) {
            navigator.playTimeline();
            std::printf("[WorldDemo] Loaded and started startup timeline: %s\n", cli.timelinePath.c_str());
        }
    }

    std::printf("===================================================================\n");
    std::printf("  epiC Procedural Dedicated World Showcase (Modular Navigator)\n");
    std::printf("  World [%zu/%zu]: %s (%s)\n",
                navigator.activeIndex() + 1U, navigator.count(),
                navigator.activeEntry().title.data(),
                worldGeneratorName(navigator.activeRecipe().generator).data());
    std::printf("  Interactive Controls:\n");
    std::printf("    LMB / RMB drag  : Orbit / pan; wheel: dolly; Home: authored camera\n");
    std::printf("    Right / N       : Next World Preset\n");
    std::printf("    Left / Shift+N  : Previous World Preset\n");
    std::printf("    P               : Toggle Side-By-Side (SBS) Stereoscopic 3D Mode\n");
    std::printf("    T / O           : Toggle Autonomous Flight Tour / Autopilot\n");
    std::printf("    Y / Shift+Y     : Cycle Curated Thematic Tour / Cycle Category Tour\n");
    std::printf("    M               : Geodesic Parameter Mutation\n");
    std::printf("    C               : Dynamic Palette Drift / Randomize\n");
    std::printf("    V               : Cycle World Presentation Mode (Cloud/Relief/Hybrid)\n");
    std::printf("    B               : Cycle Non-Euclidean Relief Projection\n");
    std::printf("    A               : Toggle Dynamic Fluid Atmosphere & Volumetric Mist\n");
    std::printf("    G               : Cycle Variable-Resolution Governor Scale\n");
    std::printf("    4               : Toggle Dynamic 4D Spacetime Foliation Wave\n");
    std::printf("    X / Shift+X     : Cycle 4D Manifold / Cycle Polychoron Source (Ctrl+X: Target, Alt+X: Morph)\n");
    std::printf("    Z               : Toggle Living Quasi-Periodic OKLab Palette Agent\n");
    std::printf("    [ / ]           : Scrub 4D Hyperplane Slicing Coordinate (W -0.05 / +0.05)\n");
    std::printf("    ; / '           : Scrub Left SO(4) Rotor Angle Alpha (-0.05 / +0.05 rad)\n");
    std::printf("    \\               : Reset 4D Rotors & W Slice to Canonical Baseline\n");
    std::printf("    L / Shift+L     : Cycle Solo Learn Stage / Reset to Full Pipeline\n");
    std::printf("    J               : Cycle Diagnostic Oracle Stage (CPU/GPU)\n");
    std::printf("    R / F           : Reset Camera Framing\n");
    std::printf("    Space / F8      : Capture Snapshot / Reload Latest Snapshot Bundle\n");
    std::printf("    F9              : Toggle Continuous Frame Sequence Recording (captures/seq_*)\n");
    std::printf("    F10 / Shift+F10 : Toggle Timeline / Generate Beat-Synced Timeline (Ctrl+F10: Export)\n");
    std::printf("    F11 / Shift+F11 : Cycle Target Frame Rate (60/120/144/240/Unlimited) / Toggle Monitor\n");
    std::printf("    KP + / =        : Increase Simulation Speed (1.2x)\n");
    std::printf("    KP - / -        : Decrease Simulation Speed (1.2x)\n");
    std::printf("    0               : Reset Simulation Speed (1.0x)\n");
    std::printf("    . / Num *       : Pause / Resume Simulation\n");
    std::printf("    Q / ESC         : Quit\n");
    std::printf("===================================================================\n");
    const auto start = std::chrono::steady_clock::now();

    // Initial dynamic PBR atmosphere and lighting baseline
    navigator.applyDynamicLighting(engine, 0.0f, coordinator.audioResonance().metrics());

    // -------------------------------------------------------------------------
    // Educational Annotation: DYNAMIC FLUID & ATMOSPHERIC MIST MANAGER
    // -------------------------------------------------------------------------
    // Encapsulates OpenCL multi-physics fluid/droplet simulation, zero-copy Vulkan
    // buffer ring synchronization, fixed-rate physics accumulator, and Mie/Rayleigh
    // rainbow scattering into SculptureFluidAtmosphereManager, removing 100+ lines
    // of low-level boilerplate from the application presentation loop.
    // -------------------------------------------------------------------------
    auto atmosphere = std::make_shared<SculptureFluidAtmosphereManager>(engine);
    navigator.setAtmosphereManager(atmosphere);

    auto lastTick = start;
    while (!ctx.shouldClose()) {
        ctx.pollEvents();
        const auto now = std::chrono::steady_clock::now();
        const float delta = std::chrono::duration<float>(now - lastTick).count();
        lastTick = now;

        // ---------------------------------------------------------------------
        // Educational Annotation: UNIFIED FAULT-TOLERANT RENDER EXECUTION FLOW
        // ---------------------------------------------------------------------
        // Drains pending action commands, updates audio-reactive environment lights,
        // advances closed-loop simulation & camera geodesics, coordinates snapshots,
        // and renders multi-pass presentation + HUD telemetry in a single shielded call.
        // ---------------------------------------------------------------------
        navigator.renderFrame(engine, loop, producer, delta);
    }
    producer.waitIdle();
    engine.waitIdle();
    return 0;
} catch (const std::exception& e) {
    std::fprintf(stderr, "Fatal error: %s\n", e.what());
    return 1;
}
