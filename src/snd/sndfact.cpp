// sndfact.cpp - CSoundFactory reversed from PSX SNDFACT.CPP
// PSX source: C:\CHAN\GAME\SRC\SND\SNDFACT.CPP
#include "snd/sndfact.h"
#include "snd/basesnd.h"
#include "snd/trnssnd.h"
#include "snd/prstsnd.h"

// PSX: CreateObject__13CSoundFactoryUlPP6CSoundUl (SNDFACT.CPP:178, 0x8005759C)
// Creates a CSound-derived object by type ID and loads the soundId into it.
// PSX flow: switch(typeId) -> allocate -> construct -> LoadObject -> Load(soundId data)
s32 CSoundFactory::CreateObject(u32 typeId, CSound** outObj, u32 soundId) {
    MARKFUNCTION(0x8005759C);

    *outObj = nullptr;

    switch (typeId) {
        case 10070: {
            // PSX: CreateGenericTransientSound (SNDFDB.CPP)
            // Validates: soundId != 0xFFFF, soundId < 259, sample loaded
            if (soundId == 0xFFFF)
            {
                return -5010;
            }
            if (soundId >= 259)
            {
                return -1000;
            }
            CGenericTransientSound* obj = new CGenericTransientSound();
            // PSX: writes u16 soundId at sp+18, calls Load(sp+16)
            // Load copies 4 bytes from data to obj+16, soundId at obj+18
            u32 loadData = (soundId & 0xFFFF) << 16;
            obj->Load(&loadData);
            *outObj = obj;
            break;
        }
        case 10080: {
            // PSX: CreateGenericPersistentSound (SNDFDB.CPP)
            // Validates: soundId < 45, soundId != 0xFF, sample loaded
            if (soundId >= 45)
            {
                return -1000;
            }
            if (soundId == 0xFF)
            {
                return -5010;
            }
            CGenericPersistentSound* obj = new CGenericPersistentSound();
            // PSX: writes u8 persistId at sp+17, calls Load(sp+16)
            u8 loadBytes[2] = { 0, (u8)(soundId & 0xFF) };
            obj->Load(loadBytes);
            *outObj = obj;
            break;
        }
        default:
            return -150;
    }

    if (!*outObj)
    {
        return -200;
    }

    return 0;
}

void CSoundFactory::ObjectCreated() {
    // PSX: bookkeeping counter (no-op on PC)
}

void CSoundFactory::ObjectDestroyed() {
    // PSX: bookkeeping counter (no-op on PC)
}
