#include "pddi/gl/glgamepadrumble_sdl.h"

#include <cstring>

#if defined(P3D_USE_VENDORED_SDL2)
#include "SDL.h"
#endif

static char LowerAscii(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (char)(c + ('a' - 'A'));
    }
    return c;
}

static bool GuidEquals(const char* a, const char* b) {
    if (!a || !b) {
        return false;
    }

    for (int i = 0; i < 32; i++) {
        if (a[i] == '\0' || b[i] == '\0') {
            return false;
        }
        if (LowerAscii(a[i]) != LowerAscii(b[i])) {
            return false;
        }
    }

    return a[32] == '\0' && b[32] == '\0';
}

static void CopyGuid(char* dst, int dstSize, const char* src) {
    if (!dst || dstSize <= 0) {
        return;
    }

    dst[0] = '\0';
    if (!src || src[0] == '\0') {
        return;
    }

    std::strncpy(dst, src, (size_t)(dstSize - 1));
    dst[dstSize - 1] = '\0';
}

#if defined(P3D_USE_VENDORED_SDL2)

static SDL_GameController* g_activeController = nullptr;
static SDL_Joystick* g_activeJoystick = nullptr;
static bool g_activeJoystickOpenedDirect = false;
static char g_activeGuid[64] = {};
static bool g_sdlMainReady = false;

static void CloseActiveController() {
    if (g_activeController) {
        SDL_GameControllerClose(g_activeController);
    }
    else if (g_activeJoystickOpenedDirect && g_activeJoystick) {
        SDL_JoystickClose(g_activeJoystick);
    }

    g_activeController = nullptr;
    g_activeJoystick = nullptr;
    g_activeJoystickOpenedDirect = false;
    g_activeGuid[0] = '\0';
}

static void GetJoystickGuidString(SDL_Joystick* joystick, char* outGuid, int outGuidSize) {
    if (!outGuid || outGuidSize <= 0) {
        return;
    }

    outGuid[0] = '\0';
    if (!joystick) {
        return;
    }

    const SDL_JoystickGUID guid = SDL_JoystickGetGUID(joystick);
    SDL_JoystickGetGUIDString(guid, outGuid, outGuidSize);
}

static bool BindControllerAtIndex(int sdlIndex, const char* expectedGuid, bool requireGuidMatch) {
    if (sdlIndex < 0 || sdlIndex >= SDL_NumJoysticks()) {
        return false;
    }

    if (!SDL_IsGameController(sdlIndex)) {
        return false;
    }

    SDL_GameController* controller = SDL_GameControllerOpen(sdlIndex);
    if (!controller) {
        return false;
    }

    SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);
    if (!joystick) {
        SDL_GameControllerClose(controller);
        return false;
    }

    char sdlGuidString[64] = {};
    GetJoystickGuidString(joystick, sdlGuidString, (int)sizeof(sdlGuidString));

    if (requireGuidMatch && expectedGuid && expectedGuid[0] != '\0' && !GuidEquals(sdlGuidString, expectedGuid)) {
        SDL_GameControllerClose(controller);
        return false;
    }

    g_activeController = controller;
    g_activeJoystick = joystick;
    g_activeJoystickOpenedDirect = false;
    if (expectedGuid && expectedGuid[0] != '\0') {
        CopyGuid(g_activeGuid, (int)sizeof(g_activeGuid), expectedGuid);
    }
    else {
        CopyGuid(g_activeGuid, (int)sizeof(g_activeGuid), sdlGuidString);
    }
    return true;
}

static bool BindJoystickAtIndex(int sdlIndex, const char* expectedGuid, bool requireGuidMatch) {
    if (sdlIndex < 0 || sdlIndex >= SDL_NumJoysticks()) {
        return false;
    }

    SDL_Joystick* joystick = SDL_JoystickOpen(sdlIndex);
    if (!joystick) {
        return false;
    }

    char sdlGuidString[64] = {};
    GetJoystickGuidString(joystick, sdlGuidString, (int)sizeof(sdlGuidString));

    if (requireGuidMatch && expectedGuid && expectedGuid[0] != '\0' && !GuidEquals(sdlGuidString, expectedGuid)) {
        SDL_JoystickClose(joystick);
        return false;
    }

    g_activeController = nullptr;
    g_activeJoystick = joystick;
    g_activeJoystickOpenedDirect = true;
    if (expectedGuid && expectedGuid[0] != '\0') {
        CopyGuid(g_activeGuid, (int)sizeof(g_activeGuid), expectedGuid);
    }
    else {
        CopyGuid(g_activeGuid, (int)sizeof(g_activeGuid), sdlGuidString);
    }
    return true;
}

static bool EnsureSDLGameControllerSubsystem() {
    if (!g_sdlMainReady) {
        SDL_SetMainReady();
        g_sdlMainReady = true;
    }

    if ((SDL_WasInit(SDL_INIT_JOYSTICK) & SDL_INIT_JOYSTICK) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != 0) {
            return false;
        }
    }

    if ((SDL_WasInit(SDL_INIT_GAMECONTROLLER) & SDL_INIT_GAMECONTROLLER) == 0) {
        (void)SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    }

    return true;
}

static bool ActiveBindingCanRumble() {
    if (!g_activeController && !g_activeJoystick) {
        return false;
    }

#if SDL_VERSION_ATLEAST(2, 0, 18)
    if (g_activeController && SDL_GameControllerHasRumble(g_activeController) == SDL_TRUE) {
        return true;
    }

    if (g_activeJoystick && SDL_JoystickHasRumble(g_activeJoystick) == SDL_TRUE) {
        return true;
    }
#endif

    if (g_activeController && SDL_GameControllerRumble(g_activeController, 0, 0, 0) == 0) {
        return true;
    }

    return g_activeJoystick && SDL_JoystickRumble(g_activeJoystick, 0, 0, 0) == 0;
}

static float NormalizeSignedAxis(Sint16 value) {
    if (value >= 0) {
        return (float)value / 32767.0f;
    }
    return (float)value / 32768.0f;
}

static float NormalizeTriggerAxis(Sint16 value) {
    float normalized = (float)value / 32767.0f;
    if (normalized < 0.0f) {
        normalized = 0.0f;
    }
    else if (normalized > 1.0f) {
        normalized = 1.0f;
    }

    return normalized * 2.0f - 1.0f;
}

bool glGamepadPollState(bool outButtons[15], float outAxes[6]) {
    if (outButtons) {
        std::memset(outButtons, 0, sizeof(bool) * 15);
    }
    if (outAxes) {
        std::memset(outAxes, 0, sizeof(float) * 6);
    }

    if (!EnsureSDLGameControllerSubsystem()) {
        return false;
    }

    SDL_GameControllerUpdate();

    if (g_activeController) {
        SDL_Joystick* joystick = SDL_GameControllerGetJoystick(g_activeController);
        if (!joystick || SDL_JoystickGetAttached(joystick) != SDL_TRUE) {
            CloseActiveController();
        }
    }

    if (!g_activeController) {
        const int joystickCount = SDL_NumJoysticks();
        for (int i = 0; i < joystickCount; i++) {
            if (BindControllerAtIndex(i, nullptr, false)) {
                break;
            }
        }
    }

    if (!g_activeController) {
        return false;
    }

    static const SDL_GameControllerButton kButtonMap[15] = {
        SDL_CONTROLLER_BUTTON_A,
        SDL_CONTROLLER_BUTTON_B,
        SDL_CONTROLLER_BUTTON_X,
        SDL_CONTROLLER_BUTTON_Y,
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_BACK,
        SDL_CONTROLLER_BUTTON_START,
        SDL_CONTROLLER_BUTTON_GUIDE,
        SDL_CONTROLLER_BUTTON_LEFTSTICK,
        SDL_CONTROLLER_BUTTON_RIGHTSTICK,
        SDL_CONTROLLER_BUTTON_DPAD_UP,
        SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
        SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    };

    if (outButtons) {
        for (int i = 0; i < 15; i++) {
            outButtons[i] = SDL_GameControllerGetButton(g_activeController, kButtonMap[i]) == SDL_PRESSED;
        }
    }

    if (outAxes) {
        outAxes[0] = NormalizeSignedAxis(SDL_GameControllerGetAxis(g_activeController, SDL_CONTROLLER_AXIS_LEFTX));
        outAxes[1] = NormalizeSignedAxis(SDL_GameControllerGetAxis(g_activeController, SDL_CONTROLLER_AXIS_LEFTY));
        outAxes[2] = NormalizeSignedAxis(SDL_GameControllerGetAxis(g_activeController, SDL_CONTROLLER_AXIS_RIGHTX));
        outAxes[3] = NormalizeSignedAxis(SDL_GameControllerGetAxis(g_activeController, SDL_CONTROLLER_AXIS_RIGHTY));
        outAxes[4] = NormalizeTriggerAxis(SDL_GameControllerGetAxis(g_activeController, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
        outAxes[5] = NormalizeTriggerAxis(SDL_GameControllerGetAxis(g_activeController, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));
    }

    return true;
}

bool glGamepadRumbleUpdateActive(int glfwGamepadIndex, const char* glfwGuid) {
    const bool hasGuid = glfwGuid && glfwGuid[0] != '\0';

    if (g_activeController || g_activeJoystick) {
        if (hasGuid && GuidEquals(g_activeGuid, glfwGuid)) {
            return true;
        }

        if (!hasGuid && ActiveBindingCanRumble()) {
            return true;
        }
    }

    CloseActiveController();

    if (!EnsureSDLGameControllerSubsystem()) {
        return false;
    }

    SDL_GameControllerUpdate();

    const int joystickCount = SDL_NumJoysticks();
    if (joystickCount <= 0) {
        return false;
    }

    if (glfwGamepadIndex >= 0) {
        if (BindControllerAtIndex(glfwGamepadIndex, glfwGuid, hasGuid)) {
            return true;
        }
        if (BindJoystickAtIndex(glfwGamepadIndex, glfwGuid, hasGuid)) {
            return true;
        }
    }

    if (hasGuid) {
        for (int i = 0; i < joystickCount; i++) {
            if (i == glfwGamepadIndex) {
                continue;
            }

            if (BindControllerAtIndex(i, glfwGuid, true)) {
                return true;
            }

            if (BindJoystickAtIndex(i, glfwGuid, true)) {
                return true;
            }
        }
    }

    if (joystickCount == 1) {
        if (BindControllerAtIndex(0, nullptr, false)) {
            return true;
        }

        if (BindJoystickAtIndex(0, nullptr, false)) {
            return true;
        }
    }

    // Final fallback: select the first SDL device that confirms rumble support.
    for (int i = 0; i < joystickCount; i++) {
        if (BindControllerAtIndex(i, nullptr, false)) {
            if (ActiveBindingCanRumble()) {
                return true;
            }
            CloseActiveController();
        }

        if (BindJoystickAtIndex(i, nullptr, false)) {
            if (ActiveBindingCanRumble()) {
                return true;
            }
            CloseActiveController();
        }
    }

    return false;
}

bool glGamepadRumbleSupports() {
    return ActiveBindingCanRumble();
}

static float ClampUnit(float value) {
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

bool glGamepadRumbleSet(float lowFrequency, float highFrequency) {
    if (!g_activeController && !g_activeJoystick) {
        return false;
    }

    const float low = ClampUnit(lowFrequency);
    const float high = ClampUnit(highFrequency);
    const Uint16 lowAmp = (Uint16)(low * 65535.0f);
    const Uint16 highAmp = (Uint16)(high * 65535.0f);

    const Uint32 durationMs = (lowAmp == 0 && highAmp == 0) ? 0u : 500u;

    if (g_activeController && SDL_GameControllerRumble(g_activeController, lowAmp, highAmp, durationMs) == 0) {
        return true;
    }

    return g_activeJoystick && SDL_JoystickRumble(g_activeJoystick, lowAmp, highAmp, durationMs) == 0;
}

#else

bool glGamepadRumbleUpdateActive(int /*glfwGamepadIndex*/, const char* /*glfwGuid*/) {
    return false;
}

bool glGamepadPollState(bool* /*outButtons*/, float* /*outAxes*/) {
    return false;
}

bool glGamepadRumbleSupports() {
    return false;
}

bool glGamepadRumbleSet(float /*lowFrequency*/, float /*highFrequency*/) {
    return false;
}

#endif
