#pragma once

#include "AudioEffect.h"

#include <atomic>
#include <cstdint>

class NoiseGateEffect final : public AudioEffect {
public:
    void prepare(double sampleRate, int32_t maxFramesPerBurst) override;
    void reset() override;
    float processSample(float input) noexcept override;

    void setThresholdDb(float db) noexcept;
    void setAttackMs(float ms) noexcept;
    void setReleaseMs(float ms) noexcept;

private:
    static float timeToCoefficient(float milliseconds, float sampleRate) noexcept;

    std::atomic<float> thresholdDb_{-50.0f};
    std::atomic<float> attackMs_{5.0f};
    std::atomic<float> releaseMs_{120.0f};

    float sampleRate_ = 48000.0f;
    float envelope_ = 0.0f;
    float gateGain_ = 0.0f;
};
