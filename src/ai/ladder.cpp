#include "gen/common.h"
#include "ai/ladder.h"
#include "ai/humanoid.h"
#include "ai/activezn.h"
#include "ai/platform.h"
#include "ai/player.h"
#include "gen/ai.h"
#include "gen/database.h"
#include "gen/colsect.h"
#include "gen/colmgr.h"
#include "gen/director.h"
#include "gen/game.h"
#include "gen/world.h"

static constexpr s32 LADDER_TOP = 0x358; // 856 - top-zone tolerance & ledge probe Y range
static constexpr s32 TOP_OF_LADDER_GRASP_OFFSET_Y_FUDGE = 0x50;  // 80  - bottom-vs-top approach Y threshold
static constexpr s32 LADDER_LEDGE_LOOKAHEAD = 0x118; // 280 - horizontal ledge probe distance
static constexpr s32 LADDER_LEDGE_CLEARANCE = 1024;  // PSX: 8th arg to LedgePrototype

Ladder::Ladder(const LVector* pos, u16 type) : Obstacle(pos, type) {
    MARKFUNCTION(0x80089CA4);
    ladderFaceAngle = 0;
    hatchEnabled = 1;
    deathCountdown = 3;
    state = 0;
    cutscenePending = 0;
    hatchTriggerCRC = 0;
    hatchCloseCRC = 0;
    teleportTargetCRC = 0;
    deathCheckCRC = 0;
    hatchThing = nullptr;
    deathThing = nullptr;
    hatchYTrigger = 0x7FFFFFFF;
}

Ladder::~Ladder() {
    MARKFUNCTION(0x80089D18);
}

void Ladder::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80089D40);
    Obstacle::AnalyzeMesh(root);

    if (!root) {
        return;
    }

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    DBVolume* vol = dynamic_cast<DBVolume*>(root);
    if (vol) {
        tagCollisionBox box = {};
        FillCollisionBox(box, *vol);
        box.maxY = (s16)(box.maxY + 256);
        box.maxZ = (s16)(box.maxZ + 384);
        SetCollisionBox(box);
    }

    const DBAttrib* a6 = root->FindAttrib(6);
    if (a6) {
        ladderFaceAngle = ((s32)a6->value << 16) / 360 + 0x8000;
    }

    hatchEnabled = root->FindAttrib(7) ? 1 : 0;

    char pointNameBuf[32] = {};

    // PSX field mapping (AnalyzeMesh__6Ladder):
    //   attrib 8  -> deathCheckCRC
    //   attrib 9  -> hatchTriggerCRC
    //   attrib 10 -> teleportTargetCRC
    //   attrib 11 -> hatchCloseCRC
    const DBAttrib* a8 = root->FindAttrib(8);
    deathCheckCRC = 0;
    if (a8) {
        const char* attrString = a8->GetAttribString();
        if (attrString && attrString[0] != '\0') {
            deathCheckCRC = (s32)p3dHash(attrString);
        }
    }

    const DBAttrib* a9 = root->FindAttrib(9);
    hatchTriggerCRC = 0;
    if (a9) {
        const char* attrString = a9->GetAttribString();
        if (attrString && attrString[0] != '\0') {
            hatchTriggerCRC = (s32)p3dHash(attrString);
        }
    }

    const DBAttrib* a10 = root->FindAttrib(10);
    teleportTargetCRC = 0;
    if (a10) {
        const char* attrString = a10->GetAttribString();
        if (attrString && attrString[0] != '\0') {
            teleportTargetCRC = (s32)p3dHash(attrString);
        }
    }

    const DBAttrib* a11 = root->FindAttrib(11);
    hatchCloseCRC = 0;
    if (a11) {
        const char* attrString = a11->GetAttribString();
        if (attrString && attrString[0] != '\0') {
            hatchCloseCRC = (s32)p3dHash(attrString);
        }
    }

    const DBAttrib* a12 = root->FindAttrib(12);
    hatchYTrigger = a12 ? (s32)a12->value : 0x7FFFFFFF;
}

void Ladder::CreateModel(const char* /*name*/) {
    MARKFUNCTION(0x80089F48);
    flags |= TF_MODEL_CREATED;
}

void Ladder::DeleteModel() {
    MARKFUNCTION(0x80089F5C);
    flags &= ~TF_MODEL_CREATED;
}

void Ladder::Reset() {
    MARKFUNCTION(0x80089F70);
    World* world = g_game ? g_game->GetWorld() : nullptr;
    s32 levelID = world ? world->GetCurLevelID() : 0;
    state = (hatchTriggerCRC != 0 && levelID == 7) ? 1 : 0;
    cutscenePending = 0;
    deathCountdown = 3;
}

void Ladder::Think() {
    MARKFUNCTION(0x80089FD4);

    // PSX Think__6Ladder (0x80089FD4):
    //   state == 0              -> DeathCheck (waits for combat to clear)
    //   state == 1, 2, or 4+    -> idle
    //   state == 3              -> waiting for the hatch Platform to finish opening;
    //                              when its spline reports AtEndOfPath, drop to state 2
    //                              so HandleHumanoidCollision allows the player to climb.
    if (state == 3) {
        if (hatchThing) {
            Platform* hatchPlatform = static_cast<Platform*>(hatchThing);
            if (hatchPlatform->AtEndOfPath()) {
                state = 2;
            }
        }
        return;
    }

    if (state == 0) {
        DeathCheck();
    }
}

void Ladder::Move() {
    MARKFUNCTION(0x8008A068);
}

void Ladder::UpdatePosition() {
    MARKFUNCTION(0x8008A10C);
}

void Ladder::Draw() {
    MARKFUNCTION(0x8008A114);
}

void Ladder::Trigger() {
    MARKFUNCTION(0x8008A11C);

    // PSX: only replace hatchThing when hatchTriggerCRC is set; otherwise keep previous lookup.
    if (hatchTriggerCRC != 0 && g_ai) {
        ccNode* n = g_ai->moveList.FindNodeCRC((u32)hatchTriggerCRC);
        hatchThing = n ? static_cast<Thing*>(n) : nullptr;
    }

    LOG("[Ladder::Trigger] name=%s hatchTriggerCRC=0x%08X hatchThing=%p hatchType=%u",
        GetName() ? GetName() : "null",
        (u32)hatchTriggerCRC,
        hatchThing,
        hatchThing ? hatchThing->thingType : 0);

    if (hatchThing) {
        static_cast<Obstacle*>(hatchThing)->TriggerByName(Player::s_player, nullptr, nullptr);
        state = 3;
    }
    else {
        state = 2;
    }
}

void Ladder::TeleportPlayer() {
    MARKFUNCTION(0x8008A19C);

    if (teleportTargetCRC == 0 || !g_ai) {
        return;
    }

    ccNode* n = g_ai->moveList.FindNodeCRC((u32)teleportTargetCRC);
    if (n) {
        static_cast<Obstacle*>(static_cast<Thing*>(n))->HandleHumanoidCollision(Player::s_player);
    }
}

void Ladder::CloseHatch() {
    MARKFUNCTION(0x8008A1F4);

    if (hatchCloseCRC != 0 && g_ai) {
        ccNode* n = g_ai->moveList.FindNodeCRC((u32)hatchCloseCRC);
        if (n) {
            static_cast<Obstacle*>(static_cast<Thing*>(n))->TriggerByName(Player::s_player, nullptr, nullptr);
        }
    }

    cutscenePending = 0;
}

s32 Ladder::DeathCheck() {
    MARKFUNCTION(0x8008A070);

    if (!deathThing) {
        ccNode* n = g_ai ? g_ai->activeZoneList.FindNodeCRC((u32)deathCheckCRC) : nullptr;
        deathThing = static_cast<ActiveZone*>(n);
        if (deathThing) {
            return deathCountdown;
        }
        Trigger();
        return deathCountdown;
    }

    if (deathThing->memberCount == 0) {
        deathCountdown--;
        if (deathCountdown <= 0) {
            Trigger();
        }
    }
    else {
        deathCountdown = 3;
    }

    return deathCountdown;
}

void Ladder::HandlePickupCollision(Thing* /*pickup*/) {
    MARKFUNCTION(0x8008A258);
}

void Ladder::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8008A260);

    if (!hum) return;

    if (state != 2) return;
    if (cutscenePending == 1) return;

    LVector savedHomePos = hum->homePos;

    // PSX naming in disasm: s1=pos.y+collBox.minY, s2=pos.y+collBox.maxY.
    s32 minY = pos.y + (s32)collBox.minY;
    s32 maxY = pos.y + (s32)collBox.maxY;
    s32 maxZ = pos.z + (s32)collBox.maxZ;

    s32 as = hum->actionState;

    // PSX 0x8008A2E4-0x8008A338: states 25/26 just set flag and return
    if (as == AS_LADDER_CLIMB_UP || as == AS_LADDER_CLIMB_DOWN) {
        hum->field368 |= 2;
        return;
    }

    // PSX 0x8008A33C: state 27 (climbing)
    if (as == AS_LADDER_CLIMBING) {
        hum->field368 |= 2;
        hum->groundStandHeight = hum->pos.y;

        if (teleportTargetCRC != 0) {
            if (savedHomePos.y <= hatchYTrigger) {
                if (g_director) {
                    g_director->SetCodeSnip(Director::GetNISLadder1Script(), this);
                }
                cutscenePending = 1;
            }
        }

        // PSX 0x8008A390: clamp below-ladder Y to minY.
        if (savedHomePos.y < minY) {
            savedHomePos.y = minY;
            hum->homePos = savedHomePos;
            PutHumanoidOnLadder(hum);
            return;
        }

        // PSX 0x8008A3AC: near ladder top while climbing.
        // gp[2432] = ladderTop = 856 (NOT g_climbSearchRadius=128).
        if ((maxY - LADDER_TOP) < savedHomePos.y) {
            if ((hum->commandBits >> 2) & 1) {
                // PSX checks hatchTriggerCRC or hatchEnabled at this point.
                if (!hatchEnabled) {
                    PutHumanoidOnLadder(hum);
                    return;
                }

                savedHomePos.y = maxY - LADDER_TOP;
                LVector ledgeNormal = {};
                LVector ledgePos = {};
                if (CheckForLedges(ledgeNormal, ledgePos)) {
                    hum->PrepareLedgeLatch(ledgePos, ledgeNormal);
                    hum->SetActionState(AS_LEDGE_PULLUP, 0);
                    return;
                }

                hum->homePos = savedHomePos;
            }
        }

        // PSX: keep avatar aligned to ladder while still climbing.
        PutHumanoidOnLadder(hum);
        return;
    }

    // PSX state 73 (0x49): return
    if (as == 73) return;

    // Default: player approaching ladder check for grab input
    hum->field368 |= 2;

    s32 grabButton = 0;
    s32 bits = hum->commandBits;
    if (((bits >> 7) & 1) || ((bits >> 15) & 1)) {
        grabButton = 1;
    }
    if (!grabButton) return;

    // PSX 0x8008A490: top-approach check uses current pos.y.
    // gp[2436] = TOP_OF_LADDER_GRASP_OFFSET_Y_FUDGE = 80 (NOT g_floorYSearchOffset=384).
    if (hum->pos.y > (maxY - 256 - TOP_OF_LADDER_GRASP_OFFSET_Y_FUDGE)) {
        if (hum != (Humanoid*)Player::s_player) return;
        LVector ledgeNormal = {};
        LVector ledgePos = {};
        if (!CheckForLedges(ledgeNormal, ledgePos)) {
            return;
        }

        hum->homePos = ledgePos;
        PutHumanoidOnLadder(hum);
        hum->SetActionState(AS_LADDER_CLIMB_DOWN, 0);
        return;
    }

    // PSX 0x8008A50C: bottom approach uses current pos.y.
    if (hum->pos.y < minY) return;
    if (savedHomePos.z >= maxZ - 384) return;

    PutHumanoidOnLadder(hum);
    hum->SetActionState(AS_LADDER_CLIMB_UP, 0);
}

// PSX: PutHumanoidOnLadder__6LadderP8Humanoid (0x8008A570)
// Aligns humanoid X/Z to ladder, sets facing angle
void Ladder::PutHumanoidOnLadder(Humanoid* hum) {
    // PSX: hum->orientation.y = ladder->ladderFaceAngle
    hum->orientation.y = ladderFaceAngle;

    // PSX: set hum X/Z to ladder X/Z, keep hum Y
    hum->homePos.x = pos.x;
    hum->homePos.z = pos.z;
}

// PSX: CheckForLedges__6LadderR9_RMVECT16R10tagLVector (0x8008A5DC)
bool Ladder::CheckForLedges(LVector& outNormal, LVector& outCorrectionPos) {
    LVector startPos = pos;
    LVector endPos = startPos;

    const s16 probeAngle = (s16)orientation.y;
    endPos.x += (s32)(((s64)rmSin16(probeAngle) * (s64)LADDER_LEDGE_LOOKAHEAD) >> 16);
    endPos.z += (s32)(((s64)rmSin16((s16)(probeAngle + 0x4000)) * (s64)LADDER_LEDGE_LOOKAHEAD) >> 16);

    const s32 ledgeBaseY = startPos.y + (s32)collBox.maxY;
    const s32 ledgeMinY = ledgeBaseY - LADDER_TOP;
    const s32 ledgeMaxY = ledgeBaseY + LADDER_TOP;

    u16 ledgeMaterial = 0;
    bool ok = CollisionSector::LedgePrototype(
        startPos,
        endPos,
        ledgeMinY,
        ledgeMaxY,
        outNormal,
        outCorrectionPos,
        ledgeMaterial,
        LADDER_LEDGE_CLEARANCE);
    return ok;
}
