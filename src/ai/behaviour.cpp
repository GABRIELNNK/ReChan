#include "ai/behaviour.h"
#include "ai/cominter.h"
#include "ai/humanoid.h"
#include "ai/obstacle.h"
#include "ai/thing.h"
#include "gen/director.h"
#include "gen/game.h"
#include "gen/camera.h"
#include "pc/inputaction.h"
#include "pc/debugui.h"
#include "p3d/p3dmath.h"

// PSX binary angle constants
static constexpr s32 ANGLE_FULL_ROTATION = 0xFFFF;
static constexpr s32 ANGLE_QUARTER_TURN = 0x4000;

// Direction classification thresholds (PSX angle ranges)
// angleDiff = ownerAngle - targetAngle, wrapped to [0, 0xFFFF]
// Behind:  angleDiff in [25487..40049)  ~140-220 degrees (input opposite to facing)
// Left:    angleDiff in [8193..24576)   ~45-135 degrees (input to character's left)
// Right:   angleDiff in [40961..57344)  ~225-315 degrees (input to character's right)
// Forward: angleDiff outside all ranges, near 0 degrees (input aligned with facing)
static constexpr u32 DIR_BEHIND_START = 25487;
static constexpr u32 DIR_BEHIND_RANGE = 0x38E2;
static constexpr u32 DIR_LEFT_START = 8193;
static constexpr u32 DIR_LEFT_RANGE = 0x3FFF;
static constexpr u32 DIR_RIGHT_START = 40961;
static constexpr u32 DIR_RIGHT_RANGE = 0x3FFF;

// Direction codes (bitmask values matching PSX commandList conditions)
static constexpr s32 DIR_FORWARD = 1;  // angleDiff near 0 - pushing same direction as facing
static constexpr s32 DIR_BACKWARD = 2;  // angleDiff near 180 - pushing opposite to facing
static constexpr s32 DIR_RIGHT = 4;  // angleDiff near 270 - pushing to character's right
static constexpr s32 DIR_LEFT = 8;  // angleDiff near 90 - pushing to character's left

// Fixed-point 127 << 16
static constexpr s32 STICK_MAX_FP = 0x7F0000;

extern s32 g_directorActive;

Behaviour::Behaviour(Humanoid* ownerHumanoid, u32 handlerType, s32 /*aiParam*/) {
    MARKFUNCTION(0x80069CB0);
    owner = ownerHumanoid;
    handlerThisOffset = 0;
    handlerDispatch = -1;
    handler = nullptr;
    previousButtons = 0;
    buttonHoldCounter = 0;
    behaviourFlags = 0;
    padPort = 0;
    SetAIHandler(handlerType);
}

void Behaviour::SetAIHandler(u32 handlerType) {
    MARKFUNCTION(0x80069D3C);
    handler = nullptr;

    if (handlerType == AITypes::TT_PLAYER) {
        handler = PlayerUserControl;
    }
}

void Behaviour::Process() {
    MARKFUNCTION(0x80069D78);

    if (handlerDispatch == 0) {
        return;
    }

    if (handlerDispatch <= 0) {
        if (handler) {
            u8* base = reinterpret_cast<u8*>(this);
            Behaviour* target = reinterpret_cast<Behaviour*>(base + handlerThisOffset);
            handler(target);
        }
        return;
    }
}

// PSX: PlayerUserControl__9Behaviour (BEHAVE.CPP:1474, 0x80074F5C)
void Behaviour::PlayerUserControl(Behaviour* self) {
    MARKFUNCTION(0x80074F5C);
    if (!self || !self->owner || !g_game || !g_actionInput) {
        return;
    }

    Humanoid* owner = self->owner;

    if (!DebugUI::IsPlayerInputAllowed()) {
        owner->RequestAction(GA_GUARD_RELEASE);
        return;
    }

    const bool gameplayInputEnabled = (self->behaviourFlags & Behaviour::BF_INPUT_PROCESSING) != 0;
    if (!gameplayInputEnabled) {
        self->behaviourFlags |= Behaviour::BF_INPUT_PROCESSING;
    }

    // Read movement from ActionInput (works for both keyboard and gamepad)
    s32 analogX = gameplayInputEnabled ? g_actionInput->GetMoveX() : 0;
    s32 analogY = gameplayInputEnabled ? g_actionInput->GetMoveY() : 0;
    s32 stickX = gameplayInputEnabled ? g_actionInput->GetMoveX() : 0;
    s32 stickY = gameplayInputEnabled ? g_actionInput->GetMoveY() : 0;
    s32 direction = 0;

    // Direction classification
    s32 ownerAngle = owner->orientation.y;
    s32 targetAngle = ownerAngle;

    owner->moveSpeed = 0;

    if (analogX || analogY) {
        Camera& cam = g_game->GetCamera();
        s32 cameraAngle = cam.GetOrientY();

        targetAngle = cameraAngle - rmATan216((f32)analogX, (f32)(-(s16)analogY)) + ANGLE_QUARTER_TURN;

        s32 angleDiff = ownerAngle - targetAngle;
        while (angleDiff > ANGLE_FULL_ROTATION) {
            angleDiff -= ANGLE_FULL_ROTATION;
        }
        while (angleDiff < 0) {
            angleDiff += ANGLE_FULL_ROTATION;
        }

        if (angleDiff < 0) {
            angleDiff = -angleDiff;
        }

        s32 dirCode = DIR_BACKWARD;
        if ((u32)(angleDiff - DIR_BEHIND_START) >= DIR_BEHIND_RANGE) {
            dirCode = DIR_LEFT;
            if ((u32)(angleDiff - DIR_LEFT_START) >= DIR_LEFT_RANGE) {
                dirCode = DIR_RIGHT;
                if ((u32)(angleDiff - DIR_RIGHT_START) >= DIR_RIGHT_RANGE) {
                    dirCode = DIR_FORWARD;
                }
            }
        }
        direction = dirCode;

        s32 absX = stickX;
        if (stickX < 0) {
            absX = -stickX;
        }
        s32 absY = stickY;
        if (stickY < 0) {
            absY = -stickY;
        }
        s32 maxAxis = stickX;
        if (absY >= absX) {
            maxAxis = stickY;
        }
        if (maxAxis < 0) {
            maxAxis = -maxAxis;
        }

        s32 range = (s32)(((s64)(maxAxis << 16) * (s64)rmDiv16i(g_maxAttackRange << 16, STICK_MAX_FP)) >> 16) >> 16;
        if (range) {
            owner->moveSpeed = range;
        }
        else {
            owner->moveSpeed = 0;
        }
    }

    // PSX clears Behaviour bit 0 each frame while Director input is disabled.
    // PlayerUserControl then ignores the current pad sample and re-arms the bit.
    s32 actionReq = gameplayInputEnabled ? g_actionInput->ResolveGameAction(direction)
                                         : GA_GUARD_RELEASE;

    // Set faceAngle for movement/direction-based actions
    if ((u32)(actionReq - 2) < 3
        || actionReq == GA_STRAFE
        || actionReq == 19
        || actionReq == GA_GRAB
        || actionReq == GA_GRAB_FORWARD
        || actionReq == 16) {
        owner->faceAngle = targetAngle;
    }

    owner->RequestAction((u32)actionReq);
}

// PSX: MoveToDestinationPoint__9BehaviourUl (BEHAVIOU.CPP:1736, 0x80075408)
// Returns 1 when the owner humanoid has reached destPoint within threshold distance.
// Otherwise faces the point and requests Run (far) or Walk (close) action.
s32 Behaviour::MoveToDestinationPoint(u32 threshold) {
    MARKFUNCTION(0x80075408);

    u32 dist = (u32)owner->DistanceFromPointXZ(destPoint);

    if (dist >= threshold) {
        // Far from destination - run
        owner->FacePointDesired(destPoint);
        owner->RequestAction(2);  // GA_RUN
        // PSX: sets owner->moveSpeed from animConfigPtr speed data
        if (animConfigPtr) {
            s16 spd = *(s16*)((u8*)animConfigPtr + 44);
            owner->moveSpeed = spd;
        }
        return 0;
    }

    if (dist >= (threshold >> 1)) {
        // Close to destination - walk (half speed)
        owner->FacePointDesired(destPoint);
        owner->RequestAction(6);  // GA_WALK
        // PSX: sets owner->moveSpeed to half of speed
        if (animConfigPtr) {
            s16 spd = *(s16*)((u8*)animConfigPtr + 44);
            owner->moveSpeed = (spd + ((u16)spd >> 15)) >> 1;
        }
        return 0;
    }

    // Arrived at destination
    return 1;
}

// PSX: NisControl__9Behaviour (BEHAVIOU.CPP:1774, 0x800753C4)
void Behaviour::NisControl(Behaviour* b) {
    MARKFUNCTION(0x800753C4);
    if (b->MoveToDestinationPoint(0x4B) != 0) {
        // Reached destination - restore normal player control
        b->owner->FaceThingDesired(nullptr);
        b->handlerThisOffset = 0;
        b->handlerDispatch = -1;
        b->handler = PlayerUserControl;
    }
}

// PSX: NDMS__9Behaviour (BEHAVE.CPP:2214, 0x80075BE4)
void Behaviour::NDMS(Behaviour* b) {
    MARKFUNCTION(0x80075BE4);

    if (!b || !b->owner) {
        return;
    }

    b->owner->moveSpeed = 0;
    b->owner->FaceThingDesired(nullptr);
}

// PSX: SubwayDodgeRight__9Behaviour (BEHAVE.CPP:3813, 0x80077004)
void Behaviour::SubwayDodgeRight(Behaviour* b) {
    MARKFUNCTION(0x80077004);

    if (!b || !b->owner) {
        return;
    }

    Obstacle* issuer = dynamic_cast<Obstacle*>(b->owner->GetTicketIssuer());
    if (!issuer) {
        b->handlerThisOffset = 0;
        b->handlerDispatch = -1;
        b->handler = Behaviour::NDMS;
        return;
    }

    const s32 facingCheck = (s32)((u32)(issuer->orientation.y + 0x8000 - b->owner->orientation.y) & 0xFFFFu);
    if ((u32)(facingCheck - 0x4000) <= 0x8000u) {
        b->handlerThisOffset = 0;
        b->handlerDispatch = -1;
        b->handler = Behaviour::NDMS;
        return;
    }

    tagCollisionBox testBox = issuer->collBox;
    testBox.minX = (s16)(testBox.minX + 100);
    testBox.minY = (s16)(testBox.minY + 100);
    testBox.minZ = (s16)(testBox.minZ + 100);
    testBox.maxX = (s16)(testBox.maxX - 100);
    testBox.maxY = (s16)(testBox.maxY - 100);
    testBox.maxZ = (s16)(testBox.maxZ - 100);

    LVector delta = {};
    delta.x = b->owner->pos.x - issuer->pos.x;
    delta.y = b->owner->pos.y - issuer->pos.y;
    delta.z = b->owner->pos.z - issuer->pos.z;

    const s32 sinV = rmSin16(issuer->orientation.y);
    const s32 cosV = rmSin16(issuer->orientation.y + 0x4000);
    const s32 localX = (s32)(((s64)cosV * delta.x) >> 16) + (s32)((-(s64)sinV * delta.z) >> 16);
    const s32 localZ = (s32)(((s64)sinV * delta.x) >> 16) + (s32)(((s64)cosV * delta.z) >> 16);

    if (testBox.minX > testBox.maxX || testBox.minY > testBox.maxY || testBox.minZ > testBox.maxZ
        || localX < testBox.minX || localX > testBox.maxX
        || delta.y < testBox.minY || delta.y > testBox.maxY
        || localZ < testBox.minZ || localZ > testBox.maxZ) {
        b->handlerThisOffset = 0;
        b->handlerDispatch = -1;
        b->handler = Behaviour::NDMS;
        return;
    }

    b->owner->FaceThingDesired(nullptr);
    b->owner->FaceAngleY(b->owner->faceAngle, 0);
    if (b->animConfigPtr) {
        b->owner->moveSpeed = *(s16*)((u8*)b->animConfigPtr + 44);
    }
    b->owner->SetDesiredMoveDirection(b->owner->faceAngle + 0x4000);
    b->owner->SetTarget(nullptr);
    b->owner->RequestAction(6);
}

// PSX: SubwayDodgeLeft__9Behaviour (BEHAVE.CPP:3911, 0x80077200)
void Behaviour::SubwayDodgeLeft(Behaviour* b) {
    MARKFUNCTION(0x80077200);

    if (!b || !b->owner) {
        return;
    }

    Obstacle* issuer = dynamic_cast<Obstacle*>(b->owner->GetTicketIssuer());
    if (!issuer) {
        b->handlerThisOffset = 0;
        b->handlerDispatch = -1;
        b->handler = Behaviour::NDMS;
        return;
    }

    const s32 facingCheck = (s32)((u32)(issuer->orientation.y + 0x8000 - b->owner->orientation.y) & 0xFFFFu);
    if ((u32)(facingCheck - 0x4000) <= 0x8000u) {
        b->handlerThisOffset = 0;
        b->handlerDispatch = -1;
        b->handler = Behaviour::NDMS;
        return;
    }

    tagCollisionBox testBox = issuer->collBox;
    testBox.minX = (s16)(testBox.minX + 100);
    testBox.minY = (s16)(testBox.minY + 100);
    testBox.minZ = (s16)(testBox.minZ + 100);
    testBox.maxX = (s16)(testBox.maxX - 100);
    testBox.maxY = (s16)(testBox.maxY - 100);
    testBox.maxZ = (s16)(testBox.maxZ - 100);

    LVector delta = {};
    delta.x = b->owner->pos.x - issuer->pos.x;
    delta.y = b->owner->pos.y - issuer->pos.y;
    delta.z = b->owner->pos.z - issuer->pos.z;

    const s32 sinV = rmSin16(issuer->orientation.y);
    const s32 cosV = rmSin16(issuer->orientation.y + 0x4000);
    const s32 localX = (s32)(((s64)cosV * delta.x) >> 16) + (s32)((-(s64)sinV * delta.z) >> 16);
    const s32 localZ = (s32)(((s64)sinV * delta.x) >> 16) + (s32)(((s64)cosV * delta.z) >> 16);

    if (testBox.minX > testBox.maxX || testBox.minY > testBox.maxY || testBox.minZ > testBox.maxZ
        || localX < testBox.minX || localX > testBox.maxX
        || delta.y < testBox.minY || delta.y > testBox.maxY
        || localZ < testBox.minZ || localZ > testBox.maxZ) {
        b->handlerThisOffset = 0;
        b->handlerDispatch = -1;
        b->handler = Behaviour::NDMS;
        return;
    }

    b->owner->FaceThingDesired(nullptr);
    b->owner->FaceAngleY(b->owner->faceAngle, 0);
    if (b->animConfigPtr) {
        b->owner->moveSpeed = *(s16*)((u8*)b->animConfigPtr + 44);
    }
    b->owner->SetDesiredMoveDirection(b->owner->faceAngle - 0x4000);
    b->owner->SetTarget(nullptr);
    b->owner->RequestAction(6);
}

// PSX: SubwayDodgeJump__9Behaviour (BEHAVE.CPP:4001, 0x800773FC)
void Behaviour::SubwayDodgeJump(Behaviour* b) {
    MARKFUNCTION(0x800773FC);

    if (!b || !b->owner) {
        return;
    }

    Thing* issuer = b->owner->GetTicketIssuer();
    if (!issuer) {
        b->handlerThisOffset = 0;
        b->handlerDispatch = -1;
        b->handler = Behaviour::NDMS;
        return;
    }

    const s32 facingCheck = (s32)((u32)(issuer->orientation.y + 0x8000 - b->owner->orientation.y) & 0xFFFFu);
    if ((u32)(facingCheck - 9102) <= 0xB8E3u) {
        b->handlerThisOffset = 0;
        b->handlerDispatch = -1;
        b->handler = Behaviour::NDMS;
        return;
    }

    if (b->animConfigPtr) {
        b->owner->moveSpeed = *(s16*)((u8*)b->animConfigPtr + 44);
    }
    b->owner->RequestAction(3);

    b->nextHandlerThisOffset = 0;
    b->nextHandlerDispatch = -1;
    b->nextHandler = NDMS;

    b->handlerThisOffset = 0;
    b->handlerDispatch = -1;
    b->handler = Behaviour::NDMS;
}
