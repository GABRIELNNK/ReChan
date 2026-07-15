#pragma once

#include "core.h"

// PC-only: one-off dialog line playback
namespace DialogPreview {
    // Stops/unloads any current preview, then synchronously selects and plays
    // a clip for (character, dialogId), centered (no panning)
    void Play(s32 character, s32 dialogId, s32 variant = -1, f32 maxDurationSec = 0.0f);

    // Stops and unloads the current preview, if any.
    void Stop();

    // Must be called once per frame while the custom menu is active, to
    // enforce Play's maxDurationSec cutoff.
    void Update();
}
