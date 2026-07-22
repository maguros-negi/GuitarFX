#pragma once

#include "AudioEffect.h"
#include "BiquadFilter.h"

#include <atomic>
#include <cstdint>

class CabinetEffect final : public AudioEffect {
public:
    void prepare(double sampleRate, int32_t maxFramesPerBurst) override;
    void reset() override;
    float processSample(float input) noexcept override;
    void setParameters(float cabinetModel, float micPosition) noexcept;

private:
    void updateFilters() noexcept;
    std::atomic<float> modelTarget_{0.0f};
    std::atomic<float> micTarget_{50.0f};
    float sampleRate_ = 48000.0f;
    int32_t appliedModel_ = -1;
    float appliedMic_ = -1.0f;
    int32_t samplesUntilUpdate_ = 0;
    BiquadFilter highPass_, lowPass_, bodyFilter_, micFilter_;
};
