#include "SculptureProgram.h"
#include "SculptureLearnMode.h"
#include <cstdio>

namespace epi_sculpt::learn_mode {

std::string formatParametricFormula(Stage stage, const SculptureProgramParams& params) {
    char buf[256];
    switch (stage) {
    case Stage::Transform:
        std::snprintf(buf, sizeof(buf),
            "x_{k+1} = R_{SO(4)}(alpha=%.2f, beta=%.2f, theta=%.2f) [x, y, z, w=%.2f]^T + b_k",
            params.rotorAlpha, params.rotorBeta, params.rotorTheta4D, params.latentW);
        return std::string(buf);
    case Stage::Reduce:
        std::snprintf(buf, sizeof(buf),
            "x_{red} = %s(x) [blend=%.2f with %s]",
            name(params.reduction).data(), params.topologyBlend, name(params.reductionB).data());
        return std::string(buf);
    case Stage::FeedbackSample:
        std::snprintf(buf, sizeof(buf),
            "x_{fb} = (1 - %.2f) x_{seed} + %.2f * %.3f * x_{prev}",
            params.temporalFeedbackBlend, params.temporalFeedbackBlend, params.temporalFade);
        return std::string(buf);
    case Stage::Color:
        std::snprintf(buf, sizeof(buf),
            "color = %s(mu, hue=%.2f, speed=%.3f, breath=%.2f)",
            name(params.colorEffect).data(), params.hueRotation, params.hueSpeed, params.chromaBreath);
        return std::string(buf);
    case Stage::Seed:
        std::snprintf(buf, sizeof(buf),
            "W(x) = exp(-||x - x_{seed}||^2 / (2 * %.2f^2)) [%s, alpha=%.2f, freq=%.2f]",
            params.seedWidth, name(params.seedType).data(), params.seedAlpha, params.seedPulseFrequency);
        return std::string(buf);
    case Stage::Blend:
        std::snprintf(buf, sizeof(buf),
            "x_{out} = (1 - alpha) x_{red} + alpha x_{seed}, alpha_max=%.2f",
            params.seedAlpha);
        return std::string(buf);
    }
    return "";
}

} // namespace epi_sculpt::learn_mode
