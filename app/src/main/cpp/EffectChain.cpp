#include "EffectChain.h"

#include <cstddef>

PassthroughEffect::PassthroughEffect(EffectId id) noexcept : id_(id) {}
void PassthroughEffect::prepare(double, int32_t) {}
void PassthroughEffect::reset() {}
float PassthroughEffect::processSample(float input) noexcept { return input; }

EffectChain::EffectChain() noexcept
        : effects_{&gateEffect_, &driveEffect_, &eqEffect_, &delayEffect_} {}

void EffectChain::prepare(double sampleRate, int32_t maxFramesPerBurst) noexcept {
    for (auto* effect : effects_) effect->prepare(sampleRate, maxFramesPerBurst);
}

void EffectChain::reset() noexcept {
    for (auto* effect : effects_) effect->reset();
}

void EffectChain::setEffectEnabled(EffectId id, bool enabled) noexcept {
    if (isValidEffectId(id)) effects_[static_cast<std::size_t>(id)]->setEnabled(enabled);
}

bool EffectChain::isEffectEnabled(EffectId id) const noexcept {
    return isValidEffectId(id) && effects_[static_cast<std::size_t>(id)]->isEnabled();
}

void EffectChain::setNoiseGateParameters(float thresholdDb, float attackMs, float releaseMs) noexcept {
    gateEffect_.setThresholdDb(thresholdDb);
    gateEffect_.setAttackMs(attackMs);
    gateEffect_.setReleaseMs(releaseMs);
}

void EffectChain::setThreeBandEqGains(
        float lowDb,
        float midDb,
        float highDb
) noexcept {
    eqEffect_.setGainsDb(lowDb, midDb, highDb);
}

void EffectChain::setOverdriveParameters(
        float drive,
        float tone,
        float level
) noexcept {
    driveEffect_.setParameters(drive, tone, level);
}

void EffectChain::setDelayParameters(
        float timeMs,
        float feedback,
        float mix
) noexcept {
    delayEffect_.setParameters(timeMs, feedback, mix);
}

float EffectChain::processSample(float input) noexcept {
    float signal = input;
    for (auto* effect : effects_) {
        if (
                effect == &driveEffect_ ||
                effect == &delayEffect_ ||
                effect->isEnabled()
                ) {
            signal = effect->processSample(signal);
        }
    }
    return signal;
}

bool EffectChain::isValidEffectId(EffectId id) noexcept {
    const auto value = static_cast<int32_t>(id);
    return value >= 0 && value < kEffectCount;
}
