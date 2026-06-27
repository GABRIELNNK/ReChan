#pragma once
#include "core.h"
#include "gen/cclist.h"

class tView;
struct DBPoint;
struct DBSphere;
struct DBVolume;

class AmbientLight {
public:
    u32 desiredColour = 0;

    AmbientLight();
    ~AmbientLight();

    void SetToWorldAmbient();
    void SetDesired(u32 colour);
    void SetPortToLight();
};

class HardwareLight {
public:
    s32 directionX = 0;
    s32 directionY = 0;
    s32 directionZ = 0;
    u32 colour = 0;
    u32 active = 0;
    s32 pad14 = 0;

    HardwareLight();
    ~HardwareLight();

    void SetLight(u32 newColour, const LVector* direction);
};

class RadialLight {
public:
    u16 radius = 0;
    u8 flags = 0;
    u8 colourR = 0;
    u8 colourG = 0;
    u8 colourB = 0;
    u16 pad6 = 0;
    s32 posX = 0;
    s32 posY = 0;
    s32 posZ = 0;
};

class LightColourVolume : public ccMinNode {
public:
    LVector bboxMin = {};
    LVector bboxMax = {};
    u8 colourR = 0;
    u8 colourG = 0;
    u8 colourB = 0;
    u8 addR = 0;
    u8 addG = 0;
    u8 addB = 0;
    s32 falloff = 0x8000;

    LightColourVolume();
    ~LightColourVolume() override;

    bool IsInside(const LVector& point) const;
};

class HardwareLightVolume : public ccMinNode {
public:
    LVector bboxMin = {};
    LVector bboxMax = {};
    u8 colourR = 0;
    u8 colourG = 0;
    u8 colourB = 0;
    s32 innerRadius = 0;
    s32 outerRadius = 0;
    HardwareLight* boundLight = nullptr;
    s32 directionX = 0;
    s32 directionY = -65535;
    s32 directionZ = 0;

    HardwareLightVolume();
    ~HardwareLightVolume() override;

    bool IsInside(const LVector& point) const;
};

class LightAnchor {
public:
    ccMinList colourVolumes;
    ccMinList hardwareLightVolumes;
    ccMinList radialHardwareLights;
    RadialLight* radialLights = nullptr;
    u32 radialLightCount = 0;

    LightAnchor();
    ~LightAnchor();

    void SetupLightMemory(u32 count);
    void AddColourVolume(const DBVolume* volume);
    void AddHardwareLightVolume(const DBVolume* volume);
    bool SetupLight(RadialLight* light, const DBSphere* sphere);
    bool AddRadialHardwareLight(const DBSphere* sphere, u32 index);
};

class LightingClass {
public:
    u8 slotHandles[3] = {};
    void* slotLights[3] = {};
    HardwareLight originalLights[3];
    u32 worldAmbientColour = 0x585858;
    tView* view = nullptr;
    LightAnchor* lightAnchor = nullptr;

    LightingClass();
    ~LightingClass();

    void Reset();
    void InternalOpen(tView* newView);
    void AnalyzeSphere(DBPoint* point);
    s32 DoModelLighting(class Thing* thing);
    void AddLightToPort(s32 slot, const LVector* direction, u32 colour);
    void RemoveLightFromPort(s32 slot);
    void SetupLighting();
    void SetupStageAttributes(const DBVolume* volume);
    void FindAmbientVolumes(class Thing* thing);
    s32 FindHardwareVolumes(class Thing* thing);
    const HardwareLight* GetPortLight(s32 slot) const;

    // Primary directional light heading derived from camera facing (used for
    // CSM sun direction under MODERN_GRAPHICS, see shadowcsm.cpp).
    static void ComputeLightDir(LVector* direction);

private:
    void SetupHLight(s32 slot, const LVector* direction, u32 colour);
    void SetHLightToOriginal(s32 slot);
    static void ClampWithinRGBLimit(u32* r, u32* g, u32* b);
    static void ClampWithinNormalLimit(s32* x, s32* y, s32* z);
};

const HardwareLight* GetCurrentPortHardwareLight(s32 slot);

u32 GetWorldAmbientLightColour();
u32 GetCurrentPortAmbientLightColour();
void RestoreWorldAmbientLightToPort();