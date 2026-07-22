#include "DelayEffect.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kMinimumTimeMs = 20.0f;
constexpr float kMaximumTimeMs = 1000.0f;
constexpr float kParameterSmoothingSeconds = 0.030f;
constexpr float kBypassFadeSeconds = 0.010f;
}

void DelayEffect::prepare(double sampleRate, int32_t maxFramesPerBurst) {
    sampleRate_ = static_cast<float>(std::max(1.0, sampleRate));

    // Allocate once before the stream starts. The callback never resizes this buffer.
    const int32_t maximumDelaySamples = static_cast<int32_t>(
            std::ceil(sampleRate_ * kMaximumTimeMs * 0.001f)
    );
    const int32_t safetySamples = std::max(2, maxFramesPerBurst + 2);
    delayBuffer_.assign(maximumDelaySamples + safetySamples, 0.0f);

    timeCurrentMs_ = timeTargetMs_.load(std::memory_order_relaxed);
    feedbackCurrent_ = feedbackTargetPercent_.load(std::memory_order_relaxed) * 0.01f;
    mixCurrent_ = mixTargetPercent_.load(std::memory_order_relaxed) * 0.01f;
    enabledMix_ = isEnabled() ? 1.0f : 0.0f;
    reset();
}

void DelayEffect::reset() {
    std::fill(delayBuffer_.begin(), delayBuffer_.end(), 0.0f);
    writeIndex_ = 0;
}

float DelayEffect::processSample(float input) noexcept {
    if (delayBuffer_.empty()) {
        return input;
    }

    const float parameterCoefficient =
            1.0f - std::exp(-1.0f / (kParameterSmoothingSeconds * sampleRate_));
    timeCurrentMs_ +=
            (timeTargetMs_.load(std::memory_order_relaxed) - timeCurrentMs_) *
            parameterCoefficient;
    feedbackCurrent_ +=
            (feedbackTargetPercent_.load(std::memory_order_relaxed) * 0.01f -
             feedbackCurrent_) * parameterCoefficient;
    mixCurrent_ +=
            (mixTargetPercent_.load(std::memory_order_relaxed) * 0.01f -
             mixCurrent_) * parameterCoefficient;

    const float delaySamples = std::clamp(
            timeCurrentMs_ * 0.001f * sampleRate_,
            1.0f,
            static_cast<float>(delayBuffer_.size() - 2)
    );
    const float delayed = readDelayedSample(delaySamples);

    // Keep the feedback path bounded even with loud boosted signals upstream.
    const float feedbackInput = input + delayed * feedbackCurrent_;
    delayBuffer_[static_cast<std::size_t>(writeIndex_)] = softLimit(feedbackInput);
    writeIndex_ += 1;
    if (writeIndex_ >= static_cast<int32_t>(delayBuffer_.size())) {
        writeIndex_ = 0;
    }

    const float targetEnabledMix = isEnabled() ? 1.0f : 0.0f;
    const float bypassCoefficient =
            1.0f - std::exp(-1.0f / (kBypassFadeSeconds * sampleRate_));
    enabledMix_ += (targetEnabledMix - enabledMix_) * bypassCoefficient;

    const float effectiveWet = mixCurrent_ * enabledMix_;
    return input * (1.0f - effectiveWet) + delayed * effectiveWet;
}

void DelayEffect::setParameters(
        float timeMs,
        float feedbackPercent,
        float mixPercent
) noexcept {
    timeTargetMs_.store(
            std::clamp(timeMs, kMinimumTimeMs, kMaximumTimeMs),
            std::memory_order_relaxed
    );
    feedbackTargetPercent_.store(
            std::clamp(feedbackPercent, 0.0f, 90.0f),
            std::memory_order_relaxed
    );
    mixTargetPercent_.store(
            std::clamp(mixPercent, 0.0f, 100.0f),
            std::memory_order_relaxed
    );
}

float DelayEffect::readDelayedSample(float delaySamples) const noexcept {
    float readPosition = static_cast<float>(writeIndex_) - delaySamples;
    const float bufferSize = static_cast<float>(delayBuffer_.size());
    while (readPosition < 0.0f) {
        readPosition += bufferSize;
    }
    while (readPosition >= bufferSize) {
        readPosition -= bufferSize;
    }

    const int32_t indexA = static_cast<int32_t>(readPosition);
    const int32_t indexB = (indexA + 1) % static_cast<int32_t>(delayBuffer_.size());
    const float fraction = readPosition - static_cast<float>(indexA);
    const float sampleA = delayBuffer_[static_cast<std::size_t>(indexA)];
    const float sampleB = delayBuffer_[static_cast<std::size_t>(indexB)];
    return sampleA + (sampleB - sampleA) * fraction;
}

float DelayEffect::softLimit(float value) noexcept {
    return std::tanh(value);
}
