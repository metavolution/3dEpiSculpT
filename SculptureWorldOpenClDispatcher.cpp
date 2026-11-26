#include "SculptureWorldOpenClDispatcher.h"
#include "SculptureProgramSourceFingerprint.h"
#include <CL/cl_ext.h>
#include <cstring>
#include <cstdio>
#include <exception>
#include <optional>
#include <sstream>
#include <vector>

namespace epi_sculpt {

namespace {
    bool generationNewer(std::uint32_t a, std::uint32_t b) noexcept {
        return a != b && (a - b) < 0x80000000U;
    }

    DeviceSignature queryOpenClDeviceSignature(cl_device_id device) {
        DeviceSignature sig{};
        auto queryStr = [device](cl_device_info param) -> std::string {
            std::size_t size = 0;
            if (clGetDeviceInfo(device, param, 0, nullptr, &size) != CL_SUCCESS || size == 0) {
                return "unknown";
            }
            std::vector<char> buf(size + 1, '\0');
            if (clGetDeviceInfo(device, param, size, buf.data(), nullptr) != CL_SUCCESS) {
                return "unknown";
            }
            return std::string(buf.data());
        };
        sig.deviceName = queryStr(CL_DEVICE_NAME);
        sig.vendorName = queryStr(CL_DEVICE_VENDOR);
        sig.driverVersion = queryStr(CL_DRIVER_VERSION);
        sig.openClVersion = queryStr(CL_DEVICE_VERSION);
#ifdef CL_DEVICE_UUID_KHR
        std::size_t uuidSize = 0;
        if (clGetDeviceInfo(device, CL_DEVICE_UUID_KHR, sizeof(sig.deviceUuid), sig.deviceUuid.data(), &uuidSize) == CL_SUCCESS && uuidSize == 16) {
            sig.hasUuid = true;
        }
#endif
        return sig;
    }

    std::vector<std::uint8_t> extractOpenClProgramBinary(cl_program program, cl_device_id device) {
        cl_uint numDevices = 0;
        if (clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, sizeof(cl_uint), &numDevices, nullptr) != CL_SUCCESS || numDevices == 0) {
            return {};
        }
        std::vector<cl_device_id> devices(numDevices);
        if (clGetProgramInfo(program, CL_PROGRAM_DEVICES, sizeof(cl_device_id) * numDevices, devices.data(), nullptr) != CL_SUCCESS) {
            return {};
        }
        std::vector<std::size_t> binarySizes(numDevices, 0);
        if (clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(std::size_t) * numDevices, binarySizes.data(), nullptr) != CL_SUCCESS) {
            return {};
        }
        std::size_t targetIndex = 0;
        bool found = false;
        for (std::size_t i = 0; i < numDevices; ++i) {
            if (devices[i] == device) {
                targetIndex = i;
                found = true;
                break;
            }
        }
        if (!found || binarySizes[targetIndex] == 0) return {};

        std::vector<std::vector<std::uint8_t>> allBinaries(numDevices);
        std::vector<unsigned char*> pointers(numDevices, nullptr);
        for (std::size_t i = 0; i < numDevices; ++i) {
            if (binarySizes[i] > 0) {
                allBinaries[i].resize(binarySizes[i]);
                pointers[i] = allBinaries[i].data();
            }
        }
        if (clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*) * numDevices, pointers.data(), nullptr) != CL_SUCCESS) {
            return {};
        }
        return allBinaries[targetIndex];
    }
}

SculptureWorldOpenClDispatcher::SculptureWorldOpenClDispatcher(cl_context context, cl_device_id device) {
    const cl_queue_properties queueProperties[] = {
        CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0
    };
    openCl_ = std::make_unique<SculptureOpenClContext>(context, device, queueProperties);
}
    

void SculptureWorldOpenClDispatcher::checkOpenCl(cl_int result, const char* context) const {
    if (result != CL_SUCCESS) {
        throw std::runtime_error(std::string(context) + " failed: " + std::to_string(result));
    }
}

std::string SculptureWorldOpenClDispatcher::readTextFile(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open kernel file: " + path);
    }
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

void SculptureWorldOpenClDispatcher::initializeKernel(const std::string& path) {
    if (path.empty()) return;
    if (!worldKernelSourceCode_.empty()) {
        throw std::logic_error("world kernel source already initialized; recreate the dispatcher to reload");
    }
    worldKernelSourcePath_ = path;
    worldKernelSourceCode_ = readTextFile(path);
    
    cl_int result = CL_SUCCESS;
    for (WorldSlot& slot : worldSlots_) {
        // Persistent input buffers are independent of lazy per-generator JIT.
        // The real four-argument ABI is checked on the compiled kernel.
        slot.kernel = nullptr;
        slot.couplerKernel = nullptr;
        slot.programBuffer = clCreateBuffer(openCl_->context(), CL_MEM_READ_ONLY,
                                            sizeof(slot.params), nullptr, &result);
        checkOpenCl(result, "clCreateBuffer(world program)");
        slot.frameBuffer = clCreateBuffer(openCl_->context(), CL_MEM_READ_ONLY,
                                          sizeof(slot.frame), nullptr, &result);
        checkOpenCl(result, "clCreateBuffer(world frame)");
        slot.recipeBuffer = clCreateBuffer(openCl_->context(), CL_MEM_READ_ONLY,
                                           sizeof(slot.recipe), nullptr, &result);
        checkOpenCl(result, "clCreateBuffer(world recipe)");
        slot.couplerConfigBuffer = clCreateBuffer(openCl_->context(), CL_MEM_READ_ONLY,
                                                  sizeof(slot.couplerConfig), nullptr, &result);
        checkOpenCl(result, "clCreateBuffer(world coupler config)");
    }
}

void SculptureWorldOpenClDispatcher::bindVolumetricCoupling(
    cl_mem volumetricVoxelBuffer,
    cl_mem volumetricDescriptorBuffer,
    const volumetric_gpu_abi::VolumetricCouplerConfigGpu& couplerConfig
) noexcept {
    volumetricVoxelBuffer_ = volumetricVoxelBuffer;
    volumetricDescriptorBuffer_ = volumetricDescriptorBuffer;
    volumetricCouplerConfig_ = couplerConfig;
}

void SculptureWorldOpenClDispatcher::clearVolumetricCoupling() noexcept {
    volumetricVoxelBuffer_ = nullptr;
    volumetricDescriptorBuffer_ = nullptr;
}

SculptureWorldOpenClDispatcher::CompiledWorldKernel::~CompiledWorldKernel() {
    if (couplerKernel != nullptr) clReleaseKernel(couplerKernel);
    if (kernel != nullptr) clReleaseKernel(kernel);
    if (program != nullptr) clReleaseProgram(program);
}

const SculptureWorldOpenClDispatcher::CompiledWorldKernel*
SculptureWorldOpenClDispatcher::getOrCompileWorldKernel(std::uint32_t generatorId) {
    auto [it, inserted] = worldKernelCache_.try_emplace(generatorId);
    auto& entry = it->second;
    if (entry.compiled) return entry.compiled.get();
    if (entry.failure) std::rethrow_exception(entry.failure);

    try {
        if (inserted) {
            if (worldKernelSourceCode_.empty()) {
                throw std::runtime_error("No world kernel source loaded for JIT compilation");
            }
            // The worker owns a context reference independently of the host's
            // device/backend lifetime. It captures no dispatcher or slot pointer.
            checkOpenCl(clRetainContext(openCl_->context()), "clRetainContext(world JIT)");
            const std::shared_ptr<_cl_context> contextLease(openCl_->context(), [](cl_context context) {
                clReleaseContext(context);
            });
            entry.pending = std::async(std::launch::async,
                [contextLease, device = openCl_->device(), path = worldKernelSourcePath_,
                 source = worldKernelSourceCode_, generatorId] {
                    return compileWorldKernel(contextLease.get(), device, path, source, generatorId);
                });
        }
        // The render thread only polls. A completed future transfers its sole
        // result owner into this same entry; no raw handles cross cache maps.
        if (entry.pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return nullptr;
        entry.compiled = entry.pending.get();
        return entry.compiled.get();
    } catch (...) {
        entry.failure = std::current_exception();
        throw;
    }
}

std::unique_ptr<SculptureWorldOpenClDispatcher::CompiledWorldKernel>
SculptureWorldOpenClDispatcher::compileWorldKernel(
    cl_context context, cl_device_id device, const std::string& path,
    const std::string& fallbackSource, std::uint32_t generatorId) {
    const auto requireSuccess = [generatorId](cl_int result, const char* operation) {
        if (result != CL_SUCCESS) {
            throw std::runtime_error("world generator " + std::to_string(generatorId)
                + ": " + operation + " failed: " + std::to_string(result));
        }
    };
    std::string source = fallbackSource;
    const std::filesystem::path kernelFile(path);
    const auto modularPath = kernelFile.parent_path() / "worlds" / (std::to_string(generatorId) + ".clh");
    if (std::filesystem::exists(modularPath)) {
        std::ifstream modularFile(modularPath);
        if (!modularFile) {
            throw std::runtime_error("Cannot read world component: " + modularPath.string());
        }
        std::ostringstream component;
        component << modularFile.rdbuf();
        if (modularFile.bad()) {
            throw std::runtime_error("Failed reading world component: " + modularPath.string());
        }
        source = "#include \"SculptureGpuAbi.clh\"\n"
                      "#include \"SculptureWorldGpuAbi.clh\"\n"
                      "#include \"EpiMath.clh\"\n"
                      "#include \"SculptureWorldCommon.clh\"\n"
                      "#include \"SculptureWorldVolumetricCoupler.clh\"\n"
                      "#define EPI_ACTIVE_GENERATOR_ID " + std::to_string(generatorId) + "\n"
                      + component.str() + "\n"
                      "__kernel void generate_world_instances(\n"
                      "    __constant const SculptureProgramParamsGpu* params,\n"
                      "    __constant const SculptureFrameGpu* sculptureFrame,\n"
                      "    __constant const WorldRecipeGpu* recipe,\n"
                      "    __global WorldSnapshotGpu* output) {\n"
                      "    const uint index = get_global_id(0);\n"
                      "    if (!epi_sculpt_program_abi_valid(params) || !epi_sculpt_frame_abi_valid(sculptureFrame) || !epi_sculpt_world_recipe_abi_valid(recipe)) return;\n"
                      "    epi_world_generate(index, params, sculptureFrame, recipe, output);\n"
                      "}\n";
    }

    auto compiled = std::make_unique<CompiledWorldKernel>();
    const std::string options =
            "-cl-std=CL1.2 -cl-fast-relaxed-math -cl-mad-enable "
            "-cl-no-signed-zeros -cl-denorms-are-zero -I \""
            + kernelFile.parent_path().string() + "\" "
            "-DEPI_ACTIVE_GENERATOR_ID=" + std::to_string(generatorId);
    const std::string kernelTag = "WorldGenerator_" + std::to_string(generatorId);

    // 1. Check disk binary cache first via SculptureProgramSourceFingerprint
    std::optional<ProgramBinaryKey> cacheKey;
    bool loadedFromDiskCache = false;
    try {
        const auto fingerprint = SculptureProgramSourceFingerprint::computeSourceFingerprint(
            source, options, "", kernelFile.parent_path());
        if (fingerprint.complete) {
            cacheKey = ProgramBinaryKey{
                .deviceSignature = queryOpenClDeviceSignature(device).formatDescriptor(),
                .buildOptions = options,
                .kernelTag = kernelTag,
                .sourceHash = fingerprint.digest
            };
        }
        if (cacheKey && SculptureProgramSourceFingerprint::hasBinaryOnDisk(
                SculptureProgramSourceFingerprint::kDefaultCacheDir, *cacheKey)) {
            const auto binary = SculptureProgramSourceFingerprint::loadBinaryFromDisk(
                SculptureProgramSourceFingerprint::kDefaultCacheDir, *cacheKey);
            if (!binary.empty()) {
                cl_int binaryStatus = CL_SUCCESS;
                cl_int binRes = CL_SUCCESS;
                const std::size_t length = binary.size();
                const unsigned char* bytes = binary.data();
                cl_program candidate = clCreateProgramWithBinary(
                    context, 1U, &device, &length, &bytes, &binaryStatus, &binRes);
                if (binRes == CL_SUCCESS && binaryStatus == CL_SUCCESS && candidate != nullptr) {
                    if (clBuildProgram(candidate, 1U, &device, options.c_str(), nullptr, nullptr) == CL_SUCCESS) {
                        compiled->program = candidate;
                        loadedFromDiskCache = true;
                        SculptureProgramSourceFingerprint::recordDiskHit(350.0f);
                    } else {
                        clReleaseProgram(candidate);
                    }
                } else if (candidate != nullptr) {
                    clReleaseProgram(candidate);
                }
            }
        }
    } catch (...) {
        cacheKey.reset();
    }

    // 2. On cache miss, compile from source and persist binary to disk
    cl_int result = CL_SUCCESS;
    if (!loadedFromDiskCache) {
        SculptureProgramSourceFingerprint::recordDiskMiss();
        const char* sourcePointer = source.c_str();
        const std::size_t sourceBytes = source.size();
        compiled->program = clCreateProgramWithSource(context, 1U, &sourcePointer, &sourceBytes, &result);
        requireSuccess(result, "clCreateProgramWithSource");

        result = clBuildProgram(compiled->program, 1U, &device, options.c_str(), nullptr, nullptr);
        if (result != CL_SUCCESS) {
            std::size_t logBytes = 0U;
            std::string buildLog = "build log unavailable";
            if (clGetProgramBuildInfo(compiled->program, device, CL_PROGRAM_BUILD_LOG,
                                     0U, nullptr, &logBytes) == CL_SUCCESS && logBytes > 0U) {
                std::vector<char> log(logBytes + 1U, '\0');
                if (clGetProgramBuildInfo(compiled->program, device, CL_PROGRAM_BUILD_LOG,
                                         logBytes, log.data(), nullptr) == CL_SUCCESS) buildLog = log.data();
            }
            throw std::runtime_error("OpenCL world generator " + std::to_string(generatorId)
                + " build failed (" + std::to_string(result) + "):\n" + buildLog);
        }

        try {
            if (cacheKey) {
                const auto compiledBinary = extractOpenClProgramBinary(compiled->program, device);
                if (!compiledBinary.empty()) {
                    (void)SculptureProgramSourceFingerprint::saveBinaryToDisk(
                        SculptureProgramSourceFingerprint::kDefaultCacheDir, *cacheKey, compiledBinary);
                }
            }
        } catch (...) {}
    }

    compiled->kernel = clCreateKernel(compiled->program, "generate_world_instances", &result);
    requireSuccess(result, "clCreateKernel");
    cl_uint argumentCount = 0U;
    requireSuccess(clGetKernelInfo(compiled->kernel, CL_KERNEL_NUM_ARGS,
                                  sizeof(argumentCount), &argumentCount, nullptr), "clGetKernelInfo");
    if (argumentCount != 4U) {
        throw std::runtime_error("world generator " + std::to_string(generatorId)
            + " ABI mismatch: expected four kernel arguments");
    }

    // Optional chained volumetric coupler kernel (present in modular world builds)
    cl_int couplerRes = CL_SUCCESS;
    cl_kernel coupler = clCreateKernel(compiled->program, "couple_world_volumetric_field", &couplerRes);
    if (couplerRes == CL_SUCCESS && coupler != nullptr) {
        compiled->couplerKernel = coupler;
    }

    return compiled;
}

void SculptureWorldOpenClDispatcher::bindWorldKernelArgs(WorldSlot& slot) {
    checkOpenCl(clSetKernelArg(slot.kernel, 0U, sizeof(cl_mem), &slot.programBuffer),
                "clSetKernelArg(world program)");
    checkOpenCl(clSetKernelArg(slot.kernel, 1U, sizeof(cl_mem), &slot.frameBuffer),
                "clSetKernelArg(world frame)");
    checkOpenCl(clSetKernelArg(slot.kernel, 2U, sizeof(cl_mem), &slot.recipeBuffer),
                "clSetKernelArg(world recipe)");
    checkOpenCl(clSetKernelArg(slot.kernel, 3U, sizeof(cl_mem), &slot.outputBuffer),
                "clSetKernelArg(world output)");
}



void SculptureWorldOpenClDispatcher::waitIdle() {
    std::lock_guard<std::mutex> dispatchLock(compileMutex_);
    // Every queued external wait has an already-submitted Vulkan signal:
    // bootstrap uses its import ledger; reuse uses the presentation ticket.
    // Ready-but-unpresented worlds need no final release merely to retire CL.
    checkOpenCl(clFinish(openCl_->queue()), "clFinish(dedicated world producer)");
}

void SculptureWorldOpenClDispatcher::releaseSlotEvents(WorldSlot& slot) noexcept {
    if (slot.firstEvent) clReleaseEvent(std::exchange(slot.firstEvent, nullptr));
    if (slot.kernelEvent) clReleaseEvent(std::exchange(slot.kernelEvent, nullptr));
    if (slot.readEvent) clReleaseEvent(std::exchange(slot.readEvent, nullptr));
}

SculptureWorldOpenClDispatcher::~SculptureWorldOpenClDispatcher() {
    // Nonblocking input writes and the header read borrow WorldSlot storage.
    // Neither releasing cl_mem nor destroying the queue proves those transfers
    // stopped. Fail closed if retirement cannot be established: continuing
    // destruction would let a driver access freed host memory.
    const cl_int retirement = clFinish(openCl_->queue());
    if (retirement != CL_SUCCESS) {
        std::fprintf(stderr, "Fatal: dedicated world queue retirement failed (%d); refusing unsafe teardown\n",
                     retirement);
        std::terminate();
    }
    // Destroying async futures waits for their workers and destroys even an
    // unclaimed RAII result. Do this while this dispatcher's context is alive.
    worldKernelCache_.clear();
    for (auto& slot : worldSlots_) {
        releaseSlotEvents(slot);
        if (slot.programBuffer) clReleaseMemObject(slot.programBuffer);
        if (slot.frameBuffer) clReleaseMemObject(slot.frameBuffer);
        if (slot.recipeBuffer) clReleaseMemObject(slot.recipeBuffer);
        if (slot.couplerConfigBuffer) clReleaseMemObject(slot.couplerConfigBuffer);
        if (slot.outputBuffer) clReleaseMemObject(slot.outputBuffer);
        if (slot.vulkanReleaseSemaphore) openCl_->releaseSemaphore()(slot.vulkanReleaseSemaphore);
        if (slot.openClReadySemaphore) openCl_->releaseSemaphore()(slot.openClReadySemaphore);
    }
}

void SculptureWorldOpenClDispatcher::enqueueWorld(
    WorldSlot& slot,
    const SculptureProgramParams& params,
    const WorldRecipe& recipe,
    float time,
    float morph,
    float deltaSeconds,
    std::uint32_t generation) {
    
    slot.params = gpu_abi::packProgramParams(params);
    slot.frame = gpu_abi::packFrame(time, morph, deltaSeconds, generation);
    slot.recipe = world_gpu_abi::packWorldRecipe(recipe);
    if (!world_gpu_abi::isValid(slot.recipe)) {
        throw std::runtime_error("world recipe ABI validation failed before dispatch");
    }
    // Only the header crosses to the CPU in external-memory mode; in host-staged
    // fallback mode, active instances are also read back into slot.snapshot.
    slot.snapshot.frame = {};
    slot.snapshot.recipe = {};
    
    // [EDUCATIONAL ANNOTATION]
    // JIT Specialization Request
    // Look up or compile a minimal, register-efficient kernel strictly for this generator.
    const CompiledWorldKernel* compiledKernel = getOrCompileWorldKernel(static_cast<std::uint32_t>(recipe.generator));
    if (compiledKernel == nullptr || compiledKernel->kernel == nullptr) {
        // [EDUCATIONAL ANNOTATION]
        // Zero-Stutter OpenCL Asynchronous Swap
        // The requested mathematical domain is currently compiling on a background thread.
        // We abort enqueueing this frame so the dispatcher naturally holds the previous output,
        // preventing UI locks or framerate dips.
        slot.state = SlotState::Free;
        return;
    }
    
    if (slot.outputBufferExternal) {
        if (slot.vulkanReleaseSemaphore == nullptr
            || slot.openClReadySemaphore == nullptr || !openCl_->waitSemaphores()
            || !openCl_->signalSemaphores() || !slot.bootstrap) {
            throw std::runtime_error("world dispatch requires a complete external-memory import");
        }
        const auto bootstrap = slot.bootstrap->status(slot.externalSlotIndex);
        if (bootstrap == WorldBootstrapHandoff::Status::Failed) {
            slot.state = SlotState::Quarantined;
            throw std::runtime_error("world Vulkan bootstrap failed; recreate transport and imports");
        }
        // JIT may warm up before the first frame, but no command may wait on an
        // unsubmitted bootstrap. Returning keeps UI/startup and early teardown live.
        if (bootstrap != WorldBootstrapHandoff::Status::Submitted) return;
    } else if (slot.outputBuffer == nullptr) {
        // Dual-Mode Host-Staged Fallback: Lazily allocate device-local WorldSnapshotGpu buffer
        cl_int allocRes = CL_SUCCESS;
        slot.outputBuffer = clCreateBuffer(
            openCl_->context(),
            CL_MEM_READ_WRITE,
            sizeof(world_gpu_abi::WorldSnapshotGpu),
            nullptr,
            &allocRes);
        checkOpenCl(allocRes, "clCreateBuffer(world host-staged output fallback)");
    }

    // Arguments live on the shared kernel object, not on WorldSlot. Another
    // slot may have rebound this same kernel since our last enqueue. The caller
    // holds compileMutex_ through binding and enqueue, keeping them indivisible.
    slot.kernel = compiledKernel->kernel;
    slot.couplerKernel = compiledKernel->couplerKernel;
    bindWorldKernelArgs(slot);

    // Once a native command is accepted, failure may leave a consumed semaphore
    // or pending host-buffer read. Never make that slot reusable on an exception.
    slot.state = SlotState::Quarantined;
    if (slot.outputBufferExternal) {
        checkOpenCl(openCl_->waitSemaphores()(openCl_->queue(), 1U, &slot.vulkanReleaseSemaphore, nullptr, 0U, nullptr, nullptr),
                    "clEnqueueWaitSemaphoresKHR(world Vulkan release)");
    }
    slot.lastGeneration = generation;

    checkOpenCl(clEnqueueWriteBuffer(openCl_->queue(), slot.programBuffer, CL_FALSE, 0U,
                                     sizeof(slot.params), &slot.params,
                                     0U, nullptr, &slot.firstEvent),
                "clEnqueueWriteBuffer(world program)");
    checkOpenCl(clEnqueueWriteBuffer(openCl_->queue(), slot.frameBuffer, CL_FALSE, 0U,
                                     sizeof(slot.frame), &slot.frame,
                                     0U, nullptr, nullptr),
                "clEnqueueWriteBuffer(world frame)");
    checkOpenCl(clEnqueueWriteBuffer(openCl_->queue(), slot.recipeBuffer, CL_FALSE, 0U,
                                     sizeof(slot.recipe), &slot.recipe,
                                     0U, nullptr, nullptr),
                "clEnqueueWriteBuffer(world recipe)");

    const std::size_t globalSize = std::max<std::size_t>(
        1U, std::min<std::size_t>(recipe.instanceBudget,
                                  world_gpu_abi::kMaxInstanceCount));
    constexpr std::size_t headerBytes = offsetof(world_gpu_abi::WorldSnapshotGpu, instances);
    const std::size_t readbackBytes = slot.outputBufferExternal
        ? headerBytes
        : (headerBytes + globalSize * sizeof(world_gpu_abi::WorldInstanceGpu));
    slot.metrics = {
        .generation = generation,
        .externalSlotIndex = slot.externalSlotIndex,
        .activeInstanceCount = static_cast<std::uint32_t>(globalSize),
        .readbackBytes = static_cast<std::uint32_t>(readbackBytes),
    };
    checkOpenCl(clEnqueueNDRangeKernel(openCl_->queue(), slot.kernel, 1U, nullptr,
                                        &globalSize, nullptr, 0U, nullptr,
                                        &slot.kernelEvent),
                "clEnqueueNDRangeKernel(generate_world_instances)");

    // Chained GPU-Native Volumetric Field Coupling:
    // Executes in-place on `slot.outputBuffer` on the same in-order queue prior
    // to signaling the Vulkan semaphore or reading back the snapshot.
    if (isVolumetricCouplingBound() && slot.couplerKernel != nullptr
        && (volumetricCouplerConfig_.identityAndFlags[2] & volumetric_gpu_abi::CouplerFlag_EnableCoupling) != 0U) {
        slot.couplerConfig = volumetricCouplerConfig_;
        slot.couplerConfig.timingAndProbe[0] = deltaSeconds;
        slot.couplerConfig.timingAndProbe[1] = time;
        checkOpenCl(clEnqueueWriteBuffer(openCl_->queue(), slot.couplerConfigBuffer, CL_FALSE, 0U,
                                         sizeof(slot.couplerConfig), &slot.couplerConfig,
                                         0U, nullptr, nullptr),
                    "clEnqueueWriteBuffer(world coupler config)");
        checkOpenCl(clSetKernelArg(slot.couplerKernel, 0U, sizeof(cl_mem), &slot.outputBuffer),
                    "clSetKernelArg(coupler snapshot)");
        checkOpenCl(clSetKernelArg(slot.couplerKernel, 1U, sizeof(cl_mem), &volumetricVoxelBuffer_),
                    "clSetKernelArg(coupler volume)");
        checkOpenCl(clSetKernelArg(slot.couplerKernel, 2U, sizeof(cl_mem), &volumetricDescriptorBuffer_),
                    "clSetKernelArg(coupler volumeDesc)");
        checkOpenCl(clSetKernelArg(slot.couplerKernel, 3U, sizeof(cl_mem), &slot.couplerConfigBuffer),
                    "clSetKernelArg(coupler config)");
        checkOpenCl(clEnqueueNDRangeKernel(openCl_->queue(), slot.couplerKernel, 1U, nullptr,
                                            &globalSize, nullptr, 0U, nullptr, nullptr),
                    "clEnqueueNDRangeKernel(couple_world_volumetric_field)");
        slot.metrics.gpuVolumetricCoupled = true;
    }

    if (slot.outputBufferExternal) {
        checkOpenCl(openCl_->signalSemaphores()(openCl_->queue(), 1U, &slot.openClReadySemaphore, nullptr, 0U, nullptr, nullptr),
                    "clEnqueueSignalSemaphoresKHR(world OpenCL ready)");
    }

    // The queue is in-order: this event proves kernel, ready signal, and header/snapshot
    // transfer completion. Vulkan learns topology/count from OpenCL's actual
    // result, not from an empty host sidecar or an estimate of the work budget.
    checkOpenCl(clEnqueueReadBuffer(openCl_->queue(), slot.outputBuffer, CL_FALSE,
                                   0U, readbackBytes, &slot.snapshot,
                                   0U, nullptr, &slot.readEvent),
                "clEnqueueReadBuffer(world snapshot)");

    checkOpenCl(clFlush(openCl_->queue()), "clFlush(world producer)");
    slot.state = SlotState::Producing;
}

void SculptureWorldOpenClDispatcher::pollWorldSlots() {
    for (WorldSlot& slot : worldSlots_) {
        if (slot.bootstrap && slot.bootstrap->status(slot.externalSlotIndex)
                                  == WorldBootstrapHandoff::Status::Failed) {
            slot.state = SlotState::Quarantined;
            throw std::runtime_error("world native transport failed; recreate transport and imports");
        }
        if (slot.state == SlotState::AwaitingPresentation) {
            const auto status = presentationHandoff_->status(slot.externalSlotIndex, slot.lastGeneration);
            if (status == WorldPresentationHandoff::Status::Released) {
                slot.state = SlotState::Free;
            } else if (status == WorldPresentationHandoff::Status::Quarantined
                       || status == WorldPresentationHandoff::Status::Unavailable) {
                slot.state = SlotState::Quarantined;
                throw std::runtime_error("world presentation release failed; recreate transport and imports");
            }
            continue;
        }
        if (slot.state != SlotState::Producing) continue;
        cl_event waitEvent = slot.readEvent;
        if (!waitEvent) {
            slot.state = SlotState::Quarantined;
            throw std::runtime_error("OpenCL world submission has no header completion event");
        }
        
        cl_int status = CL_QUEUED;
        const cl_int eventResult = clGetEventInfo(waitEvent, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                                sizeof(status), &status, nullptr);
        if (eventResult != CL_SUCCESS) {
            slot.state = SlotState::Quarantined;
            checkOpenCl(eventResult, "clGetEventInfo(world header event status)");
        }
        if (status < 0) {
            slot.state = SlotState::Quarantined;
            throw std::runtime_error("OpenCL world compute/read completed with error "
                                     + std::to_string(status));
        }
        if (status != CL_COMPLETE) continue;
        if (!world_gpu_abi::isValid(slot.snapshot.frame, slot.snapshot.recipe)
            || slot.snapshot.frame.generation != slot.lastGeneration
            || std::memcmp(&slot.snapshot.recipe, &slot.recipe, sizeof(slot.recipe)) != 0) {
            slot.state = SlotState::Quarantined;
            throw std::runtime_error("OpenCL world completion header does not match its dispatch");
        }
        slot.metrics.activeInstanceCount = slot.snapshot.frame.activeInstanceCount;
        cl_ulong queueBeginNanoseconds = 0U;
        cl_ulong deliveryEndNanoseconds = 0U;
        cl_ulong kernelBeginNanoseconds = 0U;
        cl_ulong kernelEndNanoseconds = 0U;
        cl_ulong readbackBeginNanoseconds = 0U;
        cl_ulong readbackEndNanoseconds = 0U;
        const cl_int beginResult = clGetEventProfilingInfo(
            slot.firstEvent, CL_PROFILING_COMMAND_START,
            sizeof(queueBeginNanoseconds), &queueBeginNanoseconds, nullptr);
        const cl_int deliveryResult = clGetEventProfilingInfo(
            waitEvent, CL_PROFILING_COMMAND_END,
            sizeof(deliveryEndNanoseconds), &deliveryEndNanoseconds, nullptr);
        const cl_int kernelBeginResult = clGetEventProfilingInfo(
            slot.kernelEvent, CL_PROFILING_COMMAND_START,
            sizeof(kernelBeginNanoseconds), &kernelBeginNanoseconds, nullptr);
        const cl_int kernelEndResult = clGetEventProfilingInfo(
            slot.kernelEvent, CL_PROFILING_COMMAND_END,
            sizeof(kernelEndNanoseconds), &kernelEndNanoseconds, nullptr);
        
        cl_int readbackBeginResult = CL_INVALID_EVENT;
        cl_int readbackEndResult = CL_INVALID_EVENT;
        if (slot.readEvent) {
            readbackBeginResult = clGetEventProfilingInfo(
                slot.readEvent, CL_PROFILING_COMMAND_START,
                sizeof(readbackBeginNanoseconds), &readbackBeginNanoseconds, nullptr);
            readbackEndResult = clGetEventProfilingInfo(
                slot.readEvent, CL_PROFILING_COMMAND_END,
                sizeof(readbackEndNanoseconds), &readbackEndNanoseconds, nullptr);
        }
        
        if (beginResult == CL_SUCCESS && deliveryResult == CL_SUCCESS
            && deliveryEndNanoseconds >= queueBeginNanoseconds) {
            slot.metrics.queueToDeliveryMilliseconds = static_cast<float>(
                deliveryEndNanoseconds - queueBeginNanoseconds) * 1.0e-6f;
            worldLastCompletionMilliseconds_ = slot.metrics.queueToDeliveryMilliseconds;
        }
        if (kernelBeginResult == CL_SUCCESS && kernelEndResult == CL_SUCCESS
            && kernelEndNanoseconds >= kernelBeginNanoseconds) {
            slot.metrics.kernelMilliseconds = static_cast<float>(
                kernelEndNanoseconds - kernelBeginNanoseconds) * 1.0e-6f;
        }
        if (readbackBeginResult == CL_SUCCESS && readbackEndResult == CL_SUCCESS
            && readbackEndNanoseconds >= readbackBeginNanoseconds) {
            slot.metrics.readbackMilliseconds = static_cast<float>(
                readbackEndNanoseconds - readbackBeginNanoseconds) * 1.0e-6f;
        }
        releaseSlotEvents(slot);
        slot.state = SlotState::Ready;
    }
}

SculptureWorldOpenClDispatcher::WorldSlot* SculptureWorldOpenClDispatcher::takeReadyWorldForDelivery() noexcept {
    // Keep the currently published world discoverable until Vulkan accepts its
    // handoff. Skipped display frames or inactive routes apply bounded pressure
    // rather than replacing a delivery whose binary signal still needs a reader.
    for (const WorldSlot& slot : worldSlots_) {
        if (slot.state == SlotState::AwaitingPresentation) return nullptr;
    }
    // A ready binary semaphore must be consumed before its slot is recycled.
    // Preserve the bounded backlog rather than silently dropping GPU handoffs.
    WorldSlot* oldest = nullptr;
    for (WorldSlot& slot : worldSlots_) {
        if (slot.state != SlotState::Ready) continue;
        if (oldest == nullptr || generationNewer(
                oldest->lastGeneration, slot.lastGeneration)) {
            oldest = &slot;
        }
    }
    return oldest;
}

std::uint32_t SculptureWorldOpenClDispatcher::pendingSlotCount() const noexcept {
    std::uint32_t count = 0U;
    for (const WorldSlot& slot : worldSlots_) {
        count += (slot.state == SlotState::Producing || slot.state == SlotState::Ready
                  || slot.state == SlotState::AwaitingPresentation) ? 1U : 0U;
    }
    return count;
}

std::uint32_t SculptureWorldOpenClDispatcher::pendingCompileCount() const noexcept {
    std::uint32_t count = 0U;
    for (const auto& [id, entry] : worldKernelCache_) {
        if (entry.pending.valid()
            && entry.pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready) ++count;
    }
    return count;
}

bool SculptureWorldOpenClDispatcher::prewarmWorldKernel(WorldGeneratorKind generator) noexcept {
    if (generator == WorldGeneratorKind::OrbitSamples) return false;
    std::lock_guard<std::mutex> dispatchLock(compileMutex_);
    if (worldKernelSourceCode_.empty()) return false;
    try {
        const CompiledWorldKernel* compiled =
            getOrCompileWorldKernel(static_cast<std::uint32_t>(generator));
        return compiled != nullptr && compiled->kernel != nullptr;
    } catch (...) {
        return false;
    }
}

bool SculptureWorldOpenClDispatcher::isWorldKernelCompiled(WorldGeneratorKind generator) const noexcept {
    std::lock_guard<std::mutex> dispatchLock(compileMutex_);
    const auto it = worldKernelCache_.find(static_cast<std::uint32_t>(generator));
    if (it == worldKernelCache_.end()) return false;
    return it->second.compiled != nullptr && it->second.compiled->kernel != nullptr;
}

std::size_t SculptureWorldOpenClDispatcher::compiledWorldKernelCount() const noexcept {
    std::lock_guard<std::mutex> dispatchLock(compileMutex_);
    std::size_t count = 0U;
    for (const auto& [id, entry] : worldKernelCache_) {
        if (entry.compiled != nullptr && entry.compiled->kernel != nullptr) {
            ++count;
        }
    }
    return count;
}

WorldSubmitResult SculptureWorldOpenClDispatcher::submitWorldAndTakeLatest(
    const SculptureProgramParams& params,
    const WorldRecipe& recipe,
    float time,
    float morph,
    float deltaSeconds,
    std::uint32_t generation) {
    
    std::lock_guard<std::mutex> dispatchLock(compileMutex_);
    WorldSubmitResult result{};
    if (worldKernelSourceCode_.empty() || !recipe.needsDedicatedGenerator()) {
        return result;
    }
    if (std::any_of(worldSlots_.begin(), worldSlots_.end(), [](const WorldSlot& slot) {
            return slot.state == SlotState::Quarantined;
        })) {
        throw std::runtime_error("world dispatch stopped after an external-slot fault; recreate transport and imports");
    }

    pollWorldSlots();
    if (WorldSlot* ready = takeReadyWorldForDelivery()) {
        if (ready->outputBufferExternal) {
            if (!presentationHandoff_->offer(ready->externalSlotIndex, ready->lastGeneration)) {
                ready->state = SlotState::Quarantined;
                throw std::runtime_error("world completion cannot acquire its presentation ticket");
            }
            result.completedTicket = {presentationHandoff_, ready->externalSlotIndex, ready->lastGeneration};
        }
        result.completed = &ready->snapshot;
        result.completedMetrics = &ready->metrics;
    }

    for (WorldSlot& slot : worldSlots_) {
        if (slot.state != SlotState::Free) continue;
        enqueueWorld(slot, params, recipe, time, morph, deltaSeconds, generation);
        result.submitted = (slot.state == SlotState::Producing);
        break;
    }
    result.completionMilliseconds = worldLastCompletionMilliseconds_;
    result.pendingSlots = pendingSlotCount();
    result.pendingCompilations = pendingCompileCount();
    return result;
}

void SculptureWorldOpenClDispatcher::acknowledgeWorldDelivery(
    const world_gpu_abi::WorldSnapshotGpu* snapshot) noexcept {
    if (snapshot == nullptr) return;
    for (WorldSlot& slot : worldSlots_) {
        if (&slot.snapshot == snapshot && slot.state == SlotState::Ready) {
            if (slot.outputBufferExternal) {
                // CPU publication is not GPU release. Only a successful FrameLoop
                // submit callback can make this slot eligible for another dispatch.
                slot.state = SlotState::AwaitingPresentation;
            } else {
                // Host-staged fallback already read back the full WorldSnapshotGpu
                // into slot.snapshot before Ready; release slot immediately upon delivery.
                slot.state = SlotState::Free;
            }
            return;
        }
    }
}



bool SculptureWorldOpenClDispatcher::importExternalOutput(
    std::uint32_t slotIndex, const WorldOpenClImportSlot& worldImport) {
    if (slotIndex >= kSlotCount || !openCl_->context() || !openCl_->device()) return false;
    if (worldImport.slotIndex != slotIndex || !worldImport.bootstrap
        || worldImport.bootstrap->status(slotIndex) != WorldBootstrapHandoff::Status::Pending) return false;
    const auto& importSlot = worldImport.handles;
    if (importSlot.memoryFd < 0 || importSlot.releaseSemaphoreFd < 0
        || importSlot.openClReadySemaphoreFd < 0
        || importSlot.byteSize < sizeof(world_gpu_abi::WorldSnapshotGpu)
        || !openCl_->createSemaphore() || !openCl_->waitSemaphores()
        || !openCl_->signalSemaphores() || !openCl_->releaseSemaphore()) {
        return false;
    }

    WorldSlot& slot = worldSlots_[slotIndex];
    if (slot.state != SlotState::Free || slot.outputBufferExternal) return false;

    cl_mem_properties memoryProperties[] = {
        CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR,
        static_cast<cl_mem_properties>(importSlot.memoryFd), 0};
    const cl_semaphore_properties_khr vulkanProperties[] = {
        CL_SEMAPHORE_TYPE_KHR, CL_SEMAPHORE_TYPE_BINARY_KHR,
        CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR,
        static_cast<cl_semaphore_properties_khr>(importSlot.releaseSemaphoreFd), 0};
    const cl_semaphore_properties_khr readyProperties[] = {
        CL_SEMAPHORE_TYPE_KHR, CL_SEMAPHORE_TYPE_BINARY_KHR,
        CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR,
        static_cast<cl_semaphore_properties_khr>(importSlot.openClReadySemaphoreFd), 0};

    cl_int result = CL_SUCCESS;
    cl_semaphore_khr importedVulkanSemaphore = nullptr;
    cl_semaphore_khr importedReadySemaphore = nullptr;
    cl_mem importedBuffer = clCreateBufferWithProperties(
        openCl_->context(), memoryProperties, CL_MEM_READ_WRITE, importSlot.byteSize, nullptr, &result);
    const auto discardImports = [&] {
        if ((importedReadySemaphore && openCl_->releaseSemaphore()(importedReadySemaphore) != CL_SUCCESS) ||
            (importedVulkanSemaphore && openCl_->releaseSemaphore()(importedVulkanSemaphore) != CL_SUCCESS) ||
            (importedBuffer && clReleaseMemObject(importedBuffer) != CL_SUCCESS)) {
            std::fprintf(stderr, "World import cleanup failed; stopping before exporter destruction\n");
            std::terminate();
        }
    };
    if (result != CL_SUCCESS || importedBuffer == nullptr) {
        discardImports();
        return false;
    }

    if (importSlot.releaseSemaphoreFd >= 0) {
        importedVulkanSemaphore = openCl_->createSemaphore()(openCl_->context(), vulkanProperties, &result);
        if (result != CL_SUCCESS || importedVulkanSemaphore == nullptr) {
            discardImports();
            return false;
        }
    }
    
    if (importSlot.openClReadySemaphoreFd >= 0) {
        importedReadySemaphore = openCl_->createSemaphore()(openCl_->context(), readyProperties, &result);
        if (result != CL_SUCCESS || importedReadySemaphore == nullptr) {
            discardImports();
            return false;
        }
    }

    // Publish the external allocation only after every import succeeded.
    if (slot.outputBuffer) {
        if (clReleaseMemObject(slot.outputBuffer) != CL_SUCCESS) {
            discardImports();
            return false;
        }
    }
    slot.outputBuffer = importedBuffer;
    slot.vulkanReleaseSemaphore = importedVulkanSemaphore;
    slot.openClReadySemaphore = importedReadySemaphore;
    slot.outputBufferExternal = true;
    slot.externalSlotIndex = slotIndex;
    slot.bootstrap = worldImport.bootstrap;

    return true;
}

bool SculptureWorldOpenClDispatcher::clearExternalOutputs() noexcept {
    if (!openCl_) return true;

    // Imports happen only during application setup, before a world dispatch.
    // Still drain defensively so a future caller cannot release an in-flight
    // external allocation while its OpenCL queue retains it.
    try {
        waitIdle();
    } catch (...) {
        return false;
    }

    for (WorldSlot& slot : worldSlots_) {
        if (!slot.outputBufferExternal) continue;
        releaseSlotEvents(slot);
        if (slot.outputBuffer && clReleaseMemObject(slot.outputBuffer) != CL_SUCCESS) return false;
        slot.outputBuffer = nullptr;
        if (slot.vulkanReleaseSemaphore) {
            if (openCl_->releaseSemaphore()(slot.vulkanReleaseSemaphore) != CL_SUCCESS) return false;
            slot.vulkanReleaseSemaphore = nullptr;
        }
        if (slot.openClReadySemaphore) {
            if (openCl_->releaseSemaphore()(slot.openClReadySemaphore) != CL_SUCCESS) return false;
            slot.openClReadySemaphore = nullptr;
        }
        slot.outputBufferExternal = false;
        slot.bootstrap.reset();
        slot.externalSlotIndex = WorldGenerationMetrics::kNoExternalSlot;
        slot.state = SlotState::Free;
    }
    return true;
}
} // namespace epi_sculpt
