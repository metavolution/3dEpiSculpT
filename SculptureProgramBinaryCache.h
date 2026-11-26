#pragma once

#include "SculptureProgramSourceFingerprint.h"

#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <atomic>
#include <thread>
#include <memory>
#include <span>
#include <array>

#ifndef EPI_SCULPT_HAS_OPENCL
#define EPI_SCULPT_HAS_OPENCL 0
#endif

#if EPI_SCULPT_HAS_OPENCL
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#endif

namespace epi_sculpt {

class SculpturePresetLibrary;
struct SculptureProgramParams;

// -----------------------------------------------------------------------------
// Educational Annotation: OPENCL PROGRAM BINARY DISK CACHE & PRE-WARMING
// -----------------------------------------------------------------------------
// Pure source dependency scanning, length-framed FNV-1a fingerprinting, and
// atomic disk binary key/file persistence live in SculptureProgramSourceFingerprint
// (epi_sculpt_kernel_assembly). This class adds live OpenCL cl_program binary
// hydration/extraction and background preset pre-warming.
// -----------------------------------------------------------------------------

class SculptureProgramBinaryCache {
public:
    static constexpr std::string_view kDefaultCacheDir =
        SculptureProgramSourceFingerprint::kDefaultCacheDir;

    // Versioned, framed FNV-1a identity of prefix/source/options and recursively
    // read literal includes. Unsupported or unreadable dependencies fail closed.
    [[nodiscard]] static ProgramSourceFingerprint computeSourceFingerprint(
        std::string_view source,
        std::string_view buildOptions = "",
        std::string_view prefix = "",
        const std::filesystem::path& includeDir = ""
    ) noexcept;

    // Compatibility digest for existing source-change observers. Not evidence
    // of cache eligibility: cache admission must inspect fingerprint.complete.
    [[nodiscard]] static std::uint64_t computeSourceHash(
        std::string_view source,
        std::string_view buildOptions = "",
        std::string_view prefix = "",
        const std::filesystem::path& includeDir = ""
    ) noexcept;

    // Resolves and returns all regular-file headers recursively included by source
    [[nodiscard]] static std::vector<std::filesystem::path> resolveIncludedHeaders(
        std::string_view source,
        std::string_view buildOptions = "",
        const std::filesystem::path& includeDir = ""
    ) noexcept;

    // Checks whether a compiled binary exists on disk for the specified key
    [[nodiscard]] static bool hasBinaryOnDisk(
        const std::filesystem::path& cacheDir,
        const ProgramBinaryKey& key
    ) noexcept;

    // Reads raw binary bytes from disk
    [[nodiscard]] static std::vector<std::uint8_t> loadBinaryFromDisk(
        const std::filesystem::path& cacheDir,
        const ProgramBinaryKey& key
    ) noexcept;

    // Best-effort publication: exclusively reserve scratch storage, check write
    // and close, then rename within the cache filesystem. Concurrent attempts
    // never share scratch files. Failure cleanup touches only owned scratch;
    // a failed rename leaves the existing target intact. No fsync durability.
    static bool saveBinaryToDisk(
        const std::filesystem::path& cacheDir,
        const ProgramBinaryKey& key,
        std::span<const std::uint8_t> binary
    ) noexcept;

    // Retrieves global cache statistics
    [[nodiscard]] static BinaryCacheStats stats() noexcept;

    // Resets cache statistics
    static void resetStats() noexcept;

#if EPI_SCULPT_HAS_OPENCL
    // Extracts device signature (name, vendor, driver version, UUID)
    [[nodiscard]] static DeviceSignature queryDeviceSignature(cl_device_id device);

    // High-level helper: checks disk cache first (clCreateProgramWithBinary);
    // on miss, builds from source (clCreateProgramWithSource) and writes binary to disk.
    // Cache failures are best-effort and fall through to source execution.
    // Every non-null return is caller-owned, including a failed source build:
    // inspect outResult, read its build log if needed, then release the program.
    // Binary extraction/persistence failure never discards a successful build.
    [[nodiscard]] static cl_program buildOrLoadProgram(
        cl_context context,
        cl_device_id device,
        const std::string& source,
        const std::string& buildOptions,
        std::string_view kernelTag,
        const std::filesystem::path& cacheDir = kDefaultCacheDir,
        const std::string& prefixMacros = "",
        cl_int* outResult = nullptr,
        const std::filesystem::path& includeDir = ""
    );

    // Extracts the compiled binary from an existing built cl_program
    [[nodiscard]] static std::vector<std::uint8_t> extractProgramBinary(
        cl_program program,
        cl_device_id device
    );
#endif

    // Background pre-warming controller
    class Prewarmer {
    public:
        Prewarmer() = default;
        ~Prewarmer();

        Prewarmer(const Prewarmer&) = delete;
        Prewarmer& operator=(const Prewarmer&) = delete;

        // Begins background compilation of all presets in library (offline dry-run / hash indexing)
        void start(
            const SculpturePresetLibrary& library,
            const std::string& kernelPath,
            const std::filesystem::path& cacheDir = kDefaultCacheDir
        );

#if EPI_SCULPT_HAS_OPENCL
        // Begins background compilation of all presets in library using active GPU device context
        void start(
            cl_context context,
            cl_device_id device,
            const SculpturePresetLibrary& library,
            const std::string& kernelPath,
            const std::filesystem::path& cacheDir = kDefaultCacheDir
        );
#endif

        // Request cancellation without waiting for a driver compilation.
        void requestStop() noexcept { stopRequested_.store(true, std::memory_order_release); }

        // Stops background pre-warming and waits for worker thread
        void stop() noexcept;

        [[nodiscard]] bool isRunning() const noexcept { return isRunning_.load(std::memory_order_acquire); }
        [[nodiscard]] std::uint32_t completedCount() const noexcept { return completedCount_.load(std::memory_order_relaxed); }
        [[nodiscard]] std::uint32_t totalCount() const noexcept { return totalCount_.load(std::memory_order_relaxed); }

    private:
        std::thread workerThread_{};
        std::atomic<bool> stopRequested_{false};
        std::atomic<bool> isRunning_{false};
        std::atomic<std::uint32_t> completedCount_{0U};
        std::atomic<std::uint32_t> totalCount_{0U};
    };
};

} // namespace epi_sculpt
