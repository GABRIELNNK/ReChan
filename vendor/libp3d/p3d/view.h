#pragma once

#include "p3d/entity.h"
#include "pddi/pddi.h"

class tCamera;

class tView : public tRefCounted {
public:
    tView();
    virtual ~tView();

    // Viewport window in normalized coords (0..1), default = full screen
    void SetWindow(f32 left, f32 top, f32 right, f32 bottom);

    // Camera
    void SetCamera(tCamera* cam);
    tCamera* GetCamera() { return camera; }

    // Clear
    void SetClearColour(pddiColour c) { clearColour = c; }
    pddiColour GetClearColour() const { return clearColour; }
    void SetClearMask(u32 mask) { clearMask = mask; }
    u32 GetClearMask() const { return clearMask; }
    void SetBackgroundColour(pddiColour c) { clearColour = c; }
    void SetAmbientLight(pddiColour c) { ambientLight = c; }
    pddiColour GetAmbientLight() const { return ambientLight; }

    // Fog (placeholder for future use)
    void EnableFog(bool b) { fogEnabled = b; }
    bool IsFogEnabled() const { return fogEnabled; }
    void SetFogColour(pddiColour c) { fogColour = c; }
    void SetFogPlanes(f32 nearFog, f32 farFog) { fogNear = nearFog; fogFar = farFog; }

    // Render pass
    void BeginRender();
    void EndRender();

private:
    f32 l, t, r, b;
    tCamera* camera;
    pddiColour clearColour;
    pddiColour ambientLight;
    u32 clearMask;
    bool fogEnabled;
    pddiColour fogColour;
    f32 fogNear, fogFar;
};
