#pragma once

#include "AudioEffect.h"
#include "EffectModelId.h"

#include <memory>

class EffectFactory final {
public:
    static std::unique_ptr<AudioEffect> create(EffectModelId modelId);
};
