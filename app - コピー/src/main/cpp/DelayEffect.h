#pragma once

#include "AudioEffect.h"

#include <atomic>
#include <cstdint>
#include <vector>

class DelayEffect final : public AudioEffect {
public:
    void prepare(double sampleRate, int32_t maxFramesPerBurst) override;
    void reset() override;
    float processSample(float input) noexcept override;

    void setParameters(float timeMs, float feedbackPercent, float mixPercent) noexcept;

private:
    float readDelayedSample(float delaySamples) const noexcept;
    static float softLimit(float value) noexcept;

    std::atomic<float> timeTargetMs_{350.0f};
    std::atomic<float> feedbackTargetPercent_{35.0f};
    std::atomic<float> mixTargetPercent_{25.0f};

    float sampleRate_ = 48000.0f;
    float timeCurrentMs_ = 350.0f;
    float feedbackCurrent_ = 0.35f;
    float mixCurrent_ = 0.25f;
    float enabledMix_ = 0.0f;

    std::vector<float> delayBuffer_;
    int32_t writeIndex_ = 0;
};
