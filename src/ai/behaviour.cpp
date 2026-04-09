#include "ai/behaviour.h"
#include "ai/cominter.h"
#include "ai/humanoid.h"
#include "ai/thing.h"
#include "gen/game.h"
#include "gen/camera.h"
#include "pc/inputaction.h"
#include "p3d/p3dmath.h"
#include "pc/log.h"

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

    // Read movement from ActionInput (works for both keyboard and gamepad)
    s32 analogX = (s16)g_actionInput->GetMoveX();
    s32 analogY = g_actionInput->GetMoveY();
    s32 stickX = g_actionInput->GetMoveX();
    s32 stickY = g_actionInput->GetMoveY();
    s32 direction = 0;

    // Direction classification
    s32 ownerAngle = owner->orientation.y;
    s32 targetAngle = ownerAngle;

    owner->attackRange = 0;

    if (analogX || (analogY << 16)) {
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
            owner->attackRange = range;
        }
        else {
            owner->attackRange = 0;
        }
    }

    // Resolve combat action from ActionInput state + direction
    s32 actionReq = g_actionInput->ResolveGameAction(direction);

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
