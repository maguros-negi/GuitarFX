#pragma once

#include "AudioEffect.h"
#include "BiquadFilter.h"

#include <atomic>
#include <cstdint>

class OverdriveEffect final : public AudioEffect {
public:
    void prepare(double sampleRate, int32_t maxFramesPerBurst) override;
    void reset() override;
    float processSample(float input) noexcept override;

    void setParameters(float drivePercent, float tonePercent, float levelPercent) noexcept;

private:
    void updateSmoothedParameters() noexcept;
    void updateToneFilter() noexcept;
    static float percentToLevelGain(float percent) noexcept;

    std::atomic<float> driveTarget_{35.0f};
    std::atomic<float> toneTarget_{50.0f};
    std::atomic<float> levelTarget_{50.0f};

    float sampleRate_ = 48000.0f;
    float driveCurrent_ = 35.0f;
    float toneCurrent_ = 50.0f;
    float levelCurrent_ = 50.0f;
    float wetMix_ = 0.0f;
    int32_t samplesUntilUpdate_ = 0;

    BiquadFilter dcBlocker_;
    BiquadFilter toneLowPass_;
};
