#pragma once
#include "snd/basesnd.h"

class Humanoid;
class CGenericPersistentSound;

// CSoundMaterial - surface material type for sound selection
// Used by footstep, land, grab, collapse, etc.
// Values 1-4 map to indices 0-3 in material arrays.
enum CSoundMaterial : s32 {
    SMAT_DEFAULT = 0,
    SMAT_CONCRETE = 1,
    SMAT_METAL = 2,
    SMAT_WOOD = 3,
    SMAT_WATER = 4,
};

// SSHumanoid - sound script event IDs for MapSoundScriptEvent
enum SSHumanoid : s32 {
    SS_FOOTSTEP = 1,
    SS_STRAFE = 2,
    SS_FALL = 3,
    SS_LAND = 4,
    SS_LAND2 = 5,
    SS_HIT_WORLD = 6,
    SS_HAND_PLANT = 7,
    SS_GRAB = 8,
    SS_GRAB_HUMANOID = 9,
    SS_FRONT_FLIP = 10,
    SS_WALL_JUMP = 11,
    SS_POLE_SWING = 12,
    SS_PUNCH_MISS = 13,
    SS_KICK_MISS = 14,
    SS_PUNCH_HIT = 15,
    SS_KICK_HIT = 16,
    SS_SUPER_PUNCH = 17,
    SS_SUPER_KICK = 18,
    SS_COLLAPSE = 19,
    SS_FLY_THROUGH_AIR = 20,
    SS_END_ALL_SOUNDS = 21,
    SS_BEGIN_STUN = 22,
    SS_END_STUN = 23,
    SS_BEGIN_SLIDE_SURFACE = 24,
    SS_END_SLIDE_SURFACE = 25,
    SS_BEGIN_SLIDE_LADDER = 26,
    SS_END_SLIDE_LADDER = 27,
    SS_GRAB_WEAPON = 28,
    SS_WEAPON_MISS = 29,
    SS_WEAPON_HIT = 30,
    SS_BREATH = 31,
    SS_GRUNT = 32,
    SS_HIT_BY_FIRE_BLAST = 33,
};

// CHumanoidSound (132 bytes) - extends CSound
// PSX source: C:\CHAN\GAME\SRC\SND\HMNDSND.CPP
class CHumanoidSound : public CSound {
public:
    // +16..+17: padding (from load buffer start)
    u8 pad16[2];
    // +18: footstep sounds per material (4 entries)
    u16 footstep[4];
    // +26: strafe sound
    u16 strafe;
    // +28: dive roll sound
    u16 diveRoll;
    // +30: hit world structure sound
    u16 hitWorldStructure;
    // +32: grab sounds per material (4 entries)
    u16 grab[4];
    // +40: hand plant sound
    u16 handPlant;
    // +42: grab humanoid sound
    u16 grabHumanoid;
    // +44: punch/kick miss sound
    u16 punchKickMiss;
    // +46: fall/frontflip/flythrough sound
    u16 fall;
    // +48: pole swing sound
    u16 poleSwing;
    // +50: punch hit sounds (3 random)
    u16 punchHit[3];
    // +56: kick hit sounds (3 random)
    u16 kickHit[3];
    // +62: super punch sound
    u16 superPunch;
    // +64: super kick sound
    u16 superKick;
    // +66: collapse sounds per material (4 entries)
    u16 collapse[4];
    // +74: dialog hit sounds (3 random)
    u16 dialogHit[3];
    // +80: dialog attack sounds (3 random)
    u16 dialogAttack[3];
    // +86: breath sound
    u16 breath;
    // +88: grunt sound
    u16 grunt;
    // +90: hit by fire blast sound
    u16 hitByFireBlast;
    // +92: persistent sound IDs
    u8 stunPersistId;
    u8 slideSurfacePersistId;
    u8 slideLadderPersistId;
    // +95: dialog chance thresholds
    u8 dialogHitChance;
    u8 dialogAttackChance;
    // +97..+99: padding to align
    u8 pad97[3];

    // sound script state (PSX +100/+104/+108)
    u32* scriptEntry;        // pointer to matched entry in script header
    u8* scriptEventData;     // pointer to event byte stream
    s32 scriptAnimId;

    // +112: persistent sound object pointers
    CGenericPersistentSound* stunPersistent;
    CGenericPersistentSound* slideSurfacePersistent;
    CGenericPersistentSound* slideLadderPersistent;

    // +124: back-pointer to owning humanoid
    Humanoid* humanoid;

    // +128: cooldown timers
    u8 attackCooldown;
    u8 hitCooldown;
    u8 fireCooldown;
    // +131: pad to 132

    CHumanoidSound();
    ~CHumanoidSound() override;

    s32 Initialize(const void* pos, const Humanoid* owner);
    s32 Load(const void* data);

    // movement sounds
    s32 Footstep(CSoundMaterial mat);
    s32 Strafe(CSoundMaterial mat);
    s32 Fall();
    s32 Land(CSoundMaterial mat);
    s32 DiveRoll(CSoundMaterial mat);
    s32 HitWorldStructure(CSoundMaterial mat);
    s32 HandPlant();
    s32 Grab(CSoundMaterial mat);
    s32 GrabHumanoid();
    s32 FrontFlip();
    s32 WallJump();
    s32 PoleSwing();
    s32 FlyThroughAir();

    // combat sounds
    s32 PunchMiss();
    s32 KickMiss();
    s32 PunchHit();
    s32 KickHit();
    s32 SuperPunch();
    s32 SuperKick();
    s32 Collapse(CSoundMaterial mat);

    // persistent state sounds
    s32 BeginStun();
    s32 EndStun();
    s32 BeginSlideOnSurface(CSoundMaterial mat);
    s32 EndSlideOnSurface();
    s32 BeginSlideDownLadder();
    s32 EndSlideDownLadder();
    s32 EndAllSounds();

    // dialog/vocal
    s32 FXDialogHit();
    s32 FXDialogAttack();
    s32 Breath();
    s32 Grunt();
    s32 HitByFireBlast();

    // weapon sounds
    s32 GrabWeapon();
    s32 WeaponMiss();
    s32 WeaponHit();

    // helpers
    s32 PlayAttack(u16 soundId);
    s32 PlayHit(u16 soundId);
    void Think();
    s32 MapSoundScriptEvent(SSHumanoid event);

    // sound script system
    s32 ProcessSoundScript(u32 animId, u32 frame);
    static s32 LoadHumanoidSoundScripts();
    static s32 UnloadHumanoidSoundScripts();
};
