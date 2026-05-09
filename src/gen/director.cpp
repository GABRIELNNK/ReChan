#include "common.h"
#include "gen/director.h"
#include "gen/ai.h"
#include "gen/effects.h"
#include "gen/model.h"
#include "ai/behaviour.h"
#include "ai/obstacle.h"
#include "ai/humanoid.h"
#include "ai/player.h"
#include "gen/charmgr.h"
#include "gen/game.h"
#include "gen/display.h"
#include "gen/camera.h"
#include "gen/animstruct.h"
#include "gen/scoremgr.h"
#include "gen/time.h"
#include "gen/world.h"
#include "fe/hud.h"
#include "snd/drctrsnd.h"
#include "snd/snddrct.h"
#include "snd/sound.h"
#include "p3d/context.h"
#include "p3d/inventory.h"
#include "p3d/p3dmath.h"
#include "p3d/ramtexanim.h"
#include "snd/rsevent.h"
#include "pc/tim.h"
#include "pc/inputaction.h"
#include "ai/door.h"
#include "ai/ladder.h"
#include "gen/director_scripts_800d6f14.inc"

    // PSX globals used by DIRECTOR.CPP script control flow.
    // - g_directorActive (defined in GAME.CPP): gp+20 symbol `directorTimeOut` (0x800DC960)
    //   used by gsPlayState to run Director-only frames.
    // - directorElapsedFrames: gp+0x42C scratch value written each Process tick.
    // - returnAddress: gp+0x420.. used by Call/Return.
    // - codeSnipThing: gp+869 (stored in global, NOT in Director object)
    static s32 directorElapsedFrames = 0;
    static s32 returnAddressIndex = 0;
    static s32 directorDialogCounter = -1;
    static s32 directorDialogLimit = 180;
    static s32* returnAddress[8] = {};
    static Thing* g_codeSnipThing = nullptr; // PSX: gp[869]
    // PSX: gp+3476 = gp[869] — same as g_codeSnipThing! ProcessDoorFunc/LadderFunc read
    // the door/ladder from this global. SetCodeSnip stores the thing here via gp[869].
    static Thing*& g_selectedDoorOrLadder = g_codeSnipThing;
    static s32 g_dialogHandle = 0;          // PSX: gp[3496/4=874]

    void runDirector(Handler* h);
    void DrawDirectorOverlays(Handler* h);

    static s32 start_generic[26] = {
        9, 6, -1, 98, 102, 40, 103, 256, 100, 0,
        99, 255, 5, 14, 84386293, 1, 6, 45, 73, 75,
        98, 99, 0, 5, 8, 4
    };

    static s32 start_frantic[48] = {
        9, 6, -1, 73, 74, 98, 102, 40, 103, 256,
        100, 0, 99, 255, 5, 14, 84386293, 73, 84, 84386293,
        85, 5, 15, 0, 346, 16, 0, 346, 1, 21,
        84386293, 346, 17, 84386293, 14, 84386293, 1, 73, 75, 98,
        99, 0, 5, 8, 22, 0, 346, 4
    };

    static s32 gotopoint[7] = { 9, 64, 19, 84386293, 20, 8, 2 };
    static s32 NISladder1[20] = {
        9, 43, 45, 65, 66, 1, 5, 6, -1, 6, 30, 43, 46, 65, 67, 5, 84, 84386293, 88, 0
    };

    static s32 victory_poor[28] = {
        15, 0, 347, 16, 0, 347, 115, 0, 65, 3,
        -2146647188, 3, -2146598624, 117, 21, 84386293, 347, 3, -2146598496, 106,
        -2146603324, 118, 73, 76, 1, 3, -2146598436, 4
    };
    static s32 victory_ok[28] = {
        15, 0, 348, 16, 0, 348, 115, 0, 62, 3,
        -2146647188, 3, -2146598624, 117, 21, 84386293, 348, 3, -2146598496, 106,
        -2146603312, 118, 73, 76, 1, 3, -2146598436, 4
    };
    static s32 victory_good[28] = {
        15, 0, 349, 16, 0, 349, 115, 0, 61, 3,
        -2146647188, 3, -2146598624, 117, 21, 84386293, 349, 3, -2146598496, 106,
        -2146603284, 118, 73, 76, 1, 3, -2146598436, 4
    };
    static s32 victory_perfect[28] = {
        15, 0, 350, 16, 0, 350, 115, 0, 60, 3,
        -2146647188, 3, -2146598624, 117, 21, 84386293, 350, 3, -2146598496, 106,
        -2146603272, 118, 73, 76, 1, 3, -2146598436, 4
    };

    static s32 death[35] = {
        9, 43, 45, 73, 74, 6, -1, 6, 30,
        120, 6, 60, 98, 102, 120, 103, 256, 100,
        0, 99, 255, 101, 9, 5, 107, 109, 6,
        90, 98, 101, 10, 5, 25, -1, 2
    };
    static s32 death_vol[35] = {
        9, 43, 45, 6, -1, 121, 120, 127, 6,
        -1, 73, 74, 98, 102, 120, 103, 256, 100,
        0, 99, 255, 101, 10, 5, 107, 109, 6,
        30, 98, 101, 10, 5, 25, -1, 2
    };
    static s32 death_fall_pavement[31] = {
        115, 0, 52, 6, 30, 43, 53, 1, 110, 10, 107, 108, 14, 6, 31, 107,
        108, 13, 6, 32, 107, 108, 12, 107, 108, 22, 117, 118, 6, 60, 4
    };
    static s32 death_fall_water[15] = {
        115, 0, 53, 6, 15, 107, 108, 25, 6, 30, 117, 118, 6, 60, 4
    };
    static s32 death_generic[10] = {
        115, 0, 51, 6, 30, 117, 118, 6, 60, 4
    };
    static s32 death_fall_goo[35] = {
        14, 84386293, 69, 3, -2146597480,
        14, 84386293, 69, 4, 9,
        56, 62, 84386293, 5, 56,
        58, 5, 6, -1, 6,
        15, 56, 59, 5, 6,
        30, 56, 63, 5, 20,
        8, 2, 9, 116, 0
    };

    // PSX: NISdoor1 at 0x800D85E4 (WORLDPTS.CPP)
    // Door enter cutscene: disable input, attach to door, open, wait, teleport, restore
    static s32 NISdoor1[23] = {
        9, 56, 62, 84386293, 5, 56, 58, 5,
        6, -1, 6, 15, 56, 59, 5, 6,
        30, 56, 63, 5, 20, 8, 2
    };

    // PSX: NISdoor1WithDialog at 0x800D8640 (WORLDPTS.CPP)
    // Same as NISdoor1 but loads and plays dialog first
    static s32 NISdoor1WithDialog[28] = {
        9, 116, 0, 109, 99, 56, 62, 84386293,
        5, 119, 56, 58, 5, 6, -1, 6,
        15, 56, 59, 5, 6, 30, 56, 63,
        5, 20, 8, 2
    };

    static s32 levelEnd[7] = { 9, 73, 74, 126, 26, 9, 2 };
    static s32 wait_subroutine[5] = { 6, -1, 6, 60, 4 };

    // PSX: globalScripts at 0x800D8734 (SWITCH.CPP)
    // Used by gfDirectorVol to resolve a script pointer from an integer index.
    // Entries that point to unresolved "slot" scripts are represented with 0.
    static constexpr u32 kGlobalScriptVirtual[] = {
        0x800D86B0u, // levelEnd
        0u,
        0u,
        0x800D83C8u, // death
        0u,
        0u,
        0x800D7170u,
        0x800D7244u,
        0x800D7358u,
        0x800D73F0u,
        0x800D7000u,
        0x800D750Cu,
        0x800D7680u,
        0x800D7784u,
        0x800D7880u,
        0x800D7974u,
        0x800D7A20u,
        0x800D7B3Cu,
        0x800D7C60u,
        0x800D7DA0u,
        0x800D7E48u,
        0x800D7F38u,
        0u,
        0u,
        0u,
        0u,
        0x80020024u, // play_intro_disco
    };

    // PSX script tables present in overlay globals.
    static s32 defaultBeginScript[21] = {
        9, 98, 100, 255, 5, 10, 30, 8, 2, 124, 25, -1, 4, 123, 25, -1, 4, 125, 25, -1, 4
    };

    // Intro/victory scripts extracted from BOL_REL.BIN (boss overlay, Region B base 0x8001A758).
    // PSX uses Call opcode (3) with raw PSX addresses for subroutine jumps.
    // ScriptPtrFromWord resolves these via RegisterScriptRegion.

    static s32 intro_all_dante[4] = {
        73, 77, -2147354060, 4
    };

    static s32 intro_chef[94] = {
        105, 113, 14, 84386293, 73, 14, 302774, 73, 15, 0,
        359, 16, 0, 359, 43, 50, 360, 18, 15, 17,
        359, 16, 17, 359, 1, 115, 0, 10, 117, 114,
        3, -2147350268, 43, 52, 84, 84386293, 89, 90, -1130, 2039,
        27285, 91, 359, 5, 84, 302774, 89, 90, -1130, 2039,
        27285, 91, 359, 5, 6, -1, 106, -2147350844, 118, 37,
        86943588, -1130, 2039, 27285, 17, 84386293, 43, 51, 14, 302774,
        1, 84, 84386293, 88, 270, 5, 3, -2147350192, 22, 0,
        359, 22, 17, 359, 73, 77, -2147354036, 15, 0, 357,
        43, 50, 358, 4
    };

    static s32 intro_every_chef[10] = {
        15, 0, 357, 43, 50, 358, 73, 77, -2147354036, 4
    };

    static s32 intro_clown[110] = {
        105, 113, 14, 84386293, 73, 14, 4863710, 73, 15, 0,
        363, 43, 50, 364, 16, 0, 363, 18, 15, 23,
        363, 16, 23, 363, 15, 13, 363, 16, 13, 363,
        1, 115, 0, 12, 117, 114, 3, -2147350340, 43, 52,
        84, 84386293, 89, 90, 55374, 15, 51534, 91, 363, 5,
        84, 4863710, 89, 90, 55374, 15, 51534, 91, 363, 5,
        84, 344117, 89, 90, 55374, 15, 51534, 91, 363, 5,
        106, -2147350728, 3, -2147350228, 17, 84386293, 43, 51, 14, 4863710,
        1, 14, 344117, 1, 84, 84386293, 88, 315, 5, 3,
        -2147350192, 22, 0, 363, 22, 13, 363, 22, 23, 363,
        73, 77, -2147353612, 15, 0, 351, 43, 50, 352, 4
    };

    static s32 intro_every_clown[10] = {
        15, 0, 351, 43, 50, 352, 73, 77, -2147353612, 4
    };

    static s32 intro_grontar[104] = {
        105, 113, 14, 84386293, 73, 14, 76059849, 73, 15, 0,
        365, 43, 50, 366, 16, 0, 365, 18, 15, 10,
        365, 16, 10, 365, 1, 115, 0, 11, 117, 114,
        3, -2147350268, 43, 52, 78, 84386293, 84, 84386293, 89, 90,
        32102, -25470, 78168, 91, 365, 5, 78, 76059849, 84, 76059849,
        89, 90, 32102, -25470, 78168, 91, 365, 5, 6, -1,
        106, -2147350772, 118, 6, 45, 36, 48921494, 33931, -25378, 78657,
        6, 96, 38, 245009797, 17, 84386293, 43, 51, 14, 76059849,
        1, 84, 84386293, 88, 90, 5, 3, -2147350192, 22, 0,
        365, 22, 10, 365, 73, 77, -2147353124, 15, 0, 355,
        43, 50, 356, 4
    };

    static s32 intro_every_grontar[10] = {
        15, 0, 355, 43, 50, 356, 73, 77, -2147353124, 4
    };

    static s32 intro_disco[28] = {
        3, -2147350248, 15, 0, 361, 43, 50, 362, 16, 0,
        361, 18, 15, 12, 361, 16, 12, 361, 1, 115,
        0, 13, 117, 98, 99, 0, 5, 4
    };

    static s32 play_intro_disco[71] = {
        14, 84386293, 1, 6, -1, 6, 2, 14, 84386293, 73,
        3, -2147350268, 43, 52, 84, 84386293, 89, 90, 27299, 13825,
        3591, 91, 361, 5, 84, 4917663, 89, 90, 27299, 13825,
        3591, 91, 361, 5, 6, -1, 106, -2147350640, 118, 17,
        84386293, 43, 51, 14, 4917663, 1, 84, 84386293, 88, 0,
        5, 3, -2147350204, 3, -2147350192, 22, 0, 361, 22, 12,
        361, 73, 77, -2147352548, 15, 0, 353, 43, 50, 354,
        4
    };

    static s32 intro_every_disco[10] = {
        15, 0, 353, 43, 50, 354, 73, 77, -2147352548, 4
    };

    static s32 victory_chef[80] = {
        6, -1, 15, 17, 357, 16, 17, 357, 6, 60,
        105, 42, 84386293, 16, 0, 357, 18, 115, 0, 59,
        117, 6, 90, 43, 52, 14, 84386293, 1, 84, 84386293,
        89, 90, 2670, 2048, 28201, 91, 357, 5, 14, 302774,
        1, 84, 302774, 89, 90, 2670, 2048, 28201, 91, 357,
        5, 3, -2147350248, 106, -2147350608, 118, 37, 244945277, 2670, 2048,
        28201, 6, 159, 33, 302774, 6, 160, 36, 48921494, 2130,
        2239, 28663, 17, 84386293, 3, -2147350120, 22, 0, 357, 4
    };

    static s32 victory_disco[108] = {
        6, -1, 15, 12, 353, 16, 12, 353, 6, 60,
        105, 16, 0, 353, 18, 115, 0, 64, 117, 39,
        6, 90, 3, -2147350248, 14, 84386293, 1, 84, 84386293, 89,
        90, 28060, 19150, 13000, 5, 43, 55, 28060, 22150, 6800,
        28060, 22150, 13000, 37, 48403593, 28000, 18600, 13000, 37, 237806245,
        28000, 18500, 13000, 37, 232925534, 28000, 18600, 23000, 6, 450,
        43, 46, 43, 52, 14, 84386293, 1, 84, 84386293, 89,
        90, 34184, 13816, 3608, 91, 353, 5, 14, 4917663, 1,
        84, 4917663, 89, 90, 34184, 13816, 3608, 91, 353, 5,
        3, -2147350248, 106, -2147350372, 118, 37, 48884857, 34184, 13816, 3608,
        17, 84386293, 3, -2147350120, 22, 0, 353, 4
    };

    static s32 victory_grontar[76] = {
        6, -1, 15, 10, 355, 16, 10, 355, 6, 60,
        105, 16, 0, 355, 18, 115, 0, 58, 117, 6,
        90, 43, 52, 14, 84386293, 1, 84, 84386293, 89, 90,
        37118, -25467, 80136, 91, 355, 5, 14, 76059849, 1, 84,
        76059849, 89, 90, 37118, -25467, 80136, 91, 355, 5, 6,
        -1, 106, -2147350568, 3, -2147350228, 6, 228, 36, 48921494, 34829,
        -25472, 80555, 17, 84386293, 3, -2147350120, 22, 0, 355, 14,
        76059849, 1, 22, 10, 355, 4
    };

    static s32 victory_clown[79] = {
        14, 344117, 73, 6, -1, 15, 13, 351, 16, 13,
        351, 6, 60, 105, 16, 0, 351, 18, 115, 0,
        63, 117, 81, 344117, 6, 90, 43, 52, 14, 84386293,
        1, 84, 84386293, 89, 90, 55374, 15, 51534, 91, 351,
        5, 14, 4863710, 1, 84, 4863710, 89, 90, 55374, 15,
        51534, 91, 351, 5, 6, -1, 106, -2147350488, 3, -2147350228,
        6, 130, 32, 4863710, 30, 17, 84386293, 3, -2147350120, 22,
        0, 351, 14, 4863710, 1, 22, 13, 351, 4
    };

    // Sound scripts for boss NIS sequences (packed event data)
    static s32 sndintrochef[18] = {
        268447745, 268447752, 268447762, 268447770, 268542070, 268509340, 268542131,
        268447916, 268542131, 268447924, 268542133, 268689606, 268447935, 268689604,
        268480714, 268447954, 268447963, 0
    };

    static s32 sndintrogrontar[11] = {
        268447751, 268447759, 268972062, 268447786, 268787755, 268447818, 268447833,
        268542049, 268447854, 268509299, 0
    };

    static s32 sndintroclown[22] = {
        268541973, 268476441, 268541981, 268480546, 268447783, 268447788, 268542009,
        268447824, 268435570, 268435575, 268435587, 268435619, 268435636, 268542143,
        268566724, 268542156, 268484817, 268484823, 268447965, 268570863, 268448002, 0
    };

    static s32 sndintrodisco[8] = {
        268447745, 268447753, 268447764, 268447771, 268447891, 268447999, 268448199, 0
    };

    static s32 sndvicchef[10] = {
        268447780, 268513322, 268447801, 268542038, 268542112, 268927137, 268447954,
        268484838, 269070566, 0
    };

    static s32 sndvicgrontar[20] = {
        268447787, 269058109, 268447810, 268447812, 268447832, 268447836, 269058146,
        268447851, 269058193, 268542124, 268484835, 268787943, 268542188, 268484844,
        268447983, 268542254, 268448058, 268448066, 268448074, 0
    };

    static s32 sndvicclown[29] = {
        268447749, 268447758, 268447762, 268447769, 268447770, 268447793, 268447801,
        268447809, 268447812, 268447826, 268447843, 268447846, 268447855, 268447860,
        268447862, 268447877, 268447894, 268484761, 268447911, 268447927, 268484791,
        268484794, 268447946, 268447977, 268447985, 269054206, 268448010, 268448011, 0
    };

    static s32 sndvicdisco[8] = {
        268447933, 268447941, 268447951, 268497114, 268497124, 268542186, 268509435, 0
    };

    // PSX: fade script at 0x800CC36C.
    // Called by victory_poor/ok/good/perfect before tally/dialog branches.
    static s32 fade[8] = {
        98, 101, 10, 100, 0, 5, 105, 4
    };

    // Boss NIS helper scripts (subroutines called by intro/victory scripts)
    static s32 boss_setup[18] = {
        9, 20, 73, 74, 42, 84386293, 14, 84386293, 1, 14,
        84386293, 73, 78, 84386293, 41, 6, -1, 4
    };

    static s32 boss_genericStartNIS[5] = {
        3, -2147350340, 3, -2147350060, 4
    };

    static s32 boss_victory_widescreen[5] = {
        3, -2147350060, 6, -1, 4
    };

    static s32 boss_dialog_and_delayed_widescreen[6] = {
        118, 6, 1, 3, -2147350060, 4
    };

    static s32 boss_checkpoint[3] = {
        28, 180140276, 4
    };

    static s32 boss_intro_end[18] = {
        84, 84386293, 87, 93, 5, 14, 84386293, 1, 43, 44,
        98, 99, 0, 5, 73, 75, 8, 4
    };

    static s32 boss_victory_done[15] = {
        14, 84386293, 1, 105, 98, 100, 255, 99, 255, 5,
        6, -1, 6, 1, 4
    };

    static s32 boss_widescreen_only[13] = {
        98, 102, 40, 103, 256, 100, 255, 99, 255, 104,
        2, 5, 4
    };

    struct DirectorScriptRegion {
        u32 virtualBase = 0;
        s32* hostBase = nullptr;
        s32 wordCount = 0;
    };

    static constexpr s32 kMaxDirectorScriptRegions = 128;
    static DirectorScriptRegion g_directorScriptRegions[kMaxDirectorScriptRegions] = {};
    static s32 g_directorScriptRegionCount = 0;

    template <size_t N>
    constexpr s32 ScriptArrayWordCount(const s32(&)[N]) {
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

        auto registerCompiledOverlayScript = [](u32 virtualBase, s32* hostBase, s32 wordCount) {
            RegisterScriptRegion(virtualBase, hostBase, wordCount);
            RegisterScriptRegion(ToVirtualAddress(hostBase), hostBase, wordCount);
        };

        // The 0x8001FA3C..0x800209D4 overlay block is not registered here yet.
        // GAME_REL.psx.lst shows labels such as intro_disco, play_intro_disco, and
        // boss_genericStartNIS in RAM byte/code regions, so resolving those addresses
        // as director word tables produces bogus script pointers.

        // Register only the overlay scripts and sound tables that we have explicitly
        // reconstructed into host arrays, using their original PSX virtual addresses.
        registerCompiledOverlayScript(0x8001FA3Cu, intro_all_dante, ScriptArrayWordCount(intro_all_dante));
        registerCompiledOverlayScript(0x8001FA54u, intro_chef, ScriptArrayWordCount(intro_chef));
        registerCompiledOverlayScript(0x8001FBCCu, intro_every_chef, ScriptArrayWordCount(intro_every_chef));
        registerCompiledOverlayScript(0x8001FBFCu, intro_clown, ScriptArrayWordCount(intro_clown));
        registerCompiledOverlayScript(0x8001FDB4u, intro_every_clown, ScriptArrayWordCount(intro_every_clown));
        registerCompiledOverlayScript(0x8001FDE4u, intro_grontar, ScriptArrayWordCount(intro_grontar));
        registerCompiledOverlayScript(0x8001FF84u, intro_every_grontar, ScriptArrayWordCount(intro_every_grontar));
        registerCompiledOverlayScript(0x8001FFACu, intro_disco, ScriptArrayWordCount(intro_disco));
        registerCompiledOverlayScript(0x80020024u, play_intro_disco, ScriptArrayWordCount(play_intro_disco));
        registerCompiledOverlayScript(0x80020140u, intro_every_disco, ScriptArrayWordCount(intro_every_disco));
        registerCompiledOverlayScript(0x80020168u, victory_chef, ScriptArrayWordCount(victory_chef));
        registerCompiledOverlayScript(0x800202A8u, victory_disco, ScriptArrayWordCount(victory_disco));
        registerCompiledOverlayScript(0x80020458u, victory_grontar, ScriptArrayWordCount(victory_grontar));
        registerCompiledOverlayScript(0x80020588u, victory_clown, ScriptArrayWordCount(victory_clown));
        registerCompiledOverlayScript(0x800206C4u, sndintrochef, ScriptArrayWordCount(sndintrochef));
        registerCompiledOverlayScript(0x8002070Cu, sndintrogrontar, ScriptArrayWordCount(sndintrogrontar));
        registerCompiledOverlayScript(0x80020738u, sndintroclown, ScriptArrayWordCount(sndintroclown));
        registerCompiledOverlayScript(0x80020790u, sndintrodisco, ScriptArrayWordCount(sndintrodisco));
        registerCompiledOverlayScript(0x800207B0u, sndvicchef, ScriptArrayWordCount(sndvicchef));
        registerCompiledOverlayScript(0x800207D8u, sndvicgrontar, ScriptArrayWordCount(sndvicgrontar));
        registerCompiledOverlayScript(0x80020828u, sndvicclown, ScriptArrayWordCount(sndvicclown));
        registerCompiledOverlayScript(0x8002089Cu, sndvicdisco, ScriptArrayWordCount(sndvicdisco));
        registerCompiledOverlayScript(0x800208BCu, boss_setup, ScriptArrayWordCount(boss_setup));
        registerCompiledOverlayScript(0x80020904u, boss_genericStartNIS, ScriptArrayWordCount(boss_genericStartNIS));
        registerCompiledOverlayScript(0x80020918u, boss_victory_widescreen, ScriptArrayWordCount(boss_victory_widescreen));
        registerCompiledOverlayScript(0x8002092Cu, boss_dialog_and_delayed_widescreen, ScriptArrayWordCount(boss_dialog_and_delayed_widescreen));
        registerCompiledOverlayScript(0x80020944u, boss_checkpoint, ScriptArrayWordCount(boss_checkpoint));
        registerCompiledOverlayScript(0x80020950u, boss_intro_end, ScriptArrayWordCount(boss_intro_end));
        registerCompiledOverlayScript(0x80020998u, boss_victory_done, ScriptArrayWordCount(boss_victory_done));
        registerCompiledOverlayScript(0x800209D4u, boss_widescreen_only, ScriptArrayWordCount(boss_widescreen_only));

        RegisterScriptRegion(0x800CC36Cu, fade, ScriptArrayWordCount(fade));
        RegisterScriptRegion(ToVirtualAddress(fade), fade, ScriptArrayWordCount(fade));

        RegisterScriptRegion(0x800D8208u, victory_poor, ScriptArrayWordCount(victory_poor));
        RegisterScriptRegion(0x800D8278u, victory_ok, ScriptArrayWordCount(victory_ok));
        RegisterScriptRegion(0x800D82E8u, victory_good, ScriptArrayWordCount(victory_good));
        RegisterScriptRegion(0x800D8358u, victory_perfect, ScriptArrayWordCount(victory_perfect));

        RegisterScriptRegion(0x800D6F14u, g_directorScripts_800D6F14, ScriptArrayWordCount(g_directorScripts_800D6F14));
        RegisterScriptRegion(ToVirtualAddress(g_directorScripts_800D6F14), g_directorScripts_800D6F14, ScriptArrayWordCount(g_directorScripts_800D6F14));

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
        RegisterScriptRegion(0x800D85E4u, NISdoor1, ScriptArrayWordCount(NISdoor1));
        RegisterScriptRegion(0x800D8640u, NISdoor1WithDialog, ScriptArrayWordCount(NISdoor1WithDialog));
        RegisterScriptRegion(0x800D86B0u, levelEnd, ScriptArrayWordCount(levelEnd));
        RegisterScriptRegion(0x800D86CCu, wait_subroutine, ScriptArrayWordCount(wait_subroutine));
        RegisterScriptRegion(0x800D86E0u, defaultBeginScript, ScriptArrayWordCount(defaultBeginScript));

        // Host aliases for any scripts compiled into this binary.
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
        RegisterScriptRegion(ToVirtualAddress(NISdoor1), NISdoor1, ScriptArrayWordCount(NISdoor1));
        RegisterScriptRegion(ToVirtualAddress(NISdoor1WithDialog), NISdoor1WithDialog, ScriptArrayWordCount(NISdoor1WithDialog));
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

    static void LogDirectorCommand(const char* streamName, const s32* commandPtr, const char* commandName, s32 commandValue) {
        const u32 scriptAddress = commandPtr ? ToVirtualAddress(commandPtr) : 0;
        LOG("[Director][%s] cmd=%s value=%d (0x%X) script=0x%08X",
            streamName ? streamName : "Main",
            commandName ? commandName : "Unknown",
            commandValue,
            static_cast<u32>(commandValue),
            scriptAddress);
    }

    static void LogDirectorUnknownCommand(const char* streamName, const s32* commandPtr, s32 commandValue) {
        LogDirectorCommand(streamName, commandPtr, "Unknown", commandValue);
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

    static Thing* FindThingInListByScriptRef(ccList& list, u32 ref) {
        for (ccMinNode* node = list.head; node; node = node->next) {
            Thing* candidate = static_cast<Thing*>(node);
            if (!candidate) {
                continue;
            }

            if (candidate->nameCRC == ref) {
                return candidate;
            }

            // Director script refs are typically CRCs. Only treat the ref as a
            // 16-bit unique ID when the script value is explicitly in UID range.
            if (ref <= 0xFFFFu && candidate->uniqueID == (u16)ref) {
                return candidate;
            }
        }

        return nullptr;
    }

    static Thing* FindThingByScriptRef(u32 ref) {
        if (!g_ai) {
            return nullptr;
        }

        Thing* thing = g_ai->FindThing(ref);
        if (thing) {
            return thing;
        }

        ccList* lists[] = {
            &g_ai->humanoidList,
            &g_ai->moveList,
            &g_ai->pickupList,
            &g_ai->inactivePickupList,
            &g_ai->thingList,
        };

        for (ccList* list : lists) {
            if (Thing* candidate = FindThingInListByScriptRef(*list, ref)) {
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

    static Humanoid* FindHumanoidInHumanoidListByScriptRef(u32 ref) {
        if (!g_ai) {
            return nullptr;
        }

        Thing* thing = FindThingInListByScriptRef(g_ai->humanoidList, ref);
        if (!thing) {
            return nullptr;
        }

        return static_cast<Humanoid*>(thing);
    }

    static Camera* GetGameCamera() {
        if (!g_display) {
            return nullptr;
        }

        return g_display->GetCamera();
    }

    static void DisablePlayerInputProcessing() {
        if (Player::s_player && Player::s_player->behaviour) {
            Player::s_player->behaviour->DisableInputProcessing();
        }
    }

    static s32 GetCurrentWorldLevelID() {
        if (!g_game || !g_game->GetWorld()) {
            return 0;
        }

        return g_game->GetWorld()->GetCurLevelID();
    }

    // PSX: sets Thing::flags bit 0x200 on all humanoids during script processing.
    static void SetDirectorFlagsOnHumanoids() {
        if (!g_ai) return;
        for (ccMinNode* n = g_ai->humanoidList.head; n; n = n->next) {
            Thing* thing = static_cast<Thing*>(n);
            thing->flags |= TF_DIRECTOR_ACTIVE;
        }
    }

    // PSX: clears Thing::flags bit 0x200 on all humanoids when script ends.
    static void ClearDirectorFlagsOnHumanoids() {
        if (!g_ai) return;
        for (ccMinNode* n = g_ai->humanoidList.head; n; n = n->next) {
            Thing* thing = static_cast<Thing*>(n);
            thing->flags &= ~TF_DIRECTOR_ACTIVE;
        }
    }

Director* g_director = nullptr;

// PSX: __8Director (DIRECTOR.CPP:2658, 0x800CA1B8)
Director::Director() {
    MARKFUNCTION(0x800CA1B8);
    scriptPtr = nullptr;
    scriptBase = nullptr;
    codeSnipPtr = nullptr;
    wsBarCurrent = 0;
    wsBarTarget = 0;
    wsBarStep = 0;
    wsMode = 2;
    wsAlphaStep = 10;
    wsAlphaCurrent = 0;
    wsAlphaTarget = 0;
    field68 = 0;
    enableInput = 0;
}

// PSX: _._8Director (DIRECTOR.CPP:2681, 0x8003BE10)
Director::~Director() {
    MARKFUNCTION(0x8003BE10);
    if (g_director == this) {
        g_director = nullptr;
    }
}

// PSX: InternalOpen__8Director (DIRECTOR.CPP:2704, 0x800CA2AC)
void Director::InternalOpen() {
    MARKFUNCTION(0x800CA2AC);
    if (g_game) {
        runDirectorHandler = g_game->GetHandlerSet1().AddHandler(runDirector, 20);
        overlayHandler = g_game->GetHandlerSet2().AddHandler(DrawDirectorOverlays, -31);
    }

    Manager::InternalOpen();
}

// PSX: InternalClose__8Director (DIRECTOR.CPP:2757, 0x8003C11C)
void Director::InternalClose() {
    MARKFUNCTION(0x8003C11C);
    cleanUpTexAnim();
    handlerSetA.PurgeHandlers();
    handlerSetB.PurgeHandlers();
    Manager::InternalClose();
}

// PSX: InternalReset__8Director (DIRECTOR.CPP:2736, 0x8003C04C)
void Director::InternalReset() {
    MARKFUNCTION(0x8003C04C);
    scriptState = 0;
    timerTarget = 0;
    timerStart = 0;
    scriptPtr = nullptr;
    codeSnipPtr = nullptr;
    scriptBase = nullptr;
    returnAddressIndex = 0;
    wsBarTarget = 0;
    wsBarCurrent = 0;
    wsMode = 2;
    wsAlphaStep = 10;
    wsAlphaCurrent = 0;
    wsAlphaTarget = 0;

    handlerSetB.PurgeHandlers();
    enableInput = 1;
}

// PSX: LevelReset__8Director (DIRECTOR.CPP:2731, 0x8003C044)
void Director::LevelReset() {
    MARKFUNCTION(0x8003C044);
    visitedLevels = 0;
}

// PSX: SetScript__8Director (DIRECTOR.CPP:2765, 0x8003C234)
void Director::SetScript() {
    MARKFUNCTION(0x8003C234);

    // PSX: a1[0x1C] = a1[0x24] = defaultBeginScript; a1[0xA8] = 534;
    //      gp[0x424] = -1; gp[0x420] = 0; gp[0x428] = 180.
    scriptPtr = defaultBeginScript;
    codeSnipPtr = defaultBeginScript;
    scriptState = 534;

    directorDialogCounter = -1;
    returnAddressIndex = 0;
    directorDialogLimit = 180;
}

// PSX: SetCodeSnip__8DirectorPlP5Thing (DIRECTOR.CPP:2782, 0x8003C268)
void Director::SetCodeSnip(s32* snip, Thing* thing) {
    MARKFUNCTION(0x8003C268);

    RegisterKnownDirectorScriptRegions();

    LOG("[Director][SetCodeSnip] script=0x%08X thing=%p", ToVirtualAddress(snip), thing);

    scriptState = 537;

    directorDialogCounter = -1;

    scriptPtr = snip;
    RegisterRuntimeScriptRegion(scriptPtr);
    codeSnipPtr = snip;
    g_codeSnipThing = thing; // PSX: gp[869] = thing

    returnAddressIndex = 0;
    directorDialogLimit = 180;
}

bool Director::TriggerDeathVolume(s32 newDeathType) {
    RegisterKnownDirectorScriptRegions();

    LOG("[Director][TriggerDeathVolume] requestedType=%d currentCodeSnip=0x%08X deathVol=0x%08X",
        newDeathType,
        ToVirtualAddress(codeSnipPtr),
        ToVirtualAddress(death_vol));

    if (codeSnipPtr == death_vol) {
        LOG("[Director][TriggerDeathVolume] ignored (already in death_vol)");
        return false;
    }

    deathType = newDeathType;
    SetCodeSnip(death_vol, nullptr);
    LOG("[Director][TriggerDeathVolume] activated deathType=%d", deathType);
    return true;
}

void Director::TriggerGotoPoint(s32 x, s32 y, s32 z, Thing* thing) {
    RegisterKnownDirectorScriptRegions();

    nisPointX = x;
    nisPointY = y;
    nisPointZ = z;
    SetCodeSnip(gotopoint, thing);
}

// PSX: Process__8Director (DIRECTOR.CPP:2806, 0x8003C298)
void Director::Process() {
    MARKFUNCTION(0x8003C298);

    // PSX iterates the two internal handler chains BEFORE the scriptPtr null check.
    // Each node: lw $v0, 0x18($a0) = funcPtr; lw $s0, 0($a0) = next; jalr $v0 (no args).
    for (ccMinNode* node = handlerSetA.handlerList.head; node; ) {
        Handler* handler = static_cast<Handler*>(node);
        node = node->next;
        if (handler->funcPtr) {
            handler->funcPtr(handler);
        }
    }

    for (ccMinNode* node = handlerSetB.handlerList.head; node; ) {
        Handler* handler = static_cast<Handler*>(node);
        node = node->next;
        if (handler->funcPtr) {
            handler->funcPtr(handler);
        }
    }

    // PSX: if scriptPtr (a1+0x1C) is null, return 1 with no further work.
    if (!scriptPtr) {
        return;
    }

    // PSX: if enableInput==0, disable player input processing
    if (!enableInput) {
        DisablePlayerInputProcessing();
    }

    // PSX: set Thing::flags bit 0x200 on all humanoids
    SetDirectorFlagsOnHumanoids();

    // PSX: compute gp+0x42C scratch elapsed value = frameCounter - timerStart
    const s32 elapsed = GetDirectorFrameCounter() - timerStart;
    directorElapsedFrames = (elapsed < 0) ? 0 : elapsed;

    if (scriptBase && *scriptBase) {
        ProcessSoundScript();
    }

    field68 = 0;

    // PSX: while(1) with field68 check at top
    for (s32 i = 0; i < 512; i++) {
        if (field68) {
            return;
        }

        if (!scriptPtr) {
            return;
        }

        const s32* opcodePtr = scriptPtr;
        const DirectorOpcode opcode = static_cast<DirectorOpcode>(*scriptPtr);
        LogDirectorCommand("Main", opcodePtr, CmdToString(opcode), static_cast<s32>(opcode));
        switch (opcode) {
            case DirectorOpcode::End:
                // PSX: clear director state, clear NIS flags, destroy directorSound
                scriptPtr = nullptr;
                scriptState = 0;
                ClearDirectorFlagsOnHumanoids();
                if (directorSound) {
                    delete directorSound;
                    directorSound = nullptr;
                }
                return;

            case DirectorOpcode::ResetTimeout:
                // PSX: clears gp+20 (`directorTimeOut`) checked by gsPlayState.
                g_directorActive = 0;
                scriptPtr += 1;
                break;

            case DirectorOpcode::EndScript:
                // PSX: if scriptState==534 (from SetScript), mark level as visited
                if (scriptState == 534 && g_game && g_game->GetWorld()) {
                    const s32 levelID = GetCurrentWorldLevelID();
                    if (levelID >= 0 && levelID < 31) {
                        visitedLevels |= (1 << levelID);
                    }
                }
                scriptPtr = nullptr;
                scriptState = 0;
                // PSX: clear NIS flags on all humanoids
                ClearDirectorFlagsOnHumanoids();
                // PSX: destroy directorSound if present
                if (directorSound) {
                    delete directorSound;
                    directorSound = nullptr;
                }
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
                }
                else {
                    scriptPtr = levelEnd;
                }
                break;

            case DirectorOpcode::Yield:
                // PSX opcode 5: consume one word and block script execution for this frame.
                scriptPtr += 1;
                field68 = 1;
                break;

            case DirectorOpcode::Timer:
            {
                // PSX: Timer returns 1=done, 0=blocked
                s32 done = TimerStep();
                if (done) {
                    field68 = 0;
                }
                else {
                    field68 = 1;
                }
                if (field68) {
                    return;
                }
                break;
            }

            case DirectorOpcode::Loop:
                Loop();
                scriptPtr += 1;
                break;

            case DirectorOpcode::EnablePlayerInput:
                enableInput = 1;
                scriptPtr += 1;
                break;

            case DirectorOpcode::DisablePlayerInput:
                enableInput = 0;
                scriptPtr += 1;
                break;

            case DirectorOpcode::DetermineLevelIntro:
                scriptPtr += 1;
                DetermineLevelIntro();
                break;

            case DirectorOpcode::FaceThing:
            {
                const u32 thingRef = static_cast<u32>(scriptPtr[1]);
                scriptPtr += 2;

                Humanoid* humanoid = FindHumanoidInHumanoidListByScriptRef(thingRef);
                if (humanoid) {
                    humanoid->FaceThing(nullptr, 0);
                    humanoid->SetDesiredMoveDirection(humanoid->orientation.y);
                }
                else
                    ASSERT(false); // PSX: logs "Can't find thing to face" if lookup fails
                break;
            }

            case DirectorOpcode::SetHumanoidAction:
            {
                // PSX: reads thingRef, calls vtable+232 (SetActionState)
                const u32 thingRef = static_cast<u32>(scriptPtr[1]);
                scriptPtr += 2;
                const s32 requestedState = *scriptPtr;
                LOG("[Director][Main][SetHumanoidAction] targetRef=%u state=%d (0x%X)",
                    thingRef,
                    requestedState,
                    static_cast<u32>(requestedState));
                Humanoid* humanoid = FindHumanoidInHumanoidListByScriptRef(thingRef);
                if (humanoid) {
                    humanoid->SetActionState(static_cast<u32>(requestedState), 0);
                }
                else
                    ASSERT(false); // PSX: logs "Can't find thing to face" if lookup fails

                scriptPtr += 1;
                break;
            }

            case DirectorOpcode::DynamicAnimLoad:
            case DirectorOpcode::DynamicAnimWaitLoaded:
            case DirectorOpcode::DynamicAnimWaitCamera:
            case DirectorOpcode::DynamicAnimUnload:
                ProcessDynamicAnimFunc();
                break;

            case DirectorOpcode::WaitAnimationDone:
            {
                s32 done = WaitAnimationDoneStep();
                if (done) {
                    field68 = 0;
                }
                else {
                    field68 = 1;
                }
                if (field68) {
                    return;
                }
                break;
            }

            case DirectorOpcode::WaitForNisControl:
            {
                const u32 thingRef = static_cast<u32>(scriptPtr[1]);
                Humanoid* humanoid = FindHumanoidInHumanoidListByScriptRef(thingRef);
                if (humanoid && humanoid->behaviour &&
                    humanoid->behaviour->handlerDispatch == -1 &&
                    humanoid->behaviour->handler == Behaviour::NisControl) {
                    // Still walking to destination - block script
                    return;
                }
                scriptPtr += 2;
                break;
            }

            case DirectorOpcode::RestorePlayerControl:
                // PSX: restores player to PlayerUserControl behaviour
                // Sets behaviour flags back to normal input processing
                if (Player::s_player) {
                    Player::s_player->FaceThingDesired(nullptr);
                    if (Player::s_player->behaviour) {
                        Player::s_player->behaviour->handlerThisOffset = 0;
                        Player::s_player->behaviour->handlerDispatch = -1;
                        Player::s_player->behaviour->handler = Behaviour::PlayerUserControl;
                    }
                }
                scriptPtr += 1;
                break;

            case DirectorOpcode::PlayThingDynamicAnim:
            {
                const u32 thingRef = static_cast<u32>(scriptPtr[1]);
                const s32 animEnumVal = scriptPtr[2];
                LOG("[Director][Main][PlayThingDynamicAnim] targetRef=%u anim=%d", thingRef, animEnumVal);
                scriptPtr += 3;
                Humanoid* humanoid = FindHumanoidInHumanoidListByScriptRef(thingRef);
                if (humanoid && humanoid->model) {
                    SModel* sm = static_cast<SModel*>(humanoid->model);
                    sm->PlayDynamicAnim(animEnumVal);
                }
                break;
            }

            case DirectorOpcode::SetupFaceTextureAnim:
            {
                if (p3d::inventory) {
                    auto* texAnim = p3d::inventory->Find<tRAMTexAnim>("TChanFace~G");
                    texAnimA = reinterpret_cast<uintptr_t>(texAnim);
                    if (texAnim) {
                        auto* flipbook = texAnim->MakePuppet();
                        flipbook->SetCycleCallback(p3dFwdCycle);
                        flipbookA = reinterpret_cast<uintptr_t>(flipbook);
                    } else {
                        flipbookA = 0;
                    }

                    texAnim = p3d::inventory->Find<tRAMTexAnim>("CChanFace~G");
                    texAnimB = reinterpret_cast<uintptr_t>(texAnim);
                    if (texAnim) {
                        auto* flipbook = texAnim->MakePuppet();
                        flipbook->SetCycleCallback(p3dFwdCycle);
                        flipbookB = reinterpret_cast<uintptr_t>(flipbook);
                    } else {
                        flipbookB = 0;
                    }
                }
                scriptPtr += 1;
                break;
            }

            case DirectorOpcode::CleanupFaceTextureAnim:
                scriptPtr += 1;
                cleanUpTexAnim();
                break;

            case DirectorOpcode::QueueDetermineNextState:
            {
                // PSX: reads optional level ID param, sets g_selectedLevel if != -1
                const s32 levelParam = scriptPtr[1];
                scriptPtr += 2;
                if (levelParam != -1) {
                    extern s16 g_selectedLevel;
                    g_selectedLevel = static_cast<s16>(levelParam);
                }
                if (g_game) {
                    g_game->SetState(GameState::DetermineNextGameState);
                }
                break;
            }

            case DirectorOpcode::SetGameState:
            {
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
            {
                // PSX: reads UID, calls setJackieCheckpoint(uid).
                // If checkpoint set fails, calls OnCheckpoint as fallback.
                scriptPtr += 2;
                if (Player::s_player) {
                    Player::s_player->OnCheckpoint();
                }
                break;
            }

            case DirectorOpcode::SetCheckpointData:
                // PSX: dword_800D6F90 = scriptPtr[1] (checkpoint data value)
                scriptPtr += 2;
                break;

            case DirectorOpcode::TriggerCheckpoint:
                scriptPtr += 1;
                if (Player::s_player) {
                    Player::s_player->OnCheckpoint();
                }
                break;

            case DirectorOpcode::ClearCheckpointValid:
                // PSX: SetValidState__14CheckpointInfo(636, 0)
                scriptPtr += 1;
                if (Player::s_player) {
                    Player::s_player->checkpoint.SetValidState(0);
                }
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

            case DirectorOpcode::DestroyDestructible:
            {
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
            {
                // PSX: if scriptPtr[1] nonzero, set flag 4 on player; else clear it
                const s32 val = scriptPtr[1];
                scriptPtr += 2;
                if (Player::s_player) {
                    if (val)
                        Player::s_player->flags |= 0x4u;
                    else
                        Player::s_player->flags &= ~0x4u;
                }
                break;
            }

            case DirectorOpcode::ClearGlobalEffectRef:
                scriptPtr += 1;
                break;

            case DirectorOpcode::DropPickup:
            {
                const u32 thingRef = static_cast<u32>(scriptPtr[1]);
                scriptPtr += 2;
                Humanoid* humanoid = FindHumanoidByScriptRef(thingRef);
                if (humanoid) {
                    humanoid->DropPickup(1, 1);
                }
                break;
            }

            case DirectorOpcode::CameraFunc:
                scriptPtr += 1;
                ProcessCameraFunc();
                break;

            case DirectorOpcode::DoorFunc:
                scriptPtr += 1;
                ProcessDoorFunc();
                break;

            case DirectorOpcode::FacePointAndNisControl:
                // PSX: face player toward NIS point, write Behaviour::destPoint, set NisControl behaviour
                scriptPtr += 1;
                if (Player::s_player) {
                    const LVector target = { nisPointX, nisPointY, nisPointZ };
                    Player::s_player->FacePointDesired(target);
                    Behaviour* beh = Player::s_player->behaviour;
                    if (beh) {
                        beh->destPoint = target;
                        beh->handlerThisOffset = 0;
                        beh->handlerDispatch = -1;
                        beh->handler = Behaviour::NisControl;
                    }
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

            case DirectorOpcode::SetThingFlag08:
            {
                const u32 thingRef = static_cast<u32>(scriptPtr[1]);
                scriptPtr += 2;
                Thing* thing = FindThingByScriptRef(thingRef);
                if (thing) {
                    thing->flags |= 0x8u;
                }
                break;
            }

            case DirectorOpcode::SetThingFlag28:
            {
                const u32 thingRef = static_cast<u32>(scriptPtr[1]);
                scriptPtr += 2;
                Thing* thing = FindThingByScriptRef(thingRef);
                if (thing) {
                    thing->flags |= 0x28u;
                }
                break;
            }

            case DirectorOpcode::ClearThingFlagsAndKill:
            {
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
            case DirectorOpcode::KillThingType88:
            {
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
                // PSX: calls cameraManager vtable func (InternalReset)
                scriptPtr += 1;
                break;

            case DirectorOpcode::SetDesiredWideScreen:
                SetDesiredWideScreen();
                break;

            case DirectorOpcode::ResetWideScreenDefaults:
                // PSX: reset to defaults and hide HUD
                wsBarTarget = 120;
                wsBarStep = 256;
                wsAlphaTarget = 255;
                wsAlphaStep = 10;
                wsMode = 2;
                scriptPtr += 1;
                // PSX: SetHUDVisible__3HUDii(0, 0, 0)
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
                g_dialogHandle = rsEvent(RS_LOAD_DIALOG, scriptPtr[1], scriptPtr[2], 0x40000100);
                directorDialogCounter = 0;
                scriptPtr += 3;
                break;

            case DirectorOpcode::LoadDialogB:
                g_dialogHandle = rsEvent(RS_LOAD_DIALOG, scriptPtr[1], scriptPtr[2], scriptPtr[3]);
                directorDialogCounter = 0;
                scriptPtr += 4;
                break;

            case DirectorOpcode::WaitDialogPlayable:
                directorDialogCounter++;
                if (g_dialogHandle && jcsValidateHandle(g_dialogHandle)
                    && directorDialogCounter < directorDialogLimit) {
                    if (!jcsIsPlayable(g_dialogHandle)) {
                        field68 = 1;
                        break;
                    }
                }
                field68 = 0;
                scriptPtr += 1;
                break;

            case DirectorOpcode::PlayDialogNear:
                if (g_dialogHandle && jcsValidateHandle(g_dialogHandle)) {
                    s32 posArg = 0;
                    if (Player::s_player) {
                        posArg = (s32)(intptr_t)&Player::s_player->pos;
                    }
                    rsEvent(RS_PLAY_DIALOG, g_dialogHandle, posArg, 64);
                }
                scriptPtr += 1;
                break;

            case DirectorOpcode::PlayDialogFar:
                if (g_dialogHandle && jcsValidateHandle(g_dialogHandle)) {
                    s32 posArg = 0;
                    if (Player::s_player) {
                        posArg = (s32)(intptr_t)&Player::s_player->pos;
                    }
                    rsEvent(RS_PLAY_DIALOG, g_dialogHandle, posArg, 100);
                }
                scriptPtr += 1;
                break;

            case DirectorOpcode::PlayPriorityDialog:
                if (Player::s_player) {
                    Player::s_player->PlayDialogBasedOnPriority(255, 1073742080);
                }
                scriptPtr += 1;
                break;

            case DirectorOpcode::SetDialogTimeout:
                directorDialogLimit = scriptPtr[1];
                scriptPtr += 2;
                break;

            case DirectorOpcode::SetNisPoint:
            {
                // PSX: reads point CRC, looks up WorldPoints, stores position
                const s32 pointIdx = scriptPtr[1];
                scriptPtr += 2;
                WorldPointNode* wpn = WorldPoints_GetNISPoint(static_cast<u32>(pointIdx));
                if (wpn) {
                    nisPointX = wpn->pos.x;
                    nisPointY = wpn->pos.y;
                    nisPointZ = wpn->pos.z;
                }
                break;
            }

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
                // PSX: unknown opcodes are silently skipped (continue)
                // PSX doesn't terminate on unknown opcodes.
                scriptPtr += 1;
                LogDirectorUnknownCommand("Main", opcodePtr, static_cast<s32>(opcode));
                ASSERT(false && "Unknown Director main opcode");
                break;
        }
    }

    field68 = 1;
}

// PSX: ProcessSoundScript__8Director (DIRECTOR.CPP:3576, 0x8003D5A4)
void Director::ProcessSoundScript() {
    MARKFUNCTION(0x8003D5A4);

    const s32 elapsed = GetDirectorFrameCounter() - timerStart;

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
// Returns 1 when timer done/reset, 0 when blocked (still waiting).
s32 Director::TimerStep() {
    MARKFUNCTION(0x8003D634);

    if (!scriptPtr) {
        return 0;
    }

    const s32 duration = scriptPtr[1];
    if (duration == -1) {
        // Reset timer - records current frame as start
        timerTarget = 0;
        timerStart = GetDirectorFrameCounter();
        scriptPtr += 2;
        return 1;
    }

    if (timerTarget) {
        // Timer already running - check if expired
        if (static_cast<s32>(GetDirectorFrameCounter()) >= timerTarget) {
            scriptPtr += 2;
            timerTarget = 0;
            return 1;
        }
        return 0;
    }

    // First frame - set target
    timerTarget = timerStart + duration;
    return 0;
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

        const s32* tokenPtr = scriptPtr;
        const DirectorWideScreenCmd token = static_cast<DirectorWideScreenCmd>(*scriptPtr);
        LogDirectorCommand("WideScreen", tokenPtr, DirectorWideScreenCmdToString(token), static_cast<s32>(token));
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

            case DirectorWideScreenCmd::SetMode:
            {
                s16* p = reinterpret_cast<s16*>(scriptPtr);
                wsMode = static_cast<u16>(*p);
                p += 2;
                scriptPtr = reinterpret_cast<s32*>(p);
                break;
            }

            default:
                LogDirectorUnknownCommand("WideScreen", tokenPtr, static_cast<s32>(token));
                ASSERT(false && "Unknown Director widescreen opcode");
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

    const s32* opcodePtr = scriptPtr;
    const DirectorEdisonCmd opcode = static_cast<DirectorEdisonCmd>(*scriptPtr);
    scriptPtr += 1;
    LogDirectorCommand("Edison", opcodePtr, DirectorEdisonCmdToString(opcode), static_cast<s32>(opcode));

    if (opcode == DirectorEdisonCmd::PlayTransient) {
        // PSX: reads u16 soundID, advances scriptPtr, calls CSoundDirect::PlayTransient(soundID, 0, 0, 0)
        s16* soundData = reinterpret_cast<s16*>(scriptPtr);
        const u16 soundID = static_cast<u16>(*soundData);
        soundData += 2;
        scriptPtr = reinterpret_cast<s32*>(soundData);

        // PSX: PlayTransient__12CSoundDirectUsPC10tagLVectorUsUl(soundID, 0, 0, 0)
        CSoundDirect::PlayTransient(soundID, nullptr, 0, 0);
    }
    else if (opcode == DirectorEdisonCmd::StopMusic) {
        LOG("[Director] Edison StopMusic opcode");
        rsEvent(RS_STOP_MUSIC, 0, 0, 0);
    }
    else {
        LogDirectorUnknownCommand("Edison", opcodePtr, static_cast<s32>(opcode));
        ASSERT(false && "Unknown Director Edison opcode");
    }
}

// PSX: ProcessModelFunc__8Director (DIRECTOR.CPP:3711, 0x8003D87C)
void Director::ProcessModelFunc() {
    MARKFUNCTION(0x8003D87C);

    if (!scriptPtr) {
        return;
    }

    LogDirectorCommand("Model", scriptPtr, "RawModelToken", *scriptPtr);
}

// PSX: ProcessCameraFunc__8Director (DIRECTOR.CPP:3716, 0x8003D884)
void Director::ProcessCameraFunc() {
    MARKFUNCTION(0x8003D884);

    if (!scriptPtr) {
        return;
    }

    const s32* opPtr = scriptPtr;
    const DirectorCameraCmd op = static_cast<DirectorCameraCmd>(*scriptPtr);
    scriptPtr += 1;
    LogDirectorCommand("Camera", opPtr, DirectorCameraCmdToString(op), static_cast<s32>(op));

    Camera* camera = GetGameCamera();

    switch (op) {
        case DirectorCameraCmd::EnableNisCamera:
            if (camera) {
                // PSX: lookAtMode = 1; then dispatch Camera::Think() via vtable slot.
                camera->SetLookAtMode(1);
                camera->Think();
            }
            break;

        case DirectorCameraCmd::ClearCameraFlag:
            if (camera) {
                camera->SetCollisionEnabled(0);
            }
            break;

        case DirectorCameraCmd::SetCameraFlag:
            if (camera) {
                camera->SetCollisionEnabled(1);
            }
            break;

        case DirectorCameraCmd::CopyP3DFov:
            if (camera) {
                // PSX: GetFOV from global G_2ptcam, then SetFOV(rmDiv16i(fovA, 87162)).
                s32 fovA, fovB;
                G_2ptcam.GetFOV(&fovA, &fovB);
                camera->SetFOV(rmDiv16i(fovA, 87162));
            }
            break;

        case DirectorCameraCmd::ResetCameraFov:
            if (camera) {
                // PSX: SetCurFOV(theCamera->curFOV)
                camera->SetCurFOV(camera->GetCurFOV());
            }
            break;

        case DirectorCameraCmd::SetCameraMode:
        {
            const s32 mode = *scriptPtr;
            scriptPtr += 1;
            if (camera && mode >= 0 && mode <= 2) {
                camera->SetMode(static_cast<CameraMode>(mode));
            }
            break;
        }

        case DirectorCameraCmd::LoadAsyncAnim:
            if (camera) {
                camera->LoadAsyncAnim(*scriptPtr);
            }
            scriptPtr += 1;
            break;

        case DirectorCameraCmd::DeleteAsyncAnim:
            if (camera) {
                camera->DeleteAsyncAnim();
            }
            break;

        case DirectorCameraCmd::PlayAsyncAnim:
            if (camera) {
                camera->PlayAsyncAnim();
            }
            break;

        case DirectorCameraCmd::ShakeCamera:
        {
            const s32 selector = *scriptPtr;
            scriptPtr += 1;

            s32 shakeX = 80;
            s32 shakeY = 80;
            s32 shakeZ = 80;

            if (selector == 0) {
                shakeX = *scriptPtr;
                scriptPtr += 1;
            }
            else if (selector == 1) {
                shakeY = *scriptPtr;
                scriptPtr += 1;
            }
            else {
                // PSX treats any non-0/1 selector as Z strength.
                shakeZ = *scriptPtr;
                scriptPtr += 1;
            }

            const s32 frames = *scriptPtr;
            scriptPtr += 1;

            if (camera) {
                camera->SetShakeStrength(shakeX, shakeY, shakeZ);
                camera->ShakeCamera(frames);
            }
            break;
        }

        case DirectorCameraCmd::LookAtNisPoint:
        {
            if (camera) {
                camera->SetCollisionEnabled(0);
            }

            const s32 pointID = *scriptPtr;
            scriptPtr += 1;

            WorldPointNode* nisPoint = WorldPoints_GetNISPoint(static_cast<u32>(pointID));

            if (camera && nisPoint) {
                LVector target = nisPoint->pos;

                World* world = g_game ? g_game->GetWorld() : nullptr;
                const bool isChinaLevel2 = (world && world->GetCurLevelID() == 3 && world->GetCurrentPetalIndex() == 2);
                if (isChinaLevel2) {
                    target.z -= 812;
                }
                else {
                    target.x -= 812;
                    target.y -= 108;
                }

                camera->LookAtTarget(&target);
            }

            break;
        }

        case DirectorCameraCmd::SetCameraAndLookAt:
        {
            if (camera) {
                camera->SetCollisionEnabled(0);
            }

            const LVector position = { scriptPtr[0], scriptPtr[1], scriptPtr[2] };
            const LVector target = { scriptPtr[3], scriptPtr[4], scriptPtr[5] };
            scriptPtr += 6;

            if (camera) {
                camera->SetPositionAndPrev(position.x, position.y, position.z);
                camera->LookAtTarget(&target);
            }
            break;
        }

        default:
            // PSX default path just returns for unknown camera tokens.
            return;
    }
}

// PSX: ProcessHudFunc__8Director (DIRECTOR.CPP:3845, 0x8003DC44)
void Director::ProcessHudFunc() {
    MARKFUNCTION(0x8003DC44);

    if (!scriptPtr) {
        return;
    }

    const s32* opPtr = scriptPtr;
    const DirectorHudCmd op = static_cast<DirectorHudCmd>(*scriptPtr);
    scriptPtr += 1;
    LogDirectorCommand("Hud", opPtr, DirectorHudCmdToString(op), static_cast<s32>(op));

    switch (op) {
        case DirectorHudCmd::HideHud:
            if (g_hud) {
                g_hud->SetHUDVisible(0, 1);
            }
            break;

        case DirectorHudCmd::ShowHud:
            if (g_hud) {
                g_hud->SetHUDVisible(1, 1);
            }
            break;

        case DirectorHudCmd::DisplayTally:
        {
            const s32 tallyType = *scriptPtr;
            scriptPtr += 1;
            if (g_hud) {
                g_hud->DisplayTally(tallyType);
            }
            break;
        }

        case DirectorHudCmd::ShowBossHealth:
            scriptPtr += 1;
            break;

        default:
            LogDirectorUnknownCommand("Hud", opPtr, static_cast<s32>(op));
            ASSERT(false && "Unknown Director HUD opcode");
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
    LOG("[Director][Humanoid] targetRef=%u (0x%X)", thingRef, thingRef);

    Humanoid* humanoid = FindHumanoidInHumanoidListByScriptRef(thingRef);
    if (!humanoid) {
        LOG("[Director][Humanoid] targetRef=%u not found", thingRef);
    }

    for (s32 i = 0; i < 128; i++) {
        if (!scriptPtr) {
            return;
        }

        const s32* opPtr = scriptPtr;
        const DirectorHumanoidCmd op = static_cast<DirectorHumanoidCmd>(*scriptPtr);
        LogDirectorCommand("Humanoid", opPtr, DirectorHumanoidCmdToString(op), static_cast<s32>(op));
        if (op == DirectorHumanoidCmd::End) {
            scriptPtr += 1;
            return;
        }

        scriptPtr += 1;

        switch (op) {
            case DirectorHumanoidCmd::EnterNis:
                if (humanoid) {
                    const u32 flags2 = static_cast<u32>(humanoid->flags2);
                    if (((flags2 >> 4) & 1u) == 0) {
                        humanoid->flags2 = static_cast<s32>(flags2 | 0x30u);
                        humanoid->field516 = 0;
                        humanoid->field520 = 0;
                        humanoid->field524 = 0;
                    }
                }
                break;

            case DirectorHumanoidCmd::EnterNisMove:
                if (humanoid) {
                    const u32 flags2 = static_cast<u32>(humanoid->flags2);
                    if (((flags2 >> 4) & 1u) == 0 || ((((flags2 >> 5) & 1u) != 0) && (((flags2 >> 6) & 1u) != 0))) {
                        humanoid->flags2 = static_cast<s32>((flags2 | 0x10u) & ~0x60u);
                        humanoid->field516 = 0;
                        humanoid->field520 = 0;
                        humanoid->field524 = 0;
                    }
                }
                break;

            case DirectorHumanoidCmd::ExitNis:
                if (humanoid) {
                    humanoid->flags2 &= ~0x70u;
                }
                break;

            case DirectorHumanoidCmd::FaceAngleDegrees:
            {
                const s32 deg = *scriptPtr;
                scriptPtr += 1;
                const s32 angle = (deg << 16) / 360;
                if (humanoid) {
                    humanoid->orientation.y = angle;
                    humanoid->SetDesiredMoveDirection(angle);
                    humanoid->FaceAngleY(angle, 0);
                }
                break;
            }

            case DirectorHumanoidCmd::StandFacingZero:
                if (humanoid) {
                    LOG("[Director][Humanoid][StandFacingZero] ref=%u before pos=(%d,%d,%d) home=(%d,%d,%d) state=%d flags2=0x%X",
                        thingRef,
                        humanoid->pos.x,
                        humanoid->pos.y,
                        humanoid->pos.z,
                        humanoid->homePos.x,
                        humanoid->homePos.y,
                        humanoid->homePos.z,
                        humanoid->actionState,
                        static_cast<u32>(humanoid->flags2));
                    humanoid->flags2 &= ~0x70u;
                    humanoid->SetActionState(AS_NIS_MODE, 0);
                    humanoid->flags &= ~0x800u;
                    humanoid->orientation.y = 0;
                    humanoid->SetDesiredMoveDirection(0);
                    humanoid->FaceAngleY(0, 0);

                    const u32 flags2 = static_cast<u32>(humanoid->flags2);
                    if (((flags2 >> 4) & 1u) == 0 || ((((flags2 >> 5) & 1u) != 0) && (((flags2 >> 6) & 1u) != 0))) {
                        humanoid->flags2 = static_cast<s32>((flags2 | 0x10u) & ~0x60u);
                        humanoid->field516 = 0;
                        humanoid->field520 = 0;
                        humanoid->field524 = 0;
                    }

                    humanoid->flags |= 8u;

                    LOG("[Director][Humanoid][StandFacingZero] ref=%u after pos=(%d,%d,%d) home=(%d,%d,%d) state=%d flags2=0x%X",
                        thingRef,
                        humanoid->pos.x,
                        humanoid->pos.y,
                        humanoid->pos.z,
                        humanoid->homePos.x,
                        humanoid->homePos.y,
                        humanoid->homePos.z,
                        humanoid->actionState,
                        static_cast<u32>(humanoid->flags2));
                }
                break;

            case DirectorHumanoidCmd::SetPosition:
            {
                const LVector position = { scriptPtr[0], scriptPtr[1], scriptPtr[2] };
                LOG("[Director][Humanoid][SetPosition] ref=%u target=(%d,%d,%d)",
                    thingRef,
                    position.x,
                    position.y,
                    position.z);
                scriptPtr += 3;
                if (humanoid) {
                    humanoid->pos = position;
                    humanoid->homePos = position;
                    LOG("[Director][Humanoid][SetPosition] ref=%u after pos=(%d,%d,%d) home=(%d,%d,%d)",
                        thingRef,
                        humanoid->pos.x,
                        humanoid->pos.y,
                        humanoid->pos.z,
                        humanoid->homePos.x,
                        humanoid->homePos.y,
                        humanoid->homePos.z);
                }
                break;
            }

            case DirectorHumanoidCmd::PlayDynamicAnim:
            {
                const s32 animEnumVal = *scriptPtr;
                LOG("[Director][Humanoid][PlayDynamicAnim] ref=%u anim=%d state=%d flags2=0x%X",
                    thingRef,
                    animEnumVal,
                    humanoid ? humanoid->actionState : -1,
                    humanoid ? static_cast<u32>(humanoid->flags2) : 0u);
                scriptPtr += 1;
                if (humanoid && humanoid->model) {
                    SModel* sm = static_cast<SModel*>(humanoid->model);
                    sm->PlayDynamicAnim(animEnumVal);
                }
                break;
            }

            case DirectorHumanoidCmd::SetStandState:
                if (humanoid) {
                    humanoid->SetActionState(AS_NIS_MODE, 0);
                    humanoid->flags &= ~0x800u;
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
                LogDirectorUnknownCommand("Humanoid", opPtr, static_cast<s32>(op));
                ASSERT(false && "Unknown Director humanoid opcode");
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
        const s32* opPtr = scriptPtr;
        const DirectorLadderCmd op = static_cast<DirectorLadderCmd>(*scriptPtr);
        LogDirectorCommand("Ladder", opPtr, DirectorLadderCmdToString(op), static_cast<s32>(op));
        if (op == DirectorLadderCmd::End) {
            scriptPtr += 1;
            return;
        }

        scriptPtr += 1;

        switch (op) {
            case DirectorLadderCmd::FaceLadderPoint:
            {
                const s32 axis = *scriptPtr;
                scriptPtr += 1;

                if (Player::s_player) {
                    LVector target = { nisPointX, nisPointY, nisPointZ };
                    if (axis == 1 && g_game && g_game->GetWorld() && g_game->GetWorld()->GetCurrentLevelIndex() == 3) {
                        target.x += 1024;
                    }
                    else {
                        target.z += 1024;
                    }
                    Player::s_player->FacePointDesired(target);
                    if (Player::s_player->behaviour) {
                        Player::s_player->behaviour->destPoint = target;
                        Player::s_player->behaviour->handlerThisOffset = 0;
                        Player::s_player->behaviour->handlerDispatch = -1;
                        Player::s_player->behaviour->handler = Behaviour::NisControl;
                    }
                }
                break;
            }

            case DirectorLadderCmd::TeleportPlayer:
            {
                Ladder* ladder = dynamic_cast<Ladder*>(g_selectedDoorOrLadder);
                if (ladder) {
                    ladder->TeleportPlayer();
                }
                break;
            }

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
            {
                Ladder* ladder = dynamic_cast<Ladder*>(g_selectedDoorOrLadder);
                if (ladder) {
                    ladder->CloseHatch();
                }
                break;
            }

            case DirectorLadderCmd::ClearNis:
                if (Player::s_player) {
                    Player::s_player->FaceThingDesired(nullptr);
                    if (Player::s_player->behaviour) {
                        Player::s_player->behaviour->handlerThisOffset = 0;
                        Player::s_player->behaviour->handlerDispatch = 0;
                        Player::s_player->behaviour->handler = nullptr;
                    }
                }
                break;

            default:
                LogDirectorUnknownCommand("Ladder", opPtr, static_cast<s32>(op));
                ASSERT(false && "Unknown Director ladder opcode");
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
        const s32* opPtr = scriptPtr;
        const DirectorDoorCmd op = static_cast<DirectorDoorCmd>(*scriptPtr);
        LogDirectorCommand("Door", opPtr, DirectorDoorCmdToString(op), static_cast<s32>(op));
        if (op == DirectorDoorCmd::End) {
            scriptPtr += 1;
            return;
        }

        scriptPtr += 1;

        switch (op) {
            case DirectorDoorCmd::SetDoor:
            {
                const u32 thingRef = static_cast<u32>(*scriptPtr);
                scriptPtr += 1;

                Thing* thing = nullptr;
                if (g_ai) {
                    ccNode* n = g_ai->moveList.FindNodeCRC(thingRef);
                    if (n) {
                        thing = static_cast<Thing*>(n);
                    }
                }

                if (thing) {
                    g_selectedDoorOrLadder = thing;
                }
                break;
            }

            case DirectorDoorCmd::OpenDoor:
            {
                Door* door = dynamic_cast<Door*>(g_selectedDoorOrLadder);
                if (door) {
                    door->Open();
                }
                break;
            }

            case DirectorDoorCmd::SetDoorState:
            {
                Door* door = dynamic_cast<Door*>(g_selectedDoorOrLadder);
                if (door) {
                    door->doorState = 4;
                }
                break;
            }

            case DirectorDoorCmd::FaceDoorPoint:
            {
                const u32 thingRef = static_cast<u32>(*scriptPtr);
                scriptPtr += 1;

                Humanoid* humanoid = FindHumanoidByScriptRef(thingRef);
                Door* door = dynamic_cast<Door*>(g_selectedDoorOrLadder);
                if (humanoid && door) {
                    LVector localCenter = {};
                    localCenter.x = ((s32)door->collBox.minX + (s32)door->collBox.maxX) / 2;
                    localCenter.y = ((s32)door->collBox.minY + (s32)door->collBox.maxY) / 2;
                    localCenter.z = ((s32)door->collBox.minZ + (s32)door->collBox.maxZ) / 2;

                    s32 sinY = rmSin16(door->orientation.y);
                    s32 cosY = rmSin16(door->orientation.y + 0x4000);

                    LVector worldCenter = {};
                    worldCenter.x = door->pos.x + (s32)(((s64)cosY * localCenter.x) >> 16) + (s32)(((s64)sinY * localCenter.z) >> 16);
                    worldCenter.y = door->pos.y + localCenter.y;
                    worldCenter.z = door->pos.z + (s32)((-(s64)sinY * localCenter.x) >> 16) + (s32)(((s64)cosY * localCenter.z) >> 16);
                    humanoid->FacePointDesired(worldCenter);
                }
                break;
            }

            case DirectorDoorCmd::FaceDoorAngle:
            {
                const u32 thingRef = static_cast<u32>(*scriptPtr);
                scriptPtr += 1;

                Humanoid* humanoid = FindHumanoidByScriptRef(thingRef);
                Door* door = dynamic_cast<Door*>(g_selectedDoorOrLadder);
                if (humanoid && door) {
                    humanoid->FaceAngleY(door->orientation.y, 1);
                }
                break;
            }

            case DirectorDoorCmd::AttachToDoor:
            {
                const u32 thingRef = static_cast<u32>(*scriptPtr);
                scriptPtr += 1;

                Humanoid* humanoid = FindHumanoidByScriptRef(thingRef);
                Door* door = dynamic_cast<Door*>(g_selectedDoorOrLadder);
                if (humanoid && door && humanoid->behaviour) {
                    LVector localCenter = {};
                    localCenter.x = ((s32)door->collBox.minX + (s32)door->collBox.maxX) / 2;
                    localCenter.y = ((s32)door->collBox.minY + (s32)door->collBox.maxY) / 2;
                    localCenter.z = ((s32)door->collBox.minZ + (s32)door->collBox.maxZ) / 2;

                    s32 sinY = rmSin16(door->orientation.y);
                    s32 cosY = rmSin16(door->orientation.y + 0x4000);

                    LVector attachPos = {};
                    attachPos.x = door->pos.x + (s32)(((s64)cosY * localCenter.x) >> 16) + (s32)(((s64)sinY * localCenter.z) >> 16);
                    attachPos.y = door->pos.y;
                    attachPos.z = door->pos.z + (s32)((-(s64)sinY * localCenter.x) >> 16) + (s32)(((s64)cosY * localCenter.z) >> 16);

                    humanoid->behaviour->destPoint = attachPos;
                    humanoid->behaviour->handlerThisOffset = 0;
                    humanoid->behaviour->handlerDispatch = -1;
                    humanoid->behaviour->handler = Behaviour::NisControl;
                }
                break;
            }

            case DirectorDoorCmd::TeleportThroughDoor:
            {
                Door* door = dynamic_cast<Door*>(g_selectedDoorOrLadder);
                if (door) {
                    door->TeleportPlayer();
                    door->Reset();
                }
                break;
            }

            default:
                LogDirectorUnknownCommand("Door", opPtr, static_cast<s32>(op));
                ASSERT(false && "Unknown Director door opcode");
                break;
        }
    }

    scriptPtr = nullptr;
}

// PSX: DetermineVictoryIdle__8Director (DIRECTOR.CPP:4170, 0x8003E71C)
void Director::DetermineVictoryIdle() {
    MARKFUNCTION(0x8003E71C);

    s32* victoryScript = victory_poor;

    if (victoryBossCRC == 4883877) {
        victoryScript = wait_subroutine;
    }
    else if (victoryBossCRC == 4917663) {
        victoryScript = victory_disco;
    }
    else if (victoryBossCRC == 76059849) {
        victoryScript = victory_grontar;
    }
    else if (victoryBossCRC == 302774) {
        victoryScript = victory_chef;
    }
    else if (victoryBossCRC == 4863710) {
        victoryScript = victory_clown;
    }
    else {
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
    s32 levelID = GetCurrentWorldLevelID();

    if (levelID == 7) {
        visitedLevels = 0;
    }

    // PSX: if (IsValid__14CheckpointInfo(player+636) && checkpoint_killCount > 0)
    //          KillThings__2AIl(0, checkpoint_killCount);
    if (Player::s_player && Player::s_player->checkpoint.IsValid()) {
        if (Player::s_player->checkpoint.field28 > 0) {
            g_ai->KillThings(Player::s_player->checkpoint.field28);
        }
    }

    const bool visitedLevel = (levelID >= 0 && levelID < 31) ? ((visitedLevels & (1 << levelID)) != 0) : false;
    if (visitedLevel) {
        g_directorActive = 0;

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
    }
    else {
        switch (levelID) {
            case 1:
                if (g_game && g_game->GetWorld() && g_game->GetWorld()->GetCurrentPetalIndex() == 0) {
                    introScript = start_frantic;
                }
                else {
                    g_directorActive = 0;
                    introScript = start_generic;
                }
                break;

            case 8:
                g_directorActive = 0;
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
                g_directorActive = 0;
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
    const s32 levelID = GetCurrentWorldLevelID();
    const char* deathScriptName = "death_generic";

    if (deathType <= 0) {
        if (levelID < 2 || levelID >= 4) {
            deathScript = death_fall_pavement;
            deathScriptName = "death_fall_pavement";
        }
        else {
            deathScript = death_fall_water;
            deathScriptName = "death_fall_water";
        }
    }
    else {
        switch (deathType) {
            case 1:
                deathScript = death_fall_pavement;
                deathScriptName = "death_fall_pavement";
                break;
            case 2:
                deathScript = death_fall_water;
                deathScriptName = "death_fall_water";
                break;
            case 4:
                deathScript = death_fall_goo;
                deathScriptName = "death_fall_goo";
                break;
            default:
                deathScript = death_generic;
                deathScriptName = "death_generic";
                break;
        }
    }

    LOG("[Director][DetermineDeath] deathType=%d level=%d selected=%s script=0x%08X",
        deathType,
        levelID,
        deathScriptName,
        ToVirtualAddress(deathScript));

    PushScriptReturnAddress(this, deathScript);
}

// PSX: WaitAnimationDone__8Director (DIRECTOR.CPP:4443, 0x8003EB14)
// Returns 1 when animation done, 0 when still waiting.
s32 Director::WaitAnimationDoneStep() {
    MARKFUNCTION(0x8003EB14);

    if (!scriptPtr) {
        return 0;
    }

    // PSX: FindNodeCRC(0x34=humanoidList, scriptPtr[1], 0)
    // then checks thing->model->animStructure->loopCount at +84.
    Humanoid* humanoid = FindHumanoidInHumanoidListByScriptRef(static_cast<u32>(scriptPtr[1]));
    if (humanoid && humanoid->model) {
        Model* model = static_cast<Model*>(humanoid->model);
        AnimStructure* anim = model ? static_cast<AnimStructure*>(model->animStructure) : nullptr;
        if (anim && anim->loopCount != 0) {
            scriptPtr += 2;
            return 1;
        }
        return 0;
    }

    return 0;
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
    LogDirectorCommand("DynamicAnim", cmd, CmdToString(opcode), static_cast<s32>(opcode));

    if (opcode == DirectorOpcode::DynamicAnimWaitLoaded) {
        const u32 thingType = static_cast<u32>(cmd[1]);
        const s32 animEnum = cmd[2];
        scriptPtr = cmd + 3;

        const bool hasAnim =
            (g_characterManager && (g_characterManager->GetAnimation(thingType, animEnum) != nullptr));
        if (hasAnim) {
            field68 = 0;
        }
        else {
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
        // PSX gates this on camera async animation data (camera+0x1D0),
        // not just camera object existence.
        Camera* camera = nullptr;
        if (g_display) {
            camera = g_display->GetCamera();
        }

        if (camera && camera->HasAsyncAnimLoaded()) {
            scriptPtr = cmd + 1;
            field68 = 0;
        }
        else {
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
    LogDirectorUnknownCommand("DynamicAnim", cmd, static_cast<s32>(opcode));
    ASSERT(false && "Unknown Director dynamic animation opcode");
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
        }
        else if (barCurrent >= barDesired) {
            barCurrent -= barStep;
            if (barCurrent < barDesired) {
                barCurrent = barDesired;
            }
        }
        else {
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
            }
            else {
                alphaCurrent -= alphaStep;
                if (alphaCurrent < alphaDesired) {
                    alphaCurrent = alphaDesired;
                }
            }
        }
    }
}

// PSX: DrawWideScreenPolys__8Director (DIRECTOR.CPP:4582, 0x8003ED90)
// Draws two horizontal letterbox bars (top and bottom) and a blend mode triangle.
// PSX uses POLY_F4 primitives in the GPU ordering table at layer 3.
// PC: uses ScreenDraw to render equivalent overlay quads.
void Director::DrawWideScreenPolys() {
    MARKFUNCTION(0x8003ED90);

    if (wsBarCurrent == 0 || wsAlphaCurrent == 0) {
        return;
    }

    const f32 barFrac = static_cast<f32>(wsBarCurrent) / 240.0f;
    f32 drawFrac = barFrac;
    u8 alpha = static_cast<u8>((wsAlphaCurrent < 255) ? wsAlphaCurrent : 255);

#if DIRECTOR_WIDESCREEN_SLIDE_BARS
    const f32 alphaTarget = (wsAlphaTarget > 0) ? static_cast<f32>(wsAlphaTarget) : 255.0f;
    const f32 slideProgress = Clamp(static_cast<f32>(wsAlphaCurrent) / alphaTarget, 0.0f, 1.0f);
    drawFrac *= slideProgress;
    alpha = 255;
#endif

    const f32 barHeight = drawFrac * SCREEN_HEIGHT;
    if (barHeight <= 0.0f) {
        return;
    }

    // Top bar
    ScreenDraw::DrawColoredRect(0.0f, 0.0f, SCREEN_WIDTH, barHeight,
                                0, 0, 0, alpha);
    // Bottom bar
    ScreenDraw::DrawColoredRect(0.0f, SCREEN_HEIGHT - barHeight, SCREEN_WIDTH, barHeight,
                                0, 0, 0, alpha);
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

    bool updated = false;

    if (flipbookA) {
        reinterpret_cast<tFlipbook*>(flipbookA)->AdvanceFrame();
        updated = true;
    }
    if (flipbookB) {
        reinterpret_cast<tFlipbook*>(flipbookB)->AdvanceFrame();
        updated = true;
    }

    if (updated && g_game && g_game->GetWorld()) {
        g_game->GetWorld()->RefreshVRAMTexture();
    }
}

// PSX: cleanUpTexAnim__8Director (DIRECTOR.CPP:2611, 0x8003BBE0)
void Director::cleanUpTexAnim() {
    MARKFUNCTION(0x8003BBE0);

    if (flipbookA) {
        delete reinterpret_cast<tFlipbook*>(flipbookA);
        flipbookA = 0;
    }
    if (texAnimA) {
        auto* texAnim = reinterpret_cast<tRAMTexAnim*>(texAnimA);
        if (p3d::inventory) {
            for (s32 i = 0; i < texAnim->GetNumTextures(); ++i) {
                if (tTexture* texture = texAnim->GetTexture(i)) {
                    p3d::inventory->Remove(texture->GetName());
                }
            }
            p3d::inventory->Remove(texAnim->GetName());
        }
        texAnimA = 0;
    }

    if (flipbookB) {
        delete reinterpret_cast<tFlipbook*>(flipbookB);
        flipbookB = 0;
    }
    if (texAnimB) {
        auto* texAnim = reinterpret_cast<tRAMTexAnim*>(texAnimB);
        if (p3d::inventory) {
            for (s32 i = 0; i < texAnim->GetNumTextures(); ++i) {
                if (tTexture* texture = texAnim->GetTexture(i)) {
                    p3d::inventory->Remove(texture->GetName());
                }
            }
            p3d::inventory->Remove(texAnim->GetName());
        }
        texAnimB = 0;
    }
}

// Script accessor methods - provide access to file-local script arrays
// for obstacle.cpp cutscene triggers (Door/Ladder).
s32* Director::GetNISDoor1Script() {
    return NISdoor1;
}

s32* Director::GetNISDoor1WithDialogScript() {
    return NISdoor1WithDialog;
}

s32* Director::GetNISLadder1Script() {
    return NISladder1;
}

s32* Director::GetDeathScript() {
    return death;
}

s32* Director::GetLevelEndScript() {
    return levelEnd;
}

s32* Director::GetGlobalScriptByIndex(s32 index) {
    RegisterKnownDirectorScriptRegions();

    if (index < 0 || index >= static_cast<s32>(sizeof(kGlobalScriptVirtual) / sizeof(kGlobalScriptVirtual[0]))) {
        return nullptr;
    }

    const u32 virtualAddress = kGlobalScriptVirtual[index];
    if (virtualAddress == 0) {
        return nullptr;
    }

    return ScriptPtrFromWord(static_cast<s32>(virtualAddress));
}

// PSX: runDirector (DIRECTOR.CPP:2570, 0x8003BB0C) - handler callback
void runDirector(Handler* h) {
    MARKFUNCTION(0x8003BB0C);
    if (g_director) {
        g_director->Process();
    }
}

// PSX: DrawDirectorOverlays (DIRECTOR.CPP:2578, 0x8003BB34) - handler callback
// PSX: EnterLayer(view, 3) -> HandleWideScreen -> DrawWideScreenPolys
//      -> DrawEffects(4096) -> ExitLayer(view, 3)
void DrawDirectorOverlays(Handler* h) {
    MARKFUNCTION(0x8003BB34);
    if (g_director) {
        g_director->HandleWideScreen();
        g_director->DrawWideScreenPolys();
        Effects_DrawEffects(4096);
    }
}
