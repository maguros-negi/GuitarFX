#include "DynamicEffectInstance.h"

#include "DelayEffect.h"
#include "NoiseGateEffect.h"
#include "OverdriveEffect.h"
#include "ThreeBandEqEffect.h"
#include "DistortionEffect.h"
#include "FuzzEffect.h"
#include "PreampEffect.h"
#include "CabinetEffect.h"

#include <utility>

DynamicEffectInstance::DynamicEffectInstance(
        std::string instanceId,
        EffectModelId modelId,
        std::unique_ptr<AudioEffect> effect
) : instanceId_(std::move(instanceId)),
    modelId_(modelId),
    effect_(std::move(effect)) {}

void DynamicEffectInstance::prepare(double sampleRate, int32_t maxFramesPerBurst) {
    if (effect_) effect_->prepare(sampleRate, maxFramesPerBurst);
}

void DynamicEffectInstance::reset() noexcept {
    if (effect_) effect_->reset();
}

float DynamicEffectInstance::processSample(float input) noexcept {
    if (!effect_) return input;
    if (!effect_->isEnabled() && !effect_->requiresContinuousProcessing()) return input;
    return effect_->processSample(input);
}

void DynamicEffectInstance::setEnabled(bool enabled) noexcept {
    if (effect_) effect_->setEnabled(enabled);
}

bool DynamicEffectInstance::isEnabled() const noexcept {
    return effect_ && effect_->isEnabled();
}

bool DynamicEffectInstance::setNoiseGateParameters(
        float thresholdDb,
        float attackMs,
        float releaseMs
) noexcept {
    auto* effect = dynamic_cast<NoiseGateEffect*>(effect_.get());
    if (!effect) return false;
    effect->setThresholdDb(thresholdDb);
    effect->setAttackMs(attackMs);
    effect->setReleaseMs(releaseMs);
    return true;
}

bool DynamicEffectInstance::setOverdriveParameters(
        float drive,
        float tone,
        float level
) noexcept {
    auto* effect = dynamic_cast<OverdriveEffect*>(effect_.get());
    if (!effect) return false;
    effect->setParameters(drive, tone, level);
    return true;
}

bool DynamicEffectInstance::setThreeBandEqGains(
        float lowDb,
        float midDb,
        float highDb
) noexcept {
    auto* effect = dynamic_cast<ThreeBandEqEffect*>(effect_.get());
    if (!effect) return false;
    effect->setGainsDb(lowDb, midDb, highDb);
    return true;
}

bool DynamicEffectInstance::setDelayParameters(
        float timeMs,
        float feedback,
        float mix
) noexcept {
    auto* effect = dynamic_cast<DelayEffect*>(effect_.get());
    if (!effect) return false;
    effect->setParameters(timeMs, feedback, mix);
    return true;
}


bool DynamicEffectInstance::setDistortionParameters(float distortion, float bass, float middle, float treble, float level) noexcept {
    auto* effect = dynamic_cast<DistortionEffect*>(effect_.get());
    if (!effect) return false;
    effect->setParameters(distortion, bass, middle, treble, level);
    return true;
}
bool DynamicEffectInstance::setFuzzParameters(float fuzz, float tone, float bias, float level) noexcept {
    auto* effect = dynamic_cast<FuzzEffect*>(effect_.get());
    if (!effect) return false;
    effect->setParameters(fuzz, tone, bias, level);
    return true;
}
bool DynamicEffectInstance::setPreampParameters(float gain, float bass, float middle, float treble, float presence, float master) noexcept {
    auto* effect = dynamic_cast<PreampEffect*>(effect_.get());
    if (!effect) return false;
    effect->setParameters(gain, bass, middle, treble, presence, master);
    return true;
}
bool DynamicEffectInstance::setCabinetParameters(float cabinetModel, float micPosition) noexcept {
    auto* effect = dynamic_cast<CabinetEffect*>(effect_.get());
    if (!effect) return false;
    effect->setParameters(cabinetModel, micPosition);
    return true;
}

const std::string& DynamicEffectInstance::instanceId() const noexcept {
    return instanceId_;
}

EffectModelId DynamicEffectInstance::modelId() const noexcept {
    return modelId_;
}
