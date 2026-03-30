// context.cpp — tContext + tPlatform implementation
#include "p3d/context.h"
#include "p3d/inventory.h"
#include "pddi/pddidev.h"

// Global access pointers

namespace p3d {
    pddiDevice* device = nullptr;
    pddiDisplay* display = nullptr;
    pddiRenderContext* context = nullptr;
    tInventory* inventory = nullptr;
}

// tContext

tContext::tContext() = default;

tContext::~tContext() {
    Shutdown();
}

bool tContext::Setup(const tContextInitData& init) {
    mDevice = pddiCreate();
    if (!mDevice) return false;

    mDisplay = mDevice->NewDisplay();
    pddiDisplayInit di;
    di.xSize = init.xSize;
    di.ySize = init.ySize;
    di.title = init.title;
    if (!mDisplay->InitDisplay(di))
        return false;

    mRenderContext = mDevice->NewRenderContext(mDisplay);
    mInventory = new tInventory();

    return true;
}

void tContext::Shutdown() {
    delete mInventory;   mInventory = nullptr;
    if (mRenderContext) { mRenderContext->Release(); mRenderContext = nullptr; }
    if (mDisplay) { mDisplay->Release();       mDisplay = nullptr; }
    if (mDevice) { mDevice->Release();        mDevice = nullptr; }
}

void tContext::BeginFrame() {
    if (mDisplay)       mDisplay->PollEvents();
    if (mRenderContext) mRenderContext->BeginFrame();
}

void tContext::EndFrame() {
    if (mRenderContext) mRenderContext->EndFrame();
    if (mDisplay)       mDisplay->SwapBuffers();
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
    if (ctx == mActiveContext)
        SetActiveContext(nullptr);
    delete ctx;
}

void tPlatform::SetActiveContext(tContext* ctx) {
    mActiveContext = ctx;
    if (ctx) {
        p3d::device = ctx->GetDevice();
        p3d::display = ctx->GetDisplay();
        p3d::context = ctx->GetContext();
        p3d::inventory = ctx->GetInventory();
    }
    else {
        p3d::device = nullptr;
        p3d::display = nullptr;
        p3d::context = nullptr;
        p3d::inventory = nullptr;
    }
}
