// context.h — tContext + tPlatform
#ifndef P3D_CONTEXT_H
#define P3D_CONTEXT_H

#include "core.h"

class pddiDevice;
class pddiDisplay;
class pddiRenderContext;
class tInventory;

// Context init params
struct tContextInitData {
    int xSize = 960;
    int ySize = 720;
    const char* title = "libp3d";
};

// Context — owns all engine subsystems
class tContext {
public:
    tContext();
    ~tContext();

    bool Setup(const tContextInitData& init);
    void Shutdown();

    pddiDevice* GetDevice() { return mDevice; }
    pddiDisplay* GetDisplay() { return mDisplay; }
    pddiRenderContext* GetContext() { return mRenderContext; }
    tInventory* GetInventory() { return mInventory; }

    void BeginFrame();
    void EndFrame();

private:
    pddiDevice* mDevice = nullptr;
    pddiDisplay* mDisplay = nullptr;
    pddiRenderContext* mRenderContext = nullptr;
    tInventory* mInventory = nullptr;
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
    tContext* GetActiveContext() { return mActiveContext; }

private:
    tPlatform() = default;
    ~tPlatform() = default;

    static tPlatform* sInstance;
    tContext* mActiveContext = nullptr;
};

// Global access (Pure3D convention — set by tPlatform::SetActiveContext)
namespace p3d {
    extern pddiDevice* device;
    extern pddiDisplay* display;
    extern pddiRenderContext* context;
    extern tInventory* inventory;
}

#endif // P3D_CONTEXT_H
