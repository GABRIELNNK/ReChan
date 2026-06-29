#include "snd/esound.h"

#include "snd/prstsnd.h"
#include "snd/rsdworld.h"

#include <cstring>

static bool RefreshOrClearPersistent(CGenericPersistentSound*& persistent) {
    if (!persistent) {
        return false;
    }

    if (!rsdPersistent::ObjectExists(persistent->persist)) {
        persistent->End();
        delete persistent;
        persistent = nullptr;
        return false;
    }

    persistent->persist->UpdateSpatial();
    return true;
}

// PSX: _20CParticleEffectSound (ESOUND.CPP:23, 0x800ACC9C)
CParticleEffectSound::CParticleEffectSound() {
    MARKFUNCTION(0x800ACC9C);
    loadByte0 = 0;
    persistentId = -1;
    pad18 = 0;
    persistent = nullptr;
}

// PSX: __20CParticleEffectSound (ESOUND.CPP:43, 0x800ACCDC)
CParticleEffectSound::~CParticleEffectSound() {
    MARKFUNCTION(0x800ACCDC);
    StopAnimating();
}

// PSX: Initialize__20CParticleEffectSoundPC10tagLVector (ESOUND.CPP:61, 0x800ACD38)
s32 CParticleEffectSound::Initialize(const LVector* pos) {
    MARKFUNCTION(0x800ACD38);
    return CSound::Initialize(const_cast<LVector*>(pos));
}

// PSX: Load__20CParticleEffectSoundPCc (ESOUND.CPP:84, 0x800ACD58)
s32 CParticleEffectSound::Load(const void* data) {
    MARKFUNCTION(0x800ACD58);

    const u8* bytes = static_cast<const u8*>(data);
    loadByte0 = bytes[0];
    persistentId = static_cast<s8>(bytes[1]);
    return 0;
}

// PSX: StartAnimating__20CParticleEffectSound (ESOUND.CPP:105, 0x800ACD70)
s32 CParticleEffectSound::StartAnimating() {
    MARKFUNCTION(0x800ACD70);
    if (RefreshOrClearPersistent(persistent)) {
        return -3000;
    }
    return BeginPersistent(static_cast<u8>(persistentId), &persistent);
}

// PSX: StopAnimating__20CParticleEffectSound (ESOUND.CPP:123, 0x800ACD94)
s32 CParticleEffectSound::StopAnimating() {
    MARKFUNCTION(0x800ACD94);
    return EndPersistent(&persistent);
}

// PSX: _17CWorldEffectSound (ESOUND.CPP:141, 0x800ACDB4)
CWorldEffectSound::CWorldEffectSound() {
    MARKFUNCTION(0x800ACDB4);
    loadByte0 = 0;
    persistentId = -1;
    startFrame = 0;
    endFrame = 0;
    soundFlags = 0;
    updateFlags = 0;
    pad26 = 0;
    persistent = nullptr;
}

// PSX: __17CWorldEffectSound (ESOUND.CPP:163, 0x800ACDF8)
CWorldEffectSound::~CWorldEffectSound() {
    MARKFUNCTION(0x800ACDF8);
    StopAnimating();
}

// PSX: Initialize__17CWorldEffectSoundPC10tagLVector (ESOUND.CPP:181, 0x800ACE54)
s32 CWorldEffectSound::Initialize(const LVector* pos) {
    MARKFUNCTION(0x800ACE54);
    return CSound::Initialize(const_cast<LVector*>(pos));
}

// PSX: Load__17CWorldEffectSoundPCc (ESOUND.CPP:204, 0x800ACE74)
s32 CWorldEffectSound::Load(const void* data) {
    MARKFUNCTION(0x800ACE74);

    u32 packed0 = 0;
    u32 packed1 = 0;
    u16 packed2 = 0;

    std::memcpy(&packed0, data, sizeof(packed0));
    std::memcpy(&packed1, static_cast<const u8*>(data) + 4, sizeof(packed1));
    std::memcpy(&packed2, static_cast<const u8*>(data) + 8, sizeof(packed2));

    loadByte0 = static_cast<u8>(packed0 & 0xFFu);
    persistentId = static_cast<s8>((packed0 >> 8) & 0xFFu);
    startFrame = static_cast<u16>((packed0 >> 16) & 0xFFFFu);

    endFrame = static_cast<u16>(packed1 & 0xFFFFu);
    soundFlags = static_cast<u16>((packed1 >> 16) & 0xFFFFu);
    updateFlags = packed2;

    // PSX mirrors the loaded sound flags into CSound::flags.
    flags = soundFlags;
    return 0;
}

// PSX: StartAnimating__17CWorldEffectSound (ESOUND.CPP:224, 0x800ACEB0)
s32 CWorldEffectSound::StartAnimating() {
    MARKFUNCTION(0x800ACEB0);
    if (RefreshOrClearPersistent(persistent)) {
        return -3000;
    }
    return BeginPersistent(static_cast<u8>(persistentId), &persistent);
}

// PSX: Update__17CWorldEffectSoundUl (ESOUND.CPP:246, 0x800ACED4)
s32 CWorldEffectSound::Update(u32 frame) {
    MARKFUNCTION(0x800ACED4);

    u32 start = startFrame;
    if (startFrame != 0) {
        if (frame < start) {
            if (frame < endFrame) {
                return 0;
            }
        }
        else if (frame < endFrame) {
            StartAnimating();
            VolRiseAndFall(frame);
            return 0;
        }

        StopAnimating();
        return 0;
    }

    if (endFrame != 0) {
        start = startFrame;
        if (frame < start) {
            if (frame < endFrame) {
                return 0;
            }
        }
        else if (frame < endFrame) {
            StartAnimating();
            VolRiseAndFall(frame);
            return 0;
        }

        StopAnimating();
        return 0;
    }

    StartAnimating();
    return 0;
}

// PSX: StopAnimating__17CWorldEffectSound (ESOUND.CPP:278, 0x800ACF90)
s32 CWorldEffectSound::StopAnimating() {
    MARKFUNCTION(0x800ACF90);
    return EndPersistent(&persistent);
}

// PSX: VolRiseAndFall__17CWorldEffectSoundUl (ESOUND.CPP:297, 0x800ACFB0)
s32 CWorldEffectSound::VolRiseAndFall(u32 frame) {
    MARKFUNCTION(0x800ACFB0);

    if ((updateFlags & 1u) != 0u) {
        const u32 midpoint = (static_cast<u32>(startFrame) + static_cast<u32>(endFrame)) >> 1;
        ASSERT(midpoint != 0);
        if (midpoint == 0) {
            return 0;
        }

        u32 volume = 0;
        if (frame >= midpoint) {
            volume = 100u - (100u * (frame - midpoint) / midpoint);
        }
        else {
            volume = 100u * frame / midpoint;
        }

        if (persistent) {
            persistent->SetVol(static_cast<u8>(volume));
        }
    }

    return 0;
}
