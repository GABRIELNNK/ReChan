// sndfact.h - CSoundFactory reversed from PSX SNDFACT.CPP
// PSX source: C:\CHAN\GAME\SRC\SND\SNDFACT.CPP
// CSoundFactory creates sound objects by type ID.
// On PSX it uses memory pools; on PC it uses new/delete.
#pragma once

#include "core.h"

class CSound;

// PSX: CSoundFactory (SNDFACT.CPP)
// Type IDs:
//   10070 = CGenericTransientSound (28 bytes)
//   10080 = CGenericPersistentSound (24 bytes)
//   (other types not yet reversed)
class CSoundFactory {
public:
    // PSX: CreateObject__13CSoundFactoryUlPP6CSoundUl (SNDFACT.CPP:178, 0x8005759C)
    // Creates a sound object of the given type, sets soundId via Load.
    // outObj: receives the created object pointer (set to null on failure)
    // Returns 0 on success, negative error code on failure
    static s32 CreateObject(u32 typeId, CSound** outObj, u32 soundId = 0);

    // PSX: ObjectCreated__13CSoundFactory / ObjectDestroyed__13CSoundFactory
    // Bookkeeping (no-op on PC)
    static void ObjectCreated();
    static void ObjectDestroyed();
};
