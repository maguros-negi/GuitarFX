#pragma once

#include "AudioEffect.h"
#include "DelayEffect.h"
#include "NoiseGateEffect.h"
#include "OverdriveEffect.h"
#include "ThreeBandEqEffect.h"

#include <array>
#include <cstdint>

class PassthroughEffect final : public AudioEffect {
public:
    explicit PassthroughEffect(EffectId id) noexcept;
    void prepare(double sampleRate, int32_t maxFramesPerBurst) override;
    void reset() override;
    float processSample(float input) noexcept override;

private:
    EffectId id_;
};

class EffectChain {
public:
    EffectChain() noexcept;
    void prepare(double sampleRate, int32_t maxFramesPerBurst) noexcept;
    void reset() noexcept;
    void setEffectEnabled(EffectId id, bool enabled) noexcept;
    [[nodiscard]] bool isEffectEnabled(EffectId id) const noexcept;
    void setNoiseGateParameters(float thresholdDb, float attackMs, float releaseMs) noexcept;
    void setThreeBandEqGains(float lowDb, float midDb, float highDb) noexcept;
    void setOverdriveParameters(float drive, float tone, float level) noexcept;
    void setDelayParameters(float timeMs, float feedback, float mix) noexcept;
    float processSample(float input) noexcept;

private:
    static constexpr int32_t kEffectCount = static_cast<int32_t>(EffectId::Count);
    static bool isValidEffectId(EffectId id) noexcept;

    NoiseGateEffect gateEffect_;
    OverdriveEffect driveEffect_;
    ThreeBandEqEffect eqEffect_;
    DelayEffect delayEffect_;
    std::array<AudioEffect*, kEffectCount> effects_;
};
