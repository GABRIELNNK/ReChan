#pragma once
#include "core.h"

class CSound;

// PSX: CSoundFactory (SNDFACT.CPP)
// Type IDs:
//   10000 = CParticleEffectSound (24 bytes)
//   10010 = CWorldEffectSound (32 bytes)
//   10020 = CPushableSound (36 bytes)
//   10030 = CKickNRollSound (32 bytes)
//   10040 = CPlatformSound (56 bytes)
//   10060 = CHumanoidSound (132 bytes)
//   10070 = CGenericTransientSound (28 bytes)
//   10080 = CGenericPersistentSound (24 bytes)
//   10090 = CDestructibleSound (28 bytes)
//   10130 = CKnockDownSound (32 bytes)
//   10120 = CPendulumSound (24 bytes)
class CSoundFactory {
public:
    // Creates a sound object of the given type, sets soundId via Load.
    // outObj: receives the created object pointer (set to null on failure)
    // Returns 0 on success, negative error code on failure
    static s32 CreateObject(u32 typeId, CSound** outObj, u32 soundId = 0);

    // Bookkeeping (no-op on PC)
    static void ObjectCreated();
    static void ObjectDestroyed();
};
