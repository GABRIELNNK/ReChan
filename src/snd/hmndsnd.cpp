#include "snd/hmndsnd.h"
#include "snd/prstsnd.h"
#include "snd/wpnsnd.h"
#include "ai/pickup.h"
#include "ai/humanoid.h"
#include "gen/common.h"
#include "p3d/p3dmath.h"
#include "xclib/xcfile.h"
#include <string.h>

// globals for sound script data
static u8* g_soundScriptData = nullptr;
static u8* g_soundScriptEvents = nullptr;

CHumanoidSound::CHumanoidSound() {
    MARKFUNCTION(0x80060678);
    memset(pad16, 0, 2);
    memset(footstep, 0, sizeof(footstep));
    strafe = 0;
    diveRoll = 0;
    hitWorldStructure = 0;
    memset(grab, 0, sizeof(grab));
    handPlant = 0;
    grabHumanoid = 0;
    punchKickMiss = 0;
    fall = 0;
    poleSwing = 0;
    memset(punchHit, 0, sizeof(punchHit));
    memset(kickHit, 0, sizeof(kickHit));
    superPunch = 0;
    superKick = 0;
    memset(collapse, 0, sizeof(collapse));
    memset(dialogHit, 0, sizeof(dialogHit));
    memset(dialogAttack, 0, sizeof(dialogAttack));
    breath = 0;
    grunt = 0;
    hitByFireBlast = 0;
    stunPersistId = 0;
    slideSurfacePersistId = 0;
    slideLadderPersistId = 0;
    dialogHitChance = 0;
    dialogAttackChance = 0;
    memset(pad97, 0, 3);
    scriptEntry = nullptr;
    scriptEventData = nullptr;
    scriptAnimId = -1;
    stunPersistent = nullptr;
    slideSurfacePersistent = nullptr;
    slideLadderPersistent = nullptr;
    humanoid = nullptr;
    attackCooldown = 0;
    hitCooldown = 0;
    fireCooldown = 0;
}

CHumanoidSound::~CHumanoidSound() {
    MARKFUNCTION(0x800606E0);
    EndStun();
    EndSlideOnSurface();
    EndSlideDownLadder();
}

s32 CHumanoidSound::Initialize(const void* pos, const Humanoid* owner) {
    MARKFUNCTION(0x80060658);
    humanoid = const_cast<Humanoid*>(owner);
    return CSound::Initialize(const_cast<void*>(pos));
}

s32 CHumanoidSound::Load(const void* data) {
    MARKFUNCTION(0x80060C74);
    // PSX: copies 80 bytes (20 dwords) + 2 extra bytes from data to this+16
    const u8* src = (const u8*)data;
    u8* dst = (u8*)pad16;
    memcpy(dst, src, 82);
    return 0;
}

// movement sounds

s32 CHumanoidSound::Footstep(CSoundMaterial mat) {
    MARKFUNCTION(0x8006074C);
    u32 idx = (u32)(mat - 1);
    u32 i = 0;
    if (idx < 4) {
        i = idx;
    }
    return PlayTransient(footstep[i], 8, 0);
}

s32 CHumanoidSound::Strafe(CSoundMaterial mat) {
    MARKFUNCTION(0x80060790);
    return PlayTransient(strafe, 8, 0);
}

s32 CHumanoidSound::Fall() {
    MARKFUNCTION(0x800607B8);
    return PlayTransient(fall, 8, 0);
}

s32 CHumanoidSound::Land(CSoundMaterial mat) {
    MARKFUNCTION(0x800607E0);
    u32 idx = (u32)(mat - 1);
    u32 i = 0;
    if (idx < 4) {
        i = idx;
    }
    PlayTransient(footstep[i], 8, 0);
    PlayTransient(footstep[i], 8, 7);
    return 0;
}

s32 CHumanoidSound::DiveRoll(CSoundMaterial mat) {
    MARKFUNCTION(0x80060850);
    return PlayTransient(diveRoll, 8, 0);
}

s32 CHumanoidSound::HitWorldStructure(CSoundMaterial mat) {
    MARKFUNCTION(0x80060878);
    return PlayTransient(hitWorldStructure, 8, 0);
}

s32 CHumanoidSound::HandPlant() {
    MARKFUNCTION(0x800608A0);
    return PlayTransient(handPlant, 8, 0);
}

s32 CHumanoidSound::Grab(CSoundMaterial mat) {
    MARKFUNCTION(0x800608C8);
    u32 idx = (u32)(mat - 1);
    u32 i = 0;
    if (idx < 4) {
        i = idx;
    }
    return PlayTransient(grab[i], 8, 0);
}

s32 CHumanoidSound::GrabHumanoid() {
    MARKFUNCTION(0x8006090C);
    return PlayTransient(grabHumanoid, 8, 0);
}

s32 CHumanoidSound::FrontFlip() {
    MARKFUNCTION(0x80060934);
    return PlayTransient(fall, 8, 0);
}

s32 CHumanoidSound::WallJump() {
    MARKFUNCTION(0x8006095C);
    return Footstep(SMAT_CONCRETE);
}

s32 CHumanoidSound::PoleSwing() {
    MARKFUNCTION(0x8006097C);
    return PlayTransient(poleSwing, 8, 0);
}

s32 CHumanoidSound::FlyThroughAir() {
    MARKFUNCTION(0x80060B48);
    return PlayTransient(fall, 8, 0);
}

// combat sounds

s32 CHumanoidSound::PunchMiss() {
    MARKFUNCTION(0x800609A4);
    return PlayAttack(punchKickMiss);
}

s32 CHumanoidSound::KickMiss() {
    MARKFUNCTION(0x800609C8);
    return PlayAttack(punchKickMiss);
}

s32 CHumanoidSound::PunchHit() {
    MARKFUNCTION(0x800609EC);
    s32 idx = (s32)rmRangedRandom(3);
    return PlayHit(punchHit[idx]);
}

s32 CHumanoidSound::KickHit() {
    MARKFUNCTION(0x80060A28);
    s32 idx = (s32)rmRangedRandom(3);
    return PlayHit(kickHit[idx]);
}

s32 CHumanoidSound::SuperPunch() {
    MARKFUNCTION(0x80060A64);
    if (rmRangedRandom(100) < 20) {
        return PlayHit(superPunch);
    }
    return PunchHit();
}

s32 CHumanoidSound::SuperKick() {
    MARKFUNCTION(0x80060AB4);
    if (rmRangedRandom(100) < 20) {
        return PlayHit(superKick);
    }
    return KickHit();
}

s32 CHumanoidSound::Collapse(CSoundMaterial mat) {
    MARKFUNCTION(0x80060B04);
    u32 idx = (u32)(mat - 1);
    u32 i = 0;
    if (idx < 4) {
        i = idx;
    }
    return PlayTransient(collapse[i], 8, 0);
}

// persistent state sounds

s32 CHumanoidSound::BeginStun() {
    MARKFUNCTION(0x80060B70);
    return BeginPersistent(stunPersistId, &stunPersistent);
}

s32 CHumanoidSound::EndStun() {
    MARKFUNCTION(0x80060B94);
    return EndPersistent(&stunPersistent);
}

s32 CHumanoidSound::BeginSlideOnSurface(CSoundMaterial mat) {
    MARKFUNCTION(0x80060BB4);
    return BeginPersistent(slideSurfacePersistId, &slideSurfacePersistent);
}

s32 CHumanoidSound::EndSlideOnSurface() {
    MARKFUNCTION(0x80060BD8);
    return EndPersistent(&slideSurfacePersistent);
}

s32 CHumanoidSound::BeginSlideDownLadder() {
    MARKFUNCTION(0x80060BF8);
    return BeginPersistent(slideLadderPersistId, &slideLadderPersistent);
}

s32 CHumanoidSound::EndSlideDownLadder() {
    MARKFUNCTION(0x80060C1C);
    return EndPersistent(&slideLadderPersistent);
}

s32 CHumanoidSound::EndAllSounds() {
    MARKFUNCTION(0x80060C3C);
    EndStun();
    EndSlideOnSurface();
    EndSlideDownLadder();
    return 0;
}

// dialog/vocal

s32 CHumanoidSound::FXDialogHit() {
    MARKFUNCTION(0x80060D1C);
    if (dialogHitChance < rmRangedRandom(100)) {
        return 0;
    }
    s32 idx = (s32)rmRangedRandom(3);
    return PlayTransient(dialogHit[idx], 1, 0);
}

s32 CHumanoidSound::FXDialogAttack() {
    MARKFUNCTION(0x80060D7C);
    if (dialogAttackChance < rmRangedRandom(100)) {
        return 0;
    }
    s32 idx = (s32)rmRangedRandom(3);
    return PlayTransient(dialogAttack[idx], 1, 0);
}

s32 CHumanoidSound::Breath() {
    MARKFUNCTION(0x80061260);
    return PlayTransient(breath, 8, 0);
}

s32 CHumanoidSound::Grunt() {
    MARKFUNCTION(0x80061288);
    if (rmRangedRandom(100) < 20) {
        return PlayTransient(grunt, 1, 0);
    }
    return 0;
}

s32 CHumanoidSound::HitByFireBlast() {
    MARKFUNCTION(0x800612D4);
    if (fireCooldown) {
        return 100;
    }
    u16 snd = hitByFireBlast;
    fireCooldown = 60;
    return PlayTransient(snd, 8, 0);
}

// weapon sounds

s32 CHumanoidSound::GrabWeapon() {
    MARKFUNCTION(0x80061170);
    if (!humanoid || !humanoid->rightHandObj) {
        return 0;
    }

    Pickup* pickup = static_cast<Pickup*>(humanoid->rightHandObj);
    CWeaponSound* weaponSound = pickup->GetWeaponSoundPtr();
    if (!weaponSound) {
        return 0;
    }

    return weaponSound->Grab();
}

s32 CHumanoidSound::WeaponMiss() {
    MARKFUNCTION(0x800611C0);
    if (!humanoid || !humanoid->rightHandObj) {
        return 0;
    }

    Pickup* pickup = static_cast<Pickup*>(humanoid->rightHandObj);
    CWeaponSound* weaponSound = pickup->GetWeaponSoundPtr();
    if (!weaponSound) {
        return 0;
    }

    return weaponSound->Miss();
}

s32 CHumanoidSound::WeaponHit() {
    MARKFUNCTION(0x80061210);
    if (!humanoid || !humanoid->rightHandObj) {
        return 0;
    }

    Pickup* pickup = static_cast<Pickup*>(humanoid->rightHandObj);
    CWeaponSound* weaponSound = pickup->GetWeaponSoundPtr();
    if (!weaponSound) {
        return 0;
    }

    return weaponSound->HitHumanoid();
}

// helpers

s32 CHumanoidSound::PlayAttack(u16 soundId) {
    MARKFUNCTION(0x80060DDC);
    if (attackCooldown) {
        return 100;
    }
    attackCooldown = 2;
    FXDialogAttack();
    return PlayTransient(soundId, 8, 0);
}

s32 CHumanoidSound::PlayHit(u16 soundId) {
    MARKFUNCTION(0x80060E40);
    if (hitCooldown) {
        return 100;
    }
    hitCooldown = 2;
    FXDialogHit();
    return PlayTransient(soundId, 8, 0);
}

void CHumanoidSound::Think() {
    MARKFUNCTION(0x80060EA4);
    if (hitCooldown) {
        hitCooldown--;
    }
    if (attackCooldown) {
        attackCooldown--;
    }
    if (fireCooldown) {
        fireCooldown--;
    }
}

s32 CHumanoidSound::MapSoundScriptEvent(SSHumanoid event) {
    MARKFUNCTION(0x80060EE8);
    if (!humanoid) {
        return 0;
    }

    switch (event) {
    case SS_FOOTSTEP:
        Footstep((CSoundMaterial)humanoid->field436);
        break;
    case SS_STRAFE:
        Strafe((CSoundMaterial)humanoid->field436);
        break;
    case SS_FALL:
        Fall();
        break;
    case SS_LAND:
    case SS_LAND2:
        Land((CSoundMaterial)humanoid->field436);
        break;
    case SS_HIT_WORLD:
        HitWorldStructure((CSoundMaterial)humanoid->field436);
        break;
    case SS_HAND_PLANT:
        HandPlant();
        break;
    case SS_GRAB:
        Grab(SMAT_CONCRETE);
        break;
    case SS_GRAB_HUMANOID:
        GrabHumanoid();
        break;
    case SS_FRONT_FLIP:
        FrontFlip();
        break;
    case SS_WALL_JUMP:
        WallJump();
        break;
    case SS_POLE_SWING:
        PoleSwing();
        break;
    case SS_PUNCH_MISS:
        PunchMiss();
        break;
    case SS_KICK_MISS:
        KickMiss();
        break;
    case SS_PUNCH_HIT:
        PunchHit();
        break;
    case SS_KICK_HIT:
        KickHit();
        break;
    case SS_SUPER_PUNCH:
        SuperPunch();
        break;
    case SS_SUPER_KICK:
        SuperKick();
        break;
    case SS_COLLAPSE:
        Collapse((CSoundMaterial)humanoid->field436);
        break;
    case SS_FLY_THROUGH_AIR:
        FlyThroughAir();
        break;
    case SS_END_ALL_SOUNDS:
        EndAllSounds();
        break;
    case SS_BEGIN_STUN:
        BeginStun();
        break;
    case SS_END_STUN:
        EndStun();
        break;
    case SS_BEGIN_SLIDE_SURFACE:
        BeginSlideOnSurface((CSoundMaterial)humanoid->field436);
        break;
    case SS_END_SLIDE_SURFACE:
        EndSlideOnSurface();
        break;
    case SS_BEGIN_SLIDE_LADDER:
        BeginSlideDownLadder();
        break;
    case SS_END_SLIDE_LADDER:
        EndSlideDownLadder();
        break;
    case SS_GRAB_WEAPON:
        GrabWeapon();
        break;
    case SS_WEAPON_MISS:
        WeaponMiss();
        break;
    case SS_WEAPON_HIT:
        WeaponHit();
        break;
    case SS_BREATH:
        Breath();
        break;
    case SS_GRUNT:
        Grunt();
        break;
    case SS_HIT_BY_FIRE_BLAST:
        HitByFireBlast();
        break;
    }
    return 0;
}

// sound script system

s32 CHumanoidSound::ProcessSoundScript(u32 animId, u32 frame) {
    MARKFUNCTION(0x800613EC);
    if (!g_soundScriptData) {
        return -100;
    }
    if (!g_soundScriptEvents) {
        return -100;
    }
    if (!humanoid) {
        return -100;
    }

    u32* scriptHeader = (u32*)g_soundScriptData;
    u32* entries = scriptHeader + 1;

    if (scriptAnimId != (s32)animId) {
        scriptAnimId = (s32)animId;
        scriptEntry = nullptr;
        scriptEventData = nullptr;

        // search for matching animation entry
        u32 numEntries = scriptHeader[0];
        for (u32 i = 0; i < numEntries; i++) {
            u32 entry = entries[i];
            u32 entryAnimId = (entry & 0xFF800000) >> 23;
            if (entryAnimId == animId) {
                u32 entryCharType = (entry & 0x7C0000) >> 18;
                if (entryCharType == (u32)humanoid->thingType) {
                    scriptEntry = &entries[i];
                    break;
                }
                if (entryCharType == 31) {
                    scriptEntry = &entries[i];
                }
            }
        }

        if (scriptEntry) {
            u32 dataOffset = (*scriptEntry >> 3) & 0x7FFE;
            scriptEventData = g_soundScriptEvents + dataOffset;
        }
    }

    if (!scriptEventData) {
        return 0;
    }

    if (!scriptEntry) {
        return 0;
    }
    u32 numFrameEvents = (*scriptEntry) & 0xF;
    if (numFrameEvents == 0) {
        return 0;
    }

    for (u32 i = 0; i < numFrameEvents; i++) {
        if (scriptEventData[i * 2] == (u8)frame) {
            MapSoundScriptEvent((SSHumanoid)scriptEventData[i * 2 + 1]);
            return 0;
        }
    }

    return 0;
}

s32 CHumanoidSound::LoadHumanoidSoundScripts() {
    MARKFUNCTION(0x80061314);
    u8* fileData = nullptr;
    u32 fileSize = 0;
    if (!xcReadFileLow("SOUND/SCRIPTS/HMND.SS", &fileData, &fileSize)) {
        g_soundScriptData = nullptr;
        g_soundScriptEvents = nullptr;
        return -301;
    }
    g_soundScriptData = fileData;
    // PSX: events start after the entry table
    // file layout: u32 numEntries, u32 entries[numEntries], u8 eventData[...]
    u32 numEntries = *(u32*)g_soundScriptData;
    g_soundScriptEvents = g_soundScriptData + 4 * numEntries + 4;
    LOG("[SndScript] Loaded HMND.SS: %u bytes, %u entries, events at offset %u",
        fileSize, numEntries, (u32)(g_soundScriptEvents - g_soundScriptData));
    return 0;
}

s32 CHumanoidSound::UnloadHumanoidSoundScripts() {
    MARKFUNCTION(0x800613C0);
    if (g_soundScriptData) {
        delete[] g_soundScriptData;
        g_soundScriptData = nullptr;
    }
    g_soundScriptEvents = nullptr;
    return 0;
}
