#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

enum class EffectModelId : int32_t {
    NoiseGate = 0,
    ClassicOverdrive = 1,
    ThreeBandEq = 2,
    DigitalDelay = 3
};

inline std::optional<EffectModelId> effectModelIdFromString(
        std::string_view persistentId
) noexcept {
    if (persistentId == "noise_gate") return EffectModelId::NoiseGate;
    if (persistentId == "classic_overdrive") return EffectModelId::ClassicOverdrive;
    if (persistentId == "three_band_eq") return EffectModelId::ThreeBandEq;
    if (persistentId == "digital_delay") return EffectModelId::DigitalDelay;
    return std::nullopt;
}

inline std::string_view effectModelIdToString(EffectModelId modelId) noexcept {
    switch (modelId) {
        case EffectModelId::NoiseGate: return "noise_gate";
        case EffectModelId::ClassicOverdrive: return "classic_overdrive";
        case EffectModelId::ThreeBandEq: return "three_band_eq";
        case EffectModelId::DigitalDelay: return "digital_delay";
    }
    return "unknown";
}
