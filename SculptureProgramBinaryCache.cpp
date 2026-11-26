#include "SculptureProgramBinaryCache.h"
#include "SculpturePresetLibrary.h"
#include "SculptureFractalCatalog.h"
#include "SculptureKernelAssembler.h"
#include "SculptureCompactBuild.h"

#include <fstream>
#include <optional>
#include <sstream>
#include <utility>

namespace epi_sculpt {

namespace {

#if EPI_SCULPT_HAS_OPENCL
// A cache candidate remains locally owned until explicitly returned. In
// particular, a non-null binary handle can accompany a per-device error.
class OwnedProgram {
public:
    explicit OwnedProgram(cl_program value) noexcept : value_(value) {}
    ~OwnedProgram() { if (value_) clReleaseProgram(value_); }
    OwnedProgram(const OwnedProgram&) = delete;
    OwnedProgram& operator=(const OwnedProgram&) = delete;
    cl_program get() const noexcept { return value_; }
    cl_program release() noexcept { return std::exchange(value_, nullptr); }
private:
    cl_program value_;
};
#endif

} // namespace

std::vector<std::filesystem::path> SculptureProgramBinaryCache::resolveIncludedHeaders(
    std::string_view source, std::string_view buildOptions,
    const std::filesystem::path& includeDir
) noexcept {
    return SculptureProgramSourceFingerprint::resolveIncludedHeaders(source, buildOptions, includeDir);
}

ProgramSourceFingerprint SculptureProgramBinaryCache::computeSourceFingerprint(
    std::string_view source, std::string_view buildOptions, std::string_view prefix,
    const std::filesystem::path& includeDir
) noexcept {
    return SculptureProgramSourceFingerprint::computeSourceFingerprint(source, buildOptions, prefix, includeDir);
}

std::uint64_t SculptureProgramBinaryCache::computeSourceHash(
    std::string_view source, std::string_view buildOptions, std::string_view prefix,
    const std::filesystem::path& includeDir
) noexcept {
    return SculptureProgramSourceFingerprint::computeSourceHash(source, buildOptions, prefix, includeDir);
}

bool SculptureProgramBinaryCache::hasBinaryOnDisk(
    const std::filesystem::path& cacheDir,
    const ProgramBinaryKey& key
) noexcept {
    return SculptureProgramSourceFingerprint::hasBinaryOnDisk(cacheDir, key);
}

std::vector<std::uint8_t> SculptureProgramBinaryCache::loadBinaryFromDisk(
    const std::filesystem::path& cacheDir,
    const ProgramBinaryKey& key
) noexcept {
    return SculptureProgramSourceFingerprint::loadBinaryFromDisk(cacheDir, key);
}

bool SculptureProgramBinaryCache::saveBinaryToDisk(
    const std::filesystem::path& cacheDir,
    const ProgramBinaryKey& key,
    std::span<const std::uint8_t> binary
) noexcept {
    return SculptureProgramSourceFingerprint::saveBinaryToDisk(cacheDir, key, binary);
}

BinaryCacheStats SculptureProgramBinaryCache::stats() noexcept {
    return SculptureProgramSourceFingerprint::stats();
}

void SculptureProgramBinaryCache::resetStats() noexcept {
    SculptureProgramSourceFingerprint::resetStats();
}


#if EPI_SCULPT_HAS_OPENCL

DeviceSignature SculptureProgramBinaryCache::queryDeviceSignature(cl_device_id device) {
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

std::vector<std::uint8_t> SculptureProgramBinaryCache::extractProgramBinary(
    cl_program program,
    cl_device_id device
) {
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

    // Find matching device index
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

cl_program SculptureProgramBinaryCache::buildOrLoadProgram(
    cl_context context,
    cl_device_id device,
    const std::string& source,
    const std::string& buildOptions,
    std::string_view kernelTag,
    const std::filesystem::path& cacheDir,
    const std::string& prefixMacros,
    cl_int* outResult,
    const std::filesystem::path& includeDir
) {
    if (outResult) *outResult = CL_SUCCESS;
    std::optional<ProgramBinaryKey> key;
    cl_int res = CL_SUCCESS;

    // Cache metadata, lookup and hydration are optional. A filesystem or
    // metadata exception must not prevent the authoritative source build.
    try {
        const auto fingerprint = computeSourceFingerprint(source, buildOptions, prefixMacros, includeDir);
        if (fingerprint.complete) {
            key = ProgramBinaryKey{
                .deviceSignature = queryDeviceSignature(device).formatDescriptor(),
                .buildOptions = buildOptions,
                .kernelTag = std::string(kernelTag),
                .sourceHash = fingerprint.digest
            };
        }
        if (key && hasBinaryOnDisk(cacheDir, *key)) {
            const auto binary = loadBinaryFromDisk(cacheDir, *key);
            if (!binary.empty()) {
                cl_int binaryStatus = CL_SUCCESS;
                const std::size_t length = binary.size();
                const unsigned char* bytes = binary.data();

                OwnedProgram program(clCreateProgramWithBinary(
                    context, 1U, &device, &length, &bytes, &binaryStatus, &res));

                if (res == CL_SUCCESS && binaryStatus == CL_SUCCESS && program.get() != nullptr) {
                    res = clBuildProgram(program.get(), 1U, &device, buildOptions.c_str(), nullptr, nullptr);
                    if (res == CL_SUCCESS) {
                        SculptureProgramSourceFingerprint::recordDiskHit(350.0f);
                        if (outResult) *outResult = CL_SUCCESS;
                        return program.release();
                    }
                }
            }
            // Do not unlink by pathname after rejecting the bytes we read:
            // another process may already have atomically published a newer
            // valid binary there. A successful source build replaces the entry;
            // if it fails, leave the current publisher's artifact untouched.
        }
    } catch (...) {
        // Disable persistence for this attempt too: cache identity may not have
        // been fully established. Local candidate handles have already retired.
        key.reset();
    }

    // 2. SLOW PATH: Compile from Source and Persist Binary
    SculptureProgramSourceFingerprint::recordDiskMiss();

    const char* sources[2] = {prefixMacros.c_str(), source.c_str()};
    const std::size_t sourceLengths[2] = {prefixMacros.size(), source.size()};
    const cl_uint count = prefixMacros.empty() ? 1U : 2U;
    const char** srcPtr = prefixMacros.empty() ? &sources[1] : sources;
    const std::size_t* lenPtr = prefixMacros.empty() ? &sourceLengths[1] : sourceLengths;

    OwnedProgram program(clCreateProgramWithSource(
        context, count, srcPtr, lenPtr, &res));
    if (res != CL_SUCCESS || program.get() == nullptr) {
        if (res == CL_SUCCESS) res = CL_INVALID_PROGRAM;
        if (outResult) *outResult = res;
        return nullptr;
    }

    res = clBuildProgram(program.get(), 1U, &device, buildOptions.c_str(), nullptr, nullptr);
    if (res != CL_SUCCESS) {
        if (outResult) *outResult = res;
        // Compatibility contract: callers own the failed source program so
        // they can inspect its native build log before releasing it.
        return program.release();
    }

    // Persistence is synchronous and best-effort. Even an allocation exception
    // while extracting the binary must not discard a usable OpenCL program.
    try {
        if (key) {
            const auto compiledBinary = extractProgramBinary(program.get(), device);
            if (!compiledBinary.empty()) {
                (void)saveBinaryToDisk(cacheDir, *key, compiledBinary);
            }
        }
    } catch (...) { /* Keep the successfully built program. */ }

    if (outResult) *outResult = CL_SUCCESS;
    return program.release();
}

#endif // EPI_SCULPT_HAS_OPENCL

SculptureProgramBinaryCache::Prewarmer::~Prewarmer() {
    stop();
}

void SculptureProgramBinaryCache::Prewarmer::stop() noexcept {
    requestStop();
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    isRunning_.store(false, std::memory_order_release);
}

void SculptureProgramBinaryCache::Prewarmer::start(
    const SculpturePresetLibrary& library,
    const std::string& kernelPath,
    const std::filesystem::path& cacheDir
) {
    if (isRunning_.load(std::memory_order_acquire)) return;
    stopRequested_.store(false, std::memory_order_release);
    isRunning_.store(true, std::memory_order_release);
    completedCount_.store(0U, std::memory_order_relaxed);
    totalCount_.store(static_cast<std::uint32_t>(library.entries().size()), std::memory_order_relaxed);

    workerThread_ = std::thread([this, &library, kernelPath, cacheDir]() {
        try {
            std::ifstream kernelFile(kernelPath, std::ios::binary);
            if (!kernelFile.is_open()) {
                isRunning_.store(false, std::memory_order_release);
                return;
            }
            std::ostringstream ss;
            ss << kernelFile.rdbuf();
            const std::string source = ss.str();
            const auto parent = std::filesystem::path(kernelPath).parent_path();

            for (const auto& entry : library.entries()) {
                if (stopRequested_.load(std::memory_order_acquire)) break;

                const auto build = SculptureCompactBuild::describe(entry.params, parent);
                const auto& prefix = build.prefix;
                const auto& buildOptions = build.options;

                // Hash key calculation and verification
                const std::uint64_t hash = computeSourceHash(source, buildOptions, prefix, parent);
                ProgramBinaryKey key{
                    .deviceSignature = "offline_prewarm",
                    .buildOptions = buildOptions,
                    .kernelTag = SculptureCompactBuild::kernelTag,
                    .sourceHash = hash
                };

                // Track progress
                completedCount_.fetch_add(1U, std::memory_order_relaxed);
            }
        } catch (...) {
        }
        isRunning_.store(false, std::memory_order_release);
    });
}

#if EPI_SCULPT_HAS_OPENCL
void SculptureProgramBinaryCache::Prewarmer::start(
    cl_context context,
    cl_device_id device,
    const SculpturePresetLibrary& library,
    const std::string& kernelPath,
    const std::filesystem::path& cacheDir
) {
    if (isRunning_.load(std::memory_order_acquire)) return;
    stopRequested_.store(false, std::memory_order_release);
    isRunning_.store(true, std::memory_order_release);
    completedCount_.store(0U, std::memory_order_relaxed);
    totalCount_.store(static_cast<std::uint32_t>(library.entries().size()), std::memory_order_relaxed);

    workerThread_ = std::thread([this, context, device, &library, kernelPath, cacheDir]() {
        try {
            std::ifstream kernelFile(kernelPath, std::ios::binary);
            if (!kernelFile.is_open()) {
                isRunning_.store(false, std::memory_order_release);
                return;
            }
            std::ostringstream ss;
            ss << kernelFile.rdbuf();
            const std::string source = ss.str();
            const auto parent = std::filesystem::path(kernelPath).parent_path();

            for (const auto& entry : library.entries()) {
                if (stopRequested_.load(std::memory_order_acquire)) break;

                const auto build = SculptureCompactBuild::describe(entry.params, parent);
                const auto& prefix = build.prefix;
                const auto& buildOptions = build.options;

                cl_int res = CL_SUCCESS;
                cl_program program = buildOrLoadProgram(
                    context, device, source, buildOptions, SculptureCompactBuild::kernelTag, cacheDir, prefix, &res, parent);
                if (program) {
                    clReleaseProgram(program);
                }

                completedCount_.fetch_add(1U, std::memory_order_relaxed);
            }
        } catch (...) {
        }
        isRunning_.store(false, std::memory_order_release);
    });
}
#endif

} // namespace epi_sculpt
