#include "FuzzEffect.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr int32_t kUpdateInterval = 32;
}

void FuzzEffect::prepare(double sampleRate, int32_t) {
    sampleRate_ = static_cast<float>(std::max(1.0, sampleRate));
    fuzz_ = fuzzTarget_.load(std::memory_order_relaxed);
    tone_ = toneTarget_.load(std::memory_order_relaxed);
    bias_ = biasTarget_.load(std::memory_order_relaxed);
    level_ = levelTarget_.load(std::memory_order_relaxed);
    wetMix_ = isEnabled() ? 1.0f : 0.0f;
    samplesUntilUpdate_ = 0;

    const float cutoff = 18.0f;
    const float k = std::tan(kPi * cutoff / sampleRate_);
    const float norm = 1.0f / (1.0f + k);
    dcBlocker_.setCoefficients(norm, -norm, 0.0f, (k - 1.0f) * norm, 0.0f);
    updateToneFilter();
    reset();
}

void FuzzEffect::reset() {
    dcBlocker_.reset();
    toneFilter_.reset();
}

float FuzzEffect::processSample(float input) noexcept {
    if (--samplesUntilUpdate_ <= 0) {
        samplesUntilUpdate_ = kUpdateInterval;
        const float c = 0.08f;
        fuzz_ += (fuzzTarget_.load(std::memory_order_relaxed) - fuzz_) * c;
        tone_ += (toneTarget_.load(std::memory_order_relaxed) - tone_) * c;
        bias_ += (biasTarget_.load(std::memory_order_relaxed) - bias_) * c;
        level_ += (levelTarget_.load(std::memory_order_relaxed) - level_) * c;
        updateToneFilter();
    }

    const float gain = 8.0f + 72.0f * fuzz_ * 0.01f;
    const float bias = (bias_ - 50.0f) * 0.012f;
    const float shifted = input * gain + bias;
    const float positive = std::tanh(shifted * 1.8f);
    const float negative = std::tanh(shifted * (0.8f + bias_ * 0.012f));
    const float shaped = shifted >= 0.0f ? positive : negative;
    float wet = toneFilter_.process(dcBlocker_.process(shaped));
    wet *= levelGain(level_);

    const float target = isEnabled() ? 1.0f : 0.0f;
    const float coefficient = 1.0f - std::exp(-1.0f / (0.010f * sampleRate_));
    wetMix_ += (target - wetMix_) * coefficient;
    return input * (1.0f - wetMix_) + wet * wetMix_;
}

void FuzzEffect::setParameters(
        float fuzzPercent,
        float tonePercent,
        float biasPercent,
        float levelPercent
) noexcept {
    fuzzTarget_.store(std::clamp(fuzzPercent, 0.0f, 100.0f), std::memory_order_relaxed);
    toneTarget_.store(std::clamp(tonePercent, 0.0f, 100.0f), std::memory_order_relaxed);
    biasTarget_.store(std::clamp(biasPercent, 0.0f, 100.0f), std::memory_order_relaxed);
    levelTarget_.store(std::clamp(levelPercent, 0.0f, 100.0f), std::memory_order_relaxed);
}

void FuzzEffect::updateToneFilter() noexcept {
    const float cutoff = 500.0f * std::pow(9000.0f / 500.0f, tone_ * 0.01f);
    const float omega = 2.0f * kPi * cutoff / sampleRate_;
    const float cosine = std::cos(omega);
    const float alpha = std::sin(omega) / (2.0f * 0.70710678f);
    const float b0 = (1.0f - cosine) * 0.5f;
    const float b1 = 1.0f - cosine;
    const float b2 = b0;
    const float a0 = 1.0f + alpha;
    toneFilter_.setCoefficients(b0 / a0, b1 / a0, b2 / a0, -2.0f * cosine / a0, (1.0f - alpha) / a0);
}

float FuzzEffect::levelGain(float percent) noexcept {
    if (percent <= 0.0f) return 0.0f;
    return std::pow(10.0f, (-36.0f + 42.0f * percent * 0.01f) / 20.0f);
}
