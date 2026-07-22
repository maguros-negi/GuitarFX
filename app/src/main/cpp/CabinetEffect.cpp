#include "CabinetEffect.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
void lowPass(BiquadFilter& f, float sr, float hz) {
    hz = std::clamp(hz, 100.0f, sr * 0.45f);
    const float w = 2.0f * kPi * hz / sr;
    const float c = std::cos(w), a = std::sin(w) / (2.0f * 0.70710678f);
    const float a0 = 1.0f + a;
    f.setCoefficients((1.0f-c)*0.5f/a0, (1.0f-c)/a0, (1.0f-c)*0.5f/a0, -2.0f*c/a0, (1.0f-a)/a0);
}
void highPass(BiquadFilter& f, float sr, float hz) {
    const float w = 2.0f * kPi * hz / sr;
    const float c = std::cos(w), a = std::sin(w) / (2.0f * 0.70710678f);
    const float a0 = 1.0f + a;
    f.setCoefficients((1.0f+c)*0.5f/a0, -(1.0f+c)/a0, (1.0f+c)*0.5f/a0, -2.0f*c/a0, (1.0f-a)/a0);
}
void peak(BiquadFilter& f, float sr, float hz, float q, float db) {
    const float A=std::pow(10.0f,db/40.0f), w=2.0f*kPi*hz/sr, c=std::cos(w), a=std::sin(w)/(2.0f*q), a0=1.0f+a/A;
    f.setCoefficients((1.0f+a*A)/a0,-2.0f*c/a0,(1.0f-a*A)/a0,-2.0f*c/a0,(1.0f-a/A)/a0);
}
}

void CabinetEffect::prepare(double sampleRate, int32_t) {
    sampleRate_ = static_cast<float>(std::max(1.0, sampleRate));
    appliedModel_ = -1; appliedMic_ = -1.0f; samplesUntilUpdate_ = 0;
    updateFilters(); reset();
}
void CabinetEffect::reset() { highPass_.reset(); lowPass_.reset(); bodyFilter_.reset(); micFilter_.reset(); }
float CabinetEffect::processSample(float input) noexcept {
    if (!isEnabled()) return input;
    if (--samplesUntilUpdate_ <= 0) { samplesUntilUpdate_ = 128; updateFilters(); }
    float signal = highPass_.process(input);
    signal = bodyFilter_.process(signal);
    signal = micFilter_.process(signal);
    return lowPass_.process(signal);
}
void CabinetEffect::setParameters(float cabinetModel, float micPosition) noexcept {
    modelTarget_.store(std::clamp(cabinetModel, 0.0f, 2.0f));
    micTarget_.store(std::clamp(micPosition, 0.0f, 100.0f));
}
void CabinetEffect::updateFilters() noexcept {
    const int32_t model = static_cast<int32_t>(std::round(modelTarget_.load()));
    const float mic = micTarget_.load();
    if (model == appliedModel_ && std::abs(mic - appliedMic_) < 0.1f) return;
    appliedModel_ = model; appliedMic_ = mic;
    const float hp[] = {95.0f, 80.0f, 70.0f};
    const float lp[] = {6200.0f, 5600.0f, 5000.0f};
    const float bodyHz[] = {170.0f, 140.0f, 115.0f};
    highPass(highPass_, sampleRate_, hp[model]);
    lowPass(lowPass_, sampleRate_, lp[model] + (mic - 50.0f) * 20.0f);
    peak(bodyFilter_, sampleRate_, bodyHz[model], 0.9f, 2.0f + model * 1.5f);
    peak(micFilter_, sampleRate_, 3500.0f, 1.0f, (mic - 50.0f) * 0.12f);
}
