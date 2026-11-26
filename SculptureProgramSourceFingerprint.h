// Pure, device-free OpenCL program source dependency scanner, length-framed
// FNV-1a source fingerprinting, and atomic disk binary cache persistence.
//
// Owned by the renderer/device-free `epi_sculpt_kernel_assembly` layer so
// offline kernel assemblers, source watchers, and cache transaction verifiers
// do not depend on live OpenCL `cl_program` handles or the full runtime.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace epi_sculpt {

struct DeviceSignature {
    std::string deviceName{};
    std::string vendorName{};
    std::string driverVersion{};
    std::string openClVersion{};
    std::array<std::uint8_t, 16> deviceUuid{};
    bool hasUuid = false;

    [[nodiscard]] std::string formatDescriptor() const;
};

struct ProgramBinaryKey {
    std::string deviceSignature{};
    std::string buildOptions{};
    std::string kernelTag{};
    std::uint64_t sourceHash = 0U;

    [[nodiscard]] std::string computeFileName() const;
    [[nodiscard]] std::filesystem::path resolvePath(const std::filesystem::path& cacheDir) const;
};

struct BinaryCacheStats {
    std::uint64_t memoryHits = 0U;
    std::uint64_t diskHits = 0U;
    std::uint64_t diskMisses = 0U;
    std::uint64_t bytesSavedToDisk = 0U;
    std::uint64_t bytesLoadedFromDisk = 0U;
    float totalCompileTimeSavedMs = 0.0f;
    std::uint32_t prewarmedPresetCount = 0U;
};

// A digest alone cannot prove that all compiler inputs were readable/resolved.
// Incomplete evidence is diagnostic only; source compilation remains authoritative.
struct ProgramSourceFingerprint {
    std::uint64_t digest = 0U;
    bool complete = false;
};

class SculptureProgramSourceFingerprint final {
public:
    static constexpr std::string_view kDefaultCacheDir = "cache/clbin";

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

    [[nodiscard]] static BinaryCacheStats stats() noexcept;
    static void resetStats() noexcept;
    static void recordDiskHit(float savedCompileTimeMs = 350.0f) noexcept;
    static void recordDiskMiss() noexcept;
};

} // namespace epi_sculpt
