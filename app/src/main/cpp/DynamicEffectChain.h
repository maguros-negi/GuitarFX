#pragma once

#include "DynamicEffectInstance.h"
#include "EffectModelId.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class DynamicEffectChain final {
public:
    DynamicEffectChain() = default;
    DynamicEffectChain(const DynamicEffectChain&) = delete;
    DynamicEffectChain& operator=(const DynamicEffectChain&) = delete;
    DynamicEffectChain(DynamicEffectChain&&) noexcept = default;
    DynamicEffectChain& operator=(DynamicEffectChain&&) noexcept = default;

    bool addEffect(std::string instanceId, EffectModelId modelId);
    bool removeEffect(const std::string& instanceId);
    bool moveEffect(const std::string& instanceId, std::size_t targetIndex);
    void clear() noexcept;

    void prepare(double sampleRate, int32_t maxFramesPerBurst);
    void reset() noexcept;
    float processSample(float input) noexcept;

    DynamicEffectInstance* find(const std::string& instanceId) noexcept;
    const DynamicEffectInstance* find(const std::string& instanceId) const noexcept;

    bool setEffectEnabled(const std::string& instanceId, bool enabled) noexcept;
    bool setEffectParameters(
            const std::string& instanceId,
            const std::vector<float>& parameters
    ) noexcept;

    std::size_t size() const noexcept;
    bool isPrepared() const noexcept;
    bool isAnyModelEnabled(EffectModelId modelId) const noexcept;

private:
    std::vector<std::unique_ptr<DynamicEffectInstance>> instances_;
    double sampleRate_ = 0.0;
    int32_t maxFramesPerBurst_ = 0;
    bool prepared_ = false;
};
