#include "snd/prstsnd.h"
#include "snd/sndmath.h"
#include "snd/rsdworld.h"

// PSX: g_persistData (0x800DB2EC) - extracted from GAME_REL.CPE binary
// 46 entries, each 2 x u32 (8 bytes), accessed as byte arrays
u32 g_persistData[92] = {
    787410480, 25640, 787410480, 25640, 787410480, 25640, 12643891, 25606,
    264253, 25690, 573, 25690, 61, 25690, 134217789, 25690,
    234922038, 25607, 15395893, 25660, 12584456, 25600, 4211270, 25614,
    2147483149, 25645, 250490419, 25605, 32432180, 25604, 247513151, 25620,
    2105, 25665, 917574, 25675, 2617, 25630, 41020, 25700,
    16438, 25660, 2113, 25675, 401473, 25645, 6291521, 25650,
    14680131, 25635, 49213, 25640, 234881087, 28180, 3636, 25635,
    565, 25602, 924219, 25608, 15597634, 25620, 23068736, 25700,
    920127, 25660, 536870962, 25620, 67108917, 25650, 551567421, 25625,
    2147483167, 25623, 235667512, 25675, 235667517, 25665, 247463998, 25612,
    917566, 25660, 1308622900, 25685, 917562, 25675, 12582978, 25606,
    8388668, 25635, 8, 0
};

// PSX: _23CGenericPersistentSound (PRSTSND.CPP:18, 0x800AC280)
CGenericPersistentSound::CGenericPersistentSound() {
    MARKFUNCTION(0x800AC280);
    // PSX: CSound base, then vtable, persist=0, persistId=0xFF
    loadByte0 = 0;
    persistId = 0xFF;
    pad18 = 0;
    persist = nullptr;
}

// PSX: __23CGenericPersistentSound (PRSTSND.CPP:22, 0x800AC2CC)
CGenericPersistentSound::~CGenericPersistentSound() {
    MARKFUNCTION(0x800AC2CC);
    // PSX: set vtable, End(), then ~CSound
    End();
}

// PSX: Initialize__23CGenericPersistentSoundPC10tagLVectorUs (PRSTSND.CPP:45, 0x800AC3AC)
s32 CGenericPersistentSound::Initialize(void* pos, u16 f) {
    MARKFUNCTION(0x800AC3AC);
    flags = f;
    posPtr = pos;
    return 0;
}

// PSX: Initialize__23CGenericPersistentSoundPC10tagLVector (PRSTSND.CPP:117, 0x800AC564)
s32 CGenericPersistentSound::Initialize(void* pos) {
    MARKFUNCTION(0x800AC564);
    posPtr = pos;
    return 0;
}

// PSX: Begin__23CGenericPersistentSound (PRSTSND.CPP:51, 0x800AC3CC)
s32 CGenericPersistentSound::Begin() {
    MARKFUNCTION(0x800AC3CC);

    if (persist) {
        if (rsdPersistent::ObjectExists(persist)) {
            return -3000;
        }

        delete persist;
        persist = nullptr;
    }

    const u8* data = reinterpret_cast<const u8*>(&g_persistData[2 * persistId]);

    u8 sampleId = data[0];
    u16 psxVol = PsxVol100(data[4]);
    s16 psxPitch = PsxPitch200(data[5]);
    u8 reverb = data[6];

    persist = new rsdPersistent(sampleId, posPtr, reverb, psxVol, psxPitch, flags);
    if (!persist) {
        return -200;
    }

    return 0;
}

// PSX: End__23CGenericPersistentSound (PRSTSND.CPP:77, 0x800AC498)
s32 CGenericPersistentSound::End() {
    MARKFUNCTION(0x800AC498);

    if (!persist) {
        return -3001;
    }

    delete persist;
    persist = nullptr;
    return 0;
}

// PSX: SetVol__23CGenericPersistentSoundUc (PRSTSND.CPP:27, 0x800AC328)
s32 CGenericPersistentSound::SetVol(u8 vol) {
    MARKFUNCTION(0x800AC328);

    if (!persist) {
        return -3001;
    }

    // PSX: reads g_persistData[2*persistId] byte 4 as base volume,
    // multiplies by vol/100, converts to PSX SPU volume
    const u8* data = reinterpret_cast<const u8*>(&g_persistData[2 * persistId]);
    u16 baseVol = data[4];
    u16 scaledVol = (u16)(baseVol * (u32)vol / 100);
    u16 psxVol = PsxVol100(scaledVol);

    persist->SetVolume(psxVol);
    return 0;
}

// PSX: Load__23CGenericPersistentSoundPCc (PRSTSND.CPP:93, 0x800AC4F8)
s32 CGenericPersistentSound::Load(const void* data) {
    MARKFUNCTION(0x800AC4F8);
    // PSX: copies 2 bytes from load data to +16 and +17
    const u8* bytes = reinterpret_cast<const u8*>(data);
    loadByte0 = bytes[0];
    persistId = bytes[1];
    return 0;
}
