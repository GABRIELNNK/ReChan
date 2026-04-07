// time.cpp - Time manager reversed from PSX TIME.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\TIME.CPP
#include "gen/time.h"

#include <chrono>
#include <thread>

// PSX: gp-relative global
Time* g_time = nullptr;

Time::Time() { MARKFUNCTION(0x80044950); }
Time::~Time() { MARKFUNCTION(0x800449E8); }

void Time::InternalOpen() { MARKFUNCTION(0x80044A10); }
void Time::InternalClose() { MARKFUNCTION(0x80044A18); }

void Time::InternalReset() {
    MARKFUNCTION(0x80044A38);
    frameCounter = 0;
}

void Time::Step() {
    MARKFUNCTION(0x80044A40);
    frameCounter++;
}

void Time::Tick(f32 realDt) {
    if (realDt < 0.0001f) realDt = 0.0001f;
    if (realDt > 0.25f)  realDt = 0.25f;
    deltaTime = realDt;
    fps = 1.0f / realDt;
}

f64 Time::GetTimeInSeconds() {
    static auto sStart = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<f64>(now - sStart).count();
}

void Time::Sleep(f32 seconds) {
    if (seconds > 0.0f) {
        std::this_thread::sleep_for(std::chrono::duration<f32>(seconds));
    }
}

void Time::WaitForFrameEnd(f64 frameStart) const {
    f32 target = GetTargetDt();
    if (target <= 0.0f) return;

    // Hybrid: sleep for the bulk, spin for the last 2ms
    constexpr f32 spinMargin = 0.002f;
    f64 now = GetTimeInSeconds();
    f32 remaining = target - (f32)(now - frameStart);
    if (remaining > spinMargin) {
        Sleep(remaining - spinMargin);
    }
    // Spin-wait for the remainder
    while ((f32)(GetTimeInSeconds() - frameStart) < target) {
        // spin
    }
}
