#pragma once

#include "AudioEffect.h"
#include "BiquadFilter.h"

#include <atomic>
#include <cstdint>

class ThreeBandEqEffect final : public AudioEffect {
public:
    void prepare(double sampleRate, int32_t maxFramesPerBurst) override;
    void reset() override;
    float processSample(float input) noexcept override;

    void setGainsDb(float lowDb, float midDb, float highDb) noexcept;

private:
    void updateSmoothedParameters() noexcept;
    void updateCoefficients() noexcept;
    void configureLowShelf(float frequency, float gainDb) noexcept;
    void configurePeaking(float frequency, float q, float gainDb) noexcept;
    void configureHighShelf(float frequency, float gainDb) noexcept;

    std::atomic<float> lowTargetDb_{0.0f};
    std::atomic<float> midTargetDb_{0.0f};
    std::atomic<float> highTargetDb_{0.0f};

    float sampleRate_ = 48000.0f;
    float lowCurrentDb_ = 0.0f;
    float midCurrentDb_ = 0.0f;
    float highCurrentDb_ = 0.0f;
    int32_t samplesUntilUpdate_ = 0;

    BiquadFilter lowShelf_;
    BiquadFilter midPeak_;
    BiquadFilter highShelf_;
};
