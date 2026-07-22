#pragma once

#include "AudioEffect.h"
#include "EffectModelId.h"

#include <cstdint>
#include <memory>
#include <string>

class DynamicEffectInstance final {
public:
    DynamicEffectInstance(
            std::string instanceId,
            EffectModelId modelId,
            std::unique_ptr<AudioEffect> effect
    );

    DynamicEffectInstance(const DynamicEffectInstance&) = delete;
    DynamicEffectInstance& operator=(const DynamicEffectInstance&) = delete;
    DynamicEffectInstance(DynamicEffectInstance&&) noexcept = default;
    DynamicEffectInstance& operator=(DynamicEffectInstance&&) noexcept = default;

    void prepare(double sampleRate, int32_t maxFramesPerBurst);
    void reset() noexcept;
    float processSample(float input) noexcept;

    void setEnabled(bool enabled) noexcept;
    bool isEnabled() const noexcept;

    bool setNoiseGateParameters(float thresholdDb, float attackMs, float releaseMs) noexcept;
    bool setOverdriveParameters(float drive, float tone, float level) noexcept;
    bool setThreeBandEqGains(float lowDb, float midDb, float highDb) noexcept;
    bool setDelayParameters(float timeMs, float feedback, float mix) noexcept;
    bool setDistortionParameters(float distortion, float bass, float middle, float treble, float level) noexcept;
    bool setFuzzParameters(float fuzz, float tone, float bias, float level) noexcept;
    bool setPreampParameters(float gain, float bass, float middle, float treble, float presence, float master) noexcept;
    bool setCabinetParameters(float cabinetModel, float micPosition) noexcept;

    const std::string& instanceId() const noexcept;
    EffectModelId modelId() const noexcept;

private:
    std::string instanceId_;
    EffectModelId modelId_;
    std::unique_ptr<AudioEffect> effect_;
};
