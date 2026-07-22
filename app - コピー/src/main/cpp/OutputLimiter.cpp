#include "OutputLimiter.h"

#include <algorithm>
#include <cmath>

void OutputLimiter::prepare(double sampleRate) noexcept {
    sampleRate_ = static_cast<float>(std::max(1.0, sampleRate));
    attackCoefficient_ = 1.0f - std::exp(-1.0f / (0.001f * sampleRate_));
    releaseCoefficient_ = 1.0f - std::exp(-1.0f / (0.080f * sampleRate_));
    reset();
}

void OutputLimiter::reset() noexcept {
    gain_ = 1.0f;
    gainReductionDb_.store(0.0f, std::memory_order_relaxed);
}

void OutputLimiter::setEnabled(bool enabled) noexcept {
    enabled_.store(enabled, std::memory_order_relaxed);
}

bool OutputLimiter::isEnabled() const noexcept {
    return enabled_.load(std::memory_order_relaxed);
}

float OutputLimiter::processSample(float input) noexcept {
    if (!std::isfinite(input)) {
        gain_ = 1.0f;
        gainReductionDb_.store(0.0f, std::memory_order_relaxed);
        return 0.0f;
    }

    if (!isEnabled()) {
        gain_ = 1.0f;
        gainReductionDb_.store(0.0f, std::memory_order_relaxed);
        return input;
    }

    const float absoluteInput = std::abs(input);
    const float targetGain = absoluteInput > kThresholdLinear
            ? kThresholdLinear / absoluteInput
            : 1.0f;
    const float coefficient = targetGain < gain_
            ? attackCoefficient_
            : releaseCoefficient_;
    gain_ += (targetGain - gain_) * coefficient;
    gain_ = std::clamp(gain_, 0.0f, 1.0f);

    const float reductionDb = gain_ < 0.999999f
            ? 20.0f * std::log10(std::max(gain_, 0.000001f))
            : 0.0f;
    gainReductionDb_.store(reductionDb, std::memory_order_relaxed);

    return std::clamp(input * gain_, -1.0f, 1.0f);
}

float OutputLimiter::gainReductionDb() const noexcept {
    return gainReductionDb_.load(std::memory_order_relaxed);
}
