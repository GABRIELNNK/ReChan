// player.h - Player class (the player character)
// Reversed from PSX C:\CHAN\GAME\SRC\AI\PLAYER.CPP / PLAYER.HPP
#pragma once

#include "ai/humanoid.h"

// Player flags (s32 playerFlags bitmask)
enum PlayerFlags : s32 {
    PF_COMBAT_READY = 0x04,  // bit 2: combat-ready
};

// Player - the player character (Jackie Chan)
// PSX: ~764 bytes. Extends Humanoid with combat combos, platforming states,
// and player-specific action handlers.
// Source: C:\CHAN\GAME\SRC\AI\PLAYER.CPP
class Player : public Humanoid {
public:
    // PSX +616 (s32): combat state
    s32 field616 = 0;
    // PSX +620 (s32): combat state
    s32 field620 = 0;

    // PSX +624 (u16): hit combo counter (incremented by SignalEnemyGetUp)
    u16 hitCombo = 0;
    // PSX +628 (s32): combo timer in frames (max 1800)
    s32 comboTimer = 0;

    // PSX +636..+688: embedded sub-object (targeting/combat, 52 bytes)
    s32 subObject[13] = {};
    // PSX +688 (ptr): sub-object vtable
    void* subVtable = nullptr;

    // PSX +692 (s32): current lives left
    s32 livesLeft = 4;
    static constexpr s32 kMaxLives = 4;

    // PSX +696 (s32): player flags (bit 2 = combat-ready)
    s32 playerFlags = 0;

    // PSX +704 (u16): reserved
    u16 field704 = 0;
    // PSX +706 (u16): reserved
    u16 field706 = 0;
    // PSX +708 (u16): weapon dialog ID (set from global data)
    u16 weaponDialogID = 0;

    // PSX +712 (s32): reserved
    s32 field712 = 0;
    // PSX +716 (s32): jump return height (set to homePos.y in DoJump)
    s32 jumpReturnHeight = 0;
    // PSX +720 (s32): reserved
    s32 field720 = 0;
    // PSX +724 (s32): reserved
    s32 field724 = 0;
    // PSX +728 (s32): reserved
    s32 field728 = 0;

    // PSX +732 (u16): cleared at SetActionState entry
    u16 actionStateFlag = 0;

    // PSX +736,+740,+744: last position (copied from orientation on Reset)
    LVector lastPos = {};

    // PSX +748 (s32): CharMgrCallback first field (animation callback data)
    s32 animCallbackData = 0;
    // PSX +752 (ptr): CharMgrCallback vtable
    void* animCallbackVtable = nullptr;
    // PSX +756 (s32): current animation enum
    s32 currentAnimEnum = 0;
    // PSX +760 (s32): animation load state (set to 2 during loads)
    s32 animLoadState = 0;

    // Global player pointer - PSX: gp+3432
    static Player* s_player;


    // PSX: __6PlayerPC10tagLVector (PLAYER.CPP:1014)
    Player(const LVector* initialPos);

    // PSX: _._6Player (PLAYER.CPP:1050)
    ~Player() override;


    // PSX: Think__6Player (PLAYER.CPP:1155)
    void Think() override;

    // PSX: Reset__6Player (PLAYER.CPP:1056)
    void Reset() override;

    // PSX: Move__6Player (PLAYER.CPP:1408)
    void Move() override;

    // PSX: CreateModel__6PlayerPCc (PLAYER.CPP:1111)
    void CreateModel(const char* name) override;

    // PSX: SetActionState__6PlayerUll (PLAYER.CPP:1579)
    void SetActionState(u32 state, s32 param) override;

    // PSX: ProcessAction dispatches SD_HARDFALL/SD_HARDLAND via function pointers
    void ProcessAction() override;

    // PSX: GetViewSpot__6PlayerP10tagLVectorT1 (PLAYER.CPP:1460)
    void GetViewSpot(LVector* outPos, LVector* outTarget) override;


    void _Stand() override;
    void _Run() override;
    void _Jump() override;
    void _Fall() override;
    void _Straif() override;
    void _Collapse() override;
    void _Dead() override;

    // Player-only states
    virtual void _InactiveIdle();
    virtual void _Flip();
    virtual void _HardFall();
    virtual void _HardLand();
    virtual void _Push();
    virtual void _PushObject();
    virtual void _Teetering();
    virtual void _WallJump();
    virtual void _DoStand();
    virtual void _HorizontalPoleSwing();
    virtual void _LedgeLatch();
    virtual void _LedgePullup();
    virtual void _SlopeSlide();
    virtual void _LadderDismount();
    virtual void _ClimbLadder();
    virtual void _TableRoll();


    // PSX: DoJump__6Player (PLAYER.CPP:1424)
    void DoJump();

    // PSX: DoJump__6Playerl (PLAYER.CPP:1437)
    void DoJump(s32 height);

    // PSX: FallingPhysics__6Player (PLAYER.CPP:3187)
    void FallingPhysics();

    // PSX: CheckForLanding__6Player (PLAYER.CPP:4366)
    void CheckForLanding();

    // PSX: OnCheckpoint__6Player (PLAYER.CPP:4424)
    void OnCheckpoint();

    // PSX: SetLivesLeft__6Playerl (PLAYER.CPP:4457)
    void SetLivesLeft(s32 lives);

    s32 GetLivesLeft() const { return livesLeft; }

    // PSX: SignalEnemyGetUp__6Player (PLAYER.CPP:1382)
    void SignalEnemyGetUp();

    // PSX: SignalEnemyDead__6PlayerP8Humanoid (PLAYER.CPP:4828)
    void SignalEnemyDead(Humanoid* enemy);

    // PSX: EnterCombatCombo__6Player (PLAYER.CPP:4967)
    void EnterCombatCombo();

    // PSX: LoadCombatDialog__6Player (PLAYER.CPP:5000)
    void LoadCombatDialog();

    // PSX: PlayCombatKnockDownDialog__6Player15DamageTypesTags (PLAYER.CPP:5094)
    void PlayCombatKnockDownDialog(s32 damageType);

    // PSX: HandleHitShock__6Player15DamageTypesTags (PLAYER.CPP:5155)
    void HandleHitShock(s32 damageType);

    // PSX: PlayCombatThrowDialog__6Player (PLAYER.HPP:496)
    void PlayCombatThrowDialog();

    // PSX: PlayerSingleEncounterCheak__6Player (PLAYER.CPP:4660)
    void PlayerSingleEncounterCheak();

    // PC: reads InputManager → sets commandBits + faceAngle
    // (PSX: done via Behaviour::Process in the player's Behaviour object)
    void ReadPlayerInput();

    // PSX: LoadPlayerTauntResponse__6PlayerP8Humanoid (PLAYER.CPP:4712)
    void LoadPlayerTauntResponse(Humanoid* target);

    // PSX: PlayPlayerTauntResponse__6Player (PLAYER.CPP:4786)
    void PlayPlayerTauntResponse();

    // PC helpers for debug/iteration: direct animation control on player model.
    bool PlayAnimation(s32 animEnum, s32 loopType);
    void PauseAnimation();
    void ResumeAnimation();
    void StopAnimation();
    bool IsAnimationPaused() const;
};
