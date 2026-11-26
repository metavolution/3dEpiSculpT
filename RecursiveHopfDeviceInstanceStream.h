#pragma once

#include <memory>
#include <mgfx/Drawable.h>

namespace mgfx::vk { class Mesh; }

namespace epi_sculpt::recursive_hopf {

struct RecursiveHopfDelivery;
struct RecursiveHopfRenderOptions;
struct RecursiveHopfRenderBatches;
class RecursiveHopfExternalGrammarView;

// Renderer-local expansion of an immutable domain grammar. A qualified resident
// external view binds canonical OpenCL bytes directly; host grammar remains the
// fallback. Presentation settings are separate; InstanceData stays device-local.
class RecursiveHopfDeviceInstanceStream final {
public:
    RecursiveHopfDeviceInstanceStream();
    ~RecursiveHopfDeviceInstanceStream();
    RecursiveHopfDeviceInstanceStream(const RecursiveHopfDeviceInstanceStream&) = delete;
    RecursiveHopfDeviceInstanceStream& operator=(const RecursiveHopfDeviceInstanceStream&) = delete;

    [[nodiscard]] static bool accepts(const RecursiveHopfDelivery& delivery,
                                      const RecursiveHopfRenderOptions& options) noexcept;

    // False means unsupported geometry or bounded resource/admission pressure;
    // the caller can use its existing CPU presentation path. No draws are emitted
    // before the complete derivative and its frame-completion lease are admitted.
    bool tryDraw(mgfx::Drawable& drawable,
                 const RecursiveHopfDelivery& delivery,
                 const RecursiveHopfRenderOptions& options,
                 std::shared_ptr<const mgfx::vk::Mesh> lod0,
                 std::shared_ptr<const mgfx::vk::Mesh> lod1,
                 mgfx::MaterialHandle material,
                 RecursiveHopfRenderBatches& statistics,
                 std::shared_ptr<const RecursiveHopfExternalGrammarView> externalGrammar = {});

private:
    struct Resources;
    std::shared_ptr<Resources> resources_;
};

} // namespace epi_sculpt::recursive_hopf
