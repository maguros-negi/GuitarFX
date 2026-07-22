#include "NoiseGateEffect.h"

#include <algorithm>
#include <cmath>

void NoiseGateEffect::prepare(double sampleRate, int32_t) {
    sampleRate_ = static_cast<float>(std::max(1.0, sampleRate));
    reset();
}

void NoiseGateEffect::reset() {
    envelope_ = 0.0f;
    gateGain_ = 0.0f;
}

float NoiseGateEffect::processSample(float input) noexcept {
    const float absoluteInput = std::abs(input);

    // Fast peak detector with a short release avoids reacting to one sample only.
    const float detectorRelease = timeToCoefficient(10.0f, sampleRate_);
    if (absoluteInput > envelope_) {
        envelope_ = absoluteInput;
    } else {
        envelope_ += (absoluteInput - envelope_) * detectorRelease;
    }

    const float thresholdLinear = std::pow(
            10.0f,
            thresholdDb_.load(std::memory_order_relaxed) / 20.0f
    );
    const float targetGain = envelope_ >= thresholdLinear ? 1.0f : 0.0f;
    const float timeMs = targetGain > gateGain_
                         ? attackMs_.load(std::memory_order_relaxed)
                         : releaseMs_.load(std::memory_order_relaxed);
    const float coefficient = timeToCoefficient(timeMs, sampleRate_);
    gateGain_ += (targetGain - gateGain_) * coefficient;

    return input * gateGain_;
}

void NoiseGateEffect::setThresholdDb(float db) noexcept {
    thresholdDb_.store(std::clamp(db, -80.0f, -10.0f), std::memory_order_relaxed);
}

void NoiseGateEffect::setAttackMs(float ms) noexcept {
    attackMs_.store(std::clamp(ms, 1.0f, 100.0f), std::memory_order_relaxed);
}

void NoiseGateEffect::setReleaseMs(float ms) noexcept {
    releaseMs_.store(std::clamp(ms, 10.0f, 1000.0f), std::memory_order_relaxed);
}

float NoiseGateEffect::timeToCoefficient(float milliseconds, float sampleRate) noexcept {
    const float seconds = std::max(0.001f, milliseconds * 0.001f);
    return 1.0f - std::exp(-1.0f / (seconds * std::max(1.0f, sampleRate)));
}
