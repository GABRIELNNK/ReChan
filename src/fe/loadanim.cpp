// loadanim.cpp - PSX VBlankLogo loading bar
// PSX source: C:\CHAN\GAME\SRC\FE\LOADANIM.CPP
#include "fe/loadanim.h"
#include "gen/display.h"
#include "pc/tim.h"
#include "gen/config.h"
#include "p3d/context.h"
#include "p3d/texture.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"

// PSX: VBlankLogo object (32 bytes on PSX, gp+1356)
static struct {
    tTexture* bgTexture;
    u8 currentFill;
    u8 targetFill;
    bool active;
} s_logo = {};

// PSX: color pulse state (gp+1360..gp+1364)
// gp+1360 = R channel (5-bit, pulses 5-28), gp+1361 = G (0), gp+1362 = B (0)
// gp+1364 = direction flag (0=decrement, 1=increment)
static u8 s_pulseR = 28;
static s32 s_pulseDir = 0;

// PSX bar position from Update__10VBlankLogoP6_RTASK math:
// Y = dispEnv.y + 140, tile 8x6, scale gp+3540 = 2.28 (16.16)
// Left edge: ((-50.0 * scale) >> 32) + 233 = 119
// Right edge at 100%: computed from FillMeter target = 339
static constexpr f32 kBarY = 138.5f;
static constexpr f32 kBarH = 6.0f;
static constexpr f32 kBarLeftX = 119.0f;
static constexpr f32 kBarMaxW = 229.0f;

// PSX: Update__10VBlankLogoP6_RTASK (LOADANIM.CPP:85, 0x80047778)
static void DrawLoadingScreen(u8 fill) {
    if (!s_logo.bgTexture)
        return;

    ScreenDraw::DrawFullscreen(s_logo.bgTexture);

    if (fill == 0)
        return;
    if (fill > 100) 
        fill = 100;

    f32 nx = kBarLeftX / PSX_SCREEN_WIDTH;
    f32 ny = 1.0f - (kBarY + kBarH) / PSX_SCREEN_HEIGHT;
    f32 nw = kBarMaxW * (fill / 100.0f) / PSX_SCREEN_WIDTH;
    f32 nh = kBarH / PSX_SCREEN_HEIGHT;

    u8 r8 = s_pulseR << 3;
    ScreenDraw::DrawColoredRect(nx, ny, nw, nh, r8, 0, 0, 255);

    // PSX: pulse R channel each VBlank
    if (s_pulseDir) {
        s_pulseR++;
        if (s_pulseR >= 28)
            s_pulseDir = 0;
    } else {
        s_pulseR--;
        if (s_pulseR < 5)
            s_pulseDir = 1;
    }
}

static void PresentLoadingFrame(u8 fill) {
    g_display->BeginFrame();
    DrawLoadingScreen(fill);
    g_display->EndFrame();
}

static void PresentLoadingFrame_Tex(tTexture* tex) {
    g_display->BeginFrame();
    ScreenDraw::DrawFullscreen(tex);
    g_display->EndFrame();
}

// PSX: StartLogo__10VBlankLogol (LOADANIM.CPP:167, 0x80047968)
void StartLogo(const char* timFile) {
    MARKFUNCTION(0x80047968);

    // Load background TIM
    if (!s_logo.bgTexture) {
        TimImage* img = Tim::LoadFromFile(timFile);
        if (img) {
            s_logo.bgTexture = Tim::CreateTexture(img);
            delete img;
        }
    }

    // PSX: obj+8 = 0, obj+12 = 0, SetActive(1)
    s_logo.currentFill = 0;
    s_logo.targetFill = 0;
    s_logo.active = true;

    // PSX: initial data values from SLUS binary at gp+1360/1364
    s_pulseR = 28;
    s_pulseDir = 0;

    // Present initial loading screen with empty bar
    PresentLoadingFrame(0);
}

// PSX: FillMeter__10VBlankLogoUc (LOADANIM.CPP:207, 0x80047A68)
void FillMeter(u8 target) {
    MARKFUNCTION(0x80047A68);

    while (true) {
        if (s_logo.targetFill == target)
            break;

        // PSX: target increments by +1 each call until it reaches the requested value
        if (s_logo.targetFill < target) {
			s_logo.currentFill++;
            s_logo.targetFill++;
        } else {
			s_logo.currentFill--;
            s_logo.targetFill--;
        }

        // PSX: Update is called each VBlank until target is reached, so we present a new frame here
        PresentLoadingFrame(s_logo.targetFill);
    }
    s_logo.targetFill = target;
}

// PSX: StopLogo__10VBlankLogo (LOADANIM.CPP:183, 0x800479BC)
void StopLogo() {
    MARKFUNCTION(0x800479BC);

    if (!s_logo.active)
        return;

    // PSX: fast-fill loop increments +8 by 0x10000 each iteration until >= +12,
    // calling Update + VSync(0) each step.
    for (s32 i = 0; i < 20; i++) {
        u8 fill = (u8)(s_logo.currentFill + (s_logo.targetFill - s_logo.currentFill) * (i + 1) / 20);
        PresentLoadingFrame(fill);
    }

    // Hold the completed bar for a moment (~10 frames)
    for (s32 i = 0; i < 10; i++) {
        PresentLoadingFrame(s_logo.targetFill);
    }

    // PSX: SetActive(0), destructor frees tile buffer + removes VBlank task
    if (s_logo.bgTexture) {
        s_logo.bgTexture->Release();
        s_logo.bgTexture = nullptr;
    }
    s_logo.active = false;
}

// PSX: DisplayTIM__FPCc (GAME.CPP:862, 0x80029200)
// PSX: ClearImage + LoadImage (direct VRAM ops). PC: draw fullscreen quad.
void DisplayTIM(const char* filename) {
    MARKFUNCTION(0x80029200);

    TimImage* img = Tim::LoadFromFile(filename);
    if (!img) {
        LOG("[LoadAnim] DisplayTIM: failed to load %s", filename);
        return;
    }
    tTexture* tex = Tim::CreateTexture(img);
    delete img;
    if (!tex)
        return;

    // Present the image (double-buffer: draw twice, swap once)
    PresentLoadingFrame_Tex(tex);

    tex->Release();
}
