#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

enum class EffectModelId : int32_t {
    NoiseGate = 0,
    ClassicOverdrive = 1,
    ThreeBandEq = 2,
    DigitalDelay = 3,
    Distortion = 4,
    VintageFuzz = 5,
    Preamp = 6,
    Cabinet = 7,
    TightNoiseGate = 8,
    MidBoostOverdrive = 9,
    TransparentOverdrive = 10,
    ModernDistortion = 11,
    GatedFuzz = 12,
    ScoopEq = 13,
    SlapbackDelay = 14,
    AmbientDelay = 15,
    CleanPreamp = 16,
    HighGainPreamp = 17,
    Cabinet1x12 = 18,
    Cabinet2x12 = 19,
    Cabinet4x12 = 20
};

inline std::optional<EffectModelId> effectModelIdFromString(
        std::string_view persistentId
) noexcept {
    if (persistentId == "noise_gate") return EffectModelId::NoiseGate;
    if (persistentId == "classic_overdrive") return EffectModelId::ClassicOverdrive;
    if (persistentId == "three_band_eq") return EffectModelId::ThreeBandEq;
    if (persistentId == "digital_delay") return EffectModelId::DigitalDelay;
    if (persistentId == "distortion") return EffectModelId::Distortion;
    if (persistentId == "vintage_fuzz") return EffectModelId::VintageFuzz;
    if (persistentId == "preamp") return EffectModelId::Preamp;
    if (persistentId == "cabinet") return EffectModelId::Cabinet;
    if (persistentId == "tight_noise_gate") return EffectModelId::TightNoiseGate;
    if (persistentId == "mid_boost_overdrive") return EffectModelId::MidBoostOverdrive;
    if (persistentId == "transparent_overdrive") return EffectModelId::TransparentOverdrive;
    if (persistentId == "modern_distortion") return EffectModelId::ModernDistortion;
    if (persistentId == "gated_fuzz") return EffectModelId::GatedFuzz;
    if (persistentId == "scoop_eq") return EffectModelId::ScoopEq;
    if (persistentId == "slapback_delay") return EffectModelId::SlapbackDelay;
    if (persistentId == "ambient_delay") return EffectModelId::AmbientDelay;
    if (persistentId == "clean_preamp") return EffectModelId::CleanPreamp;
    if (persistentId == "high_gain_preamp") return EffectModelId::HighGainPreamp;
    if (persistentId == "cabinet_1x12") return EffectModelId::Cabinet1x12;
    if (persistentId == "cabinet_2x12") return EffectModelId::Cabinet2x12;
    if (persistentId == "cabinet_4x12") return EffectModelId::Cabinet4x12;
    return std::nullopt;
}

inline std::string_view effectModelIdToString(EffectModelId modelId) noexcept {
    switch (modelId) {
        case EffectModelId::NoiseGate: return "noise_gate";
        case EffectModelId::ClassicOverdrive: return "classic_overdrive";
        case EffectModelId::ThreeBandEq: return "three_band_eq";
        case EffectModelId::DigitalDelay: return "digital_delay";
        case EffectModelId::Distortion: return "distortion";
        case EffectModelId::VintageFuzz: return "vintage_fuzz";
        case EffectModelId::Preamp: return "preamp";
        case EffectModelId::Cabinet: return "cabinet";
        case EffectModelId::TightNoiseGate: return "tight_noise_gate";
        case EffectModelId::MidBoostOverdrive: return "mid_boost_overdrive";
        case EffectModelId::TransparentOverdrive: return "transparent_overdrive";
        case EffectModelId::ModernDistortion: return "modern_distortion";
        case EffectModelId::GatedFuzz: return "gated_fuzz";
        case EffectModelId::ScoopEq: return "scoop_eq";
        case EffectModelId::SlapbackDelay: return "slapback_delay";
        case EffectModelId::AmbientDelay: return "ambient_delay";
        case EffectModelId::CleanPreamp: return "clean_preamp";
        case EffectModelId::HighGainPreamp: return "high_gain_preamp";
        case EffectModelId::Cabinet1x12: return "cabinet_1x12";
        case EffectModelId::Cabinet2x12: return "cabinet_2x12";
        case EffectModelId::Cabinet4x12: return "cabinet_4x12";
    }
    return "unknown";
}
