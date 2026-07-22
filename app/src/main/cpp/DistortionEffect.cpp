#include "DistortionEffect.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr int32_t kUpdateInterval = 32;
constexpr float kParameterSmoothingSeconds = 0.020f;
constexpr float kBypassFadeSeconds = 0.010f;

void configurePeak(BiquadFilter& filter, float sampleRate, float frequency, float q, float gainDb) {
    const float a = std::pow(10.0f, gainDb / 40.0f);
    const float omega = 2.0f * kPi * frequency / sampleRate;
    const float cosine = std::cos(omega);
    const float alpha = std::sin(omega) / (2.0f * q);
    const float b0 = 1.0f + alpha * a;
    const float b1 = -2.0f * cosine;
    const float b2 = 1.0f - alpha * a;
    const float a0 = 1.0f + alpha / a;
    const float a1 = -2.0f * cosine;
    const float a2 = 1.0f - alpha / a;
    filter.setCoefficients(b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}
}

void DistortionEffect::prepare(double sampleRate, int32_t) {
    sampleRate_ = static_cast<float>(std::max(1.0, sampleRate));
    distortion_ = distortionTarget_.load(std::memory_order_relaxed);
    bass_ = bassTarget_.load(std::memory_order_relaxed);
    middle_ = middleTarget_.load(std::memory_order_relaxed);
    treble_ = trebleTarget_.load(std::memory_order_relaxed);
    level_ = levelTarget_.load(std::memory_order_relaxed);
    wetMix_ = isEnabled() ? 1.0f : 0.0f;
    samplesUntilUpdate_ = 0;

    const float cutoff = 20.0f;
    const float k = std::tan(kPi * cutoff / sampleRate_);
    const float norm = 1.0f / (1.0f + k);
    dcBlocker_.setCoefficients(norm, -norm, 0.0f, (k - 1.0f) * norm, 0.0f);
    updateFilters();
    reset();
}

void DistortionEffect::reset() {
    dcBlocker_.reset();
    bassFilter_.reset();
    middleFilter_.reset();
    trebleFilter_.reset();
}

float DistortionEffect::processSample(float input) noexcept {
    if (--samplesUntilUpdate_ <= 0) {
        samplesUntilUpdate_ = kUpdateInterval;
        updateParameters();
        updateFilters();
    }

    const float amount = distortion_ * 0.01f;
    const float preGain = std::pow(10.0f, (12.0f + 42.0f * amount) / 20.0f);
    const float driven = input * preGain;
    const float clipped = std::atan(driven * 1.8f) * (2.0f / kPi);
    float wet = dcBlocker_.process(clipped);
    wet = bassFilter_.process(wet);
    wet = middleFilter_.process(wet);
    wet = trebleFilter_.process(wet);
    wet *= levelGain(level_);

    const float targetMix = isEnabled() ? 1.0f : 0.0f;
    const float coefficient = 1.0f - std::exp(-1.0f / (kBypassFadeSeconds * sampleRate_));
    wetMix_ += (targetMix - wetMix_) * coefficient;
    return input * (1.0f - wetMix_) + wet * wetMix_;
}

void DistortionEffect::setParameters(
        float distortionPercent,
        float bassPercent,
        float middlePercent,
        float treblePercent,
        float levelPercent
) noexcept {
    distortionTarget_.store(std::clamp(distortionPercent, 0.0f, 100.0f), std::memory_order_relaxed);
    bassTarget_.store(std::clamp(bassPercent, 0.0f, 100.0f), std::memory_order_relaxed);
    middleTarget_.store(std::clamp(middlePercent, 0.0f, 100.0f), std::memory_order_relaxed);
    trebleTarget_.store(std::clamp(treblePercent, 0.0f, 100.0f), std::memory_order_relaxed);
    levelTarget_.store(std::clamp(levelPercent, 0.0f, 100.0f), std::memory_order_relaxed);
}

void DistortionEffect::updateParameters() noexcept {
    const float blockSeconds = static_cast<float>(kUpdateInterval) / sampleRate_;
    const float coefficient = 1.0f - std::exp(-blockSeconds / kParameterSmoothingSeconds);
    distortion_ += (distortionTarget_.load(std::memory_order_relaxed) - distortion_) * coefficient;
    bass_ += (bassTarget_.load(std::memory_order_relaxed) - bass_) * coefficient;
    middle_ += (middleTarget_.load(std::memory_order_relaxed) - middle_) * coefficient;
    treble_ += (trebleTarget_.load(std::memory_order_relaxed) - treble_) * coefficient;
    level_ += (levelTarget_.load(std::memory_order_relaxed) - level_) * coefficient;
}

void DistortionEffect::updateFilters() noexcept {
    configurePeak(bassFilter_, sampleRate_, 120.0f, 0.75f, percentToDb(bass_, 12.0f));
    configurePeak(middleFilter_, sampleRate_, 900.0f, 0.85f, percentToDb(middle_, 12.0f));
    configurePeak(trebleFilter_, sampleRate_, 4200.0f, 0.75f, percentToDb(treble_, 12.0f));
}

float DistortionEffect::percentToDb(float percent, float rangeDb) noexcept {
    return (percent - 50.0f) * 0.02f * rangeDb;
}

float DistortionEffect::levelGain(float percent) noexcept {
    if (percent <= 0.0f) return 0.0f;
    return std::pow(10.0f, (-36.0f + 42.0f * percent * 0.01f) / 20.0f);
}
