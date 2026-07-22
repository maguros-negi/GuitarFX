#pragma once

#include "AudioEffect.h"
#include "BiquadFilter.h"

#include <atomic>
#include <cstdint>

class FuzzEffect final : public AudioEffect {
public:
    void prepare(double sampleRate, int32_t maxFramesPerBurst) override;
    void reset() override;
    float processSample(float input) noexcept override;
    [[nodiscard]] bool requiresContinuousProcessing() const noexcept override { return true; }

    void setParameters(float fuzzPercent, float tonePercent, float biasPercent, float levelPercent) noexcept;

private:
    void updateToneFilter() noexcept;
    static float levelGain(float percent) noexcept;

    std::atomic<float> fuzzTarget_{65.0f};
    std::atomic<float> toneTarget_{50.0f};
    std::atomic<float> biasTarget_{50.0f};
    std::atomic<float> levelTarget_{50.0f};

    float sampleRate_ = 48000.0f;
    float fuzz_ = 65.0f;
    float tone_ = 50.0f;
    float bias_ = 50.0f;
    float level_ = 50.0f;
    float wetMix_ = 0.0f;
    int32_t samplesUntilUpdate_ = 0;
    BiquadFilter dcBlocker_;
    BiquadFilter toneFilter_;
};
