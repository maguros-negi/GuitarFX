#include "OutputLimiter.h"

#include <algorithm>
#include <cmath>

void OutputLimiter::prepare(double sampleRate) noexcept {
    const float validSampleRate = static_cast<float>(std::max(1.0, sampleRate));
    releaseCoefficient_ = 1.0f - std::exp(-1.0f / (0.080f * validSampleRate));
    meterHoldLengthSamples_ = std::max(1, static_cast<int32_t>(validSampleRate * 0.250f));
    reset();
}

void OutputLimiter::reset() noexcept {
    gain_ = 1.0f;
    meterHoldSamples_ = 0;
    displayedGainReductionDb_.store(0.0f, std::memory_order_relaxed);
}

void OutputLimiter::setEnabled(bool enabled) noexcept {
    enabled_.store(enabled, std::memory_order_relaxed);
    if (!enabled) {
        gain_ = 1.0f;
        meterHoldSamples_ = 0;
        displayedGainReductionDb_.store(0.0f, std::memory_order_relaxed);
    }
}

bool OutputLimiter::isEnabled() const noexcept {
    return enabled_.load(std::memory_order_relaxed);
}

float OutputLimiter::processSample(float input) noexcept {
    if (!std::isfinite(input)) {
        return 0.0f;
    }

    if (!isEnabled()) {
        return input;
    }

    const float absoluteInput = std::abs(input);
    const float requiredGain = absoluteInput > kThresholdLinear
            ? kThresholdLinear / absoluteInput
            : 1.0f;

    // No-lookahead limiter: reduce immediately, recover slowly.
    if (requiredGain < gain_) {
        gain_ = requiredGain;
    } else {
        gain_ += (1.0f - gain_) * releaseCoefficient_;
    }
    gain_ = std::clamp(gain_, 0.0f, 1.0f);

    const float currentReductionDb = gain_ < 0.999999f
            ? 20.0f * std::log10(std::max(gain_, 0.000001f))
            : 0.0f;
    float displayed = displayedGainReductionDb_.load(std::memory_order_relaxed);
    if (currentReductionDb < displayed) {
        displayed = currentReductionDb;
        meterHoldSamples_ = meterHoldLengthSamples_;
    } else if (meterHoldSamples_ > 0) {
        --meterHoldSamples_;
    } else {
        displayed += (0.0f - displayed) * releaseCoefficient_;
        if (displayed > -0.01f) displayed = 0.0f;
    }
    displayedGainReductionDb_.store(displayed, std::memory_order_relaxed);

    return std::clamp(input * gain_, -kThresholdLinear, kThresholdLinear);
}

float OutputLimiter::gainReductionDb() const noexcept {
    return displayedGainReductionDb_.load(std::memory_order_relaxed);
}
