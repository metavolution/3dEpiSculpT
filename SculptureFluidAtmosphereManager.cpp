#include "SculptureFluidAtmosphereManager.h"
#include "ExperimentSprinklerProducer.h"
#include "ExperimentMistFieldAbi.h"
#include "ExperimentZeroCopyHandoff.h"

#include <cmath>
#include <cstdio>
#include <algorithm>
#include <stdexcept>

namespace epi_sculpt {

SculptureFluidAtmosphereManager::~SculptureFluidAtmosphereManager() {
    if (pendingDelivery_ && producer_) {
        static_cast<void>(producer_->releaseGeneric(*pendingDelivery_));
    }
}

// -----------------------------------------------------------------------------
// Educational Annotation: ZERO-COPY VULKAN-OPENCL INTEROP INITIALIZATION
// -----------------------------------------------------------------------------
// The fluid/mist system operates via cross-API POSIX file descriptor export.
// When Vulkan is active and supports cl_khr_external_memory / semaphore extensions,
// ExperimentResourceExternalTransport establishes a triple-buffered ring of
// GPU-allocated memory shared directly between Vulkan rasterizers/raymarchers
// and OpenCL physics compute kernels.
// -----------------------------------------------------------------------------
SculptureFluidAtmosphereManager::SculptureFluidAtmosphereManager(
    mgfx::Engine* engine,
    Config config,
    std::unique_ptr<IExperimentExternalProducer> customProducer
) : engine_(engine),
    config_(config),
    producer_(std::move(customProducer)) {
    if (producer_ == nullptr) {
        producer_ = std::make_unique<ExperimentSprinklerProducer>();
    }
    // The presentation bridge and external allocation must use the producer's
    // declared dense-field axis. A hard-coded manager default otherwise makes
    // a valid 96³ sprinkler field look like a differently sized allocation.
    if (const std::uint32_t axis = producer_->genericDenseFieldAxis(); axis != 0U) {
        config_.mistAxis = axis;
    }

    if (engine_ != nullptr && ExperimentResourceExternalTransport::isSupported(engine_->context())) {
        transport_ = std::make_unique<ExperimentResourceExternalTransport>(
            engine_->context(),
            ExperimentResourceExternalTransport::Config{
                // The external field stores one complete MistVoxelGpu per
                // element (water moments + mean velocity), never a scalar.
                .elementStrideBytes = experiment_mist_field_gpu_abi::kVoxelBytes,
                .elementCount = config_.mistAxis * config_.mistAxis * config_.mistAxis,
                .allocationGeneration = 1U,
            }
        );

        bool ringReady = false;
        if (auto* sprinkler = dynamic_cast<ExperimentSprinklerProducer*>(producer_.get())) {
            sprinklerRing_ = std::make_unique<ExperimentResourceExternalComputeRing>();
            ringReady = sprinkler->dispatcher() != nullptr
                && sprinklerRing_->initialize(*sprinkler->dispatcher(), *transport_);
            if (!ringReady) sprinklerRing_.reset();
        } else {
            ring_ = std::make_unique<ExperimentGenericExternalComputeRing>();
            ringReady = producer_->genericDispatcher() != nullptr
                && ring_->initialize(*producer_->genericDispatcher(), *transport_);
            if (!ringReady) ring_.reset();
        }
        if (!ringReady) {
            std::fprintf(stderr,
                "[SculptureFluidAtmosphereManager] Warning: Physics transport ring failed to initialize; external-required routes remain paused rather than fabricating a host-staged fallback.\n");
            transport_.reset();
        } else {
            std::printf("[SculptureFluidAtmosphereManager] Initialized zero-copy OpenCL-Vulkan transport ring (%u^3 mist voxels).\n",
                        config_.mistAxis);
        }
    }

    if (engine_ != nullptr) {
        renderBridge_ = std::make_unique<SculptureExperimentRenderBridge>(*engine_, true, config_.mistAxis);
    }
}

void SculptureFluidAtmosphereManager::setNozzlePosition(float x, float y, float z) noexcept {
    if (auto* sprinkler = dynamic_cast<ExperimentSprinklerProducer*>(producer_.get())) {
        sprinkler->setNozzlePosition(x, y, z);
    }
}

// -----------------------------------------------------------------------------
// Educational Annotation: FAULT-TOLERANT FIXED-RATE PHYSICS ACCUMULATOR
// -----------------------------------------------------------------------------
// Variable render frametimes (e.g. 16ms to 33ms or transient hitches) can cause
// numerical explosions in fluid/particle advection if delta-time is fed raw.
// We accumulate deltaSeconds into physicsAccumulator_ and consume it in fixed
// increments (fixedPhysicsTickRate, e.g. 33.3ms = 30Hz).
// An upper cap (maxAccumulatorCap = 0.25s) guarantees that a system freeze will
// not trigger an unbounded loop ("spiral of death").
// -----------------------------------------------------------------------------
void SculptureFluidAtmosphereManager::update(
    float deltaSeconds,
    float simulatedTime,
    const AudioResonanceMetrics& metrics,
    const glm::vec3& lightDir
) {
    if (!config_.enabled || producer_ == nullptr) return;

    // 1. Dynamic kinematic nozzle trajectory (sinusoidal 3D sweep)
    if (config_.autoKinematicNozzle) {
        const float nozzleX = std::sin(simulatedTime * 0.4f) * 1.5f;
        const float nozzleZ = std::cos(simulatedTime * 0.5f) * 1.5f;
        const float nozzleY = 0.5f + std::sin(simulatedTime * 0.2f) * 0.5f;
        setNozzlePosition(nozzleX, nozzleY, nozzleZ);
    }

    // 2. Audio-reactive parameter entrainment
    producer_->setAudioMetricsGeneric(metrics);

    // 3. Fixed-timestep physics sub-stepping with spiral-of-death cap
    physicsAccumulator_ += deltaSeconds;
    if (physicsAccumulator_ > config_.maxAccumulatorCap) {
        physicsAccumulator_ = config_.fixedPhysicsTickRate;
    }

    std::uint32_t subSteps = 0U;
    while (!pendingDelivery_ && physicsAccumulator_ >= config_.fixedPhysicsTickRate && subSteps < config_.maxSubStepsPerFrame) {
        const auto status = stepController_.advance(*producer_, transport_ != nullptr);
        if (status != ExperimentStepController::Status::Advanced) {
            // Preserve the accumulated fixed tick. A future frame may receive
            // the required release ticket; consuming time here would silently
            // skip a physics ordinal and break deterministic replay.
            break;
        }
        physicsAccumulator_ -= config_.fixedPhysicsTickRate;
        subSteps++;
    }

    // 4. Ingest latest simulation delivery and synthesize dynamic Mie scattering optics
    if (!pendingDelivery_) {
        pendingDelivery_ = producer_->acquireLatestGeneric();
        if (pendingDelivery_ && pendingDelivery_->generation == scheduledExternalGeneration_)
            scheduledExternalGeneration_ = 0U;
    }
    if (pendingDelivery_) {
        latestOptics_.rainbowIntensity = 18.0f + metrics.kuramotoOrderR * 20.0f;
        latestOptics_.sunRadiance = {
            8.0f + metrics.acousticEnergy * 10.0f,
            7.6f + metrics.acousticEnergy * 5.0f,
            6.9f
        };
        latestOptics_.ambientRadiance = {
            0.08f + (metrics.spectralCentroidHz / 3000.0f) * 0.2f,
            0.12f,
            0.16f + metrics.spectralEntrainment * 0.2f
        };

        const glm::vec3 safeLight = glm::length(lightDir) > 1e-4f ? glm::normalize(lightDir) : glm::vec3(0.0f, 1.0f, 0.0f);
        latestOptics_.sunDirection = SculptureVec3{-safeLight.x, -safeLight.y, -safeLight.z};
        // A headless manager has no drawable consumer and must not retain a
        // payload borrow indefinitely between update-only simulation steps.
        if (!renderBridge_) {
            static_cast<void>(producer_->releaseGeneric(*pendingDelivery_));
            pendingDelivery_.reset();
        }
    }
}

// -----------------------------------------------------------------------------
// Educational Annotation: VOLUMETRIC & SUBSTRATE RENDER SUBMISSION
// -----------------------------------------------------------------------------
// Draws the raymarched volumetric density fog and PBR substrate wetness meshes
// into the active mGFX drawable hierarchy with the canonical root transform.
// -----------------------------------------------------------------------------
void SculptureFluidAtmosphereManager::render(
    mgfx::Drawable& draw,
    const glm::mat4& parentTransform
) {
    if (!config_.enabled || !renderBridge_ || !engine_) return;

    if (pendingDelivery_) {
        // This is called from the application's drawable callback. Keeping the
        // borrow until here makes resource acquisition, semaphore submission
        // and release one frame-owned transaction rather than an update-time
        // approximation of one.
        SculptureExperimentRenderBridge::DeliveryResult result;
        try {
            result = renderBridge_->processDelivery(*engine_, *pendingDelivery_, transport_.get(), latestOptics_);
        } catch (...) {
            // The external helper quarantines an acquired native lease on an
            // exception; release the producer's exclusive borrow exactly once.
            static_cast<void>(producer_->releaseGeneric(*pendingDelivery_));
            pendingDelivery_.reset();
            throw;
        }
        if (result != SculptureExperimentRenderBridge::DeliveryResult::Retryable) {
            static_cast<void>(producer_->releaseGeneric(*pendingDelivery_));
            pendingDelivery_.reset();
        }
    }

    // Arm exactly one free slot for the producer's next fixed step. The Vulkan
    // release semaphore is queued in this active frame; transport ownership is
    // committed only after that queue submission succeeds.
    if (transport_ && producer_ && !pendingDelivery_ && !producer_->hasExternalWorkBoundGeneric()) {
        // A consumed work item that failed before publication must not leave
        // the single-slot generic handoff permanently marked as scheduled.
        scheduledExternalGeneration_ = 0U;
        if (auto* sprinkler = dynamic_cast<ExperimentSprinklerProducer*>(producer_.get());
            sprinkler && sprinklerRing_) {
            scheduleExternalZeroCopyHandoff(*transport_, *sprinklerRing_, *sprinkler,
                engine_->frameLoop(), scheduledExternalGeneration_);
        } else if (ring_) {
            scheduleExternalZeroCopyHandoff(*transport_, *ring_, *producer_,
                engine_->frameLoop(), scheduledExternalGeneration_);
        }
    }

    glm::mat4 fluidRoot = glm::translate(parentTransform, config_.rootTranslation);
    fluidRoot = glm::scale(fluidRoot, config_.rootScale);
    renderBridge_->draw(*engine_, draw, fluidRoot);
}

} // namespace epi_sculpt
