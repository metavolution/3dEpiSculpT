#pragma once

#include "RecursiveHopfTorusGpuAbi.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vulkan/vulkan.h>

namespace mgfx { class Engine; }
namespace mgfx::vk { class Context; class Buffer; }
namespace epi_sculpt { class ExperimentResourceExternalTransport; }

namespace epi_sculpt::recursive_hopf {

class RecursiveHopfOpenClProducer;
struct RecursiveHopfDelivery;

// A selected grammar remains Vulkan-resident across frames. Its binary ready
// semaphore is consumed exactly once, by the transport, never by draw playback.
// The view cannot be constructed by callers or rebound to another publication.
class RecursiveHopfExternalGrammarView final {
public:
    [[nodiscard]] bool matches(const mgfx::vk::Context& context,
                               std::uint32_t sourceGeneration) const noexcept;
    [[nodiscard]] bool matchesDelivery(const mgfx::vk::Context& context,
                                       const RecursiveHopfDelivery& delivery) const noexcept;
    [[nodiscard]] const mgfx::vk::Buffer* buffer() const noexcept;
    [[nodiscard]] const std::string& resourceName() const noexcept { return name_; }
    [[nodiscard]] std::uint64_t retainedBytes() const noexcept;
    [[nodiscard]] std::uint64_t allocationGeneration() const noexcept;
    [[nodiscard]] std::uint64_t handoffSerial() const noexcept { return transfer_; }

private:
    friend class RecursiveHopfExternalGrammarTransport;
    RecursiveHopfExternalGrammarView() = default;
    std::shared_ptr<ExperimentResourceExternalTransport> owner_;
    const mgfx::vk::Context* context_ = nullptr;
    VkDevice device_ = VK_NULL_HANDLE;
    const mgfx::vk::Buffer* buffer_ = nullptr;
    std::string name_;
    std::uint32_t generation_ = 0U;
    std::uint32_t slot_ = 0U;
    std::uint64_t transfer_ = 0U;
    std::uint64_t epoch_ = 0U;
    bool usable_ = false;
    gpu_abi::RecursiveHopfGrammarGpu grammar_{};
};

class RecursiveHopfExternalGrammarTransport final {
public:
    // Owner-thread API: production/publication and draw collection do not race.
    // Context and producer outlive this adapter. Import storage is separately
    // retained through graphics completion; callbacks never borrow the adapter.
    RecursiveHopfExternalGrammarTransport(mgfx::vk::Context& context,
                                          RecursiveHopfOpenClProducer& producer);
    ~RecursiveHopfExternalGrammarTransport();
    RecursiveHopfExternalGrammarTransport(const RecursiveHopfExternalGrammarTransport&) = delete;
    RecursiveHopfExternalGrammarTransport& operator=(const RecursiveHopfExternalGrammarTransport&) = delete;

    // Host control-plane call after accepted production. Copies already
    // evaluated OpenCL bytes, never advances domain time or generation.
    bool publish(const RecursiveHopfDelivery& delivery);

    // Called once during draw collection (identical repeated calls reuse it).
    // A null selection withdraws the route and retires pending/resident reads.
    // A null view selects ordinary host delivery under capability/slot pressure.
    [[nodiscard]] std::shared_ptr<const RecursiveHopfExternalGrammarView>
        prepare(mgfx::Engine& engine, const RecursiveHopfDelivery* selection);
    [[nodiscard]] const std::string& diagnostic() const noexcept { return diagnostic_; }

private:
    struct State;
    bool initialize();
    mgfx::vk::Context* context_ = nullptr;
    RecursiveHopfOpenClProducer* producer_ = nullptr;
    std::shared_ptr<State> state_;
    bool attempted_ = false;
    bool ownsImports_ = false;
    std::string diagnostic_ = "host grammar (external route dormant)";
};

} // namespace epi_sculpt::recursive_hopf
