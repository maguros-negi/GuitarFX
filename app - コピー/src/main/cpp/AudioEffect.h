#pragma once

#include <atomic>
#include <cstdint>

enum class EffectId : int32_t {
    Gate = 0,
    Drive = 1,
    Eq = 2,
    Delay = 3,
    Count = 4
};

class AudioEffect {
public:
    virtual ~AudioEffect() = default;
    virtual void prepare(double sampleRate, int32_t maxFramesPerBurst) = 0;
    virtual void reset() = 0;
    virtual float processSample(float input) noexcept = 0;

    void setEnabled(bool enabled) noexcept {
        enabled_.store(enabled, std::memory_order_release);
    }

    [[nodiscard]] bool isEnabled() const noexcept {
        return enabled_.load(std::memory_order_acquire);
    }

protected:
    std::atomic<bool> enabled_{false};
};
