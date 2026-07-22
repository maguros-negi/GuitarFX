#include "DynamicEffectChain.h"

#include "EffectFactory.h"

#include <algorithm>
#include <utility>

bool DynamicEffectChain::addEffect(
        std::string instanceId,
        EffectModelId modelId
) {
    if (instanceId.empty() || find(instanceId) != nullptr) return false;
    auto effect = EffectFactory::create(modelId);
    if (!effect) return false;
    auto instance = std::make_unique<DynamicEffectInstance>(
            std::move(instanceId),
            modelId,
            std::move(effect)
    );
    if (prepared_) instance->prepare(sampleRate_, maxFramesPerBurst_);
    instances_.push_back(std::move(instance));
    return true;
}

bool DynamicEffectChain::removeEffect(const std::string& instanceId) {
    const auto iterator = std::find_if(
            instances_.begin(),
            instances_.end(),
            [&instanceId](const auto& instance) {
                return instance->instanceId() == instanceId;
            }
    );
    if (iterator == instances_.end()) return false;
    instances_.erase(iterator);
    return true;
}

bool DynamicEffectChain::moveEffect(
        const std::string& instanceId,
        std::size_t targetIndex
) {
    if (instances_.empty()) return false;
    const auto iterator = std::find_if(
            instances_.begin(),
            instances_.end(),
            [&instanceId](const auto& instance) {
                return instance->instanceId() == instanceId;
            }
    );
    if (iterator == instances_.end()) return false;
    const std::size_t sourceIndex = static_cast<std::size_t>(
            std::distance(instances_.begin(), iterator)
    );
    targetIndex = std::min(targetIndex, instances_.size() - 1);
    if (sourceIndex == targetIndex) return true;
    auto moving = std::move(instances_[sourceIndex]);
    instances_.erase(instances_.begin() + static_cast<std::ptrdiff_t>(sourceIndex));
    instances_.insert(
            instances_.begin() + static_cast<std::ptrdiff_t>(targetIndex),
            std::move(moving)
    );
    return true;
}

void DynamicEffectChain::clear() noexcept {
    instances_.clear();
    prepared_ = false;
    sampleRate_ = 0.0;
    maxFramesPerBurst_ = 0;
}

void DynamicEffectChain::prepare(double sampleRate, int32_t maxFramesPerBurst) {
    sampleRate_ = sampleRate;
    maxFramesPerBurst_ = maxFramesPerBurst;
    for (auto& instance : instances_) {
        instance->prepare(sampleRate, maxFramesPerBurst);
    }
    prepared_ = true;
}

void DynamicEffectChain::reset() noexcept {
    for (auto& instance : instances_) instance->reset();
}

float DynamicEffectChain::processSample(float input) noexcept {
    float signal = input;
    for (auto& instance : instances_) signal = instance->processSample(signal);
    return signal;
}

DynamicEffectInstance* DynamicEffectChain::find(const std::string& instanceId) noexcept {
    const auto iterator = std::find_if(
            instances_.begin(),
            instances_.end(),
            [&instanceId](const auto& instance) {
                return instance->instanceId() == instanceId;
            }
    );
    return iterator == instances_.end() ? nullptr : iterator->get();
}

const DynamicEffectInstance* DynamicEffectChain::find(
        const std::string& instanceId
) const noexcept {
    const auto iterator = std::find_if(
            instances_.begin(),
            instances_.end(),
            [&instanceId](const auto& instance) {
                return instance->instanceId() == instanceId;
            }
    );
    return iterator == instances_.end() ? nullptr : iterator->get();
}

bool DynamicEffectChain::setEffectEnabled(
        const std::string& instanceId,
        bool enabled
) noexcept {
    auto* instance = find(instanceId);
    if (!instance) return false;
    instance->setEnabled(enabled);
    return true;
}

bool DynamicEffectChain::setEffectParameters(
        const std::string& instanceId,
        const std::vector<float>& parameters
) noexcept {
    auto* instance = find(instanceId);
    if (!instance) return false;

    switch (instance->modelId()) {
        case EffectModelId::NoiseGate:
            return parameters.size() >= 3 && instance->setNoiseGateParameters(
                    parameters[0], parameters[1], parameters[2]
            );
        case EffectModelId::ClassicOverdrive:
            return parameters.size() >= 3 && instance->setOverdriveParameters(
                    parameters[0], parameters[1], parameters[2]
            );
        case EffectModelId::ThreeBandEq:
            return parameters.size() >= 3 && instance->setThreeBandEqGains(
                    parameters[0], parameters[1], parameters[2]
            );
        case EffectModelId::DigitalDelay:
            return parameters.size() >= 3 && instance->setDelayParameters(
                    parameters[0], parameters[1], parameters[2]
            );
    }
    return false;
}

std::size_t DynamicEffectChain::size() const noexcept {
    return instances_.size();
}

bool DynamicEffectChain::isPrepared() const noexcept {
    return prepared_;
}

bool DynamicEffectChain::isAnyModelEnabled(EffectModelId modelId) const noexcept {
    return std::any_of(
            instances_.begin(),
            instances_.end(),
            [modelId](const auto& instance) {
                return instance->modelId() == modelId && instance->isEnabled();
            }
    );
}
