#pragma once

#include "core.h"

class Game;

#define SAVEGAME_SLOT_COUNT 8

struct SaveGameSlotInfo {
    bool occupied = false;
    char dateText[32] = {};
};

// Reads slot metadata for menu display. Returns true when a valid save exists.
bool SaveGameQuerySlotInfo(s32 slotIndex, SaveGameSlotInfo* outInfo);

// Serializes the current runtime save state into a slot.
bool SaveGameWriteSlot(s32 slotIndex);

// Deletes a save slot file if it exists.
bool SaveGameDeleteSlot(s32 slotIndex);

// Loads a slot into runtime state and stages a pending level transition.
bool SaveGameLoadSlot(s32 slotIndex);

// Returns true while a loaded slot is waiting to be applied by game state code.
bool SaveGameHasPendingLoad();

// Applies a pending loaded target level/petal to the given game instance.
// Returns true when a pending load existed and was applied.
bool SaveGameApplyPendingLoad(Game* game);

// Applies pending loaded lives after a new player instance exists.
void SaveGameApplyPendingLives();
