#pragma once

#include "SculptureCapturePixels.h"
#include "SculptureSequenceRecorder.h"

namespace epi_sculpt {

// Value-owned request intent. A queued readback can complete after the world
// changes; this records what was requested, not presentation confirmation.
struct SequenceCaptureRequest {
    std::uint64_t session = 0U;
    SequenceFrameMetadata metadata{};
};

template<class Record>
bool deliverSequenceCapture(SculptureSequenceRecorder& recorder,
                            const SequenceCaptureRequest& request,
                            const mgfx::vk::CaptureResult& image,
                            Record&& record) {
    // This is a cheap early rejection only. The record callback must forward
    // request.session to recordFrameForSession for the authoritative check.
    if (request.session == 0U || recorder.captureSession() != request.session) return false;
    return withSequenceCapturePixels(image, [&](const auto& pixels, bool isBgra) {
        return record(request.session, pixels.width, pixels.height, pixels.pixels,
                      request.metadata, isBgra);
    });
}

} // namespace epi_sculpt
