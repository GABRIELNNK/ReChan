// time.h - Time manager reversed from PSX TIME.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\TIME.CPP
// Simple frame counter manager. Step() increments each frame.
#pragma once

#include "gen/manager.h"

// Time (40 bytes on PSX) - frame timing manager
// PSX layout:
//   +0:  Manager base (28 bytes: ccNode(24) + isOpen(s16))
//   +28: frameCounter (u32) - incremented each Step()
class Time : public Manager {
public:
    u32 frameCounter = 0; // +28: incremented each frame by Step()

    // PC: target framerate — the game loop sleeps to maintain this rate.
    // PSX ran at ~30 fps; all physics values are tuned for 30 fps per-frame.
    // Set to 0 to uncap.
    s32 targetFPS = 30;

    // PC: measured real delta time (seconds) and FPS, updated each frame.
    f32 deltaTime = 1.0f / 30.0f;
    f32 fps       = 30.0f;

    // PSX: __4Time (TIME.CPP, 0x80044950)
    Time() { MARKFUNCTION(0x80044950); }

    // PSX: _._4Time (TIME.CPP, 0x800449E8)
    ~Time() override { MARKFUNCTION(0x800449E8); }

    // PSX: InternalOpen__4Time (0x80044A10) - NOP stub
    void InternalOpen() override { MARKFUNCTION(0x80044A10); }

    // PSX: InternalClose__4Time (0x80044A18) - calls base
    void InternalClose() override { MARKFUNCTION(0x80044A18); }

    // PSX: InternalReset__4Time (0x80044A38) - resets counter
    void InternalReset() override {
        MARKFUNCTION(0x80044A38);
        frameCounter = 0;
    }

    // PSX: Step__4Time (0x80044A40) - increment frame counter
    void Step() {
        MARKFUNCTION(0x80044A40);
        frameCounter++;
    }

    // PC: call each frame with the real wall-clock elapsed time.
    void Tick(f32 realDt) {
        if (realDt < 0.0001f) realDt = 0.0001f;
        if (realDt > 0.25f)  realDt = 0.25f; // clamp to avoid spiral of death
        deltaTime = realDt;
        fps = 1.0f / realDt;
    }

    u32 GetFrameCounter() const { return frameCounter; }
    f32 GetDeltaTime() const { return deltaTime; }
    f32 GetFPS() const { return fps; }
    f32 GetTargetDt() const { return (targetFPS > 0) ? (1.0f / (f32)targetFPS) : 0.0f; }
};

// PSX: gp-relative global, defined in time.cpp
extern Time* g_time;
