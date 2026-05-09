#pragma once

#include "core.h"

inline u32 FeSoundSliderToFlag(u32 sliderVal) {
    constexpr u32 kSliderMax = 65535;
    constexpr u32 kFlagMax = 100;

    if (sliderVal > kSliderMax) {
        sliderVal = kSliderMax;
    }

    return (sliderVal * kFlagMax + (kSliderMax / 2)) / kSliderMax;
}

inline u32 FeSoundFlagToSlider(u32 flagVal) {
    constexpr u32 kSliderMax = 65535;
    constexpr u32 kFlagMax = 100;

    if (flagVal > kFlagMax) {
        flagVal = kFlagMax;
    }

    return (flagVal * kSliderMax + (kFlagMax / 2)) / kFlagMax;
}