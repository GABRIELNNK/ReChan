#include "ai/behaviour.h"
#include "ai/activezn.h"
#include "ai/colfight.h"
#include "ai/cominter.h"
#include "ai/humanoid.h"
#include "ai/obstacle.h"
#include "ai/player.h"
#include "ai/thing.h"
#include "gen/ai.h"
#include "gen/colsect.h"
#include "gen/fxp.h"
#include "gen/director.h"
#include "gen/game.h"
#include "gen/camera.h"
#include "gen/model.h"
#include "gen/path.h"
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

// PSX gp+0x7FC / gp+0x800 / gp+0x814 threshold table entries (800DD148/4C/60).
static constexpr s32 PATH_BREAKOFF_DIST_THRESHOLD = 0x3E8;
static constexpr s32 PATH_NODE_REACHED_THRESHOLD = 0x12C;
static constexpr s32 REJOIN_ACTIVE_ZONE_PLAYER_DIST_THRESHOLD = 0x311;
static constexpr s32 NDMS_SLOWDOWN_DIST_THRESHOLD = 0x9C4;
static constexpr s32 NDMS_FAR_DIST_THRESHOLD = 0x6D6;
static constexpr s32 NDMS_MID_DIST_THRESHOLD = 0x4B0;

static constexpr s32 NDMS_HEIGHT_DELTA_THRESHOLD = 0x100;
static constexpr s32 NDMS_FLOOR_SAFE_DELTA_THRESHOLD = 0x101;

// PSX BreakOffPathAndFight field-of-view checks.
static constexpr s32 BREAKOFF_FIELD_LEFT = 0x3555;
static constexpr s32 BREAKOFF_FIELD_RIGHT = 0x3555;

// PSX global at 0x800DD9B8 set by SetAIHandler for Oscar henchmen paths.
static Humanoid* OscarsHenchman = nullptr;

static void RestoreDeferredHandlerOrNDMS(Behaviour* self) {
    if (self->nextHandlerDispatch != 0) {
        self->handlerThisOffset = self->nextHandlerThisOffset;
        self->handlerDispatch = self->nextHandlerDispatch;
        self->handler = self->nextHandler;
    }
    else {
        self->handlerThisOffset = 0;
        self->handlerDispatch = -1;
        self->handler = Behaviour::NDMS;
    }

    self->nextHandlerThisOffset = 0;
    self->nextHandlerDispatch = 0;
    self->nextHandler = nullptr;
}

Behaviour::Behaviour(Humanoid* ownerHumanoid, u32 handlerType, s32 inAiParam) {
    MARKFUNCTION(0x8007458C);
    owner = ownerHumanoid;
    aiType = handlerType;
    currentPath = nullptr;
    field36 = 0;
    currentPathNodeIndex = 0;
    ndmsThinkCounter = 0;
    ndmsThinkDelay = 45;
    ndmsDecision = 5;
    aiParam = inAiParam;
    field60 = 0;
    navDecisionCounter = 50;
    navDecision = 2;
    comboScriptCursor = 0;
    comboScriptIndex = -1;
    worldNavCached = 0;
    worldNavCachedTicks = 0;
    worldNavBlocked = 0;
    worldNavDirection = 0;
    handlerThisOffset = 0;
    handlerDispatch = -1;
    handler = nullptr;
    ndmsRangeBand = 0;
    previousButtons = 0;
    buttonHoldCounter = 0;
    behaviourFlags = 0;
    padPort = 1;
    field276 = 0;
    SetAIHandler(handlerType);
}

void Behaviour::SetAIHandler(u32 handlerType) {
    MARKFUNCTION(0x800746DC);

    nextHandlerThisOffset = 0;
    nextHandlerDispatch = 0;
    nextHandler = nullptr;

    const u32 behaviourHash = owner ? owner->behaviourNameHash : 0;
    if (g_ai && behaviourHash != 0) {
        animConfigPtr = static_cast<BehaviourAttrib*>(g_ai->behaviourList.FindNodeCRC(behaviourHash));
    }
    else {
        animConfigPtr = nullptr;
    }

    if (!animConfigPtr && g_ai) {
        animConfigPtr = static_cast<BehaviourAttrib*>(g_ai->behaviourList.FindNode("Default"));
    }

    handlerThisOffset = 0;
    handlerDispatch = -1;
    handler = Idle;

    if (owner && behaviourHash == p3dHash("OscarHenchmen")) {
		// TODO
        OscarsHenchman = owner;
        return;
    }

    switch (handlerType) {
    case AITypes::TT_PLAYER:
        padPort = 0;
        handler = PlayerUserControl;
        break;
    case AITypes::TT_GRONTAR:
    case AITypes::TT_PAUL:
    case AITypes::TT_OSCAR:
    case AITypes::TT_DANTE:
    case AITypes::TT_BUTCH:
        // TODO
        break;
    default:
        break;
    }
}

bool Behaviour::InActiveZone() const {
    MARKFUNCTION(0x80074888);
    return owner && owner->IsInActiveZone();
}

void Behaviour::Process() {
    MARKFUNCTION(0x800748AC);

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

// PSX: BreakOffPathAndFight__9Behaviour (BEHAVE.CPP:1201, 0x80074BA0)
s32 Behaviour::BreakOffPathAndFight() {
    MARKFUNCTION(0x80074BA0);

    if (!owner) {
        return 0;
    }

    if ((u32)(owner->actionState - 25) < 3u) {
        return 0;
    }

    Player* player = Player::s_player;
    if (!player) {
        return 0;
    }

    if (owner->DistanceFromPointXZ(player->pos) <= PATH_BREAKOFF_DIST_THRESHOLD) {
        return 0;
    }

    return IsPointInFieldOf(player->pos, owner->pos, owner->orientation.y, BREAKOFF_FIELD_LEFT, BREAKOFF_FIELD_RIGHT)
        ? 1
        : 0;
}

// PSX: InitPathAIState__9BehaviourP10LinearPath (BEHAVE.CPP:4299, 0x800778C4)
void Behaviour::InitPathAIState(LinearPath* path) {
    MARKFUNCTION(0x800778C4);

    currentPath = path;
    currentPathNodeIndex = 0;
    field36 = 0;

    if (currentPath) {
        handlerThisOffset = 0;
        handlerDispatch = -1;
        handler = AiFollowPath;
    }
}

// PSX: AiFollowPath__9Behaviour (BEHAVE.CPP:1248, 0x80074C80)
void Behaviour::AiFollowPath(Behaviour* self) {
    MARKFUNCTION(0x80074C80);

    if (!self || !self->owner) {
        return;
    }

    self->field276 = 0;

    if (!self->InActiveZone()) {
        self->handlerThisOffset = 0;
        self->handlerDispatch = -1;
        self->handler = GetBackIntoActiveZone;
        return;
    }

    if (self->BreakOffPathAndFight() != 0) {
        RestoreDeferredHandlerOrNDMS(self);
        return;
    }

    ActiveZone* activeZone = self->owner->activeZone;
    LinearPath* path = self->currentPath;
    if (!activeZone || !path || self->currentPathNodeIndex >= (u32)path->numPoints) {
        RestoreDeferredHandlerOrNDMS(self);
        return;
    }

    const u32 nodeIndex = self->currentPathNodeIndex;
    const u32 distToNode = (u32)self->owner->DistanceFromPointXZ(path->positions[nodeIndex]);

    if (distToNode < PATH_NODE_REACHED_THRESHOLD) {
        activeZone->DoActionsAtNode(path, (s32)nodeIndex, self->owner);

        self->currentPathNodeIndex = nodeIndex + 1;
        if (self->currentPathNodeIndex >= (u32)path->numPoints
            || activeZone->DoAICheck(path, (s32)self->currentPathNodeIndex, self->owner) == 0) {
            RestoreDeferredHandlerOrNDMS(self);
            return;
        }
    }
    else {
        if (activeZone->AllowBreakoffOfDestinationNode(path, (s32)nodeIndex)
            && activeZone->DoAICheck(path, (s32)nodeIndex, self->owner) == 0) {
            RestoreDeferredHandlerOrNDMS(self);
            return;
        }
    }

    if (activeZone->IsPathNodeTerminator(path, (s32)self->currentPathNodeIndex)) {
        if (self->currentPathNodeIndex != 0) {
            self->currentPathNodeIndex--;
        }
        return;
    }

    if (self->animConfigPtr) {
        self->owner->moveSpeed = (s16)self->animConfigPtr->runningSpeed;
    }

    self->owner->FacePointDesired(path->positions[self->currentPathNodeIndex]);
    self->owner->RequestAction(2);
}

// PSX: Idle__9Behaviour (BEHAVE.CPP:1877, 0x800754DC)
void Behaviour::Idle(Behaviour* self) {
    MARKFUNCTION(0x800754DC);

    if (!self || !self->owner) {
        return;
    }

    if (g_director && g_director->scriptState != 0) {
        return;
    }

    Humanoid* owner = self->owner;
    ActiveZone* ownerZone = owner->activeZone;
    if (!ownerZone) {
        return;
    }

    ActiveZone* activeZoneFromAttrib = nullptr;
    if (g_ai && owner->field388 != 0) {
        activeZoneFromAttrib = static_cast<ActiveZone*>(g_ai->activeZoneList.FindNodeCRC((u32)owner->field388, nullptr));
    }

    bool active = true;

    if (owner->field388 == 0) {
        if (owner->field392 == 0
            && owner->field396 == 0
            && owner->field400 == 0
            && owner->field404 == 0
            && owner->field412 == 0) {
            if (!ownerZone->IsInActiveZone(Player::s_player)) {
                return;
            }
        }
    }

    if (owner->field388 != 0) {
        active = (activeZoneFromAttrib != nullptr) ? activeZoneFromAttrib->IsInActiveZone(Player::s_player) : false;
    }

    if (owner->field392 != 0 || owner->field396 != 0 || owner->field400 != 0) {
        bool inSubZone = false;

        if (owner->field392 != 0) {
            SubZoneVolume* sub = static_cast<SubZoneVolume*>(ownerZone->subZoneList.FindNodeCRC((u32)owner->field392, nullptr));
            inSubZone = (sub != nullptr) ? sub->IsInSubZoneVolume(Player::s_player) : false;
        }

        if (owner->field396 != 0) {
            SubZoneVolume* sub = static_cast<SubZoneVolume*>(ownerZone->subZoneList.FindNodeCRC((u32)owner->field396, nullptr));
            inSubZone = inSubZone || ((sub != nullptr) ? sub->IsInSubZoneVolume(Player::s_player) : false);
        }

        if (owner->field400 != 0) {
            SubZoneVolume* sub = static_cast<SubZoneVolume*>(ownerZone->subZoneList.FindNodeCRC((u32)owner->field400, nullptr));
            inSubZone = inSubZone || ((sub != nullptr) ? sub->IsInSubZoneVolume(Player::s_player) : false);
        }

        active = active && inSubZone;
    }

    if (owner->field408 != -1) {
        if (ownerZone->IsInActiveZone(Player::s_player)) {
            const s32 memberCountMinusOne = (s32)ownerZone->memberCount - 1;
            if (owner->field408 >= memberCountMinusOne) {
                active = true;
            }
        }
    }

    if (!active) {
        return;
    }

    if (owner->field404 != 0) {
        SubZoneVolume* sub = static_cast<SubZoneVolume*>(ownerZone->subZoneList.FindNodeCRC((u32)owner->field404, nullptr));
        if (sub && sub->IsInSubZoneVolume(Player::s_player)) {
            return;
        }
    }

    if (owner->field412 != 0) {
        Model* model = static_cast<Model*>(owner->model);
        const s32 modelGate = model ? ((s32)model->modelFlags >> 4) : 0;
        active = active && (modelGate != 0);
    }

    if (!active) {
        return;
    }

    if (self->BreakOffPathAndFight() == 0) {
        self->currentPath = ownerZone->FindFirstValidPath(owner);
    }

    if (self->currentPath) {
        self->nextHandlerThisOffset = 0;
        self->nextHandlerDispatch = -1;
        self->nextHandler = NDMS;
        self->InitPathAIState(self->currentPath);
    }
    else {
        self->handlerThisOffset = 0;
        self->handlerDispatch = -1;
        self->handler = NDMS;
    }
}

// PSX: GetBackIntoActiveZone__9Behaviour (BEHAVE.CPP:4174, 0x800776FC)
void Behaviour::GetBackIntoActiveZone(Behaviour* self) {
    MARKFUNCTION(0x800776FC);

    if (!self || !self->owner) {
        return;
    }

    Player* player = Player::s_player;
    if (!player) {
        return;
    }

    if (self->owner->DistanceFromPoint(player->pos) <= REJOIN_ACTIVE_ZONE_PLAYER_DIST_THRESHOLD) {
        self->handlerThisOffset = 0;
        self->handlerDispatch = -1;
        self->handler = NDMS;
        return;
    }

    ActiveZone* activeZone = self->owner->activeZone;
    if (!activeZone) {
        return;
    }

    LVector center = {};
    center.x = (activeZone->box.minX + activeZone->box.maxX) / 2;
    center.y = (activeZone->box.minY + activeZone->box.maxY) / 2;
    center.z = (activeZone->box.minZ + activeZone->box.maxZ) / 2;

    if (self->InActiveZone()) {
        s32 testX = self->owner->pos.x;
        s32 testZ = self->owner->pos.z;

        if (testX < center.x) {
            testX -= 0x400;
        }
        else {
            testX += 0x400;
        }

        if (testZ < center.z) {
            testZ -= 0x400;
        }
        else {
            testZ += 0x400;
        }

        const s32 distFromCenterXZ = self->owner->DistanceFromPointXZ(center);
        if (activeZone->box.IsInside(testX, testZ) || distFromCenterXZ < 0x101) {
            if (self->owner->IsTargetInActiveZone()) {
                self->handlerThisOffset = 0;
                self->handlerDispatch = -1;
                self->handler = NDMS;
            }
            else {
                self->owner->FaceThingDesired(player);
                self->owner->moveSpeed = 0x5A;
                self->owner->RequestAction(0x15);
            }
            return;
        }
    }

    self->owner->FacePointDesired(center);
    if (self->animConfigPtr) {
        self->owner->moveSpeed = (s16)self->animConfigPtr->runningSpeed;
    }
    self->owner->SetTarget(player);
    self->owner->RequestAction(6);
}

// PSX: PlayerUserControl__9Behaviour (BEHAVE.CPP:1474, 0x80074F5C)
void Behaviour::PlayerUserControl(Behaviour* self) {
    MARKFUNCTION(0x80074F5C);
    if (!self || !self->owner || !g_game || !g_inputManager) {
        return;
    }

    Humanoid* owner = self->owner;

    if (!DebugUI::IsPlayerInputAllowed()) {
        owner->RequestAction(GA_GUARD_RELEASE);
        return;
    }

    u32 buttons = 0;
    if ((self->behaviourFlags & Behaviour::BF_INPUT_PROCESSING) != 0) {
        buttons = (u32)g_game->GetControlVal(self->padPort);
    }
    else {
        self->behaviourFlags |= Behaviour::BF_INPUT_PROCESSING;
    }

    const Control* control = nullptr;
    if ((u16)self->padPort < 2u) {
        control = &g_inputManager->controls[self->padPort];
    }

    s32 analogX = 0;
    s32 analogY = 0;
    s32 stickX = 0;
    s32 stickY = 0;
    s32 direction = 0;
    s32 ownerAngle = owner->orientation.y;
    s32 targetAngle = ownerAngle;

    owner->moveSpeed = 0;

    if (control && (control->padType == 's' || control->padType == 'S') && buttons != 0) {
        stickX = (s32)control->analogLX - 127;
        stickY = (s32)control->analogLY - 127;

        s32 absX = stickX;
        if (absX < 0) {
            absX = -absX;
        }
        if (absX >= 64) {
            analogX = stickX;
            if (stickX > 0) {
                buttons |= PsxPad::Right;
            }
            else {
                buttons |= PsxPad::Left;
            }
        }

        s32 absY = stickY;
        if (absY < 0) {
            absY = -absY;
        }
        if (absY >= 64) {
            analogY = stickY;
            if (stickY > 0) {
                buttons |= PsxPad::Down;
            }
            else {
                buttons |= PsxPad::Up;
            }
        }
    }
    else if ((buttons & 0xF000u) != 0) {
        if ((buttons & PsxPad::Left) != 0) {
            analogX = -32;
            stickX = -127;
        }
        else if ((buttons & PsxPad::Right) != 0) {
            analogX = 32;
            stickX = 127;
        }

        if ((buttons & PsxPad::Up) != 0) {
            analogY = -32;
            stickY = -127;
        }
        else if ((buttons & PsxPad::Down) != 0) {
            analogY = 32;
            stickY = 127;
        }
    }

    if (analogX != 0 || analogY != 0) {
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
        if (absX < 0) {
            absX = -absX;
        }
        s32 absY = stickY;
        if (absY < 0) {
            absY = -absY;
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

    if (self->previousButtons == buttons) {
        self->buttonHoldCounter++;
    }
    else {
        self->buttonHoldCounter = 0;
    }
    self->previousButtons = buttons;

    s32 actionReq = FindActionRequest(self->actionRequestState, buttons & 0xFFFF0FFFu, direction, (u16)self->padPort);

    if ((u32)(actionReq - 2) < 3
        || actionReq == GA_STRAFE
        || actionReq == 19
        || actionReq == GA_GRAB
        || actionReq == GA_GRAB_FORWARD
        || actionReq == 16) {
        owner->SetDesiredMoveDirection(targetAngle);
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
            s16 spd = (s16)animConfigPtr->runningSpeed;
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
            s16 spd = (s16)animConfigPtr->runningSpeed;
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

// PSX: ComplexAttack__9Behaviour (BEHAVE.CPP:1015, 0x80074914)
void Behaviour::ComplexAttack() {
    MARKFUNCTION(0x80074914);

    if (!owner || !animConfigPtr) {
        return;
    }

    if (comboScriptIndex == -1 && animConfigPtr->comboCount > 0) {
        if (owner->field484) {
            return;
        }

        s32 randomChance = (s32)rmRangedRandom(100) + 1;
        s32 selected = 0;
        while (selected < (s32)animConfigPtr->comboCount) {
            const s32 chance = (s32)animConfigPtr->comboChances[selected];
            if (chance >= randomChance) {
                break;
            }
            randomChance -= chance;
            selected++;
        }

        if (selected < (s32)animConfigPtr->comboCount) {
            comboScriptIndex = selected;
            comboScriptCursor = 0;
        }
    }

    if (comboScriptIndex != -1 && animConfigPtr->comboStrings) {
        const char* comboString = animConfigPtr->comboStrings[comboScriptIndex];
        if (!comboString) {
            comboScriptIndex = -1;
            comboScriptCursor = 0;
            return;
        }

        const char actionCode = comboString[comboScriptCursor];
        if (actionCode == 0) {
            comboScriptIndex = -1;
            comboScriptCursor = 0;
            return;
        }

        if (comboScriptCursor != 0) {
            if (owner->field488) {
                return;
            }
        }
        else if (owner->field484) {
            return;
        }

        comboScriptCursor++;

        switch (actionCode) {
            case 'C':
                owner->RequestAction(17);
                return;
            case 'F':
                owner->RequestAction(15);
                return;
            case 'K':
                owner->RequestAction(9);
                return;
            case 'L':
                owner->RequestAction(12);
                return;
            case 'P':
                owner->RequestAction(8);
                return;
            case 'T':
                owner->RequestAction(7);
                return;
            case 'W':
                owner->RequestAction(18);
                return;
            default:
                return;
        }
    }

    s32 randomChance = (s32)rmRangedRandom(100) + 1;
    char actionCode = 0;

    if (randomChance <= (s32)animConfigPtr->punch) {
        actionCode = 'P';
    }
    else {
        randomChance -= (s32)animConfigPtr->punch;
        if (randomChance <= (s32)animConfigPtr->kick) {
            actionCode = 'K';
        }
        else {
            randomChance -= (s32)animConfigPtr->kick;
            if (randomChance <= (s32)animConfigPtr->throwChance) {
                actionCode = 'T';
            }
            else {
                owner->moveSpeed = (s32)(s16)animConfigPtr->strafingSpeed;
                owner->SetDesiredMoveDirection(owner->faceAngle + 0x8000);
                owner->SetTarget(nullptr);
                owner->RequestAction(6);
                return;
            }
        }
    }

    switch (actionCode) {
        case 'P':
            owner->RequestAction(8);
            return;
        case 'K':
            owner->RequestAction(9);
            return;
        case 'T':
            owner->RequestAction(7);
            return;
        default:
            return;
    }
}

// PSX: BackoffAndTaunt__9Behaviour (BEHAVE.CPP:2033, 0x800757EC)
void Behaviour::BackoffAndTaunt(Behaviour* b) {
    MARKFUNCTION(0x800757EC);

    if (!b || !b->owner || !b->animConfigPtr) {
        return;
    }

    b->owner->FaceThingDesired(Player::s_player);

    const s32 state = b->owner->actionState;
    const bool stateAllowsDistanceChecks =
        state == 10 || state == 12 || state == 13 || state == 15 || state == 17;

    if (!stateAllowsDistanceChecks) {
        if (b->navDecisionCounter < 15) {
            s32 floorDelta = b->LookAheadFloorCheck((s16)0x8000, 512, 256);
            if (floorDelta < 0) {
                floorDelta = -floorDelta;
            }
            if (floorDelta < NDMS_FLOOR_SAFE_DELTA_THRESHOLD) {
                b->navDecisionCounter++;
                b->owner->moveSpeed = (s32)(s16)b->animConfigPtr->strafingSpeed;
                b->owner->SetDesiredMoveDirection(b->owner->faceAngle + 0x8000);
                b->owner->SetTarget(nullptr);
                b->owner->RequestAction(6);
                return;
            }
        }

        b->navDecisionCounter = 0;
    }
    else {
        const s32 distToPlayer = b->owner->DistanceFromPoint(Player::s_player->pos);

        s32 floorDelta = b->LookAheadFloorCheck((s16)0x8000, 50, 256);
        if (floorDelta < 0) {
            floorDelta = -floorDelta;
        }

        const s32 wallBlocked = b->LookAheadWallCheck((s16)0x8000, 50, 256);
        if (distToPlayer < 1500 && floorDelta < NDMS_FLOOR_SAFE_DELTA_THRESHOLD && wallBlocked == 0) {
            b->owner->moveSpeed = (s32)(s16)b->animConfigPtr->strafingSpeed;
            b->owner->SetDesiredMoveDirection(b->owner->faceAngle + 0x8000);
            b->owner->SetTarget(nullptr);
            b->owner->RequestAction(6);
            return;
        }

        b->navDecisionCounter = 0;
    }

    if ((s32)rmRangedRandom(100) < 25 || b->owner->HasEnemyTauntDialog()) {
        const s32 tauntRoll = (s32)rmRangedRandom(4);
        if (tauntRoll == 1) {
            b->owner->field316 = 91;
        }
        else if (tauntRoll != 0) {
            b->owner->field316 = 90;
        }
        else {
            b->owner->field316 = 92;
        }

        b->owner->RequestAction(21);
    }

    RestoreDeferredHandlerOrNDMS(b);
}

// PSX: BackOutOfTheFight__9Behaviour (BEHAVE.CPP:2133, 0x80075A68)
void Behaviour::BackOutOfTheFight(Behaviour* b) {
    MARKFUNCTION(0x80075A68);

    if (!b || !b->owner || !b->animConfigPtr) {
        return;
    }

    b->owner->FaceThingDesired(Player::s_player);

    const s32 state = b->owner->actionState;
    const bool inRecoverRange = ((u32)(state - 69) < 4u) || state == 56;
    if (inRecoverRange) {
        RestoreDeferredHandlerOrNDMS(b);
        return;
    }

    if ((u32)(state - 46) < 9u) {
        return;
    }

    if (b->navDecisionCounter < 11) {
        s32 floorDelta = b->LookAheadFloorCheck((s16)0x8000, 512, 256);
        if (floorDelta < 0) {
            floorDelta = -floorDelta;
        }

        if (floorDelta < NDMS_FLOOR_SAFE_DELTA_THRESHOLD) {
            b->navDecisionCounter++;
            b->owner->moveSpeed = (s32)(s16)b->animConfigPtr->strafingSpeed;
            b->owner->SetDesiredMoveDirection(b->owner->faceAngle + 0x8000);
            b->owner->SetTarget(nullptr);
            b->owner->RequestAction(6);
            return;
        }
    }

    b->navDecisionCounter = 0;
    RestoreDeferredHandlerOrNDMS(b);
}

// PSX: NDMS__9Behaviour (BEHAVE.CPP:2214, 0x80075BE4)
void Behaviour::NDMS(Behaviour* b) {
    MARKFUNCTION(0x80075BE4);

    if (!b || !b->owner) {
        return;
    }

    Humanoid* owner = b->owner;
    b->currentPath = nullptr;
    owner->FaceThingDesired(Player::s_player);

    s32 desiredMoveDir = owner->faceAngle;
    s32 randomCircling = 0;
    s32 randomAggression = 0;
    s32 randomDistancing = 0;

    ActiveZone* activeZone = owner->activeZone;
    if (!activeZone) {
        return;
    }

    Player* player = Player::s_player;
    if (!player) {
        return;
    }

    owner->SetTarget(player);

    if (!b->InActiveZone()) {
        if (activeZone->subZoneList.tail) {
            b->handlerThisOffset = 0;
            b->handlerDispatch = -1;
            b->handler = DieWhenWeHitTheGround;
            return;
        }

        if (owner->DistanceFromPoint(player->pos) > NDMS_MID_DIST_THRESHOLD) {
            b->handlerThisOffset = 0;
            b->handlerDispatch = -1;
            b->handler = GetBackIntoActiveZone;
            return;
        }
    }

    if (activeZone && b->BreakOffPathAndFight() == 0) {
        b->currentPath = activeZone->FindFirstValidPath(owner);
    }

    if (b->currentPath) {
        b->InitPathAIState(b->currentPath);
        b->nextHandlerThisOffset = 0;
        b->nextHandlerDispatch = -1;
        b->nextHandler = NDMS;
        return;
    }

    const s32 playerState = player->actionState;
    if ((u32)(playerState - 69) < 4u || playerState == 56) {
        b->nextHandlerThisOffset = 0;
        b->nextHandlerDispatch = -1;
        b->nextHandler = NDMS;
        b->currentPathNodeIndex = 0;
        b->handlerThisOffset = 0;
        b->handlerDispatch = -1;
        b->handler = BackoffAndTaunt;
        return;
    }

    const s32 actionState = owner->actionState;
    if ((u32)(actionState - 32) >= 5u) {
        b->comboScriptIndex = -1;
        b->comboScriptCursor = 0;
    }

    if ((u32)(actionState - 69) < 4u || actionState == 56) {
        return;
    }

    Thing* pickup = g_ai ? g_ai->GetPickupWithinReach(owner) : nullptr;
    if (pickup) {
        // PSX checks pickup +0x14C before requesting throw/grab action.
        const s32 pickupStrayWeapon = *reinterpret_cast<s32*>(reinterpret_cast<u8*>(pickup) + 0x14C);
        if (pickupStrayWeapon != 0) {
            owner->RequestAction(7);
            return;
        }
    }

    b->ndmsThinkCounter++;
    if (b->ndmsThinkCounter > b->ndmsThinkDelay) {
        b->ndmsThinkCounter = 0;
        b->ndmsThinkDelay = (s32)b->animConfigPtr->attackFreq
            + (s32)rmRangedRandom((u32)((s32)b->animConfigPtr->minThink - (s32)b->animConfigPtr->attackFreq));
        randomCircling = (s32)rmRangedRandom(100);
        randomDistancing = (s32)rmRangedRandom(100);
        randomAggression = (s32)rmRangedRandom(100);
    }

    s32 decision = b->ndmsDecision;
    const s32 distanceToPlayer = owner->DistanceFromPoint(player->pos);
    const s32 facingDelta = (s32)((u32)(owner->orientation.y - player->orientation.y) & 0xFFFFu);
    const bool facingAway = (u32)(facingDelta - 0x2AAA) > 0xAAABu;

    if (distanceToPlayer > NDMS_FAR_DIST_THRESHOLD) {
        b->ndmsRangeBand = 0;

        s32 dy = owner->pos.y - player->pos.y;
        if (dy < 0) {
            dy = -dy;
        }

        if (dy <= NDMS_HEIGHT_DELTA_THRESHOLD
            && owner->DistanceFromPointXZ(player->pos) <= REJOIN_ACTIVE_ZONE_PLAYER_DIST_THRESHOLD) {
            if (randomAggression - 20 < (s32)b->animConfigPtr->aggression) {
                decision = 6;
            }
            else {
                decision = owner->HasEnemyTauntDialog() ? 6 : 5;
            }
        }
        else if (randomCircling < (s32)b->animConfigPtr->circling) {
            if (b->field60 != 0 && distanceToPlayer < NDMS_SLOWDOWN_DIST_THRESHOLD) {
                owner->moveSpeed = (s32)(s16)b->animConfigPtr->strafingSpeed;
                decision = 1;
            }
            else {
                b->field60 = 0;
                decision = 0;
                owner->moveSpeed = (s32)(s16)b->animConfigPtr->runningSpeed;
            }

            if (!owner->GetTicketIssuer()) {
                b->NavigateWorld(desiredMoveDir);
            }
        }
        else {
            if (randomCircling - 20 < (s32)b->animConfigPtr->circling) {
                decision = 6;
            }
            else if (randomCircling - 20 < (s32)b->animConfigPtr->aggression) {
                decision = 6;
            }
            else {
                decision = owner->HasEnemyTauntDialog() ? 6 : 5;
            }
        }
    }
    else if (distanceToPlayer > NDMS_MID_DIST_THRESHOLD) {
        b->ndmsRangeBand = 1;
        b->field60 = 1;

        s32 dy = owner->pos.y - player->pos.y;
        if (dy < 0) {
            dy = -dy;
        }

        if (dy <= NDMS_HEIGHT_DELTA_THRESHOLD
            && owner->DistanceFromPointXZ(player->pos) <= REJOIN_ACTIVE_ZONE_PLAYER_DIST_THRESHOLD) {
            if (randomAggression - 20 < (s32)b->animConfigPtr->aggression) {
                decision = 6;
            }
            else {
                decision = owner->HasEnemyTauntDialog() ? 6 : 5;
            }
        }
        else if (randomCircling < (s32)b->animConfigPtr->circling) {
            owner->moveSpeed = (s32)(s16)b->animConfigPtr->strafingSpeed;

            if (owner->GetTicketIssuer()) {
                decision = 1;
            }
            else {
                b->NavigateWorld(desiredMoveDir);
                decision = b->NavigateEnemies(1);
            }
        }
        else {
            if (randomCircling - 20 < (s32)b->animConfigPtr->circling) {
                decision = 6;
            }
            else if (randomCircling - 20 < (s32)b->animConfigPtr->aggression) {
                decision = 6;
            }
            else {
                decision = owner->HasEnemyTauntDialog() ? 6 : 5;
            }
        }
    }
    else if (distanceToPlayer > REJOIN_ACTIVE_ZONE_PLAYER_DIST_THRESHOLD) {
        b->ndmsRangeBand = 3;
        b->field60 = 1;

        if (b->comboScriptIndex != -1) {
            b->ComplexAttack();
            return;
        }

        if ((u32)(actionState - 46) < 9u && randomCircling < (s32)b->animConfigPtr->circling) {
            b->nextHandlerThisOffset = 0;
            b->nextHandlerDispatch = -1;
            b->nextHandler = NDMS;
            b->currentPathNodeIndex = 0;
            b->handlerThisOffset = 0;
            b->handlerDispatch = -1;
            b->handler = BackOutOfTheFight;
            return;
        }

        const bool allowedToMove =
            (b->aiParam != 0 && activeZone && activeZone->AllowedToMoveIn(owner) != 0);

        if (allowedToMove) {
            if (randomAggression < (s32)b->animConfigPtr->aggression) {
                decision = 7;
            }
            else if (facingAway && randomAggression - 70 < (s32)b->animConfigPtr->aggression) {
                decision = 7;
            }
            else if (randomCircling < (s32)b->animConfigPtr->circling) {
                b->nextHandlerThisOffset = 0;
                b->nextHandlerDispatch = -1;
                b->nextHandler = NDMS;
                b->currentPathNodeIndex = 0;
                b->handlerThisOffset = 0;
                b->handlerDispatch = -1;
                b->handler = BackOutOfTheFight;
                return;
            }
            else {
                decision = 5;
            }
        }
        else if (b->aiParam != 0 && activeZone) {
            owner->moveSpeed = (s32)(s16)b->animConfigPtr->strafingSpeed;
            decision = 4;
        }
        else {
            if (randomAggression < (s32)b->animConfigPtr->aggression) {
                decision = 7;
            }
            else if (facingAway && randomAggression - 70 < (s32)b->animConfigPtr->aggression) {
                decision = 7;
            }
            else if (randomCircling < (s32)b->animConfigPtr->circling) {
                b->nextHandlerThisOffset = 0;
                b->nextHandlerDispatch = -1;
                b->nextHandler = NDMS;
                b->currentPathNodeIndex = 0;
                b->handlerThisOffset = 0;
                b->handlerDispatch = -1;
                b->handler = BackOutOfTheFight;
                return;
            }
            else {
                decision = 5;
            }
        }
    }
    else {
        b->ndmsRangeBand = 2;
        b->field60 = 1;

        if (b->comboScriptIndex != -1) {
            b->ComplexAttack();
            return;
        }

        s32 dy = owner->pos.y - player->pos.y;
        if (dy < 0) {
            dy = -dy;
        }

        if (dy <= NDMS_HEIGHT_DELTA_THRESHOLD
            && owner->DistanceFromPointXZ(player->pos) <= REJOIN_ACTIVE_ZONE_PLAYER_DIST_THRESHOLD) {
            if (randomAggression < (s32)b->animConfigPtr->aggression) {
                if (owner->pos.y < player->pos.y) {
                    owner->RequestAction(9);
                }
                else {
                    owner->RequestAction(8);
                }
                return;
            }

            if (randomCircling < (s32)b->animConfigPtr->circling) {
                decision = 6;
            }
            else {
                decision = owner->HasEnemyTauntDialog() ? 6 : 5;
            }
        }
        else if (randomCircling < (s32)b->animConfigPtr->circling || facingAway) {
            owner->moveSpeed = (s32)(s16)b->animConfigPtr->strafingSpeed;

            if (owner->GetTicketIssuer()) {
                decision = 1;
            }
            else if (b->aiParam != 0 && activeZone) {
                if (activeZone->AllowedToMoveIn(owner) != 0) {
                    if (facingAway) {
                        decision = 1;
                    }
                    else {
                        decision = b->NavigateEnemies(2);
                    }
                }
                else {
                    decision = b->NavigateEnemies(3);
                }
            }
            else {
                decision = b->NavigateEnemies(2);
            }
        }
        else if (randomAggression < (s32)b->animConfigPtr->aggression) {
            decision = 5;
        }
        else if (randomDistancing < (s32)b->animConfigPtr->distancing) {
            decision = 6;
        }
        else {
            decision = 7;
        }
    }

    b->ndmsDecision = decision;

    if ((u32)b->ndmsDecision < 5u) {
        if (b->ndmsDecision == 0 || b->ndmsDecision == 1) {
            if (!owner->GetTicketIssuer()) {
                s32 floorDelta = b->LookAheadFloorCheck(0, 512, 256);
                s32 absFloorDelta = floorDelta;
                if (absFloorDelta < 0) {
                    absFloorDelta = -absFloorDelta;
                }

                if (absFloorDelta >= NDMS_FLOOR_SAFE_DELTA_THRESHOLD) {
                    if ((u32)(floorDelta + 0x400) < 0xA01u) {
                        owner->RequestAction(3);
                        b->nextHandlerThisOffset = 0;
                        b->nextHandlerDispatch = -1;
                        b->nextHandler = NDMS;
                        b->handlerThisOffset = 0;
                        b->handlerDispatch = -1;
                        b->handler = Jumping;
                        return;
                    }

                    s32 nextDecision = 6;
                    if ((s32)rmRangedRandom(1) == 0 && !owner->HasEnemyTauntDialog()) {
                        nextDecision = 5;
                    }
                    b->ndmsDecision = nextDecision;
                }
            }
        }
        else if (b->ndmsDecision == 2 || b->ndmsDecision == 3) {
            const s16 angleOffset = (b->ndmsDecision == 2) ? (s16)-0x4000 : (s16)0x4000;
            s32 floorDelta = b->LookAheadFloorCheck(angleOffset, 512, 256);
            if (floorDelta < 0) {
                floorDelta = -floorDelta;
            }

            if (floorDelta >= NDMS_FLOOR_SAFE_DELTA_THRESHOLD) {
                s32 nextDecision = 6;
                if ((s32)rmRangedRandom(1) == 0 && !owner->HasEnemyTauntDialog()) {
                    nextDecision = 5;
                }
                b->ndmsDecision = nextDecision;
            }
        }
        else if (b->ndmsDecision == 4) {
            s32 floorDelta = b->LookAheadFloorCheck((s16)0x8000, 512, 256);
            if (floorDelta < 0) {
                floorDelta = -floorDelta;
            }

            if (floorDelta >= NDMS_FLOOR_SAFE_DELTA_THRESHOLD) {
                s32 nextDecision = 6;
                if ((s32)rmRangedRandom(1) == 0 && !owner->HasEnemyTauntDialog()) {
                    nextDecision = 5;
                }
                b->ndmsDecision = nextDecision;
            }
        }
    }

    owner->SetDesiredMoveDirection(desiredMoveDir);
    owner->SetTarget(player);

    switch (b->ndmsDecision) {
        case 0:
            owner->RequestAction(2);
            break;
        case 1:
            owner->RequestAction(6);
            break;
        case 2:
            owner->SetDesiredMoveDirection(owner->faceAngle - 0x4000);
            owner->RequestAction(6);
            break;
        case 3:
            owner->SetDesiredMoveDirection(owner->faceAngle + 0x4000);
            owner->RequestAction(6);
            break;
        case 4:
            owner->SetDesiredMoveDirection(owner->faceAngle + 0x8000);
            owner->RequestAction(6);
            break;
        case 6:
        {
            const s32 tauntRoll = (s32)rmRangedRandom(4);
            if (tauntRoll == 0) {
                owner->field316 = 92;
            }
            else if (tauntRoll == 1) {
                owner->field316 = 91;
            }
            else {
                owner->field316 = 90;
            }
            owner->RequestAction(21);
            break;
        }
        case 7:
            b->ComplexAttack();
            break;
        default:
            owner->RequestAction(1);
            break;
    }
}

// PSX: NavigateWorld__9BehaviourRl (BEHAVE.CPP:3317, 0x80076870)
s32 Behaviour::NavigateWorld(s32& moveDirection) {
    MARKFUNCTION(0x80076870);

    if (!owner) {
        return 0;
    }

    if (worldNavCached != 0) {
        moveDirection = worldNavDirection;
        if (worldNavCachedTicks > 0) {
            worldNavCachedTicks--;
        }
        else {
            worldNavCached = 0;
        }
        return moveDirection;
    }

    s32 collisionRatio = 0;
    LVector wallNormal = {};
    LVector hitPoint = {};
    s32 verticalSpan = 0;
    s32 wallMaterial = 0;

    if (worldNavBlocked != 0) {
        s32 hit = owner->CheckWallCollision(
            (s16)moveDirection,
            1024,
            150,
            500,
            collisionRatio,
            wallNormal,
            hitPoint,
            verticalSpan,
            wallMaterial);
        if (hit == 0 && owner->CheckDWOCollision((s16)moveDirection, 1024) == 0) {
            worldNavBlocked = 0;
            return moveDirection;
        }

        hit = owner->CheckWallCollision(
            (s16)worldNavDirection,
            512,
            150,
            500,
            collisionRatio,
            wallNormal,
            hitPoint,
            verticalSpan,
            wallMaterial);
        if (hit == 0 && owner->CheckDWOCollision((s16)worldNavDirection, 1024) == 0) {
            worldNavCached = 1;
            worldNavCachedTicks = 1;
            moveDirection = worldNavDirection;
            return moveDirection;
        }

        s32 wallAngle = 0;
        if (wallNormal.x != 0) {
            if (wallNormal.z == 0) {
                wallAngle = (wallNormal.x > 0) ? 0x4000 : 0xC000;
            }
            else {
                wallAngle = -(s32)rmATan216((f32)(-wallNormal.x), (f32)(-wallNormal.z)) + 0x4000;
                s32 absAngle = wallAngle;
                if (absAngle < 0) {
                    absAngle = -absAngle;
                }
                if (absAngle > 0x7FFF) {
                    if (wallAngle > 0) {
                        wallAngle -= 0x10000;
                    }
                    else {
                        wallAngle += 0x10000;
                    }
                }
                wallAngle += 0x8000;
            }
        }
        else {
            wallAngle = (wallNormal.z < 1) ? 0x8000 : 0;
        }

        const LVector playerPos = Player::s_player ? Player::s_player->pos : owner->pos;

        s32 turnDegrees = 0;
        if (wallNormal.x != 0) {
            turnDegrees = -15;
            if (wallNormal.x <= 0) {
                turnDegrees = 15;
                if (hitPoint.z < playerPos.z) {
                    turnDegrees = -15;
                }
            }
            else if (hitPoint.z < playerPos.z) {
                turnDegrees = 15;
            }
        }
        else {
            turnDegrees = 15;
            if (wallNormal.z > 0) {
                if (hitPoint.x < playerPos.x) {
                    turnDegrees = -15;
                }
            }
            else {
                turnDegrees = -15;
                if (hitPoint.x < playerPos.x) {
                    turnDegrees = 15;
                }
            }
        }

        if (turnDegrees <= 0) {
            if (owner->CheckWallCollision(
                    (s16)(wallAngle + 0x4000),
                    512,
                    150,
                    500,
                    collisionRatio,
                    wallNormal,
                    hitPoint,
                    verticalSpan,
                    wallMaterial) != 0) {
                turnDegrees = 15;
            }
        }
        else {
            if (owner->CheckWallCollision(
                    (s16)(wallAngle - 0x4000),
                    512,
                    150,
                    500,
                    collisionRatio,
                    wallNormal,
                    hitPoint,
                    verticalSpan,
                    wallMaterial) != 0) {
                turnDegrees = -15;
            }
        }

        worldNavDirection += (turnDegrees << 16) / 360;
        moveDirection = worldNavDirection;
        return moveDirection;
    }

    s32 hit = owner->CheckWallCollision(
        (s16)moveDirection,
        1024,
        150,
        500,
        collisionRatio,
        wallNormal,
        hitPoint,
        verticalSpan,
        wallMaterial);
    if (hit != 0 || owner->CheckDWOCollision((s16)moveDirection, 1024) != 0) {
        worldNavBlocked = 1;
    }
    else {
        worldNavCached = 1;
        worldNavCachedTicks = 1;
    }

    worldNavDirection = moveDirection;
    return moveDirection;
}

// PSX: NavigateEnemies__9Behaviouri (BEHAVE.CPP:3549, 0x80076C84)
s32 Behaviour::NavigateEnemies(s32 mode) {
    MARKFUNCTION(0x80076C84);

    if (!owner || !animConfigPtr) {
        return 1;
    }

    navDecisionCounter++;

    LVector ownerPos = owner->pos;
    const s32 sideFieldAngle = owner->orientation.y - 0x4000;
    s32 bias = 50;

    if (aiParam != 0) {
        ActiveZone* zone = owner->activeZone;
        if (zone) {
            for (s32 i = 0; i < 3; i++) {
                Humanoid* other = zone->members[i];
                if (!other || other == owner) {
                    continue;
                }

                if (IsPointInFieldOf(other->pos, ownerPos, sideFieldAngle, 0x4000, 0x4000)) {
                    bias += 30;
                }
                else {
                    bias -= 30;
                }
            }
        }
    }
    else {
        Humanoid** humans = FightingCollision::GetHumanoidArray();
        for (s32 i = 0; i < FIGHTING_COLLISION_MAX; i++) {
            Humanoid* other = humans[i];
            if (!other || other == owner) {
                continue;
            }

            if (IsPointInFieldOf(other->pos, ownerPos, sideFieldAngle, 0x4000, 0x4000)) {
                bias += 30;
            }
            else {
                bias -= 30;
            }
        }
    }

    if (mode == 3) {
        if (navDecisionCounter >= 51) {
            navDecisionCounter = 0;
            const s32 roll = (s32)rmRangedRandom(100);
            if ((u32)(roll - 31) < 0x27u) {
                navDecision = ((u32)(roll - 36) < 0x1Du) ? 5 : 4;
            }
            else if (roll >= bias) {
                navDecision = 2;
            }
            else {
                navDecision = mode;
            }
        }
        return navDecision;
    }

    if (mode == 2) {
        if (navDecisionCounter < 51) {
            return navDecision;
        }

        navDecisionCounter = 0;
        if ((s32)rmRangedRandom(100) >= (s32)animConfigPtr->maxThink) {
            navDecision = ((s32)animConfigPtr->circling < (s32)rmRangedRandom(100)) ? 5 : 1;
            return navDecision;
        }

        const s32 roll = (s32)rmRangedRandom(100);
        if ((u32)(roll - 31) < 0x27u) {
            navDecision = ((u32)(roll - 41) < 0x13u) ? 5 : 4;
        }
        else if (roll >= bias) {
            navDecision = 2;
        }
        else {
            navDecision = 3;
        }
        return navDecision;
    }

    if (mode != 1) {
        return 1;
    }

    if (navDecisionCounter < 51) {
        return navDecision;
    }

    navDecisionCounter = 0;
    if ((s32)rmRangedRandom(150) >= (s32)animConfigPtr->maxThink) {
        if ((s32)animConfigPtr->circling >= (s32)rmRangedRandom(100)) {
            navDecision = mode;
        }
        else {
            navDecision = 5;
        }
        return navDecision;
    }

    const s32 roll = (s32)rmRangedRandom(100);
    if ((u32)(roll - 41) < 0x13u) {
        navDecision = 5;
    }
    else if (roll >= bias) {
        navDecision = 2;
    }
    else {
        navDecision = 3;
    }

    return navDecision;
}

// PSX: Jumping__9Behaviour (BEHAVE.CPP:4057, 0x800774BC)
void Behaviour::Jumping(Behaviour* b) {
    MARKFUNCTION(0x800774BC);

    if (!b || !b->owner) {
        return;
    }

    if ((b->owner->flags & TF_ON_GROUND) == 0) {
        b->owner->RequestAction(2);
        return;
    }

    if (b->field276 != 0) {
        b->handlerThisOffset = 0;
        b->handlerDispatch = -1;
        b->handler = AiFollowPath;
        return;
    }

    RestoreDeferredHandlerOrNDMS(b);
}

// PSX: DieWhenWeHitTheGround__9Behaviour (BEHAVE.CPP:4119, 0x8007762C)
void Behaviour::DieWhenWeHitTheGround(Behaviour* b) {
    MARKFUNCTION(0x8007762C);

    if (!b || !b->owner) {
        return;
    }

    const s32 floorHeight = g_collisionSectors[0].GetWorldFloorHeight(b->owner->pos, 0);
    if (floorHeight == (s32)0x80000001) {
        b->owner->health = 0;
    }
    else {
        if ((b->owner->flags & TF_ON_GROUND) == 0) {
            b->owner->RequestAction(2);
            return;
        }
        b->owner->health = 0;
    }

    if (b->owner->actionState != 72) {
        b->owner->SetActionState(72, 0);
    }
}

// PSX: LookAheadFloorCheck__9Behaviourlll (BEHAVE.CPP:4325, 0x800778F8)
s32 Behaviour::LookAheadFloorCheck(s16 angleOffset, s32 distance, s32 radius) {
    MARKFUNCTION(0x800778F8);

    if (!owner) {
        return (s32)0x80000001;
    }

    LVector testPos = owner->pos;
    const s16 angle = (s16)(owner->orientation.y + angleOffset);
    testPos.x += (s32)(((s64)rmSin16(angle) * distance) >> 16);
    testPos.y += 1024;
    testPos.z += (s32)(((s64)rmSin16((s16)(angle + 0x4000)) * distance) >> 16);

    const s32 floorHeight = g_collisionSectors[0].GetWorldFloorHeight(testPos, radius);
    if (floorHeight != (s32)0x80000001) {
        return testPos.y - floorHeight - 1024;
    }
    return floorHeight;
}

// PSX: LookAheadWallCheck__9Behaviourlll (BEHAVE.CPP:4355, 0x80077A14)
s32 Behaviour::LookAheadWallCheck(s16 angleOffset, s32 distance, s32 radius) {
    MARKFUNCTION(0x80077A14);

    if (!owner) {
        return 0;
    }

    return owner->QuickCheckWallCollision((s16)(owner->orientation.y + angleOffset), distance, radius, 512);
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
        b->owner->moveSpeed = (s16)b->animConfigPtr->runningSpeed;
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
        b->owner->moveSpeed = (s16)b->animConfigPtr->runningSpeed;
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
        b->owner->moveSpeed = (s16)b->animConfigPtr->runningSpeed;
    }
    b->owner->RequestAction(3);

    b->nextHandlerThisOffset = 0;
    b->nextHandlerDispatch = -1;
    b->nextHandler = NDMS;

    b->handlerThisOffset = 0;
    b->handlerDispatch = -1;
    b->handler = Behaviour::Jumping;
}
