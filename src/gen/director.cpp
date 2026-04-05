// director.cpp - Director class reversed from PSX DIRECTOR.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\DIRECTOR.CPP
#include "gen/director.h"
#include "gen/ai.h"
#include "ai/humanoid.h"
#include "ai/player.h"
#include "gen/charmgr.h"
#include "gen/game.h"
#include "gen/scoremgr.h"
#include "gen/time.h"
#include "gen/world.h"
#include "snd/drctrsnd.h"
#include "snd/snddrct.h"
#include "snd/sound.h"
#include "p3d/p3dmath.h"
#include "snd/rsevent.h"

namespace {

// PSX globals used by DIRECTOR.CPP script control flow.
// Addresses from decomp notes:
// - directorTimeOut: 0x800DC960
// - returnAddress:   0x800DFC84
static s32 directorTimeOut = 0;
static s32 returnAddressIndex = 0;
static s32 directorDialogCounter = -1;
static s32 directorDialogLimit = 180;
static s32* returnAddress[8] = {};

static s32 start_generic[10] = { 9, 6, -1, 98, 102, 40, 103, 256, 100, 0 };
static s32 start_frantic[12] = { 9, 6, -1, 73, 74, 98, 102, 40, 103, 256, 100, 0 };

static s32 gotopoint[7] = { 9, 64, 19, 84386293, 20, 8, 2 };
static s32 NISladder1[20] = {
    9, 43, 45, 65, 66, 1, 5, 6, -1, 6, 30, 43, 46, 65, 67, 5, 84, 84386293, 88, 0
};

static s32 victory_poor[2] = { 15, 0 };
static s32 victory_ok[2] = { 15, 0 };
static s32 victory_good[2] = { 15, 0 };
static s32 victory_perfect[2] = { 15, 0 };

static s32 death[19] = {
    9, 43, 45, 73, 74, 6, -1, 6, 30, 120, 6, 60, 98, 102, 120, 103, 256, 100, 0
};
static s32 death_vol[19] = {
    9, 43, 45, 6, -1, 121, 120, 127, 6, -1, 73, 74, 98, 102, 120, 103, 256, 100, 0
};
static s32 death_fall_pavement[2] = { 115, 0 };
static s32 death_fall_water[2] = { 115, 0 };
static s32 death_generic[2] = { 115, 0 };
static s32 death_fall_goo[35] = {
    14, 84386293, 69, 3, -2146597480,
    14, 84386293, 69, 4,
    9, 56, 62, 84386293, 5, 56, 58, 5,
    6, -1, 6, 15,
    56, 59, 5,
    6, 30,
    56, 63, 5,
    20, 8, 2,
    9, 116, 0
};

static s32 levelEnd[7] = { 9, 73, 74, 126, 26, 9, 2 };
static s32 wait_subroutine[5] = { 6, -1, 6, 60, 4 };

// PSX script tables present in overlay globals.
static s32 defaultBeginScript[21] = {
    9, 98, 100, 255, 5, 10, 30, 8, 2, 124, 25, -1, 4, 123, 25, -1, 4, 125, 25, -1, 4
};

// Intro/victory scripts are loaded from overlay data on PSX.
// Placeholder stubs until overlay script extraction is done.
static s32 intro_all_dante[1] = { 2 };
static s32 intro_chef[1] = { 2 };
static s32 intro_every_chef[1] = { 2 };
static s32 intro_clown[1] = { 2 };
static s32 intro_every_clown[1] = { 2 };
static s32 intro_grontar[1] = { 2 };
static s32 intro_every_grontar[1] = { 2 };
static s32 intro_disco[1] = { 2 };
static s32 intro_every_disco[1] = { 2 };
static s32 victory_chef[1] = { 2 };
static s32 victory_disco[1] = { 2 };
static s32 victory_grontar[1] = { 2 };
static s32 victory_clown[1] = { 2 };

struct DirectorScriptRegion {
    u32 virtualBase = 0;
    s32* hostBase = nullptr;
    s32 wordCount = 0;
};

static constexpr s32 kMaxDirectorScriptRegions = 96;
static DirectorScriptRegion g_directorScriptRegions[kMaxDirectorScriptRegions] = {};
static s32 g_directorScriptRegionCount = 0;

template <size_t N>
constexpr s32 ScriptArrayWordCount(const s32 (&)[N]) {
    return static_cast<s32>(N);
}

static u32 ToVirtualAddress(const s32* ptr) {
    return static_cast<u32>(reinterpret_cast<uintptr_t>(ptr));
}

static void RegisterScriptRegion(u32 virtualBase, s32* hostBase, s32 wordCount) {
    if (!virtualBase || !hostBase || wordCount <= 0) {
        return;
    }

    for (s32 i = 0; i < g_directorScriptRegionCount; i++) {
        DirectorScriptRegion& region = g_directorScriptRegions[i];
        if (region.virtualBase == virtualBase) {
            region.hostBase = hostBase;
            region.wordCount = wordCount;
            return;
        }
    }

    if (g_directorScriptRegionCount >= kMaxDirectorScriptRegions) {
        return;
    }

    DirectorScriptRegion& next = g_directorScriptRegions[g_directorScriptRegionCount++];
    next.virtualBase = virtualBase;
    next.hostBase = hostBase;
    next.wordCount = wordCount;
}

static s32* ResolveScriptAddress(u32 virtualAddress) {
    for (s32 i = 0; i < g_directorScriptRegionCount; i++) {
        const DirectorScriptRegion& region = g_directorScriptRegions[i];
        if (virtualAddress < region.virtualBase) {
            continue;
        }

        const u32 byteOffset = virtualAddress - region.virtualBase;
        if ((byteOffset & 3) != 0) {
            continue;
        }

        const s32 wordOffset = static_cast<s32>(byteOffset >> 2);
        if (wordOffset < 0 || wordOffset >= region.wordCount) {
            continue;
        }

        return region.hostBase + wordOffset;
    }

    return nullptr;
}

static void RegisterKnownDirectorScriptRegions() {
    static bool initialized = false;
    if (initialized) {
        return;
    }
    initialized = true;

    // PSX symbol bases from scripts/GAME_REL_symbols.txt.
    RegisterScriptRegion(0x8001FA3Cu, intro_all_dante, ScriptArrayWordCount(intro_all_dante));
    RegisterScriptRegion(0x8001FA54u, intro_chef, ScriptArrayWordCount(intro_chef));
    RegisterScriptRegion(0x8001FBCCu, intro_every_chef, ScriptArrayWordCount(intro_every_chef));
    RegisterScriptRegion(0x8001FBFCu, intro_clown, ScriptArrayWordCount(intro_clown));
    RegisterScriptRegion(0x8001FDB4u, intro_every_clown, ScriptArrayWordCount(intro_every_clown));
    RegisterScriptRegion(0x8001FDE4u, intro_grontar, ScriptArrayWordCount(intro_grontar));
    RegisterScriptRegion(0x8001FF84u, intro_every_grontar, ScriptArrayWordCount(intro_every_grontar));
    RegisterScriptRegion(0x8001FFACu, intro_disco, ScriptArrayWordCount(intro_disco));
    RegisterScriptRegion(0x80020140u, intro_every_disco, ScriptArrayWordCount(intro_every_disco));

    RegisterScriptRegion(0x80020168u, victory_chef, ScriptArrayWordCount(victory_chef));
    RegisterScriptRegion(0x800202A8u, victory_disco, ScriptArrayWordCount(victory_disco));
    RegisterScriptRegion(0x80020458u, victory_grontar, ScriptArrayWordCount(victory_grontar));
    RegisterScriptRegion(0x80020588u, victory_clown, ScriptArrayWordCount(victory_clown));
    RegisterScriptRegion(0x800D8208u, victory_poor, ScriptArrayWordCount(victory_poor));
    RegisterScriptRegion(0x800D8278u, victory_ok, ScriptArrayWordCount(victory_ok));
    RegisterScriptRegion(0x800D82E8u, victory_good, ScriptArrayWordCount(victory_good));
    RegisterScriptRegion(0x800D8358u, victory_perfect, ScriptArrayWordCount(victory_perfect));

    RegisterScriptRegion(0x800D7048u, start_generic, ScriptArrayWordCount(start_generic));
    RegisterScriptRegion(0x800D70B0u, start_frantic, ScriptArrayWordCount(start_frantic));
    RegisterScriptRegion(0x800D803Cu, gotopoint, ScriptArrayWordCount(gotopoint));
    RegisterScriptRegion(0x800D8058u, NISladder1, ScriptArrayWordCount(NISladder1));
    RegisterScriptRegion(0x800D83C8u, death, ScriptArrayWordCount(death));
    RegisterScriptRegion(0x800D8454u, death_vol, ScriptArrayWordCount(death_vol));
    RegisterScriptRegion(0x800D84E0u, death_fall_pavement, ScriptArrayWordCount(death_fall_pavement));
    RegisterScriptRegion(0x800D855Cu, death_fall_water, ScriptArrayWordCount(death_fall_water));
    RegisterScriptRegion(0x800D8598u, death_generic, ScriptArrayWordCount(death_generic));
    RegisterScriptRegion(0x800D85C0u, death_fall_goo, ScriptArrayWordCount(death_fall_goo));
    RegisterScriptRegion(0x800D86B0u, levelEnd, ScriptArrayWordCount(levelEnd));
    RegisterScriptRegion(0x800D86CCu, wait_subroutine, ScriptArrayWordCount(wait_subroutine));
    RegisterScriptRegion(0x800D86E0u, defaultBeginScript, ScriptArrayWordCount(defaultBeginScript));

    // Host aliases for any scripts compiled into this binary.
    RegisterScriptRegion(ToVirtualAddress(intro_all_dante), intro_all_dante, ScriptArrayWordCount(intro_all_dante));
    RegisterScriptRegion(ToVirtualAddress(intro_chef), intro_chef, ScriptArrayWordCount(intro_chef));
    RegisterScriptRegion(ToVirtualAddress(intro_every_chef), intro_every_chef, ScriptArrayWordCount(intro_every_chef));
    RegisterScriptRegion(ToVirtualAddress(intro_clown), intro_clown, ScriptArrayWordCount(intro_clown));
    RegisterScriptRegion(ToVirtualAddress(intro_every_clown), intro_every_clown, ScriptArrayWordCount(intro_every_clown));
    RegisterScriptRegion(ToVirtualAddress(intro_grontar), intro_grontar, ScriptArrayWordCount(intro_grontar));
    RegisterScriptRegion(ToVirtualAddress(intro_every_grontar), intro_every_grontar, ScriptArrayWordCount(intro_every_grontar));
    RegisterScriptRegion(ToVirtualAddress(intro_disco), intro_disco, ScriptArrayWordCount(intro_disco));
    RegisterScriptRegion(ToVirtualAddress(intro_every_disco), intro_every_disco, ScriptArrayWordCount(intro_every_disco));
    RegisterScriptRegion(ToVirtualAddress(victory_chef), victory_chef, ScriptArrayWordCount(victory_chef));
    RegisterScriptRegion(ToVirtualAddress(victory_disco), victory_disco, ScriptArrayWordCount(victory_disco));
    RegisterScriptRegion(ToVirtualAddress(victory_grontar), victory_grontar, ScriptArrayWordCount(victory_grontar));
    RegisterScriptRegion(ToVirtualAddress(victory_clown), victory_clown, ScriptArrayWordCount(victory_clown));
    RegisterScriptRegion(ToVirtualAddress(victory_poor), victory_poor, ScriptArrayWordCount(victory_poor));
    RegisterScriptRegion(ToVirtualAddress(victory_ok), victory_ok, ScriptArrayWordCount(victory_ok));
    RegisterScriptRegion(ToVirtualAddress(victory_good), victory_good, ScriptArrayWordCount(victory_good));
    RegisterScriptRegion(ToVirtualAddress(victory_perfect), victory_perfect, ScriptArrayWordCount(victory_perfect));
    RegisterScriptRegion(ToVirtualAddress(start_generic), start_generic, ScriptArrayWordCount(start_generic));
    RegisterScriptRegion(ToVirtualAddress(start_frantic), start_frantic, ScriptArrayWordCount(start_frantic));
    RegisterScriptRegion(ToVirtualAddress(death_fall_pavement), death_fall_pavement, ScriptArrayWordCount(death_fall_pavement));
    RegisterScriptRegion(ToVirtualAddress(death_fall_water), death_fall_water, ScriptArrayWordCount(death_fall_water));
    RegisterScriptRegion(ToVirtualAddress(death_generic), death_generic, ScriptArrayWordCount(death_generic));
    RegisterScriptRegion(ToVirtualAddress(death_fall_goo), death_fall_goo, ScriptArrayWordCount(death_fall_goo));
    RegisterScriptRegion(ToVirtualAddress(gotopoint), gotopoint, ScriptArrayWordCount(gotopoint));
    RegisterScriptRegion(ToVirtualAddress(NISladder1), NISladder1, ScriptArrayWordCount(NISladder1));
    RegisterScriptRegion(ToVirtualAddress(death), death, ScriptArrayWordCount(death));
    RegisterScriptRegion(ToVirtualAddress(death_vol), death_vol, ScriptArrayWordCount(death_vol));
    RegisterScriptRegion(ToVirtualAddress(levelEnd), levelEnd, ScriptArrayWordCount(levelEnd));
    RegisterScriptRegion(ToVirtualAddress(wait_subroutine), wait_subroutine, ScriptArrayWordCount(wait_subroutine));
    RegisterScriptRegion(ToVirtualAddress(defaultBeginScript), defaultBeginScript, ScriptArrayWordCount(defaultBeginScript));
}

static s32 EstimateRuntimeScriptWords(const s32* script, s32 maxWords) {
    if (!script || maxWords <= 0) {
        return 0;
    }

    for (s32 i = 0; i < maxWords; i++) {
        const DirectorOpcode op = static_cast<DirectorOpcode>(script[i]);
        if (op == DirectorOpcode::End || op == DirectorOpcode::EndScript) {
            return i + 1;
        }
    }

    return maxWords;
}

static void RegisterRuntimeScriptRegion(s32* script) {
    if (!script) {
        return;
    }

    const s32 words = EstimateRuntimeScriptWords(script, 1024);
    if (words <= 0) {
        return;
    }

    RegisterScriptRegion(ToVirtualAddress(script), script, words);
}

static s32* ScriptPtrFromWord(s32 word) {
    RegisterKnownDirectorScriptRegions();

    const u32 raw = static_cast<u32>(word);
    if (raw == 0) {
        return nullptr;
    }

    s32* resolved = ResolveScriptAddress(raw);
    if (resolved) {
        return resolved;
    }

    if constexpr (sizeof(void*) == 4) {
        return reinterpret_cast<s32*>(static_cast<uintptr_t>(raw));
    }

    return nullptr;
}

static s32 GetDirectorFrameCounter() {
    if (!g_time) {
        return 0;
    }

    return static_cast<s32>(g_time->GetFrameCounter());
}

static void PushScriptReturnAddress(Director* director, s32* nextScript) {
    if (!director || !nextScript) {
        return;
    }

    if (returnAddressIndex >= 0 && returnAddressIndex < 8) {
        returnAddress[returnAddressIndex] = director->scriptPtr;
        returnAddressIndex++;
    }

    director->scriptPtr = nextScript;
}

static Thing* FindThingByScriptRef(u32 ref) {
    if (!g_ai) {
        return nullptr;
    }

    Thing* thing = g_ai->FindThing(ref);
    if (thing) {
        return thing;
    }

    for (ccMinNode* node = g_ai->thingList.head; node; node = node->next) {
        Thing* candidate = static_cast<Thing*>(node);
        if (candidate && candidate->nameCRC == ref) {
            return candidate;
        }
    }

    return nullptr;
}

static Humanoid* FindHumanoidByScriptRef(u32 ref) {
    Thing* thing = FindThingByScriptRef(ref);
    if (!thing) {
        return nullptr;
    }

    return dynamic_cast<Humanoid*>(thing);
}

static Camera* GetGameCamera() {
    if (!g_game) {
        return nullptr;
    }

    return &g_game->GetCamera();
}

} // namespace

Director* g_director = nullptr;

// PSX: __8Director (DIRECTOR.CPP:2658, 0x800CA1B8)
Director::Director() {
    MARKFUNCTION(0x800CA1B8);
    scriptPtr = nullptr;
    scriptBase = nullptr;
    codeSnipPtr = nullptr;
    codeSnipThing = nullptr;
    processState = 0;
    timerValue = 0;
    loopCount = 0;
    dirFlags = 0;
    wideScreenDesired = 0;
    wideScreenCurrent = 0;
    wsBarCurrent = 0;
    wsBarTarget = 0;
    wsBarStep = 0;
    wsMode = 0;
    wsModePad = 0;
    wsAlphaStep = 0;
    wsAlphaCurrent = 0;
    wsAlphaTarget = 0;
}

// PSX: _._8Director (DIRECTOR.CPP:2681, 0x8003BE10)
Director::~Director() {
    MARKFUNCTION(0x8003BE10);
}

// PSX: InternalOpen__8Director (DIRECTOR.CPP:2704, 0x800CA2AC)
void Director::InternalOpen() {
    MARKFUNCTION(0x800CA2AC);
    // PSX: registers runDirector and DrawDirectorOverlays handlers
    // into Game's handlerSets. Handlers wired in Game::InternalOpen instead.
}

// PSX: InternalClose__8Director (DIRECTOR.CPP:2757, 0x8003C11C)
void Director::InternalClose() {
    MARKFUNCTION(0x8003C11C);
    PurgeAnims();
    cleanUpTexAnim();
    if (directorSound) {
        delete directorSound;
        directorSound = nullptr;
    }
}

// PSX: InternalReset__8Director (DIRECTOR.CPP:2736, 0x8003C04C)
void Director::InternalReset() {
    MARKFUNCTION(0x8003C04C);
    scriptPtr = nullptr;
    scriptBase = nullptr;
    codeSnipPtr = nullptr;
    codeSnipThing = nullptr;
    processState = 0;
    timerValue = 0;
    loopCount = 0;
    dirFlags = 0;
    wideScreenDesired = 0;
    wideScreenCurrent = 0;
    wsBarCurrent = 0;
    wsBarTarget = 0;
    wsBarStep = 0;
    wsMode = 0;
    wsModePad = 0;
    wsAlphaStep = 0;
    wsAlphaCurrent = 0;
    wsAlphaTarget = 0;
    PurgeAnims();
    cleanUpTexAnim();
}

// PSX: LevelReset__8Director (DIRECTOR.CPP:2731, 0x8003C044)
void Director::LevelReset() {
    MARKFUNCTION(0x8003C044);
    field164 = 0;
}

// PSX: SetScript__8Director (DIRECTOR.CPP:2765, 0x8003C234)
void Director::SetScript() {
    MARKFUNCTION(0x8003C234);

    RegisterKnownDirectorScriptRegions();

    scriptPtr = defaultBeginScript;
    RegisterRuntimeScriptRegion(scriptPtr);
    codeSnipPtr = defaultBeginScript;
    field168 = 534;

    directorDialogCounter = -1;
    returnAddressIndex = 0;
    directorDialogLimit = 180;
}

// PSX: SetCodeSnip__8DirectorPlP5Thing (DIRECTOR.CPP:2782, 0x8003C268)
void Director::SetCodeSnip(s32* snip, Thing* thing) {
    MARKFUNCTION(0x8003C268);

    RegisterKnownDirectorScriptRegions();

    field168 = 537;
    directorDialogCounter = -1;

    scriptPtr = snip;
    RegisterRuntimeScriptRegion(scriptPtr);
    codeSnipPtr = snip;
    codeSnipThing = thing;

    returnAddressIndex = 0;
    directorDialogLimit = 180;
}

// PSX: Process__8Director (DIRECTOR.CPP:2806, 0x8003C298)
void Director::Process() {
    MARKFUNCTION(0x8003C298);

    extern s32 g_directorActive;

    RegisterKnownDirectorScriptRegions();

    // PSX clears code snippet pointer after dispatch setup.
    if (codeSnipPtr) {
        scriptPtr = codeSnipPtr;
        RegisterRuntimeScriptRegion(scriptPtr);
        codeSnipPtr = nullptr;
        codeSnipThing = nullptr;
    }

    if (!scriptPtr) {
        g_directorActive = 0;
        return;
    }

    if (scriptBase && *scriptBase) {
        ProcessSoundScript();
    }

    field68 = 0;

    for (s32 i = 0; i < 512; i++) {
        if (!scriptPtr) {
            g_directorActive = 0;
            return;
        }

        if (field68) {
            return;
        }

        const DirectorOpcode opcode = static_cast<DirectorOpcode>(*scriptPtr);

        switch (opcode) {
        case DirectorOpcode::End:
            scriptPtr = nullptr;
            field168 = 0;
            g_directorActive = 0;
            return;

        case DirectorOpcode::ResetTimeout:
            directorTimeOut = 0;
            scriptPtr += 1;
            break;

        case DirectorOpcode::EndScript:
            if (field168 == 534 && g_game && g_game->GetWorld()) {
                const s32 levelID = static_cast<s32>(g_game->GetWorld()->GetCurrentLevelIndex());
                if (levelID >= 0 && levelID < 31) {
                    field164 |= (1 << levelID);
                }
            }

            scriptPtr = nullptr;
            field168 = 0;
            g_directorActive = 0;
            return;

        case DirectorOpcode::Call:
            if (returnAddressIndex >= 0 && returnAddressIndex < 8) {
                returnAddress[returnAddressIndex] = scriptPtr + 2;
                returnAddressIndex++;
            }
            scriptPtr = ScriptPtrFromWord(scriptPtr[1]);
            break;

        case DirectorOpcode::Return:
            if (returnAddressIndex > 0) {
                returnAddressIndex--;
                scriptPtr = returnAddress[returnAddressIndex];
            } else {
                scriptPtr = levelEnd;
            }
            break;

        case DirectorOpcode::Timer:
            Timer();
            if (field68) {
                return;
            }
            break;

        case DirectorOpcode::Loop:
            Loop();
            scriptPtr += 1;
            break;

        case DirectorOpcode::EnablePlayerInput:
            field72 = 1;
            scriptPtr += 1;
            break;

        case DirectorOpcode::DisablePlayerInput:
            field72 = 0;
            scriptPtr += 1;
            break;

        case DirectorOpcode::DetermineLevelIntro:
            scriptPtr += 1;
            DetermineLevelIntro();
            break;

        case DirectorOpcode::FaceThing: {
            const u32 thingRef = static_cast<u32>(scriptPtr[1]);
            scriptPtr += 2;

            Humanoid* humanoid = FindHumanoidByScriptRef(thingRef);
            if (humanoid) {
                humanoid->FaceAngleY(humanoid->orientation.y, 0);
            }
            break;
        }

        case DirectorOpcode::SetHumanoidAction: {
            const u32 thingRef = static_cast<u32>(scriptPtr[1]);
            const s32 state = scriptPtr[2];
            scriptPtr += 3;

            Humanoid* humanoid = FindHumanoidByScriptRef(thingRef);
            if (humanoid) {
                humanoid->SetActionState(static_cast<u32>(state), 0);
            }
            break;
        }

        case DirectorOpcode::DynamicAnimLoad:
        case DirectorOpcode::DynamicAnimWaitLoaded:
        case DirectorOpcode::DynamicAnimWaitCamera:
        case DirectorOpcode::DynamicAnimUnload:
            ProcessDynamicAnimFunc();
            break;

        case DirectorOpcode::WaitAnimationDone:
            WaitAnimationDone();
            if (field68) {
                return;
            }
            break;

        case DirectorOpcode::WaitForNisControl:
            scriptPtr += 2;
            break;

        case DirectorOpcode::RestorePlayerControl:
            scriptPtr += 1;
            break;

        case DirectorOpcode::PlayThingDynamicAnim:
            scriptPtr += 3;
            break;

        case DirectorOpcode::SetupFaceTextureAnim:
            scriptPtr += 1;
            break;

        case DirectorOpcode::CleanupFaceTextureAnim:
            scriptPtr += 1;
            cleanUpTexAnim();
            break;

        case DirectorOpcode::QueueDetermineNextState:
            scriptPtr += 2;
            if (g_game) {
                g_game->SetState(GameState::DetermineNextGameState);
            }
            break;

        case DirectorOpcode::SetGameState: {
            const s32 gameState = scriptPtr[1];
            scriptPtr += 2;
            if (g_game && gameState >= 0 && gameState < static_cast<s32>(GameState::COUNT)) {
                g_game->SetState(static_cast<GameState>(gameState));
            }
            field68 = 1;
            return;
        }

        case DirectorOpcode::SetCheckpoint:
            scriptPtr += 1;
            if (Player::s_player) {
                Player::s_player->OnCheckpoint();
            }
            break;

        case DirectorOpcode::SetCheckpointByUid:
            scriptPtr += 2;
            if (Player::s_player) {
                Player::s_player->OnCheckpoint();
            }
            break;

        case DirectorOpcode::SetCheckpointData:
            field148 = scriptPtr[1];
            scriptPtr += 2;
            break;

        case DirectorOpcode::TriggerCheckpoint:
            scriptPtr += 1;
            if (Player::s_player) {
                Player::s_player->OnCheckpoint();
            }
            break;

        case DirectorOpcode::ClearCheckpointValid:
            scriptPtr += 1;
            break;

        case DirectorOpcode::SpawnEffectFromMatrix:
            scriptPtr += 4;
            break;

        case DirectorOpcode::SpawnEffectFromAttack:
            scriptPtr += 2;
            break;

        case DirectorOpcode::SpawnEffectAtPosA:
            scriptPtr += 4;
            break;

        case DirectorOpcode::SpawnEffectAtPosB:
            scriptPtr += 4;
            break;

        case DirectorOpcode::SpawnEffectByUidAtPos:
            scriptPtr += 5;
            break;

        case DirectorOpcode::SpawnFwEffectAtPos:
            scriptPtr += 5;
            break;

        case DirectorOpcode::DestroyDestructible: {
            const u32 thingRef = static_cast<u32>(scriptPtr[1]);
            scriptPtr += 2;

            Thing* thing = FindThingByScriptRef(thingRef);
            if (thing) {
                thing->Kill();
            }
            break;
        }

        case DirectorOpcode::RemoveNisEffect:
            scriptPtr += 1;
            break;

        case DirectorOpcode::SetPlayerFlag:
            scriptPtr += 2;
            break;

        case DirectorOpcode::ClearGlobalEffectRef:
            scriptPtr += 1;
            break;

        case DirectorOpcode::DropPickup:
            scriptPtr += 2;
            break;

        case DirectorOpcode::CameraFunc:
            scriptPtr += 1;
            ProcessCameraFunc();
            break;

        case DirectorOpcode::DoorFunc:
            scriptPtr += 1;
            ProcessDoorFunc();
            break;

        case DirectorOpcode::FacePointAndNisControl:
            scriptPtr += 1;
            if (Player::s_player) {
                const LVector target = { field180, field184, field188 };
                Player::s_player->FacePoint(target, 1);
            }
            break;

        case DirectorOpcode::LadderFunc:
            scriptPtr += 1;
            ProcessLadderFunc();
            break;

        case DirectorOpcode::ModelFunc:
            scriptPtr += 1;
            ProcessModelFunc();
            break;

        case DirectorOpcode::HudFunc:
            scriptPtr += 1;
            ProcessHudFunc();
            break;

        case DirectorOpcode::SetThingFlag08: {
            const u32 thingRef = static_cast<u32>(scriptPtr[1]);
            scriptPtr += 2;
            Thing* thing = FindThingByScriptRef(thingRef);
            if (thing) {
                thing->flags |= 0x8u;
            }
            break;
        }

        case DirectorOpcode::SetThingFlag28: {
            const u32 thingRef = static_cast<u32>(scriptPtr[1]);
            scriptPtr += 2;
            Thing* thing = FindThingByScriptRef(thingRef);
            if (thing) {
                thing->flags |= 0x28u;
            }
            break;
        }

        case DirectorOpcode::ClearThingFlagsAndKill: {
            const u32 thingRef = static_cast<u32>(scriptPtr[1]);
            scriptPtr += 2;
            Thing* thing = FindThingByScriptRef(thingRef);
            if (thing) {
                thing->flags &= ~0x28u;
                thing->Kill();
            }
            break;
        }

        case DirectorOpcode::KillThingType52:
        case DirectorOpcode::KillThingType88: {
            const u32 thingRef = static_cast<u32>(scriptPtr[1]);
            scriptPtr += 2;
            Thing* thing = FindThingByScriptRef(thingRef);
            if (thing) {
                thing->Kill();
            }
            break;
        }

        case DirectorOpcode::KillThingsIfCombat:
            scriptPtr += 1;
            if (g_ai && g_game && g_game->GetWorld() && g_game->GetWorld()->GetCurrentLevelIndex() > 5) {
                g_ai->KillThings(0);
            }
            break;

        case DirectorOpcode::HumanoidFunc:
            scriptPtr += 1;
            ProcessHumanoidFunc();
            break;

        case DirectorOpcode::ResetCameraManager:
            scriptPtr += 1;
            break;

        case DirectorOpcode::SetDesiredWideScreen:
            SetDesiredWideScreen();
            break;

        case DirectorOpcode::ResetWideScreenDefaults:
            wsBarTarget = 120;
            wsBarStep = 256;
            wsAlphaTarget = 255;
            wsAlphaStep = 10;
            wsMode = 2;
            scriptPtr += 1;
            // PSX also calls SetHUDVisible(0, 0, 0) here.
            break;

        case DirectorOpcode::SetSoundScript:
            // PSX: creates CDirectorSound (type 10140) if not yet created
            if (!directorSound) {
                directorSound = new CDirectorSound();
                directorSound->Initialize();
            }
            scriptPtr += 2;
            scriptBase = ScriptPtrFromWord(scriptPtr[-1]);
            RegisterRuntimeScriptRegion(scriptBase);
            break;

        case DirectorOpcode::EdisonFunc:
            scriptPtr += 1;
            ProcessEdison();
            break;

        case DirectorOpcode::SoundCdYield:
            scriptPtr += 1;
            rsEvent(12, 6, 0, 0);
            break;

        case DirectorOpcode::SoundCdAccess:
            scriptPtr += 1;
            rsEvent(13, 6, 0, 0);
            break;

        case DirectorOpcode::LoadDialogA:
            scriptPtr += 3;
            directorDialogCounter = 0;
            break;

        case DirectorOpcode::LoadDialogB:
            scriptPtr += 4;
            directorDialogCounter = 0;
            break;

        case DirectorOpcode::WaitDialogPlayable:
            scriptPtr += 1;
            directorDialogCounter++;
            break;

        case DirectorOpcode::PlayDialogNear:
        case DirectorOpcode::PlayDialogFar:
        case DirectorOpcode::PlayPriorityDialog:
            scriptPtr += 1;
            break;

        case DirectorOpcode::SetDialogTimeout:
            directorDialogLimit = scriptPtr[1];
            scriptPtr += 2;
            break;

        case DirectorOpcode::SetNisPoint:
            scriptPtr += 2;
            break;

        case DirectorOpcode::SetGotoCheckpoint:
        case DirectorOpcode::SetFallbackCheckpoint:
        case DirectorOpcode::SetBlockCheckpoint:
            scriptPtr += 1;
            break;

        case DirectorOpcode::DetermineVictory:
            scriptPtr += 1;
            DetermineVictoryIdle();
            break;

        case DirectorOpcode::DetermineDeath:
            scriptPtr += 1;
            DetermineDeath();
            break;

        default:
            // This opcode path is intentionally terminated until the corresponding
            // subsystem command handlers are fully reversed.
            scriptPtr = nullptr;
            field168 = 0;
            g_directorActive = 0;
            return;
        }
    }

    field68 = 1;
}

// PSX: ProcessSoundScript__8Director (DIRECTOR.CPP:3576, 0x8003D5A4)
void Director::ProcessSoundScript() {
    MARKFUNCTION(0x8003D5A4);

    const s32 elapsed = GetDirectorFrameCounter() - field160;

    while (scriptBase) {
        const u32 packed = static_cast<u32>(*scriptBase);
        const s32 eventFrame = static_cast<s32>(packed & 0xFFF);
        if (packed == 0 || eventFrame >= elapsed) {
            break;
        }

        scriptBase += 1;

        // PSX: ProcessNISEvent(field192, packed >> 28, packed >> 12)
        if (directorSound) {
            const u32 eventType = packed >> 28;
            const u16 soundId = static_cast<u16>(packed >> 12);
            directorSound->ProcessNISEvent(eventType, soundId);
        }
    }
}

// PSX: Timer__8Director (DIRECTOR.CPP:3611, 0x8003D634)
void Director::Timer() {
    MARKFUNCTION(0x8003D634);

    if (!scriptPtr) {
        field68 = 1;
        return;
    }

    const s32 duration = scriptPtr[1];
    if (duration == -1) {
        field156 = 0;
        field160 = GetDirectorFrameCounter();
        scriptPtr += 2;
        field68 = 0;
        return;
    }

    if (field156) {
        if (GetDirectorFrameCounter() >= field156) {
            scriptPtr += 2;
            field156 = 0;
            field68 = 0;
            return;
        }

        field68 = 1;
        return;
    }

    field156 = field160 + duration;
    field68 = 1;
}

// PSX: Loop__8Director (DIRECTOR.CPP:3637, 0x8003D6CC)
void Director::Loop() {
    MARKFUNCTION(0x8003D6CC);
}

// PSX: SetDesiredWideScreen__8Director (DIRECTOR.CPP:3642, 0x8003D6D4)
void Director::SetDesiredWideScreen() {
    MARKFUNCTION(0x8003D6D4);

    if (!scriptPtr) {
        return;
    }

    scriptPtr += 1;

    for (s32 i = 0; i < 64; i++) {
        if (!scriptPtr) {
            return;
        }

        const DirectorWideScreenCmd token = static_cast<DirectorWideScreenCmd>(*scriptPtr);
        if (token == DirectorWideScreenCmd::End) {
            scriptPtr += 1;
            return;
        }

        scriptPtr += 1;

        switch (token) {
        case DirectorWideScreenCmd::SetAlphaTarget:
            wsAlphaTarget = *scriptPtr;
            scriptPtr += 1;
            break;

        case DirectorWideScreenCmd::SetAlphaCurrent:
            wsAlphaCurrent = *scriptPtr;
            scriptPtr += 1;
            break;

        case DirectorWideScreenCmd::SetAlphaStep:
            wsAlphaStep = *scriptPtr;
            scriptPtr += 1;
            break;

        case DirectorWideScreenCmd::SetBarTarget:
            wsBarTarget = *scriptPtr;
            scriptPtr += 1;
            break;

        case DirectorWideScreenCmd::SetBarStep:
            wsBarStep = *scriptPtr;
            scriptPtr += 1;
            break;

        case DirectorWideScreenCmd::SetMode: {
            s16* p = reinterpret_cast<s16*>(scriptPtr);
            wsMode = static_cast<u16>(*p);
            p += 2;
            scriptPtr = reinterpret_cast<s32*>(p);
            break;
        }

        default:
            break;
        }
    }

    scriptPtr = nullptr;
}

// PSX: ProcessEdison__8Director (DIRECTOR.CPP:3689, 0x8003D800)
void Director::ProcessEdison() {
    MARKFUNCTION(0x8003D800);

    if (!scriptPtr) {
        return;
    }

    const DirectorEdisonCmd opcode = static_cast<DirectorEdisonCmd>(*scriptPtr);
    scriptPtr += 1;

    if (opcode == DirectorEdisonCmd::PlayTransient) {
        // PSX: reads u16 soundID, advances scriptPtr, calls CSoundDirect::PlayTransient(soundID, 0, 0, 0)
        s16* soundData = reinterpret_cast<s16*>(scriptPtr);
        const u16 soundID = static_cast<u16>(*soundData);
        soundData += 2;
        scriptPtr = reinterpret_cast<s32*>(soundData);

        // PSX: PlayTransient__12CSoundDirectUsPC10tagLVectorUsUl(soundID, 0, 0, 0)
        CSoundDirect::PlayTransient(soundID, nullptr, 0, 0);
    } else if (opcode == DirectorEdisonCmd::StopMusic) {
        rsEvent(RS_STOP_MUSIC, 0, 0, 0);
    }
}

// PSX: ProcessModelFunc__8Director (DIRECTOR.CPP:3711, 0x8003D87C)
void Director::ProcessModelFunc() {
    MARKFUNCTION(0x8003D87C);
}

// PSX: ProcessCameraFunc__8Director (DIRECTOR.CPP:3716, 0x8003D884)
void Director::ProcessCameraFunc() {
    MARKFUNCTION(0x8003D884);

    if (!scriptPtr) {
        return;
    }

    const DirectorCameraCmd op = static_cast<DirectorCameraCmd>(*scriptPtr);
    scriptPtr += 1;

    Camera* camera = GetGameCamera();

    switch (op) {
    case DirectorCameraCmd::EnableNisCamera:
        break;

    case DirectorCameraCmd::ClearCameraFlag:
        break;

    case DirectorCameraCmd::SetCameraFlag:
        break;

    case DirectorCameraCmd::CopyP3DFov:
        if (camera) {
            constexpr f32 psxFovToRad = 0.61f / 30000.0f;
            const f32 currentFovRad = camera->GetP3DCamera()->GetFOV();
            const s32 currentFov = static_cast<s32>(currentFovRad / psxFovToRad);
            camera->SetFOV(rmDiv16i(currentFov, 87162));
        }
        break;

    case DirectorCameraCmd::ResetCameraFov:
        if (camera) {
            camera->SetCurFOV(10);
        }
        break;

    case DirectorCameraCmd::SetCameraMode: {
        const s32 mode = *scriptPtr;
        scriptPtr += 1;
        if (camera && mode >= 0 && mode <= 2) {
            camera->SetMode(static_cast<CameraMode>(mode));
        }
        break;
    }

    case DirectorCameraCmd::LoadAsyncAnim:
        scriptPtr += 1;
        break;

    case DirectorCameraCmd::DeleteAsyncAnim:
        break;

    case DirectorCameraCmd::PlayAsyncAnim:
        break;

    case DirectorCameraCmd::ShakeCamera: {
        const s32 selector = *scriptPtr;
        scriptPtr += 1;

        if (selector == 0 || selector == 1 || selector == 2) {
            scriptPtr += 1;
        }

        const s32 frames = *scriptPtr;
        scriptPtr += 1;

        if (camera) {
            camera->ShakeCamera(frames);
        }
        break;
    }

    case DirectorCameraCmd::LookAtNisPoint: {
        scriptPtr += 1;
        if (camera && Player::s_player) {
            const LVector target = Player::s_player->homePos;
            camera->LookAtTarget(&target);
        }
        break;
    }

    case DirectorCameraCmd::SetCameraAndLookAt: {
        const LVector position = { scriptPtr[0], scriptPtr[1], scriptPtr[2] };
        const LVector target = { scriptPtr[3], scriptPtr[4], scriptPtr[5] };
        scriptPtr += 6;

        if (Player::s_player) {
            Player::s_player->pos = position;
            Player::s_player->homePos = position;
        }

        if (camera) {
            camera->SetPosition(position.x, position.y, position.z);
            camera->LookAtTarget(&target);
        }
        break;
    }

    default:
        break;
    }
}

// PSX: ProcessHudFunc__8Director (DIRECTOR.CPP:3845, 0x8003DC44)
void Director::ProcessHudFunc() {
    MARKFUNCTION(0x8003DC44);

    if (!scriptPtr) {
        return;
    }

    const DirectorHudCmd op = static_cast<DirectorHudCmd>(*scriptPtr);
    scriptPtr += 1;

    switch (op) {
    case DirectorHudCmd::HideHud:
    case DirectorHudCmd::ShowHud:
        break;

    case DirectorHudCmd::DisplayTally:
    case DirectorHudCmd::ShowBossHealth:
        scriptPtr += 1;
        break;

    default:
        break;
    }
}

// PSX: ProcessHumanoidFunc__8Director (DIRECTOR.CPP:3894, 0x8003DD10)
void Director::ProcessHumanoidFunc() {
    MARKFUNCTION(0x8003DD10);

    if (!scriptPtr) {
        return;
    }

    const u32 thingRef = static_cast<u32>(*scriptPtr);
    scriptPtr += 1;

    Humanoid* humanoid = FindHumanoidByScriptRef(thingRef);

    for (s32 i = 0; i < 128; i++) {
        if (!scriptPtr) {
            return;
        }

        const DirectorHumanoidCmd op = static_cast<DirectorHumanoidCmd>(*scriptPtr);
        if (op == DirectorHumanoidCmd::End) {
            scriptPtr += 1;
            return;
        }

        scriptPtr += 1;

        switch (op) {
        case DirectorHumanoidCmd::EnterNis:
            if (humanoid) {
                humanoid->flags2 |= 0x30u;
                humanoid->velocity = { 0, 0, 0 };
            }
            break;

        case DirectorHumanoidCmd::EnterNisMove:
            if (humanoid) {
                humanoid->flags2 = (humanoid->flags2 & ~0x70u) | 0x10u;
                humanoid->velocity = { 0, 0, 0 };
            }
            break;

        case DirectorHumanoidCmd::ExitNis:
            if (humanoid) {
                humanoid->flags2 &= ~0x70u;
            }
            break;

        case DirectorHumanoidCmd::FaceAngleDegrees: {
            const s32 deg = *scriptPtr;
            scriptPtr += 1;
            const s32 angle = (deg << 16) / 360;
            if (humanoid) {
                humanoid->orientation.y = angle;
                humanoid->faceAngle = angle;
                humanoid->FaceAngleY(angle, 0);
            }
            break;
        }

        case DirectorHumanoidCmd::StandFacingZero:
            if (humanoid) {
                humanoid->flags2 &= ~0x70u;
                humanoid->orientation.y = 0;
                humanoid->faceAngle = 0;
                humanoid->FaceAngleY(0, 0);
                humanoid->flags |= 8u;
                humanoid->SetActionState(AS_STAND, 0);
            }
            break;

        case DirectorHumanoidCmd::SetPosition: {
            const LVector position = { scriptPtr[0], scriptPtr[1], scriptPtr[2] };
            scriptPtr += 3;
            if (humanoid) {
                humanoid->pos = position;
                humanoid->homePos = position;
            }
            break;
        }

        case DirectorHumanoidCmd::PlayDynamicAnim:
            scriptPtr += 1;
            break;

        case DirectorHumanoidCmd::SetStandState:
            if (humanoid) {
                humanoid->SetActionState(AS_STAND, 0);
            }
            break;

        case DirectorHumanoidCmd::SetPositionByCurrent:
            if (humanoid) {
                LVector position = humanoid->pos;
                position.y -= 400;
                humanoid->pos = position;
                humanoid->homePos = position;
            }
            break;

        default:
            break;
        }
    }

    scriptPtr = nullptr;
}

// PSX: ProcessLadderFunc__8Director (DIRECTOR.CPP:4003, 0x8003E0D4)
void Director::ProcessLadderFunc() {
    MARKFUNCTION(0x8003E0D4);

    if (!scriptPtr) {
        return;
    }

    for (s32 i = 0; i < 128; i++) {
        const DirectorLadderCmd op = static_cast<DirectorLadderCmd>(*scriptPtr);
        if (op == DirectorLadderCmd::End) {
            scriptPtr += 1;
            return;
        }

        scriptPtr += 1;

        switch (op) {
        case DirectorLadderCmd::FaceLadderPoint: {
            const s32 axis = *scriptPtr;
            scriptPtr += 1;

            if (Player::s_player) {
                LVector target = { field180, field184, field188 };
                if (axis == 1 && g_game && g_game->GetWorld() && g_game->GetWorld()->GetCurrentLevelIndex() == 3) {
                    target.x += 1024;
                } else {
                    target.z += 1024;
                }
                Player::s_player->FacePoint(target, 1);
            }
            break;
        }

        case DirectorLadderCmd::TeleportPlayer:
            break;

        case DirectorLadderCmd::CameraLookAtHatch:
            if (Player::s_player) {
                Camera* camera = GetGameCamera();
                if (camera) {
                    LVector target = Player::s_player->homePos;
                    target.y += 1536;
                    target.z += 128;
                    camera->LookAtTarget(&target);
                }
            }
            break;

        case DirectorLadderCmd::CloseHatch:
            break;

        case DirectorLadderCmd::ClearNis:
            break;

        default:
            break;
        }
    }

    scriptPtr = nullptr;
}

// PSX: ProcessDoorFunc__8Director (DIRECTOR.CPP:4069, 0x8003E378)
void Director::ProcessDoorFunc() {
    MARKFUNCTION(0x8003E378);

    if (!scriptPtr) {
        return;
    }

    for (s32 i = 0; i < 128; i++) {
        const DirectorDoorCmd op = static_cast<DirectorDoorCmd>(*scriptPtr);
        if (op == DirectorDoorCmd::End) {
            scriptPtr += 1;
            return;
        }

        scriptPtr += 1;

        switch (op) {
        case DirectorDoorCmd::SetDoor:
            scriptPtr += 1;
            break;

        case DirectorDoorCmd::OpenDoor:
            break;

        case DirectorDoorCmd::SetDoorState:
            break;

        case DirectorDoorCmd::FaceDoorPoint: {
            const u32 thingRef = static_cast<u32>(*scriptPtr);
            scriptPtr += 1;

            Humanoid* humanoid = FindHumanoidByScriptRef(thingRef);
            if (humanoid) {
                const LVector target = { field180, field184, field188 };
                humanoid->FacePoint(target, 1);
            }
            break;
        }

        case DirectorDoorCmd::FaceDoorAngle: {
            const u32 thingRef = static_cast<u32>(*scriptPtr);
            scriptPtr += 1;

            Humanoid* humanoid = FindHumanoidByScriptRef(thingRef);
            if (humanoid) {
                humanoid->FaceAngleY(humanoid->orientation.y, 1);
            }
            break;
        }

        case DirectorDoorCmd::AttachToDoor: {
            const u32 thingRef = static_cast<u32>(*scriptPtr);
            scriptPtr += 1;

            Humanoid* humanoid = FindHumanoidByScriptRef(thingRef);
            if (humanoid) {
                humanoid->SetActionState(AS_STAND, 0);
            }
            break;
        }

        case DirectorDoorCmd::TeleportThroughDoor:
            break;

        default:
            break;
        }
    }

    scriptPtr = nullptr;
}

// PSX: DetermineVictoryIdle__8Director (DIRECTOR.CPP:4170, 0x8003E71C)
void Director::DetermineVictoryIdle() {
    MARKFUNCTION(0x8003E71C);

    s32* victoryScript = victory_poor;

    if (field176 == 4883877) {
        victoryScript = wait_subroutine;
    } else if (field176 == 4917663) {
        victoryScript = victory_disco;
    } else if (field176 == 76059849) {
        victoryScript = victory_grontar;
    } else if (field176 == 302774) {
        victoryScript = victory_chef;
    } else if (field176 == 4863710) {
        victoryScript = victory_clown;
    } else {
        // PSX: rating = GetLevelEndRating__12ScoreManager(theScoreManager)
        s32 rating = 0;
        if (g_scoreManager) {
            rating = g_scoreManager->GetLevelEndRating();
        }
        switch (rating) {
        case 0:
            victoryScript = victory_poor;
            break;
        case 1:
            victoryScript = victory_ok;
            break;
        case 2:
            victoryScript = victory_good;
            break;
        case 3:
            victoryScript = victory_perfect;
            break;
        default:
            victoryScript = victory_poor;
            break;
        }
    }

    PushScriptReturnAddress(this, victoryScript);
}

// PSX: DetermineLevelIntro__8Director (DIRECTOR.CPP:4260, 0x8003E864)
void Director::DetermineLevelIntro() {
    MARKFUNCTION(0x8003E864);

    s32* introScript = start_generic;
    s32 levelID = 0;

    if (g_game && g_game->GetWorld()) {
        levelID = static_cast<s32>(g_game->GetWorld()->GetCurrentLevelIndex());
    }

    if (levelID == 7) {
        field164 = 0;
    }

    // PSX: if (IsValid__14CheckpointInfo(player+636) && checkpoint_killCount > 0)
    //          KillThings__2AIl(0, checkpoint_killCount);
    // TODO: Player doesn't expose CheckpointInfo member yet.

    const bool visitedLevel = (levelID >= 0 && levelID < 31) ? ((field164 & (1 << levelID)) != 0) : false;
    if (visitedLevel) {
        directorTimeOut = 0;

        switch (levelID) {
        case 8:
            introScript = intro_all_dante;
            break;

        case 11:
            introScript = intro_every_chef;
            break;

        case 12:
            introScript = intro_every_grontar;
            break;

        case 13:
            introScript = intro_every_clown;
            break;

        case 14:
            introScript = intro_every_disco;
            break;

        default:
            break;
        }
    } else {
        switch (levelID) {
        case 1:
            if (g_game && g_game->GetWorld() && g_game->GetWorld()->GetCurrentPetalIndex() == 0) {
                introScript = start_frantic;
            } else {
                directorTimeOut = 0;
                introScript = start_generic;
            }
            break;

        case 8:
            directorTimeOut = 0;
            introScript = intro_all_dante;
            break;

        case 11:
            introScript = intro_chef;
            break;

        case 12:
            introScript = intro_grontar;
            break;

        case 13:
            introScript = intro_clown;
            break;

        case 14:
            introScript = intro_disco;
            break;

        default:
            directorTimeOut = 0;
            introScript = start_generic;
            break;
        }
    }

    PushScriptReturnAddress(this, introScript);
}

// PSX: DetermineDeath__8Director (DIRECTOR.CPP:4382, 0x8003EA4C)
void Director::DetermineDeath() {
    MARKFUNCTION(0x8003EA4C);

    s32* deathScript = death_generic;

    if (field172 <= 0) {
        s32 levelID = 0;
        if (g_game && g_game->GetWorld()) {
            levelID = static_cast<s32>(g_game->GetWorld()->GetCurrentLevelIndex());
        }

        if (levelID < 2 || levelID >= 4) {
            deathScript = death_fall_pavement;
        } else {
            deathScript = death_fall_water;
        }
    } else {
        switch (field172) {
        case 1:
            deathScript = death_fall_pavement;
            break;
        case 2:
            deathScript = death_fall_water;
            break;
        case 4:
            deathScript = death_fall_goo;
            break;
        default:
            deathScript = death_generic;
            break;
        }
    }

    PushScriptReturnAddress(this, deathScript);
}

// PSX: WaitAnimationDone__8Director (DIRECTOR.CPP:4443, 0x8003EB14)
void Director::WaitAnimationDone() {
    MARKFUNCTION(0x8003EB14);

    if (!scriptPtr) {
        field68 = 1;
        return;
    }

    // PSX checks the model animation stop flag on the target Thing.
    // Animation internals are still partially reversed, so use model presence
    // as a non-blocking completion proxy for now.
    bool done = false;
    Thing* thing = FindThingByScriptRef(static_cast<u32>(scriptPtr[1]));
    if (thing && thing->model) {
        done = true;
    }

    if (done) {
        scriptPtr += 2;
        field68 = 0;
    } else {
        field68 = 1;
    }
}

// PSX: ProcessDynamicAnimFunc__8Director (DIRECTOR.CPP:4469, 0x8003EB88)
void Director::ProcessDynamicAnimFunc() {
    MARKFUNCTION(0x8003EB88);

    if (!scriptPtr) {
        return;
    }

    field68 = 0;

    s32* cmd = scriptPtr;
    const DirectorOpcode opcode = static_cast<DirectorOpcode>(cmd[0]);

    if (opcode == DirectorOpcode::DynamicAnimWaitLoaded) {
        const u32 thingType = static_cast<u32>(cmd[1]);
        const s32 animEnum = cmd[2];
        scriptPtr = cmd + 3;

        const bool hasAnim =
            (g_characterManager && (g_characterManager->GetAnimation(thingType, animEnum) != nullptr));
        if (hasAnim) {
            field68 = 0;
        } else {
            // PSX retries this opcode until the animation is available.
            scriptPtr = cmd;
            field68 = 1;
        }
        return;
    }

    if (opcode == DirectorOpcode::DynamicAnimLoad) {
        const u32 thingType = static_cast<u32>(cmd[1]);
        const s32 animEnum = cmd[2];
        scriptPtr = cmd + 3;
        if (g_characterManager) {
            g_characterManager->LoadAnimationBatch(thingType, animEnum, nullptr);
        }
        return;
    }

    if (opcode == DirectorOpcode::DynamicAnimWaitCamera) {
        // PSX checks a global camera pointer and retries until it is valid.
        if (g_game && g_game->GetView().GetCamera()) {
            scriptPtr = cmd + 1;
            field68 = 0;
        } else {
            field68 = 1;
        }
        return;
    }

    if (opcode == DirectorOpcode::DynamicAnimUnload) {
        const u32 thingType = static_cast<u32>(cmd[1]);
        const s32 animEnum = cmd[2];
        scriptPtr = cmd + 3;
        if (g_characterManager) {
            g_characterManager->UnloadAnimationBatch(thingType, animEnum);
        }
        return;
    }

    scriptPtr = cmd + 1;
    field68 = 0;
}

// PSX: HandleWideScreen__8Director (DIRECTOR.CPP:4539, 0x8003ECD4)
void Director::HandleWideScreen() {
    MARKFUNCTION(0x8003ECD4);

    s32& barCurrent = wsBarCurrent;
    const s32 barDesired = wsBarTarget;

    if (barCurrent != barDesired) {
        const s32 barStep = wsBarStep;
        if (barStep == 256) {
            barCurrent = barDesired;
        } else if (barCurrent >= barDesired) {
            barCurrent -= barStep;
            if (barCurrent < barDesired) {
                barCurrent = barDesired;
            }
        } else {
            barCurrent += barStep;
            if (barCurrent > barDesired) {
                barCurrent = barDesired;
            }
        }
    }

    if (barCurrent) {
        s32& alphaCurrent = wsAlphaCurrent;
        const s32 alphaDesired = wsAlphaTarget;
        if (alphaCurrent != alphaDesired) {
            const s32 alphaStep = wsAlphaStep;
            if (alphaCurrent < alphaDesired) {
                alphaCurrent += alphaStep;
                if (alphaCurrent > alphaDesired) {
                    alphaCurrent = alphaDesired;
                }
            } else {
                alphaCurrent -= alphaStep;
                if (alphaCurrent < alphaDesired) {
                    alphaCurrent = alphaDesired;
                }
            }
        }
    }
}

// PSX: DrawWideScreenPolys__8Director (DIRECTOR.CPP:4582, 0x8003ED90)
void Director::DrawWideScreenPolys() {
    MARKFUNCTION(0x8003ED90);

    // This draw path depends on PSX primitive packet APIs (SetPolyF4/AddPrim)
    // that are not exposed through the current PC renderer yet.
    if (wsBarCurrent == 0) {
        return;
    }
}

// PSX: PurgeAnims__8Director (DIRECTOR.CPP:4662, 0x8003F0E4)
void Director::PurgeAnims() {
    MARKFUNCTION(0x8003F0E4);
    cleanUpTexAnim();
}

// PSX: DoesLevelHaveExtraMem__8Directorl (DIRECTOR.CPP:4669, 0x8003F104)
bool Director::DoesLevelHaveExtraMem(s32 level) {
    MARKFUNCTION(0x8003F104);
    return level >= 6 && (level < 9 || (level >= 11 && level < 15));
}

// PSX: updateVramAnims__8Director (DIRECTOR.CPP:2597, 0x8003BB90)
void Director::updateVramAnims() {
    MARKFUNCTION(0x8003BB90);

    // Flipbook interfaces are still pending.
}

// PSX: cleanUpTexAnim__8Director (DIRECTOR.CPP:2611, 0x8003BBE0)
void Director::cleanUpTexAnim() {
    MARKFUNCTION(0x8003BBE0);

    // Texture animation inventory wiring is still pending.
}

// PSX: runDirector (DIRECTOR.CPP:2570, 0x8003BB0C) - handler callback
void runDirector(Handler* h) {
    MARKFUNCTION(0x8003BB0C);
    if (g_director) {
        g_director->Process();
    }
}

// PSX: DrawDirectorOverlays (DIRECTOR.CPP:2578, 0x8003BB34) - handler callback
void DrawDirectorOverlays(Handler* h) {
    MARKFUNCTION(0x8003BB34);
    if (g_director) {
        g_director->HandleWideScreen();
        g_director->DrawWideScreenPolys();
    }
}
