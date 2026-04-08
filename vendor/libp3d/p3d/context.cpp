// context.cpp — tContext + tPlatform implementation
#include "p3d/context.h"
#include "p3d/inventory.h"
#include "p3d/input.h"
#include "pddi/pddidev.h"

// Global access pointers

namespace p3d {
    pddiDevice* device = nullptr;
    pddiDisplay* display = nullptr;
    pddiRenderContext* context = nullptr;
    tInventory* inventory = nullptr;
    PlatformInput* input = nullptr;
}

// tContext

tContext::tContext() = default;

tContext::~tContext() {
    Shutdown();
}

bool tContext::Setup(const tContextInitData& init) {
    device = pddiCreate();
    if (!device) return false;

    display = device->NewDisplay();
    pddiDisplayInit di;
    di.xSize = init.xSize;
    di.ySize = init.ySize;
    di.title = init.title;
    di.fullscreen = init.fullscreen;
    di.vsync = init.vsync;
    di.msaa = init.msaa;
    if (!display->InitDisplay(di))
        return false;

    renderContext = device->NewRenderContext(display);
    gamepad = device->NewGamepad();
    inventory = new tInventory();
    inputManager = new PlatformInput();
    inputManager->SetDisplay(display);
    inputManager->SetGamepad(gamepad);

    return true;
}

void tContext::Shutdown() {
    if (inputManager) {
        delete inputManager;
        inputManager = nullptr;
    }

    if (inventory) {
        delete inventory;
        inventory = nullptr;
    }

    if (renderContext) {
        renderContext->Release();
        renderContext = nullptr;
    }

    if (gamepad) {
        gamepad->Release();
        gamepad = nullptr;
    }

    if (display) {
        display->Release();
        display = nullptr;
    }

    if (device) {
        device->Release();
        device = nullptr;
    }
}

void tContext::BeginFrame() {
    if (renderContext) renderContext->BeginFrame();
}

void tContext::EndFrame() {
    if (renderContext) renderContext->EndFrame();
}

void tContext::SwapBuffers() {
    if (display) display->SwapBuffers();
}

// tPlatform

tPlatform* tPlatform::sInstance = nullptr;

tPlatform* tPlatform::Create() {
    if (!sInstance)
        sInstance = new tPlatform();
    return sInstance;
}

void tPlatform::Destroy() {
    delete sInstance;
    sInstance = nullptr;
}

tPlatform* tPlatform::GetPlatform() {
    return sInstance;
}

tContext* tPlatform::CreateContext(const tContextInitData& init) {
    auto* ctx = new tContext();
    if (!ctx->Setup(init)) {
        delete ctx;
        return nullptr;
    }
    SetActiveContext(ctx);
    return ctx;
}

void tPlatform::DestroyContext(tContext* ctx) {
    if (ctx == activeContext)
        SetActiveContext(nullptr);
    delete ctx;
}

void tPlatform::SetActiveContext(tContext* ctx) {
    activeContext = ctx;
    if (ctx) {
        p3d::device = ctx->GetDevice();
        p3d::display = ctx->GetDisplay();
        p3d::context = ctx->GetContext();
        p3d::inventory = ctx->GetInventory();
        p3d::input = ctx->GetInputManager();
    }
    else {
        p3d::device = nullptr;
        p3d::display = nullptr;
        p3d::context = nullptr;
        p3d::inventory = nullptr;
        p3d::input = nullptr;
    }
}
