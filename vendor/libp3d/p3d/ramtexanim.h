#pragma once

#include "p3d/animate.h"
#include "p3d/loadmanager.h"
#include "p3d/texture.h"

class tRAMTexAnim : public tAnimation {
public:
    tRAMTexAnim();
    ~tRAMTexAnim() override;

    s32 GetNumFrames() const override;
    tFlipbook* MakePuppet() override;

    s32 GetFrame(s32 index) const;
    s32 GetNumTextures() const;
    tTexture* GetTexture(s32 index) const;
    void SetNumFrames(s32 count);
    void SetFrameData(u8* data);
    void SetNumTextures(s32 count);
    void SetTextureData(tTexture** data);

private:
    s32 numTextures = 0;
    s32 numFrames = 0;
    tTexture** textures = nullptr;
    u8* frameData = nullptr;
    s16 lastTexture = -1;
    s16 pad = 0;
};

class tRAMTexFlip : public tFlipbook {
public:
    tRAMTexFlip() = default;
    ~tRAMTexFlip() override = default;

    void Reset() override;
    void Update() override;

private:
    s32 lastUploadedFrame = -1;
};

class tRAMTexAnimLoader : public tChunkHandler {
public:
    u16 GetChunkID() override { return 0x6050; }
    void LoadChunk(tChunkFile* file, tInventory* store) override;
};