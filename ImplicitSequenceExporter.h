#pragma once
#include <string>
#include <vector>
#include "ImplicitFrameProducer.h"

namespace epi_sculpt {

class ImplicitSequenceExporter {
public:
    struct ExportConfig {
        std::string outputDirectory = "rendered_frames";
        std::uint32_t frameCount = 60;
        float fps = 30.0f;
        bool exportGBufferChannels = false;
    };

    explicit ImplicitSequenceExporter(ImplicitFrameProducer& producer, const ExportConfig& config);
    void runSequence(const SculptureProgramParams& initialParams);

private:
    ImplicitFrameProducer& producer_;
    ExportConfig config_;
};

} // namespace epi_sculpt
