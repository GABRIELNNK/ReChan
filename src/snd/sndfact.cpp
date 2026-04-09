// sndfact.cpp - CSoundFactory reversed from PSX SNDFACT.CPP
// PSX source: C:\CHAN\GAME\SRC\SND\SNDFACT.CPP
#include "snd/sndfact.h"
#include "snd/basesnd.h"
#include "snd/trnssnd.h"
#include "snd/prstsnd.h"
#include "snd/hmndsnd.h"

// PSX: CreateObject__13CSoundFactoryUlPP6CSoundUl (SNDFACT.CPP:178, 0x8005759C)
// Creates a CSound-derived object by type ID and loads the soundId into it.
// PSX flow: switch(typeId) -> allocate -> construct -> LoadObject -> Load(soundId data)
s32 CSoundFactory::CreateObject(u32 typeId, CSound** outObj, u32 soundId) {
    MARKFUNCTION(0x8005759C);

    *outObj = nullptr;

    switch (typeId) {
        case 10060: {
            // PSX: CreateHumanoidSound (SNDFDB.CPP:632, 0x800AB9BC)
            // soundId: 0 = enemy, non-zero = player (affects dialog sounds)
            CHumanoidSound* obj = new CHumanoidSound();
            struct {
                u8 pad[2];
                s16 footstep[4];
                s16 strafe;
                s16 diveRoll;
                s16 hitWorldStructure;
                s16 grab[4];
                s16 handPlant;
                s16 grabHumanoid;
                s16 punchKickMiss;
                s16 fall;
                s16 poleSwing;
                s16 punchHit[3];
                s16 kickHit[3];
                s16 superPunch;
                s16 superKick;
                s16 collapse[4];
                s16 dialogHit[3];
                s16 dialogAttack[3];
                s16 breath;
                s16 grunt;
                s16 hitByFireBlast;
                s8 stunPersistId;
                s8 slideSurfacePersistId;
                s8 slideLadderPersistId;
                s8 dialogHitChance;
                s8 dialogAttackChance;
            } data = {};
            data.footstep[0] = 3;
            data.footstep[1] = 1;
            data.footstep[2] = 0;
            data.footstep[3] = 2;
            data.strafe = 5;
            data.diveRoll = 11;
            data.hitWorldStructure = 12;
            data.grab[0] = 17;
            data.grab[1] = 18;
            data.grab[2] = 19;
            data.grab[3] = -1;
            data.handPlant = 16;
            data.grabHumanoid = 20;
            data.punchKickMiss = 26;
            data.fall = 26;
            data.poleSwing = 24;
            data.punchHit[0] = 30;
            data.punchHit[1] = 31;
            data.punchHit[2] = 32;
            data.kickHit[0] = 34;
            data.kickHit[1] = 35;
            data.kickHit[2] = 36;
            data.superPunch = 38;
            data.superKick = 37;
            data.collapse[0] = 41;
            data.collapse[1] = 42;
            data.collapse[2] = 43;
            data.collapse[3] = 44;
            data.breath = 131;
            data.hitByFireBlast = 39;
            data.stunPersistId = 12;
            data.slideSurfacePersistId = 13;
            data.slideLadderPersistId = 14;
            data.dialogHitChance = 30;
            data.dialogAttackChance = 30;
            if (soundId) {
                // player dialog sounds
                data.dialogHit[0] = 138;
                data.dialogHit[1] = 139;
                data.dialogHit[2] = 140;
                data.dialogAttack[0] = 144;
                data.dialogAttack[1] = 145;
                data.dialogAttack[2] = 146;
                data.grunt = -1;
            } else {
                // enemy dialog sounds
                data.dialogHit[0] = 135;
                data.dialogHit[1] = 136;
                data.dialogHit[2] = 137;
                data.dialogAttack[0] = 141;
                data.dialogAttack[1] = 142;
                data.dialogAttack[2] = 143;
                data.grunt = 133;
            }
            obj->Load(&data);
            *outObj = obj;
            break;
        }
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
