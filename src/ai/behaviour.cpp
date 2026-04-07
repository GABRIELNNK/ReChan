// behaviour.cpp - AI Behaviour base class
// Reversed from PSX C:\CHAN\GAME\SRC\AI\BEHAVIOU.CPP
#include "ai/behaviour.h"
#include "ai/humanoid.h"
#include "ai/thing.h"
#include "gen/control.h"
#include "gen/game.h"
#include "p3d/p3dmath.h"

static constexpr u32 ACTION_GUARD_RELEASE = 1;
static constexpr u32 ACTION_RUN = 2;
static constexpr u32 ACTION_JUMP = 3;
static constexpr u32 ACTION_GUARD = 4;
static constexpr u32 ACTION_STRAFE = 5;
static constexpr u32 ACTION_BACKFLIP = 6;
static constexpr u32 ACTION_ATTACK = 7;
static constexpr u32 ACTION_PICKUP = 15;

Behaviour::Behaviour(Humanoid* ownerHumanoid, u32 handlerType, s32 /*aiParam*/) {
    MARKFUNCTION(0x80069CB0);
    owner = ownerHumanoid;
    handlerThisOffset = 0;
    handlerDispatch = -1;
    handler = nullptr;
    previousButtons = 0;
    previousRunPressed = false;
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

    // PSX: positive dispatch values call through handler vtable indices.
    // Those behaviour subclasses are not reversed yet.
}

void Behaviour::PlayerUserControl(Behaviour* behaviour) {
    MARKFUNCTION(0x8006AFDC);
    if (!behaviour || !behaviour->owner || !g_game) {
        return;
    }

    Humanoid* owner = behaviour->owner;
    owner->commandBits = 0;

    u32 buttons = (u32)g_game->GetControlVal(0);
    s32 dx = 0;
    s32 dz = 0;

    if (buttons & PsxPad::Up) {
        dz += 1;
    }
    if (buttons & PsxPad::Down) {
        dz -= 1;
    }
    if (buttons & PsxPad::Right) {
        dx += 1;
    }
    if (buttons & PsxPad::Left) {
        dx -= 1;
    }

    bool runPressed = (dx != 0 || dz != 0);
    if (runPressed) {
        owner->RequestAction(ACTION_RUN);
        f32 rad = atan2((f32)dx, (f32)dz);
        owner->faceAngle = RAD2ANGLE(rad) & 0xFFFF;
    }

    if (!runPressed && behaviour->previousRunPressed) {
        owner->RequestAction(ACTION_GUARD_RELEASE);
    }

    behaviour->previousRunPressed = runPressed;

    if (buttons & PsxPad::Cross) {
        owner->RequestAction(ACTION_JUMP);
    }
    if (buttons & PsxPad::Square) {
        owner->RequestAction(ACTION_ATTACK);
    }
    if (buttons & PsxPad::Circle) {
        owner->RequestAction(ACTION_GUARD);
    }
    if (buttons & (PsxPad::R1 | PsxPad::R2)) {
        owner->RequestAction(ACTION_STRAFE);
    }
    if (buttons & PsxPad::L1) {
        owner->RequestAction(ACTION_BACKFLIP);
    }
    if (buttons & PsxPad::Triangle) {
        owner->RequestAction(ACTION_PICKUP);
    }

    behaviour->previousButtons = buttons;
}
