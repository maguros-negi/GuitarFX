#pragma once

#include "AudioEffect.h"
#include "BiquadFilter.h"

#include <atomic>
#include <cstdint>

class DistortionEffect final : public AudioEffect {
public:
    void prepare(double sampleRate, int32_t maxFramesPerBurst) override;
    void reset() override;
    float processSample(float input) noexcept override;
    [[nodiscard]] bool requiresContinuousProcessing() const noexcept override { return true; }

    void setParameters(
            float distortionPercent,
            float bassPercent,
            float middlePercent,
            float treblePercent,
            float levelPercent
    ) noexcept;

private:
    void updateParameters() noexcept;
    void updateFilters() noexcept;
    static float percentToDb(float percent, float rangeDb) noexcept;
    static float levelGain(float percent) noexcept;

    std::atomic<float> distortionTarget_{55.0f};
    std::atomic<float> bassTarget_{50.0f};
    std::atomic<float> middleTarget_{50.0f};
    std::atomic<float> trebleTarget_{50.0f};
    std::atomic<float> levelTarget_{50.0f};

    float sampleRate_ = 48000.0f;
    float distortion_ = 55.0f;
    float bass_ = 50.0f;
    float middle_ = 50.0f;
    float treble_ = 50.0f;
    float level_ = 50.0f;
    float wetMix_ = 0.0f;
    int32_t samplesUntilUpdate_ = 0;

    BiquadFilter dcBlocker_;
    BiquadFilter bassFilter_;
    BiquadFilter middleFilter_;
    BiquadFilter trebleFilter_;
};
