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
    static constexpr float kThresholdDb = -1.0f;
    static constexpr float kThresholdLinear = 0.89125094f;
    std::atomic<bool> enabled_{true};
    std::atomic<float> gainReductionDb_{0.0f};
    float sampleRate_ = 48000.0f;
    float gain_ = 1.0f;
    float attackCoefficient_ = 1.0f;
    float releaseCoefficient_ = 1.0f;
};
