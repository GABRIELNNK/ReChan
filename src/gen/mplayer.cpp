#include "common.h"
#include "gen/mplayer.h"
#include "gen/ai.h"
#include "gen/animstruct.h"
#include "gen/camera.h"
#include "gen/charmgr.h"
#include "gen/display.h"
#include "ai/player.h"
#include "p3d/hash.h"
#include "snd/hmndsnd.h"

CharMgrCallback* g_nisCharMgrCallback = nullptr;

// PSX: nisCharMgrCallback (MPLAYER.CPP:646)
class NisCharMgrCallback final : public CharMgrCallback {
public:
    s32 animEnums[4] = {};
    u32 thingTypes[4] = {};
    s32 index = -1;
    s32 count = 0;

    NisCharMgrCallback(s32 inCount, const u32* inThingTypes, const s32* inAnimEnums) {
        MARKFUNCTION(0x80077FA4);

        done = 0;
        index = -1;
        count = inCount;

        for (s32 i = 0; i < count && i < 4; i++) {
            thingTypes[i] = inThingTypes[i];
            animEnums[i] = inAnimEnums[i];
        }
    }

    void Callback() override {
        MARKFUNCTION(0x80078008);

        index++;
        if (index < count && g_characterManager) {
            g_characterManager->LoadAnimationBatch(thingTypes[index], animEnums[index], this);
            return;
        }

        g_nisCharMgrCallback = nullptr;
        delete this;
    }
};

// PlayerModel
PlayerModel::PlayerModel() {
    // Same as HumanoidModel on PSX
}

PlayerModel::~PlayerModel() {}

// PSX: SetupModelCallbacks__11PlayerModel (MPLAYER.CPP:361, 0x80077DF8)
void PlayerModel::SetupModelCallbacks() {
    MARKFUNCTION(0x80077DF8);
    HumanoidModel::SetupModelCallbacks();
}

s32 PlayerModel::MirrorTree() {
    MARKFUNCTION(0x80077F84);
    return SModel::MirrorTree();
}

void PlayerModel::ApplyAnimToModel(s32 thingType, s32 animEnum, s32 loopType, s32 p4, s32 p5) {
    Player* owner = (backPtr && backPtr->thingType == AITypes::TT_PLAYER)
        ? static_cast<Player*>(backPtr)
        : nullptr;

    if (owner && owner->Debug_IsAnimationOverrideActive() && !owner->Debug_IsAnimationOverrideApplying()) {
        return;
    }

    SModel::ApplyAnimToModel(thingType, animEnum, loopType, p4, p5);
}

// PSX: SetAnim__11PlayerModelllil (MPLAYER.CPP:293, 0x80077B20)
// Routes player-specific animation enums to correct loop types.
// Some anims trigger sound effects. Falls back to HumanoidModel::SetAnim
// for unrecognized enums.
void PlayerModel::SetAnim(s32 animEnum, s32 a3, s32 force, s32 extra) {
    MARKFUNCTION(0x80077B20);

    Player* owner = (backPtr && backPtr->thingType == AITypes::TT_PLAYER)
        ? static_cast<Player*>(backPtr)
        : nullptr;
    if (owner && owner->Debug_IsAnimationOverrideActive() && !owner->Debug_IsAnimationOverrideApplying()) {
        return;
    }

    AnimStructure* as = (AnimStructure*)animStructure;
    // Early exit: if not forcing and anim already matches, no-op
    if (!force && as && as->animEnum == animEnum) {
        return;
    }

    s32 loopType = ANIM_RUN_TO_LAST; // default v11=2
    bool handled = true;
    bool setBlendData = false;

    // PSX anim routing - traced from binary comparison tree
    if (animEnum == 1 || animEnum == 22) {
        // PSX: anim 1 + 22 redirect to 22 with Loop
        animEnum = 22;
        loopType = ANIM_LOOP;
    }
    else if (animEnum == 2 || animEnum == 31 || animEnum == 41 ||
             animEnum == 47 || animEnum == 48 || animEnum == 189 ||
             animEnum == 206 || animEnum == 220 || animEnum == 231 ||
             animEnum == 244 || animEnum == 254 || animEnum == 261) {
        // PSX: these anims use Loop (v11=0)
        loopType = ANIM_LOOP;
    }
    else if (animEnum == 3 || animEnum == 17 || animEnum == 21 ||
             animEnum == 152 || animEnum == 295) {
        // PSX: RunToLast, no blend data
        loopType = ANIM_RUN_TO_LAST;
    }
    else if (animEnum == 24) {
        // PSX: strafe - plays DiveRoll sound if frame < 29
        if (backPtr && backPtr->thingType < 29) {
            Humanoid* ownerHumanoid = static_cast<Humanoid*>(backPtr);
            if (ownerHumanoid->humanoidSound) {
                ownerHumanoid->humanoidSound->DiveRoll(SMAT_CONCRETE);
            }
        }
        loopType = ANIM_RUN_TO_LAST;
    }
    else if (animEnum >= 27 && animEnum <= 30) {
        // PSX: 27 gets blend data, 28 goes to LABEL_58, 29-30 are RunToLast
        if (animEnum == 27) {
            setBlendData = true;
        }
        else if (animEnum == 28) {
            HumanoidModel::SetAnim(animEnum, a3, force, extra);
            return;
        }
        loopType = ANIM_RUN_TO_LAST;
    }
    else if (animEnum >= 32 && animEnum <= 36) {
        // PSX: RunToLast, no blend data
        loopType = ANIM_RUN_TO_LAST;
    }
    else if (animEnum == 42) {
        // PSX: plays HitWorldStructure sound if frame < 29
        if (backPtr && backPtr->thingType < 29) {
            Humanoid* ownerHumanoid = static_cast<Humanoid*>(backPtr);
            if (ownerHumanoid->humanoidSound) {
                ownerHumanoid->humanoidSound->HitWorldStructure(SMAT_CONCRETE);
            }
        }
        setBlendData = true;
        loopType = ANIM_RUN_TO_LAST;
    }
    else if (animEnum == 46) {
        // PSX: RunToLast with blend data (turn animation)
        setBlendData = true;
        loopType = ANIM_RUN_TO_LAST;
    }
    else {
        // Fallback to HumanoidModel::SetAnim
        HumanoidModel::SetAnim(animEnum, a3, force, extra);
        handled = false;
    }

    if (handled) {
        ApplyAnimToModel(0, animEnum, loopType, a3, extra);
        // PSX: LABEL_59 always writes v18[0]/v18[1] into animStructure+96/+100.
        // v18 starts as {0,0} and is set to {3997696, 8} only for blend anims.
        as = (AnimStructure*)animStructure;
        if (as) {
            if (setBlendData) {
                // PSX: v18[0] = 3997696 (0x003D0000), LOWORD(v18[1]) = 8
                as->humanoidCB.offsetLo = 0;
                as->humanoidCB.offsetHi = 61;
                as->humanoidCB.funcPtr = reinterpret_cast<void*>(static_cast<intptr_t>(8));
            }
            else {
                as->humanoidCB.offsetLo = 0;
                as->humanoidCB.offsetHi = 0;
                as->humanoidCB.funcPtr = nullptr;
            }
        }
    }
}

// PSX: _Loop__11PlayerModelP13AnimStructure (MPLAYER.CPP:573)
// Simple trampoline to base Model::HandleLoop on PSX.
void PlayerModel::HandleLoop(AnimStructure* anim) {
    Model::HandleLoop(anim);
}

// PSX: _RunToLast__11PlayerModelP13AnimStructure (MPLAYER.CPP:374)
// Checks specific anim enums for animation chaining on completion.
void PlayerModel::HandleRunToLast(AnimStructure* anim) {
    s32 curAnim = anim->animEnum;

    if (curAnim == 281) {
        // PSX: anim 281 (0x119) chains to anim 282 (0x11A) on completion
        Model::HandleRunToLast(anim);
        if (anim->loopCount > 0) {
            SetAnim(282, 0, 0, 0);
        }
        return;
    }

    if (curAnim == 32) {
        // PSX: walljump anim (0x20) - on completion, calls DoWallJump then chains to anim 33
        Model::HandleRunToLast(anim);
        s16 currentFrameHi = (s16)((u32)anim->currentFrame >> 16);
        s16 endFrameHi = (s16)((u32)anim->endFrame >> 16);
        if (currentFrameHi >= endFrameHi) {
            Player* player = dynamic_cast<Player*>(backPtr);
            if (player) {
                player->DoWallJump();
            }
            SetAnim(33, 0, 0, 0);
        }
        return;
    }

    if (curAnim == 295) {
        // PSX: anim 295 (0x127) - on completion, sets action state to fall (13, 3)
        Model::HandleRunToLast(anim);
        if (anim->loopCount > 0 && backPtr) {
            Humanoid* humanoid = dynamic_cast<Humanoid*>(backPtr);
            if (humanoid) {
                humanoid->SetActionState(AS_FALL, 3);
            }
        }
        return;
    }

    // Default: just run the base handler
    Model::HandleRunToLast(anim);
}

// PSX: _IncFrame__11PlayerModelP13AnimStructure (MPLAYER.CPP:578)
// Simple trampoline to base Model::HandleIncFrame on PSX (which is a no-op).
void PlayerModel::HandleIncFrame(AnimStructure* anim) {
    Model::HandleIncFrame(anim);
}

// PSX: LoadNIS__11PlayerModelUlPPCcii (MPLAYER.CPP:676, 0x80078088)
s32 PlayerModel::LoadNIS(u32 argc, const char** argv, s32 /*a4*/, s32 /*a5*/) {
    MARKFUNCTION(0x80078088);

    if (!g_characterManager || argc == 0 || !argv || !argv[0]) {
        return 0;
    }

    const s32 baseAnim = g_characterManager->LookUpAnimation(AITypes::TT_PLAYER, argv[0]);

    s32 animEnums[4] = {};
    u32 thingTypes[4] = {};
    s32 pairCount = 0;

    for (u32 argIndex = 1; argIndex < argc && pairCount < 4; argIndex++) {
        if (!argv[argIndex]) {
            continue;
        }

        const u32 hash = p3dHash(argv[argIndex]);
        s32 thingType = 19;

        if (hash == 157076083u) {
            thingType = 5;
        }
        else if (hash == 167249827u) {
            thingType = 8;
        }
        else if (hash == 206613443u) {
            thingType = 1;
        }
        else if (hash == 164699011u) {
            thingType = 2;
        }
        else if (hash == 14425203u) {
            thingType = 18;
        }
        else if (hash == 44359011u) {
            thingType = 14;
        }
        else if (hash == 2172243u) {
            thingType = 3;
        }
        else if (hash != 223106307u) {
            ccNode* node = g_ai ? g_ai->humanoidList.FindNodeCRC(hash) : nullptr;
            if (node) {
                thingType = static_cast<s32>(static_cast<Thing*>(node)->thingType);
            }
        }

        animEnums[pairCount] = baseAnim;
        thingTypes[pairCount] = static_cast<u32>(thingType);
        pairCount++;
    }

    for (s32 i = 0; i < pairCount; i++) {
        const u32 thingType = thingTypes[i];
        if (thingType != AITypes::TT_PLAYER && thingType <= AITypes::TT_HUMANOID_LAST
            && !g_characterManager->IsCharacterLoaded(thingType)) {
            g_characterManager->OpenCharacter(thingType);
            g_characterManager->LoadCharacter(thingType, nullptr);
        }
    }

    if (g_nisCharMgrCallback) {
        delete g_nisCharMgrCallback;
        g_nisCharMgrCallback = nullptr;
    }

    CharMgrCallback* callback = nullptr;
    if (pairCount > 0) {
        callback = new NisCharMgrCallback(pairCount, thingTypes, animEnums);
        g_nisCharMgrCallback = callback;
    }

    g_characterManager->LoadAnimationBatch(AITypes::TT_PLAYER, baseAnim, callback);

    if (g_display) {
        Camera* camera = g_display->GetCamera();
        if (camera) {
            camera->LoadAsyncAnim(baseAnim + 1);
        }
    }

    return 1;
}
