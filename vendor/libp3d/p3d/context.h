// context.h = tContext + tPlatform
#pragma once

#include "core.h"

class pddiDevice;
class pddiDisplay;
class pddiRenderContext;
class pddiGamepad;
class tInventory;
class PlatformInput;

// Context init params
struct tContextInitData {
    int xSize = 960;
    int ySize = 720;
    const char* title = "libp3d";
    bool fullscreen = false;
    bool vsync = true;
    int msaa = 0;
};

// Context = owns all engine subsystems
class tContext {
public:
    tContext();
    ~tContext();

    bool Setup(const tContextInitData& init);
    void Shutdown();

    pddiDevice* GetDevice() { return device; }
    pddiDisplay* GetDisplay() { return display; }
    pddiRenderContext* GetContext() { return renderContext; }
    pddiGamepad* GetGamepad() { return gamepad; }
    tInventory* GetInventory() { return inventory; }
    PlatformInput* GetInputManager() { return inputManager; }

    void BeginFrame();
    void EndFrame();
    void SwapBuffers();

private:
    pddiDevice* device = nullptr;
    pddiDisplay* display = nullptr;
    pddiRenderContext* renderContext = nullptr;
    pddiGamepad* gamepad = nullptr;
    tInventory* inventory = nullptr;
    PlatformInput* inputManager = nullptr;
};

// Platform singleton
class tPlatform {
public:
    static tPlatform* Create();
    static void       Destroy();
    static tPlatform* GetPlatform();

    tContext* CreateContext(const tContextInitData& init);
    void     DestroyContext(tContext* ctx);
    void     SetActiveContext(tContext* ctx);
    tContext* GetActiveContext() { return activeContext; }

private:
    tPlatform() = default;
    ~tPlatform() = default;

    static tPlatform* sInstance;
    tContext* activeContext = nullptr;
};

// Global access (Pure3D convention = set by tPlatform::SetActiveContext)
namespace p3d {
    extern pddiDevice* device;
    extern pddiDisplay* display;
    extern pddiRenderContext* context;
    extern tInventory* inventory;
    extern PlatformInput* input;
}
