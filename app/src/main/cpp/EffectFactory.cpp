#include "EffectFactory.h"

#include "DelayEffect.h"
#include "NoiseGateEffect.h"
#include "OverdriveEffect.h"
#include "ThreeBandEqEffect.h"

std::unique_ptr<AudioEffect> EffectFactory::create(EffectModelId modelId) {
    switch (modelId) {
        case EffectModelId::NoiseGate:
            return std::make_unique<NoiseGateEffect>();
        case EffectModelId::ClassicOverdrive:
            return std::make_unique<OverdriveEffect>();
        case EffectModelId::ThreeBandEq:
            return std::make_unique<ThreeBandEqEffect>();
        case EffectModelId::DigitalDelay:
            return std::make_unique<DelayEffect>();
    }
    return nullptr;
}
