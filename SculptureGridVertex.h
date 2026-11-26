#pragma once
#ifdef __cplusplus
#include <cstdint>
namespace epi_sculpt {
    // ABI matcher for mgfx::vk::Vertex
    struct GridVertex {
        float pos[3];
        float norm[3];
        float uv[2];
        float color[4];
        float tangent[4];
        std::uint32_t joints[4];
        float weights[4];
    };
    static_assert(sizeof(GridVertex) == 96, "GridVertex must match mgfx::vk::Vertex (96 bytes)");
}
#endif
