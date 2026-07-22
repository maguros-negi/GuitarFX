#include "EffectFactory.h"

#include "DelayEffect.h"
#include "NoiseGateEffect.h"
#include "OverdriveEffect.h"
#include "ThreeBandEqEffect.h"
#include "DistortionEffect.h"
#include "FuzzEffect.h"
#include "PreampEffect.h"
#include "CabinetEffect.h"

std::unique_ptr<AudioEffect> EffectFactory::create(EffectModelId modelId) {
    switch (modelId) {
        case EffectModelId::NoiseGate:
        case EffectModelId::TightNoiseGate:
            return std::make_unique<NoiseGateEffect>();
        case EffectModelId::ClassicOverdrive:
        case EffectModelId::MidBoostOverdrive:
        case EffectModelId::TransparentOverdrive:
            return std::make_unique<OverdriveEffect>();
        case EffectModelId::ThreeBandEq:
        case EffectModelId::ScoopEq:
            return std::make_unique<ThreeBandEqEffect>();
        case EffectModelId::DigitalDelay:
        case EffectModelId::SlapbackDelay:
        case EffectModelId::AmbientDelay:
            return std::make_unique<DelayEffect>();
        case EffectModelId::Distortion:
        case EffectModelId::ModernDistortion:
            return std::make_unique<DistortionEffect>();
        case EffectModelId::VintageFuzz:
        case EffectModelId::GatedFuzz:
            return std::make_unique<FuzzEffect>();
        case EffectModelId::Preamp:
        case EffectModelId::CleanPreamp:
        case EffectModelId::HighGainPreamp:
            return std::make_unique<PreampEffect>();
        case EffectModelId::Cabinet:
        case EffectModelId::Cabinet1x12:
        case EffectModelId::Cabinet2x12:
        case EffectModelId::Cabinet4x12:
            return std::make_unique<CabinetEffect>();
    }
    return nullptr;
}
