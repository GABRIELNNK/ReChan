#include "ai/cominter.h"
#include "gen/control.h"

// Button index sentinels in commandList buttonPair field
static constexpr s16 BTN_NONE = 16;
static constexpr s16 BTN_ANY = 17;

// Condition types
static constexpr u32 COND_UNCONDITIONAL = (u32)-1;
static constexpr u32 COND_ANALOG = 15;

static constexpr s32 DIR_MASK = 0xF;

struct CommandEntry {
    s32 buttonPair;
    s32 condition;
    s32 threshold;
    s32 actionId;
};

static constexpr s32 COMMAND_LIST_SIZE = 20;

// PSX: commandList at 0x800D45B0, commandListSize at gp+2940
static const CommandEntry s_commandList[COMMAND_LIST_SIZE] = {
    { 0x00110003, -1,  0,  5 },
    { 0x00050004, -1,  0, 20 },
    { 0x00070004, -1,  0, 12 },
    { 0x00060004, -1,  0, 14 },
    { 0x00060007, -1,  0, 14 },
    { 0x00100007,  2,  0, 10 },
    { 0x00100004,  2,  0, 11 },
    { 0x00100007, -1,  8, 12 },
    { 0x00100004, -1, 10, 13 },
    { 0x00100007, -1,  0,  8 },
    { 0x00100004, -1,  0,  9 },
    { 0x00100006, 15,  0,  4 },
    { 0x00100006, -1,  0,  3 },
    { 0x00100005,  1,  6, 18 },
    { 0x00100005, -1,  6, 17 },
    { 0x00100005,  1,  0, 15 },
    { 0x00100005, -1,  0,  7 },
    { 0x00110002, -1,  0, 20 },
    { 0x00100001, -1,  0,  6 },
    { 0x00110011, 15,  0,  2 },
};

// PSX: FindActionRequest__FRUlUlUlPC7Control (COMINTER.CPP:84, 0x800B0E18)
s32 FindActionRequest(u32* state, u32 buttons, s32 direction, u16 padIndex) {
    s32 action = 1;

    for (s32 i = 0; i < COMMAND_LIST_SIZE; i++) {
        if (action != 1) {
            return action;
        }

        const CommandEntry* entry = &s_commandList[i];
        s16 btn1 = (s16)(entry->buttonPair & 0xFFFF);
        s16 btn2 = (s16)(entry->buttonPair >> 16);
        u32 threshold = (u32)entry->threshold;

        s32 btn1Mask = 0;
        if (btn1 != BTN_NONE) {
            btn1Mask = 1 << btn1;
        }
        s32 btn2Mask = 0;
        if (btn2 != BTN_NONE) {
            btn2Mask = 1 << btn2;
        }
        if (btn1 == BTN_ANY) {
            btn1Mask = (s32)buttons;
        }
        if (btn2 == BTN_ANY) {
            btn2Mask = (s32)buttons;
        }

        // PSX: GetMappedButton hold duration queries
        u32 holdTime1, holdTime2;
        if (btn1Mask && g_inputManager) {
            Button* btn = g_inputManager->GetButtonForBit(padIndex, (u8)(entry->buttonPair & 0xFF));
            holdTime1 = (u32)(u16)btn->duration;
        }
        else {
            holdTime1 = 0;
        }
        if (btn2Mask && g_inputManager) {
            Button* btn = g_inputManager->GetButtonForBit(padIndex, (u8)((entry->buttonPair >> 16) & 0xFF));
            holdTime2 = (u32)(u16)btn->duration;
        }
        else {
            holdTime2 = 0;
        }

        s32 combinedMask = btn1Mask | btn2Mask;
        u32 condition = (u32)entry->condition;
        bool checkHold = false;

        if (condition == COND_ANALOG) {
            s32 match;
            if (combinedMask) {
                match = ((s32)buttons & combinedMask) == combinedMask;
            }
            else {
                match = buttons == 0;
            }

            if (threshold) {
                s32 actionId = entry->actionId;
                if (((*state >> actionId) & 1) != 0 && holdTime1 < threshold && holdTime2 < threshold) {
                    *state &= ~(1u << actionId);
                }
                s32 dirMatch = 0;
                if ((direction & DIR_MASK) != 0) {
                    dirMatch = match;
                }
                if (!dirMatch) {
                    continue;
                }
                if (!direction) {
                    checkHold = true;
                }
                else {
                    continue;
                }
            }
            else {
                s32 dirMatch = 0;
                if ((direction & DIR_MASK) != 0) {
                    dirMatch = match;
                }
                if (!dirMatch) {
                    continue;
                }
                action = entry->actionId;
                continue;
            }
        }
        else if (condition >= 0x10) {
            if (condition == COND_UNCONDITIONAL) {
                if (threshold) {
                    s32 actionId = entry->actionId;
                    if (((*state >> actionId) & 1) != 0) {
                        if (holdTime1 < threshold && holdTime2 < threshold) {
                            *state &= ~(1u << actionId);
                        }
                        checkHold = true;
                    }
                    if (!checkHold) {
                        if ((btn1Mask && holdTime1 >= threshold) || (btn2Mask && holdTime2 >= threshold)) {
                            action = entry->actionId;
                            *state |= 1u << action;
                        }
                        continue;
                    }
                }
                else {
                    s32 match;
                    if (combinedMask) {
                        match = ((s32)buttons & combinedMask) == combinedMask;
                    }
                    else {
                        match = buttons == 0;
                    }
                    if (!match) {
                        continue;
                    }
                    action = entry->actionId;
                    continue;
                }
            }
            else {
                s32 dirMask = entry->condition & direction;
                if (!threshold) {
                    s32 match;
                    if (combinedMask) {
                        match = ((s32)buttons & combinedMask) == combinedMask;
                    }
                    else {
                        match = buttons == 0;
                    }
                    s32 dirMatch = 0;
                    if (dirMask) {
                        dirMatch = match;
                    }
                    if (!dirMatch) {
                        continue;
                    }
                    action = entry->actionId;
                    continue;
                }
                s32 actionId = entry->actionId;
                if (((*state >> actionId) & 1) != 0 && holdTime1 < threshold && holdTime2 < threshold) {
                    *state &= ~(1u << actionId);
                }
                if (dirMask) {
                    checkHold = true;
                }
                else {
                    continue;
                }
            }
        }
        else {
            if (condition) {
                s32 dirMask = entry->condition & direction;
                if (!threshold) {
                    s32 match;
                    if (combinedMask) {
                        match = ((s32)buttons & combinedMask) == combinedMask;
                    }
                    else {
                        match = buttons == 0;
                    }
                    s32 dirMatch = 0;
                    if (dirMask) {
                        dirMatch = match;
                    }
                    if (!dirMatch) {
                        continue;
                    }
                    action = entry->actionId;
                    continue;
                }
                s32 actionId = entry->actionId;
                if (((*state >> actionId) & 1) != 0 && holdTime1 < threshold && holdTime2 < threshold) {
                    *state &= ~(1u << actionId);
                }
                if (dirMask) {
                    checkHold = true;
                }
                else {
                    continue;
                }
            }
            else {
                if (!threshold) {
                    s32 match;
                    if (combinedMask) {
                        match = ((s32)buttons & combinedMask) == combinedMask;
                    }
                    else {
                        match = buttons == 0;
                    }
                    if (direction) {
                        continue;
                    }
                    if (!match) {
                        continue;
                    }
                    action = entry->actionId;
                    continue;
                }
                s32 actionId = entry->actionId;
                if (((*state >> actionId) & 1) != 0 && holdTime1 < threshold && holdTime2 < threshold) {
                    *state &= ~(1u << actionId);
                }
                if (!direction) {
                    checkHold = true;
                }
                else {
                    continue;
                }
            }
        }

        // hold activation gate (PSX LABEL_74)
        if (checkHold) {
            if (((*state >> entry->actionId) & 1) != 0) {
                continue;
            }
            if (threshold && ((btn1Mask && holdTime1 >= threshold) || (btn2Mask && holdTime2 >= threshold))) {
                action = entry->actionId;
                *state |= 1u << action;
            }
            continue;
        }
    }

    return action;
}
