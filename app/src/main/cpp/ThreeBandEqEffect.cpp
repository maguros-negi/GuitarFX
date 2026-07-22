#include "ThreeBandEqEffect.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr int32_t kCoefficientUpdateInterval = 32;
constexpr float kParameterSmoothingSeconds = 0.020f;
}

void ThreeBandEqEffect::prepare(double sampleRate, int32_t) {
    sampleRate_ = static_cast<float>(std::max(1.0, sampleRate));
    lowCurrentDb_ = lowTargetDb_.load(std::memory_order_relaxed);
    midCurrentDb_ = midTargetDb_.load(std::memory_order_relaxed);
    highCurrentDb_ = highTargetDb_.load(std::memory_order_relaxed);
    samplesUntilUpdate_ = 0;
    updateCoefficients();
    reset();
}

void ThreeBandEqEffect::reset() {
    lowShelf_.reset();
    midPeak_.reset();
    highShelf_.reset();
}

float ThreeBandEqEffect::processSample(float input) noexcept {
    if (--samplesUntilUpdate_ <= 0) {
        samplesUntilUpdate_ = kCoefficientUpdateInterval;
        updateSmoothedParameters();
        updateCoefficients();
    }

    float signal = lowShelf_.process(input);
    signal = midPeak_.process(signal);
    signal = highShelf_.process(signal);
    return signal;
}

void ThreeBandEqEffect::setGainsDb(
        float lowDb,
        float midDb,
        float highDb
) noexcept {
    lowTargetDb_.store(std::clamp(lowDb, -12.0f, 12.0f), std::memory_order_relaxed);
    midTargetDb_.store(std::clamp(midDb, -12.0f, 12.0f), std::memory_order_relaxed);
    highTargetDb_.store(std::clamp(highDb, -12.0f, 12.0f), std::memory_order_relaxed);
}

void ThreeBandEqEffect::updateSmoothedParameters() noexcept {
    const float blockSeconds =
            static_cast<float>(kCoefficientUpdateInterval) / sampleRate_;
    const float smoothing =
            1.0f - std::exp(-blockSeconds / kParameterSmoothingSeconds);

    lowCurrentDb_ +=
            (lowTargetDb_.load(std::memory_order_relaxed) - lowCurrentDb_) * smoothing;
    midCurrentDb_ +=
            (midTargetDb_.load(std::memory_order_relaxed) - midCurrentDb_) * smoothing;
    highCurrentDb_ +=
            (highTargetDb_.load(std::memory_order_relaxed) - highCurrentDb_) * smoothing;
}

void ThreeBandEqEffect::updateCoefficients() noexcept {
    configureLowShelf(250.0f, lowCurrentDb_);
    configurePeaking(1000.0f, 0.8f, midCurrentDb_);
    configureHighShelf(2500.0f, highCurrentDb_);
}

void ThreeBandEqEffect::configureLowShelf(
        float frequency,
        float gainDb
) noexcept {
    const float a = std::pow(10.0f, gainDb / 40.0f);
    const float omega = 2.0f * kPi * frequency / sampleRate_;
    const float cosine = std::cos(omega);
    const float sine = std::sin(omega);
    const float alpha = sine * 0.5f * std::sqrt(2.0f);
    const float twoSqrtAAlpha = 2.0f * std::sqrt(a) * alpha;

    const float b0 = a * ((a + 1.0f) - (a - 1.0f) * cosine + twoSqrtAAlpha);
    const float b1 = 2.0f * a * ((a - 1.0f) - (a + 1.0f) * cosine);
    const float b2 = a * ((a + 1.0f) - (a - 1.0f) * cosine - twoSqrtAAlpha);
    const float a0 = (a + 1.0f) + (a - 1.0f) * cosine + twoSqrtAAlpha;
    const float a1 = -2.0f * ((a - 1.0f) + (a + 1.0f) * cosine);
    const float a2 = (a + 1.0f) + (a - 1.0f) * cosine - twoSqrtAAlpha;

    lowShelf_.setCoefficients(
            b0 / a0,
            b1 / a0,
            b2 / a0,
            a1 / a0,
            a2 / a0
    );
}

void ThreeBandEqEffect::configurePeaking(
        float frequency,
        float q,
        float gainDb
) noexcept {
    const float a = std::pow(10.0f, gainDb / 40.0f);
    const float omega = 2.0f * kPi * frequency / sampleRate_;
    const float cosine = std::cos(omega);
    const float alpha = std::sin(omega) / (2.0f * q);

    const float b0 = 1.0f + alpha * a;
    const float b1 = -2.0f * cosine;
    const float b2 = 1.0f - alpha * a;
    const float a0 = 1.0f + alpha / a;
    const float a1 = -2.0f * cosine;
    const float a2 = 1.0f - alpha / a;

    midPeak_.setCoefficients(
            b0 / a0,
            b1 / a0,
            b2 / a0,
            a1 / a0,
            a2 / a0
    );
}

void ThreeBandEqEffect::configureHighShelf(
        float frequency,
        float gainDb
) noexcept {
    const float a = std::pow(10.0f, gainDb / 40.0f);
    const float omega = 2.0f * kPi * frequency / sampleRate_;
    const float cosine = std::cos(omega);
    const float sine = std::sin(omega);
    const float alpha = sine * 0.5f * std::sqrt(2.0f);
    const float twoSqrtAAlpha = 2.0f * std::sqrt(a) * alpha;

    const float b0 = a * ((a + 1.0f) + (a - 1.0f) * cosine + twoSqrtAAlpha);
    const float b1 = -2.0f * a * ((a - 1.0f) + (a + 1.0f) * cosine);
    const float b2 = a * ((a + 1.0f) + (a - 1.0f) * cosine - twoSqrtAAlpha);
    const float a0 = (a + 1.0f) - (a - 1.0f) * cosine + twoSqrtAAlpha;
    const float a1 = 2.0f * ((a - 1.0f) - (a + 1.0f) * cosine);
    const float a2 = (a + 1.0f) - (a - 1.0f) * cosine - twoSqrtAAlpha;

    highShelf_.setCoefficients(
            b0 / a0,
            b1 / a0,
            b2 / a0,
            a1 / a0,
            a2 / a0
    );
}
