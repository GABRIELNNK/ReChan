#include "snd/sndfact.h"
#include "snd/basesnd.h"
#include "snd/esound.h"
#include "snd/trnssnd.h"
#include "snd/prstsnd.h"
#include "snd/hmndsnd.h"
#include "snd/kndnsnd.h"
#include "snd/kicksnd.h"
#include "snd/dstrsnd.h"
#include "snd/platsnd.h"
#include "snd/pendsnd.h"
#include "snd/pushsnd.h"
#include "snd/wpnsnd.h"
#include "snd/rsevent.h"

struct PlatformSoundLoadData {
    u8 pad[2];
    s16 beginMoveSfx;
    s16 tiltSfx;
    s16 impactSfx;
    s16 hitHumanoidSfx;
    s16 hitPathNodeSfx;
    s16 endMoveSfx;
    s8 movePersistentId;
    s8 idlePersistentId;
    s32 material;
    s16 soundFlags;
    s8 hitPathNodeCooldownReset;
    s8 pad23;
};

struct WorldEffectSoundLoadData {
    u8 pad0;
    s8 persistentId;
    u16 startFrame;
    u16 endFrame;
    u16 soundFlags;
    u8 updateFlags;
    u8 pad9;
};

struct ParticleEffectSoundLoadData {
    u8 pad0;
    s8 persistentId;
};

struct PushableSoundLoadData {
    u8 pad0;
    u8 pad1;
    s16 kickSfx;
    s8 loopPersistentId;
    s8 pad5;
    u16 pad6;
    s32 material;
};

struct DestructibleSoundLoadData {
    u16 pad16;
    u16 smashSfx;
    s32 material;
};

struct PendulumSoundLoadData {
    u16 pad16;
    s16 swingSfx;
    s16 hitHumanoidSfx;
};

CKnockDownSound::CKnockDownSound() {
    MARKFUNCTION(0x800AD4DC);
    persistentFallSound = nullptr;
    multipleKickTimer = 0;
    multipleImpactTimer = 0;
}

CKnockDownSound::~CKnockDownSound() {
    MARKFUNCTION(0x800AD51C);
    EndFall();
}

s32 CKnockDownSound::Initialize(const LVector* pos) {
    MARKFUNCTION(0x800AD3A8);
    return CSound::Initialize((void*)pos);
}

s32 CKnockDownSound::BeginFall() {
    MARKFUNCTION(0x800AD3C8);
    return BeginPersistent(pub.fallPersistentId, &persistentFallSound);
}

s32 CKnockDownSound::EndFall() {
    MARKFUNCTION(0x800AD3EC);
    return EndPersistent(&persistentFallSound);
}

s32 CKnockDownSound::Kick() {
    MARKFUNCTION(0x800AD40C);

    if (multipleKickTimer == 0) {
        multipleKickTimer = 100;
        PlayTransient(pub.kickSound, 5, 8);
    }

    return 0;
}

s32 CKnockDownSound::Impact() {
    MARKFUNCTION(0x800AD44C);

    if (multipleImpactTimer == 0) {
        multipleImpactTimer = 100;
        PlayTransient(pub.impactSound, 5, 8);
    }

    return 0;
}

s32 CKnockDownSound::HitHumanoid() {
    MARKFUNCTION(0x800AD48C);
    return Kick();
}

s32 CKnockDownSound::Think() {
    MARKFUNCTION(0x800AD4AC);

    if (multipleKickTimer) {
        multipleKickTimer--;
    }
    if (multipleImpactTimer) {
        multipleImpactTimer--;
    }

    return 0;
}

s32 CKnockDownSound::Load(const void* data) {
    MARKFUNCTION(0x800AD578);
    std::memcpy(&pub, data, sizeof(pub));
    return 0;
}

CKickNRollSound::CKickNRollSound() {
    MARKFUNCTION(0x800AD2B4);
    rollPersistent = nullptr;
    kickCooldown = 0;
}

CKickNRollSound::~CKickNRollSound() {
    MARKFUNCTION(0x800AD2F0);
    EndRoll();
}

s32 CKickNRollSound::BeginRoll() {
    MARKFUNCTION(0x800AD210);
    return BeginPersistent(rollPersistentId, &rollPersistent);
}

s32 CKickNRollSound::EndRoll() {
    MARKFUNCTION(0x800AD234);
    EndPersistent(&rollPersistent);
    return 0;
}

s32 CKickNRollSound::Kick() {
    MARKFUNCTION(0x800AD254);

    if (kickCooldown == 0) {
        kickCooldown = 5;
        return PlayTransient(kickSound, 8, 0);
    }

    return 100;
}

s32 CKickNRollSound::HitHumanoid() {
    MARKFUNCTION(0x800AD294);
    return Kick();
}

s32 CKickNRollSound::Load(const void* data) {
    MARKFUNCTION(0x800AD34C);
    std::memcpy(&pad16, data, sizeof(u32));
    rollPersistentId = *(static_cast<const u8*>(data) + 4);
    return 0;
}

s32 CKickNRollSound::Initialize(const LVector* pos) {
    MARKFUNCTION(0x800AD36C);
    return CSound::Initialize((void*)pos);
}

s32 CKickNRollSound::Think() {
    MARKFUNCTION(0x800AD38C);
    if (kickCooldown != 0) {
        kickCooldown--;
    }
    return 0;
}

// PSX: IsBasicSoundLoaded__21CSoundFactoryDatabaseUl (SNDFDB.CPP:1011, 0x800AC240)
// LocInfo[loc][0] values at 0x800D6588 (GAME_REL.psx.lst):
//   loc 0..20 -> 1..21
//   loc 21..24 -> 22
static const u8 s_locInfoLoadIndex[25] = {
    1, 2, 3, 4, 5,
    6, 7, 8, 9, 10,
    11, 12, 13, 14, 15,
    16, 17, 18, 19, 20,
    21, 22, 22, 22, 22
};

static s32 IsBasicSoundLoaded(u32 packedSoundWord) {
    if (g_currentSoundLocation < 0 || g_currentSoundLocation >= 25) {
        return -5000;
    }

    const u32 locationIndex = s_locInfoLoadIndex[g_currentSoundLocation];
    const u32 requiredMask = (1u << (locationIndex + 8));
    return (packedSoundWord & requiredMask) ? 0 : -5000;
}

// PSX: CreatePushableSound__21CSoundFactoryDatabaseP6CSoundUl (SNDFDB.CPP:299, 0x800AB234)
static s32 FillPushableSoundLoadData(u32 soundId, PushableSoundLoadData& data) {
    data = {};
    data.kickSfx = -1;
    data.loopPersistentId = -1;
    data.material = 2;

    switch (soundId) {
        case 0x06922904:
            data.loopPersistentId = 7;
            data.kickSfx = 184;
            return 0;
        case 0x0C47AC72:
            data.loopPersistentId = 5;
            data.kickSfx = 183;
            data.material = 4;
            return 0;
        case 0x096B8430:
        case 0x0DAA3081:
        case 0x003C2533:
            data.loopPersistentId = 3;
            data.kickSfx = 181;
            data.material = 3;
            return 0;
        case 0x000483F5:
            data.loopPersistentId = 4;
            data.kickSfx = 185;
            return 0;
        case 0x00047674:
            data.loopPersistentId = 6;
            data.kickSfx = 182;
            return 0;
        default:
            return -100;
    }
}

// PSX: CreatePlatformSound__21CSoundFactoryDatabaseP6CSoundUl (SNDFDB.CPP:372)
static void FillPlatformSoundLoadData(u32 soundId, PlatformSoundLoadData& data) {
    data = {};
    data.beginMoveSfx = -1;
    data.tiltSfx = -1;
    data.impactSfx = -1;
    data.hitHumanoidSfx = -1;
    data.hitPathNodeSfx = -1;
    data.endMoveSfx = -1;
    data.movePersistentId = -1;
    data.idlePersistentId = -1;
    data.material = 2;
    data.soundFlags = 0;
    data.hitPathNodeCooldownReset = 60;

    if (soundId == 113565765) {
        data.movePersistentId = 19;
        data.hitHumanoidSfx = 215;
        data.beginMoveSfx = 199;
        data.hitPathNodeSfx = 199;
        data.soundFlags = 20000;
        return;
    }

    if (soundId <= 0x6C4E045) {
        if (soundId == 65551800) {
            goto LABEL_70;
        }

        if (soundId <= 0x3E83DB8) {
            if (soundId == 6862914) {
                goto LABEL_68;
            }

            if (soundId <= 0x68B842) {
                if (soundId != 101) {
                    if (soundId < 0x66) {
                        if (soundId != 97) {
                            data.impactSfx = 193;
                            return;
                        }
                        goto LABEL_70;
                    }

                    if (soundId != 453670) {
                        data.impactSfx = 193;
                        return;
                    }

LABEL_68:
                    data.movePersistentId = 18;
                    data.tiltSfx = 195;
                    data.hitHumanoidSfx = 215;
                    data.material = 4;
                    data.soundFlags = 2000;
                    return;
                }

                goto LABEL_70;
            }

            if (soundId == 7587989) {
                goto LABEL_68;
            }

            if (soundId <= 0x73C895) {
                if (soundId != 6874290) {
                    data.impactSfx = 193;
                    return;
                }
                goto LABEL_70;
            }

            if (soundId == 28780548) {
LABEL_70:
                data.beginMoveSfx = 204;
                data.impactSfx = 193;
                goto LABEL_82;
            }

            if (soundId != 46960034) {
                data.impactSfx = 193;
                return;
            }

LABEL_72:
            data.movePersistentId = 15;
            data.beginMoveSfx = 210;
            data.endMoveSfx = 210;
            return;
        }

        if (soundId > 0x5809832) {
            if (soundId == 93670758) {
                data.movePersistentId = 23;
                data.beginMoveSfx = 205;
                data.endMoveSfx = 206;
                data.soundFlags = 4000;
                return;
            }

            if (soundId < 0x5954D66 || soundId > 0x6A9C359 || soundId < 0x6A9C351) {
                goto LABEL_85;
            }
        }
        else {
            if (soundId >= 0x5809831) {
                goto LABEL_72;
            }

            if (soundId != 85907506) {
                if (soundId <= 0x51ED832) {
                    if (soundId <= 0x4A0AC22 && soundId >= 0x4A0AC21) {
                        goto LABEL_72;
                    }
                    goto LABEL_85;
                }

                if (soundId != 88477860) {
                    data.impactSfx = 193;
                    return;
                }

LABEL_71:
                data.tiltSfx = 194;
                goto LABEL_82;
            }

            data.movePersistentId = 26;
        }

LABEL_84:
        data.material = 1;
        return;
    }

    if (soundId == 177858177) {
        goto LABEL_74;
    }

    if (soundId > 0xA99E681) {
        if (soundId == 198801723) {
LABEL_76:
            data.movePersistentId = 27;
            data.hitPathNodeSfx = 200;
            data.endMoveSfx = 201;
LABEL_77:
            data.hitHumanoidSfx = 215;
            data.soundFlags = 7000;
            return;
        }

        if (soundId > 0xBD9793B) {
            if (soundId == 243816935) {
                data.movePersistentId = 20;
                return;
            }

            if (soundId <= 0xE8859E7) {
                if (soundId <= 0xE44C202 && soundId >= 0xE44C201) {
                    data.beginMoveSfx = 203;
                    data.endMoveSfx = 203;
                    data.movePersistentId = 24;
                    return;
                }

                goto LABEL_85;
            }

            if (soundId != 245997170) {
                data.impactSfx = 193;
                return;
            }
        }
        else if (soundId != 177858401) {
            if (soundId > 0xA99E761) {
                if (soundId == 178009377) {
                    goto LABEL_84;
                }

                if (soundId != 179951756) {
                    data.impactSfx = 193;
                    return;
                }

                data.movePersistentId = 17;
                data.beginMoveSfx = 210;
                goto LABEL_77;
            }

            if (soundId != 177858179) {
                data.impactSfx = 193;
                return;
            }
        }

LABEL_74:
        data.beginMoveSfx = 197;
        data.hitHumanoidSfx = 215;
        return;
    }

    if (soundId > 0x7B9BA12) {
        if (soundId == 159859508) {
            data.beginMoveSfx = 203;
            goto LABEL_85;
        }

        if (soundId > 0x9874334) {
            if (soundId != 159873444) {
                data.impactSfx = 193;
                return;
            }

            data.movePersistentId = 25;
            data.beginMoveSfx = 207;
            data.endMoveSfx = 208;
LABEL_82:
            data.material = 3;
            return;
        }

        if (soundId == 145433249) {
            data.movePersistentId = 22;
        }
        else {
            data.impactSfx = 193;
        }
        return;
    }

    if (soundId >= 0x7B9BA11) {
        goto LABEL_76;
    }

    if (soundId > 0x7358985) {
        if (soundId == 121252748) {
            data.movePersistentId = 21;
            data.beginMoveSfx = 210;
            data.soundFlags = 6000;
        }
        else {
            data.impactSfx = 193;
        }
        return;
    }

    if (soundId >= 0x7358982) {
        goto LABEL_71;
    }

    if (soundId == 120076687) {
        data.movePersistentId = 16;
        data.beginMoveSfx = 202;
        data.hitPathNodeSfx = 201;
        data.hitHumanoidSfx = 215;
        data.hitPathNodeCooldownReset = 30;
        data.soundFlags = 3200;
    }
    else {
LABEL_85:
        data.impactSfx = 193;
    }
}

// PSX: CreateWorldEffectSound__21CSoundFactoryDatabaseP6CSoundUl (SNDFDB.CPP:216)
static s32 FillWorldEffectSoundLoadData(u32 soundId, WorldEffectSoundLoadData& data) {
    data = {};
    data.persistentId = -1;

    switch (soundId) {
        case 0x06270125:
            data.persistentId = 30;
            data.startFrame = 19;
            data.endFrame = 63;
            return 0;
        case 0x0AC36654:
            data.persistentId = 43;
            return 0;
        case 0x0D5083CC:
            data.persistentId = 38;
            return 0;
        case 0x0CA0D800:
        case 0x0E402AA0:
        case 0x02E7CA59:
            data.persistentId = 31;
            data.soundFlags = 5300;
            return 0;
        case 0x0655C107:
        case 0x0465C147:
            data.persistentId = 28;
            data.startFrame = 1;
            data.endFrame = 15;
            return 0;
        case 0x08A88A45:
            data.persistentId = 32;
            return 0;
        case 0x04A6117F:
            data.persistentId = 33;
            data.soundFlags = 2000;
            return 0;
        case 0x04B061A3:
            data.persistentId = 44;
            return 0;
        case 0x05850678:
            data.persistentId = 40;
            return 0;
        case 0x004C4252:
            data.persistentId = 41;
            return 0;
        default:
            return -100;
    }
}

// PSX: CreateParticleEffectSound__21CSoundFactoryDatabaseP6CSoundUl (SNDFDB.CPP:165)
static s32 FillParticleEffectSoundLoadData(u32 soundId, ParticleEffectSoundLoadData& data) {
    data = {};
    data.persistentId = -1;

    switch (soundId) {
        case 88635922:
        case 8889404:
            data.persistentId = 35;
            return 0;
        case 60675069:
        case 208540507:
            data.persistentId = 29;
            return 0;
        case 102753076:
            data.persistentId = 39;
            return 0;
        case 223416785:
            data.persistentId = 37;
            return 0;
        case 75930324:
        case 185475524:
            data.persistentId = 36;
            return 0;
        default:
            return -100;
    }
}

// PSX: CreateDestructibleSound__21CSoundFactoryDatabaseP6CSoundUl (SNDFDB.CPP:718)
static void FillDestructibleSoundLoadData(u32 soundId, DestructibleSoundLoadData& data) {
    data = {};
    data.material = 3;

    u16 smashSfx = 177;

    if (soundId == 0x05064231) {
        smashSfx = 169;
    }
    else if (soundId > 0x05064231) {
        if (soundId == 0x096B8433) {
            smashSfx = 163;
        }
        else if (soundId > 0x096B8433) {
            if (soundId == 0x0A6C4D22) {
                smashSfx = 176;
            }
            else if (soundId > 0x0A6C4D22) {
                if (soundId == 0x0A7A14E1) {
                    smashSfx = 166;
                }
                else if (soundId > 0x0A7A14E1) {
                    if (soundId == 0x0C995E24) {
                        smashSfx = 163;
                    }
                    else {
                        smashSfx = 177;
                    }
                }
                else {
                    smashSfx = 177;
                    if (soundId >= 0x0A790AE1 && soundId <= 0x0A790AE2) {
                        data.material = 2;
                        smashSfx = 171;
                    }
                }
            }
            else {
                if (soundId == 0x0997D201) {
                    data.material = 2;
                    smashSfx = 171;
                }
                else if (soundId <= 0x0997D201) {
                    if (soundId == 0x0995E2B2) {
                        smashSfx = 163;
                    }
                    else {
                        smashSfx = 177;
                    }
                }
                else if (soundId == 0x09C33D91 || soundId == 0x09C34277) {
                    smashSfx = 162;
                }
                else {
                    smashSfx = 177;
                }
            }
        }
        else {
            if (soundId == 0x06E3A834) {
                smashSfx = 163;
            }
            else if (soundId <= 0x06E3A834) {
                if (soundId == 0x06922904) {
                    data.material = 2;
                    smashSfx = 168;
                }
                else {
                    smashSfx = 177;
                    if (soundId >= 0x06B9C331 && soundId <= 0x06B9C334) {
                        smashSfx = 162;
                    }
                }
            }
            else if (soundId == 0x08243154 || soundId == 0x08743364) {
                smashSfx = 161;
            }
            else if (soundId <= 0x08243154) {
                if (soundId == 0x08085A3E) {
                    data.material = 2;
                    smashSfx = 165;
                }
                else {
                    smashSfx = 177;
                }
            }
            else if (soundId == 0x090AEE4C) {
                data.material = 2;
                smashSfx = 171;
            }
            else {
                smashSfx = 177;
            }
        }
    }
    else if (soundId == 0x004B14A2) {
        smashSfx = 167;
    }
    else if (soundId > 0x004B14A2) {
        if (soundId == 0x02FDFA44) {
            smashSfx = 169;
        }
        else if (soundId > 0x02FDFA44) {
            if (soundId == 0x04C9F96C) {
                data.material = 2;
                smashSfx = 170;
            }
            else if (soundId <= 0x04C9F96C) {
                smashSfx = 177;
                if (soundId >= 0x04317272 && soundId <= 0x04317273) {
                    smashSfx = 161;
                }
            }
            else if (soundId == 0x04D82842) {
                smashSfx = 161;
            }
            else {
                smashSfx = 177;
            }
        }
        else if (soundId == 0x00551544) {
            data.material = 1;
            smashSfx = 172;
        }
        else if (soundId <= 0x00551544) {
            if (soundId == 0x004CCDD5) {
                data.material = 1;
                smashSfx = 175;
            }
            else {
                smashSfx = 177;
            }
        }
        else if (soundId == 0x0284C70C) {
            smashSfx = 161;
        }
        else {
            smashSfx = 177;
        }
    }
    else if (soundId == 0x000079DA || soundId == 0x000079E2 || soundId == 0x000079D7) {
        smashSfx = 173;
    }
    else if (soundId >= 0x000079DB) {
        if (soundId == 0x00047796) {
            smashSfx = 174;
        }
        else if (soundId <= 0x00047796) {
            if (soundId == 0x000079E2) {
                smashSfx = 173;
            }
            else {
                smashSfx = 177;
            }
        }
        else if (soundId == 0x00068B7D) {
            smashSfx = 164;
        }
        else if (soundId == 0x0007C895) {
            data.material = 1;
            smashSfx = 172;
        }
        else {
            smashSfx = 177;
        }
    }
    else if (soundId == 0x00000453) {
        data.material = 2;
        smashSfx = 165;
    }
    else if (soundId >= 0x00000454) {
        if (soundId == 0x000079D7) {
            smashSfx = 173;
        }
        else {
            smashSfx = 177;
        }
    }
    else if (soundId == 0x00000063) {
        smashSfx = 163;
    }
    else {
        smashSfx = 177;
    }

    data.smashSfx = smashSfx;
}

// PSX: CreatePendulumSound__21CSoundFactoryDatabaseP6CSoundUl (SNDFDB.CPP:859, 0x800ABF54)
static s32 FillPendulumSoundLoadData(u32 soundId, PendulumSoundLoadData& data) {
    data.pad16 = 0;
    data.swingSfx = -1;
    data.hitHumanoidSfx = 192;

    switch (soundId) {
        case 0x0076F84F:
            data.swingSfx = 190;
            data.hitHumanoidSfx = 191;
            return 0;
        case 0x04BC3645:
            data.swingSfx = 186;
            return 0;
        case 0x04BC3644:
            data.swingSfx = 187;
            return 0;
        case 0x04BC3641:
        case 0x0A4A4208:
            data.swingSfx = 188;
            return 0;
        case 0x0329A5F1:
        case 0x07329A54:
            data.swingSfx = 189;
            return 0;
        default:
            return -100;
    }
}

// PSX: CreateObject__13CSoundFactoryUlPP6CSoundUl (SNDFACT.CPP:178, 0x8005759C)
// Creates a CSound-derived object by type ID and loads the soundId into it.
// PSX flow: switch(typeId) -> allocate -> construct -> LoadObject -> Load(soundId data)
s32 CSoundFactory::CreateObject(u32 typeId, CSound** outObj, u32 soundId) {
    MARKFUNCTION(0x8005759C);

    *outObj = nullptr;

    switch (typeId) {
        case 10000:
        {
            CParticleEffectSound* obj = new CParticleEffectSound();
            ParticleEffectSoundLoadData data = {};
            s32 loadResult = FillParticleEffectSoundLoadData(soundId, data);
            if (loadResult < 0) {
                delete obj;
                return loadResult;
            }

            obj->Load(&data);
            *outObj = obj;
            break;
        }
        case 10010:
        {
            CWorldEffectSound* obj = new CWorldEffectSound();
            WorldEffectSoundLoadData data = {};
            s32 loadResult = FillWorldEffectSoundLoadData(soundId, data);
            if (loadResult < 0) {
                delete obj;
                return loadResult;
            }

            obj->Load(&data);
            *outObj = obj;
            break;
        }
        case 10020:
        {
            CPushableSound* obj = new CPushableSound();
            PushableSoundLoadData data = {};
            s32 loadResult = FillPushableSoundLoadData(soundId, data);
            if (loadResult < 0) {
                delete obj;
                return loadResult;
            }

            obj->Load(&data);
            *outObj = obj;
            break;
        }
        case 10030:
        {
            CKickNRollSound* obj = new CKickNRollSound();
            struct {
                u8 pad[2];
                s16 kickSfx;
                s8 rollPersistentId;
                s8 pad5;
            } data = {};
            data.kickSfx = 180;
            data.rollPersistentId = 9;
            obj->Load(&data);
            *outObj = obj;
            break;
        }
        case 10050:
        {
            CWeaponSound* obj = new CWeaponSound();
            u32 weaponIndex = 6;
            if (soundId != 101) {
                weaponIndex = soundId - 301;
            }
            if (weaponIndex >= 28) {
                delete obj;
                return -100;
            }

            struct {
                u16 field16;
                u16 hitHumanoid;
                u16 explodePrimary;
                u16 grab;
                u16 explodeSecondary;
                u16 miss;
            } data = {};

            data.hitHumanoid = static_cast<u16>(3 * weaponIndex + 47);
            data.explodePrimary = static_cast<u16>(3 * weaponIndex + 48);
            data.grab = static_cast<u16>(3 * weaponIndex + 46);
            data.explodeSecondary = 45;
            data.miss = 29;

            obj->Load(&data);
            *outObj = obj;
            break;
        }
        case 10040:
        {
            CPlatformSound* obj = new CPlatformSound();
            PlatformSoundLoadData data = {};
            FillPlatformSoundLoadData(soundId, data);
            obj->Load(&data);
            *outObj = obj;
            break;
        }
        case 10120:
        {
            CPendulumSound* obj = new CPendulumSound();
            PendulumSoundLoadData data = {};
            s32 loadResult = FillPendulumSoundLoadData(soundId, data);
            if (loadResult < 0) {
                delete obj;
                return loadResult;
            }

            obj->Load(&data);
            *outObj = obj;
            break;
        }
        case 10060:
        {
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
            }
            else {
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
        case 10070:
        {
            // PSX: CreateGenericTransientSound (SNDFDB.CPP)
            // Validates: soundId != 0xFFFF, soundId < 259, sample loaded
            if (soundId == 0xFFFF) {
                return -5010;
            }
            if (soundId >= 259) {
                return -1000;
            }
            if (IsBasicSoundLoaded(g_transData[2 * soundId]) < 0) {
                return -5000;
            }
            CGenericTransientSound* obj = new CGenericTransientSound();
            // PSX: writes u16 soundId at sp+18, calls Load(sp+16)
            // Load copies 4 bytes from data to obj+16, soundId at obj+18
            u32 loadData = (soundId & 0xFFFF) << 16;
            obj->Load(&loadData);
            *outObj = obj;
            break;
        }
        case 10080:
        {
            // PSX: CreateGenericPersistentSound (SNDFDB.CPP)
            // Validates: soundId < 45, soundId != 0xFF, sample loaded
            if (soundId >= 45) {
                return -1000;
            }
            if (soundId == 0xFF) {
                return -5010;
            }
            if (IsBasicSoundLoaded(g_persistData[2 * soundId]) < 0) {
                return -5000;
            }
            CGenericPersistentSound* obj = new CGenericPersistentSound();
            // PSX: writes u8 persistId at sp+17, calls Load(sp+16)
            u8 loadBytes[2] = { 0, (u8)(soundId & 0xFF) };
            obj->Load(loadBytes);
            *outObj = obj;
            break;
        }
        case 10090:
        {
            CDestructibleSound* obj = new CDestructibleSound();
            DestructibleSoundLoadData data = {};
            FillDestructibleSoundLoadData(soundId, data);
            obj->Load(&data);
            *outObj = obj;
            break;
        }
        case 10130:
        {
            CKnockDownSound* obj = new CKnockDownSound();
            CKnockDownPublishedData data = {};
            data.kickSound = 0x00B3;
            data.impactSound = 0x00B2;
            data.fallPersistentId = 0x0A;
            obj->Load(&data);
            *outObj = obj;
            break;
        }
        default:
            return -150;
    }

    if (!*outObj) {
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
