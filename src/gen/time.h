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
#if HIGH_FPS_PLAY_PRESENTATION
    s32 targetFPS = 60;

    // PC: measured real delta time (seconds) and FPS, updated each frame.
    f32 deltaTime = 1.0f / 60.0f;
    f32 fps = 60.0f;
#else
    s32 targetFPS = 30;  // render frame rate cap (logic always runs at 30Hz)

    // PC: measured real delta time (seconds) and FPS, updated each frame.
    f32 deltaTime = 1.0f / 30.0f;
    f32 fps = 30.0f;
#endif

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

#if HIGH_FPS_PLAY_PRESENTATION
    void ResetPlayPresentationState();
    s32 BeginPlayFixedStep();
    bool DidPlayLogicStepThisFrame() const { return playLogicStepCount > 0; }
    s32 GetLogicStepCount() const { return playLogicStepCount; }
    f32 GetPlayPresentationAlpha() const { return playPresentationAlpha; }
#endif

private:
#if HIGH_FPS_PLAY_PRESENTATION
    f32 playLogicAccumulator = 0.0f;
    f32 playPresentationAlpha = 0.0f;
    s32 playLogicStepCount = 1;
#endif
};

// PSX: gp-relative global, defined in time.cpp
extern Time* g_time;

struct FrameProfile {
    f32 logicMs = 0.0f;
    f32 drawSubmitMs = 0.0f;
    f32 swapMs = 0.0f;
};
extern FrameProfile g_frameProfile;
