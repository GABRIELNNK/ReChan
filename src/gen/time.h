#pragma once
#include "gen/manager.h"
#include "config.h"

// Time (40 bytes on PSX) - frame timing manager
// PSX layout:
//   +0:  Manager base (28 bytes: ccNode(24) + isOpen(s16))
//   +28: frameCounter (u32) - incremented each Step()
class Time : public Manager {
public:
    u32 frameCounter = 0; // +28: incremented each frame by Step()
    s32 targetFPS = 30;  // render frame rate cap (logic always runs at 30Hz)

    // PC: measured real delta time (seconds) and FPS, updated each frame.
    f32 deltaTime = 1.0f / 30.0f;
    f32 fps = 30.0f;

    // PSX: __4Time (TIME.CPP, 0x80044950)
    Time();

    // PSX: _._4Time (TIME.CPP, 0x800449E8)
    ~Time() override;

    void InternalOpen() override;
    void InternalClose() override;
    void InternalReset() override;
    void Step();

    // PC: call each frame with the real wall-clock elapsed time.
    void Tick(f32 realDt);

    // High-resolution wall clock
    static f64 GetTimeInSeconds();

    // Sleep the calling thread for the given duration
    static void Sleep(f32 seconds);

    // Precise frame limiter: hybrid sleep + spin-wait. Call at end of frame.
    void WaitForFrameEnd(f64 frameStart) const;

    u32 GetFrameCounter() const { return frameCounter; }
    f32 GetDeltaTime() const { return deltaTime; }
    f32 GetFPS() const { return fps; }
    f32 GetTargetDt() const { return (targetFPS > 0) ? (1.0f / (f32)targetFPS) : 0.0f; }
};

// PSX: gp-relative global, defined in time.cpp
extern Time* g_time;
