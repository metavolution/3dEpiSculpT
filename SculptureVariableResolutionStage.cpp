/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - VARIABLE-RESOLUTION MULTI-STAGE UPSCALER
 * ============================================================================
 */

#include "SculptureVariableResolutionStage.h"
#include <sstream>
#include <iomanip>

namespace epi_sculpt {

namespace {

inline float lerpFloat(float a, float b, float t) noexcept {
    return a + t * (b - a);
}

inline float smoothstep(float t) noexcept {
    return t * t * (3.0f - 2.0f * t);
}

inline void catmullRomWeights(float t, std::array<float, 4>& w) noexcept {
    const float t2 = t * t;
    const float t3 = t2 * t;
    w[0] = -0.5f * t + t2 - 0.5f * t3;
    w[1] = 1.0f - 2.5f * t2 + 1.5f * t3;
    w[2] = 0.5f * t + 2.0f * t2 - 1.5f * t3;
    w[3] = -0.5f * t2 + 0.5f * t3;
}

inline std::uint32_t toScaleFactor(StageResolutionScale scale) noexcept {
    switch (scale) {
    case StageResolutionScale::Full: return 1U;
    case StageResolutionScale::Half: return 2U;
    case StageResolutionScale::Quarter: return 4U;
    case StageResolutionScale::Eighth: return 8U;
    default: return 1U;
    }
}

} // namespace

SculptureVariableResolutionStage::SculptureVariableResolutionStage(
    std::uint32_t fullWidth,
    std::uint32_t fullHeight,
    StageResolutionScale scale,
    UpsampleFilter filter
) : scale_(scale), filter_(filter) {
    configure(fullWidth, fullHeight, scale);
}

void SculptureVariableResolutionStage::configure(
    std::uint32_t fullWidth,
    std::uint32_t fullHeight,
    StageResolutionScale scale
) {
    fullWidth_ = std::max(1U, fullWidth);
    fullHeight_ = std::max(1U, fullHeight);
    scale_ = scale;
    recomputeDimensions();
}

void SculptureVariableResolutionStage::setResolutionScale(StageResolutionScale scale) {
    if (scale_ != scale) {
        scale_ = scale;
        recomputeDimensions();
    }
}

void SculptureVariableResolutionStage::recomputeDimensions() {
    const std::uint32_t factor = toScaleFactor(scale_);
    auxWidth_ = std::max(1U, fullWidth_ / factor);
    auxHeight_ = std::max(1U, fullHeight_ / factor);

    auxBuffer_.assign(static_cast<std::size_t>(auxWidth_) * auxHeight_ * 4U, 0.0f);
    upsampledBuffer_.assign(static_cast<std::size_t>(fullWidth_) * fullHeight_ * 4U, 0.0f);

    metrics_.fullWidth = fullWidth_;
    metrics_.fullHeight = fullHeight_;
    metrics_.auxWidth = auxWidth_;
    metrics_.auxHeight = auxHeight_;
    metrics_.activeScale = scale_;
    metrics_.pixelSavingsRatio = static_cast<float>(factor * factor);
}

std::array<float, 4> SculptureVariableResolutionStage::sampleNormalized(float u, float v) const noexcept {
    if (auxBuffer_.empty() || auxWidth_ == 0 || auxHeight_ == 0) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }

    // Clamp normalized UV to edge domain
    const float clampedU = std::clamp(u, 0.0f, 1.0f);
    const float clampedV = std::clamp(v, 0.0f, 1.0f);

    const float px = clampedU * static_cast<float>(auxWidth_ - 1U);
    const float py = clampedV * static_cast<float>(auxHeight_ - 1U);

    const auto x0 = static_cast<std::uint32_t>(px);
    const auto y0 = static_cast<std::uint32_t>(py);
    const std::uint32_t x1 = std::min(x0 + 1U, auxWidth_ - 1U);
    const std::uint32_t y1 = std::min(y0 + 1U, auxHeight_ - 1U);

    float fx = px - static_cast<float>(x0);
    float fy = py - static_cast<float>(y0);

    if (filter_ == UpsampleFilter::CatmullRomBicubic) {
        std::array<float, 4> wx{};
        std::array<float, 4> wy{};
        catmullRomWeights(fx, wx);
        catmullRomWeights(fy, wy);

        const std::uint32_t xm1 = (x0 == 0) ? 0U : x0 - 1U;
        const std::uint32_t xp1 = std::min(x0 + 1U, auxWidth_ - 1U);
        const std::uint32_t xp2 = std::min(x0 + 2U, auxWidth_ - 1U);

        const std::uint32_t ym1 = (y0 == 0) ? 0U : y0 - 1U;
        const std::uint32_t yp1 = std::min(y0 + 1U, auxHeight_ - 1U);
        const std::uint32_t yp2 = std::min(y0 + 2U, auxHeight_ - 1U);

        const std::array<std::uint32_t, 4> xs = {xm1, x0, xp1, xp2};
        const std::array<std::uint32_t, 4> ys = {ym1, y0, yp1, yp2};

        std::array<float, 4> result{0.0f, 0.0f, 0.0f, 0.0f};
        for (std::size_t j = 0; j < 4U; ++j) {
            const std::size_t rowOffset = static_cast<std::size_t>(ys[j]) * auxWidth_ * 4U;
            std::array<float, 4> rowVal{0.0f, 0.0f, 0.0f, 0.0f};
            for (std::size_t i = 0; i < 4U; ++i) {
                const std::size_t idx = rowOffset + static_cast<std::size_t>(xs[i]) * 4U;
                for (std::size_t c = 0; c < 4U; ++c) {
                    rowVal[c] += wx[i] * auxBuffer_[idx + c];
                }
            }
            for (std::size_t c = 0; c < 4U; ++c) {
                result[c] += wy[j] * rowVal[c];
            }
        }
        return result;
    }

    if (filter_ == UpsampleFilter::SmoothstepBilinear) {
        fx = smoothstep(fx);
        fy = smoothstep(fy);
    }

    const std::size_t idx00 = (static_cast<std::size_t>(y0) * auxWidth_ + x0) * 4U;
    const std::size_t idx10 = (static_cast<std::size_t>(y0) * auxWidth_ + x1) * 4U;
    const std::size_t idx01 = (static_cast<std::size_t>(y1) * auxWidth_ + x0) * 4U;
    const std::size_t idx11 = (static_cast<std::size_t>(y1) * auxWidth_ + x1) * 4U;

    std::array<float, 4> result{};
    for (std::size_t c = 0; c < 4U; ++c) {
        const float top = lerpFloat(auxBuffer_[idx00 + c], auxBuffer_[idx10 + c], fx);
        const float bot = lerpFloat(auxBuffer_[idx01 + c], auxBuffer_[idx11 + c], fx);
        result[c] = lerpFloat(top, bot, fy);
    }

    return result;
}

void SculptureVariableResolutionStage::executeUpsample() {
    if (auxBuffer_.empty() || fullWidth_ == 0 || fullHeight_ == 0) {
        return;
    }

    const float invW = 1.0f / static_cast<float>(fullWidth_);
    const float invH = 1.0f / static_cast<float>(fullHeight_);

    for (std::uint32_t y = 0; y < fullHeight_; ++y) {
        const float v = (static_cast<float>(y) + 0.5f) * invH;
        const std::size_t rowOffset = static_cast<std::size_t>(y) * fullWidth_ * 4U;

        for (std::uint32_t x = 0; x < fullWidth_; ++x) {
            const float u = (static_cast<float>(x) + 0.5f) * invW;
            const auto sample = sampleNormalized(u, v);
            const std::size_t pxOffset = rowOffset + static_cast<std::size_t>(x) * 4U;

            upsampledBuffer_[pxOffset + 0U] = sample[0];
            upsampledBuffer_[pxOffset + 1U] = sample[1];
            upsampledBuffer_[pxOffset + 2U] = sample[2];
            upsampledBuffer_[pxOffset + 3U] = sample[3];
        }
    }

    ++metrics_.totalUpsampledFrames;
}

void SculptureVariableResolutionStage::compositeOnto(
    const std::vector<float>& carrierFrame,
    std::vector<float>& outDestination,
    float opacity,
    bool isAdditive
) const {
    const std::size_t pixelCount = static_cast<std::size_t>(fullWidth_) * fullHeight_;
    const std::size_t totalFloats = pixelCount * 4U;

    if (carrierFrame.size() < totalFloats || upsampledBuffer_.size() < totalFloats) {
        return;
    }

    if (outDestination.size() != totalFloats) {
        outDestination.resize(totalFloats);
    }

    const float clampedOpacity = std::clamp(opacity, 0.0f, 1.0f);

    for (std::size_t i = 0; i < totalFloats; i += 4U) {
        const float auxR = upsampledBuffer_[i + 0U];
        const float auxG = upsampledBuffer_[i + 1U];
        const float auxB = upsampledBuffer_[i + 2U];
        const float auxA = upsampledBuffer_[i + 3U];

        const float carR = carrierFrame[i + 0U];
        const float carG = carrierFrame[i + 1U];
        const float carB = carrierFrame[i + 2U];
        const float carA = carrierFrame[i + 3U];

        if (isAdditive) {
            // Additive injection: carrier + aux * opacity * alpha
            const float weight = clampedOpacity * auxA;
            outDestination[i + 0U] = carR + auxR * weight;
            outDestination[i + 1U] = carG + auxG * weight;
            outDestination[i + 2U] = carB + auxB * weight;
            outDestination[i + 3U] = std::max(carA, auxA);
        } else {
            // Standard linear alpha blend: lerp(carrier, aux, alpha * opacity)
            const float alpha = std::clamp(auxA * clampedOpacity, 0.0f, 1.0f);
            outDestination[i + 0U] = lerpFloat(carR, auxR, alpha);
            outDestination[i + 1U] = lerpFloat(carG, auxG, alpha);
            outDestination[i + 2U] = lerpFloat(carB, auxB, alpha);
            outDestination[i + 3U] = std::clamp(carA + alpha * (1.0f - carA), 0.0f, 1.0f);
        }
    }
}

const std::vector<float>& SculptureVariableResolutionStage::processAndUpsample(
    const std::vector<float>& inputBuffer,
    std::uint32_t inputWidth,
    std::uint32_t inputHeight,
    std::uint32_t targetFullWidth,
    std::uint32_t targetFullHeight,
    StageResolutionScale scale
) {
    if (inputBuffer.empty() || inputWidth == 0 || inputHeight == 0 || targetFullWidth == 0 || targetFullHeight == 0) {
        return inputBuffer;
    }

    // Fast-path: Native 1:1 resolution requested and matched
    if (scale == StageResolutionScale::Full && inputWidth == targetFullWidth && inputHeight == targetFullHeight) {
        return inputBuffer;
    }

    // Configure internal geometry if resolution parameters changed
    if (fullWidth_ != targetFullWidth || fullHeight_ != targetFullHeight || scale_ != scale) {
        configure(targetFullWidth, targetFullHeight, scale);
    }

    // Case 1: The input buffer is already downscaled to match the auxiliary target dimensions
    if (inputWidth == auxWidth_ && inputHeight == auxHeight_) {
        auxBuffer_ = inputBuffer;
        executeUpsample();
        return upsampledBuffer_;
    }

    // Case 2: The input buffer is at full resolution, but downsampling is enabled for filtering/savings
    if (inputWidth == fullWidth_ && inputHeight == fullHeight_ && (auxWidth_ != fullWidth_ || auxHeight_ != fullHeight_)) {
        // Decimate full resolution input into auxBuffer_ via box area sampling
        const float stepX = static_cast<float>(fullWidth_) / static_cast<float>(auxWidth_);
        const float stepY = static_cast<float>(fullHeight_) / static_cast<float>(auxHeight_);
        for (std::uint32_t ay = 0; ay < auxHeight_; ++ay) {
            const std::uint32_t srcY = std::min(static_cast<std::uint32_t>(ay * stepY), fullHeight_ - 1U);
            const std::size_t auxRow = static_cast<std::size_t>(ay) * auxWidth_ * 4U;
            const std::size_t srcRow = static_cast<std::size_t>(srcY) * fullWidth_ * 4U;
            for (std::uint32_t ax = 0; ax < auxWidth_; ++ax) {
                const std::uint32_t srcX = std::min(static_cast<std::uint32_t>(ax * stepX), fullWidth_ - 1U);
                const std::size_t auxIdx = auxRow + static_cast<std::size_t>(ax) * 4U;
                const std::size_t srcIdx = srcRow + static_cast<std::size_t>(srcX) * 4U;
                auxBuffer_[auxIdx + 0] = inputBuffer[srcIdx + 0];
                auxBuffer_[auxIdx + 1] = inputBuffer[srcIdx + 1];
                auxBuffer_[auxIdx + 2] = inputBuffer[srcIdx + 2];
                auxBuffer_[auxIdx + 3] = inputBuffer[srcIdx + 3];
            }
        }
        executeUpsample();
        return upsampledBuffer_;
    }

    // Fallback: If dimensions do not match standard grid, perform direct upsample
    executeUpsample();
    return upsampledBuffer_;
}

std::string SculptureVariableResolutionStage::formatStatus() const {
    std::ostringstream ss;
    ss << "VariableResolutionStage ["
       << fullWidth_ << "x" << fullHeight_ << " <- "
       << auxWidth_ << "x" << auxHeight_
       << " | Scale: " << name(scale_)
       << " (" << std::fixed << std::setprecision(1) << metrics_.pixelSavingsRatio << "x savings)"
       << " | Filter: " << (filter_ == UpsampleFilter::Bilinear ? "Bilinear" :
                            (filter_ == UpsampleFilter::SmoothstepBilinear ? "Smoothstep" : "CatmullRom"))
       << " | Frames: " << metrics_.totalUpsampledFrames << "]";
    return ss.str();
}

} // namespace epi_sculpt
