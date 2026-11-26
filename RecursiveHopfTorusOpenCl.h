// OpenCL domain producer for the Recursive Counter-Winding Hopf Torus.
//
// Evaluates the finite grammar ABI on OpenCL when available and provides
// a deterministic CPU reference fallback for device-free testing and recovery.

#pragma once

#include "RecursiveHopfTorusGpuAbi.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace epi_sculpt { struct ExperimentOpenClImportSlot; }

namespace epi_sculpt::recursive_hopf {

struct RecursiveHopfDelivery {
    std::uint32_t generation = 0U;
    bool producedByOpenCl = false;
    float evaluationMilliseconds = 0.0f;
    Grammar grammar = Grammar::prepare(Recipe{}, 0.0);
    gpu_abi::RecursiveHopfGrammarGpu gpuGrammar{};
};

struct RecursiveHopfExpandedDelivery {
    std::uint32_t generation = 0U;
    bool producedByOpenCl = false;
    float evaluationMilliseconds = 0.0f;
    Grammar grammar = Grammar::prepare(Recipe{}, 0.0);
    gpu_abi::RecursiveHopfGrammarGpu gpuGrammar{};
    std::vector<gpu_abi::RecursiveHopfInstanceGpu> gpuInstances{};
};

class RecursiveHopfOpenClProducer final {
public:
    using DeviceUuid = std::array<std::uint8_t, 16>;
    enum class ExternalGrammarCopyResult : std::uint8_t { Rejected, Queued, Failed };

    struct Config {
        std::filesystem::path kernelSourcePath{};
        bool preferOpenCl = true;
        // Portable identity only: domain production never depends on Vulkan
        // handles. A requested UUID must be nonzero and verified exactly;
        // absence of a match selects the explicit CPU reference fallback.
        std::optional<DeviceUuid> requiredDeviceUuid{};
    };

    RecursiveHopfOpenClProducer();
    explicit RecursiveHopfOpenClProducer(const Config& config);
    ~RecursiveHopfOpenClProducer();

    RecursiveHopfOpenClProducer(const RecursiveHopfOpenClProducer&) = delete;
    RecursiveHopfOpenClProducer& operator=(const RecursiveHopfOpenClProducer&) = delete;

    [[nodiscard]] bool openClAvailable() const noexcept;
    [[nodiscard]] std::string deviceDescription() const;
    [[nodiscard]] std::optional<DeviceUuid> selectedDeviceUuid() const noexcept;
    [[nodiscard]] bool requiredDeviceUuidMatched() const noexcept;
    [[nodiscard]] std::string deviceSelectionDiagnostic() const;

    // Transport-only operations: no new mathematical generation is evaluated.
    // Imports consume only successful FD handles (set to -1); the caller closes
    // every remaining FD. One native transport session owns the complete set.
    [[nodiscard]] bool supportsExternalGrammar() const noexcept;
    [[nodiscard]] bool hasExternalGrammarImports() const noexcept;

private:
    friend class RecursiveHopfExternalGrammarTransport;
    [[nodiscard]] bool importExternalGrammar(ExperimentOpenClImportSlot& slot);
    void clearExternalGrammarImports() noexcept;
    [[nodiscard]] ExternalGrammarCopyResult copyExternalGrammar(
        std::uint32_t slotIndex, const RecursiveHopfDelivery& source) noexcept;

public:
    [[nodiscard]] RecursiveHopfDelivery produce(
        const Recipe& recipe,
        double domainTime);

    [[nodiscard]] RecursiveHopfExpandedDelivery produceExpandedInstances(
        const Recipe& recipe,
        double domainTime,
        const TraversalBudget& budget = {});

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace epi_sculpt::recursive_hopf
