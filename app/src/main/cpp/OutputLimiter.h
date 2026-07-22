#pragma once

#include <atomic>

class OutputLimiter final {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setEnabled(bool enabled) noexcept;
    [[nodiscard]] bool isEnabled() const noexcept;
    float processSample(float input) noexcept;
    [[nodiscard]] float gainReductionDb() const noexcept;

private:
    static constexpr float kThresholdLinear = 0.89125094f; // -1.0 dBFS
    std::atomic<bool> enabled_{true};
    std::atomic<float> displayedGainReductionDb_{0.0f};
    float gain_ = 1.0f;
    float releaseCoefficient_ = 1.0f;
    int32_t meterHoldSamples_ = 0;
    int32_t meterHoldLengthSamples_ = 1;
};
