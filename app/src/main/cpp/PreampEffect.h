#pragma once

#include "AudioEffect.h"
#include "BiquadFilter.h"

#include <atomic>
#include <cstdint>

class PreampEffect final : public AudioEffect {
public:
    void prepare(double sampleRate, int32_t maxFramesPerBurst) override;
    void reset() override;
    float processSample(float input) noexcept override;
    [[nodiscard]] bool requiresContinuousProcessing() const noexcept override { return true; }

    void setParameters(float gain, float bass, float middle, float treble, float presence, float master) noexcept;

private:
    void updateFilters() noexcept;
    std::atomic<float> gainTarget_{40.0f}, bassTarget_{50.0f}, middleTarget_{50.0f};
    std::atomic<float> trebleTarget_{50.0f}, presenceTarget_{50.0f}, masterTarget_{50.0f};
    float sampleRate_ = 48000.0f;
    float gain_ = 40.0f, bass_ = 50.0f, middle_ = 50.0f;
    float treble_ = 50.0f, presence_ = 50.0f, master_ = 50.0f;
    float wetMix_ = 0.0f;
    int32_t samplesUntilUpdate_ = 0;
    BiquadFilter dcBlocker_, bassFilter_, middleFilter_, trebleFilter_, presenceFilter_;
};
