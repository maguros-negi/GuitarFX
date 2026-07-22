#include "PreampEffect.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr int32_t kUpdateInterval = 32;
void peak(BiquadFilter& f, float sr, float hz, float q, float db) {
    const float a = std::pow(10.0f, db / 40.0f);
    const float w = 2.0f * kPi * hz / sr;
    const float c = std::cos(w);
    const float alpha = std::sin(w) / (2.0f * q);
    const float a0 = 1.0f + alpha / a;
    f.setCoefficients((1.0f + alpha * a) / a0, -2.0f * c / a0,
                      (1.0f - alpha * a) / a0, -2.0f * c / a0,
                      (1.0f - alpha / a) / a0);
}
float toneDb(float value, float range) { return (value - 50.0f) * 0.02f * range; }
}

void PreampEffect::prepare(double sampleRate, int32_t) {
    sampleRate_ = static_cast<float>(std::max(1.0, sampleRate));
    gain_ = gainTarget_.load(); bass_ = bassTarget_.load(); middle_ = middleTarget_.load();
    treble_ = trebleTarget_.load(); presence_ = presenceTarget_.load(); master_ = masterTarget_.load();
    wetMix_ = isEnabled() ? 1.0f : 0.0f;
    samplesUntilUpdate_ = 0;
    const float k = std::tan(kPi * 20.0f / sampleRate_);
    const float norm = 1.0f / (1.0f + k);
    dcBlocker_.setCoefficients(norm, -norm, 0.0f, (k - 1.0f) * norm, 0.0f);
    updateFilters();
    reset();
}

void PreampEffect::reset() {
    dcBlocker_.reset(); bassFilter_.reset(); middleFilter_.reset();
    trebleFilter_.reset(); presenceFilter_.reset();
}

float PreampEffect::processSample(float input) noexcept {
    if (--samplesUntilUpdate_ <= 0) {
        samplesUntilUpdate_ = kUpdateInterval;
        const float c = 0.08f;
        gain_ += (gainTarget_.load() - gain_) * c; bass_ += (bassTarget_.load() - bass_) * c;
        middle_ += (middleTarget_.load() - middle_) * c; treble_ += (trebleTarget_.load() - treble_) * c;
        presence_ += (presenceTarget_.load() - presence_) * c; master_ += (masterTarget_.load() - master_) * c;
        updateFilters();
    }
    const float preGain = std::pow(10.0f, (3.0f + 33.0f * gain_ * 0.01f) / 20.0f);
    float wet = std::tanh(input * preGain);
    wet = dcBlocker_.process(wet);
    wet = bassFilter_.process(wet); wet = middleFilter_.process(wet);
    wet = trebleFilter_.process(wet); wet = presenceFilter_.process(wet);
    const float masterDb = -36.0f + 42.0f * master_ * 0.01f;
    wet *= master_ <= 0.0f ? 0.0f : std::pow(10.0f, masterDb / 20.0f);
    const float target = isEnabled() ? 1.0f : 0.0f;
    wetMix_ += (target - wetMix_) * (1.0f - std::exp(-1.0f / (0.010f * sampleRate_)));
    return input * (1.0f - wetMix_) + wet * wetMix_;
}

void PreampEffect::setParameters(float gain, float bass, float middle, float treble, float presence, float master) noexcept {
    gainTarget_.store(std::clamp(gain, 0.0f, 100.0f)); bassTarget_.store(std::clamp(bass, 0.0f, 100.0f));
    middleTarget_.store(std::clamp(middle, 0.0f, 100.0f)); trebleTarget_.store(std::clamp(treble, 0.0f, 100.0f));
    presenceTarget_.store(std::clamp(presence, 0.0f, 100.0f)); masterTarget_.store(std::clamp(master, 0.0f, 100.0f));
}

void PreampEffect::updateFilters() noexcept {
    peak(bassFilter_, sampleRate_, 110.0f, 0.75f, toneDb(bass_, 12.0f));
    peak(middleFilter_, sampleRate_, 750.0f, 0.85f, toneDb(middle_, 14.0f));
    peak(trebleFilter_, sampleRate_, 3200.0f, 0.75f, toneDb(treble_, 12.0f));
    peak(presenceFilter_, sampleRate_, 5200.0f, 0.9f, toneDb(presence_, 10.0f));
}
