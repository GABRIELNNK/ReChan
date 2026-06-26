#include "gen/common.h"
#include "snd/rsevent.h"
#include "snd/sound.h"
#include "snd/rsdworld.h"
#include "snd/adpcm.h"
#include "gen/time.h"
#include "p3d/p3dmath.h"
#include "xclib/xcfile.h"
#include "gen/blockmgr.h"
#include "ai/player.h"
#ifdef MOD_LOADER
#include "extra/modloader.h"
#endif

static constexpr u32 DIALOG_SECTOR_SIZE = 2048;
static constexpr u32 DIALOG_RATE = 11025;

static std::vector<u8> g_dialogFileData;
static std::vector<u8> g_dialogHeader;
static u32 g_dialogHeaderSize = 0;
static u32 g_dialogDataBaseOffset = 0;
static u16 g_lastDialogChoiceOffset = 0;
static s32 g_forcedDialogVariant = -1;

static void SetDlgStatus(s32 status);

static u32 GetDialogPhonographIndexForActiveBank() {
    // PSX LocInfo[loc][6] values at 0x800D6588:
    // normal levels use 46, Chinatown/Sewer bosses use 48, Rooftop boss uses 47.
    if (!g_sound) {
        return 46;
    }

    const s32 bank = g_sound->activeSfxBank;
    if (bank == 3 || bank == 11) {
        return 48;
    }
    if (bank == 15) {
        return 47;
    }
    return 46;
}

static u32 GetDialogTransferLimitBytes() {
    if (!g_sound || g_sound->activeSfxBank < 0) {
        return 0;
    }

    const u32 bank = (u32)g_sound->activeSfxBank;
    if (bank >= MAX_WAX_BANKS) {
        return 0;
    }

    const u32 phonographIndex = GetDialogPhonographIndexForActiveBank();
    if (phonographIndex >= MAX_SAMPLES_PER_BANK) {
        return 0;
    }

    // PSX rsdAllocPhonograph returns 2 * descriptor[+8] into gp+1224.
    const u32 halfwords = g_sound->banks[bank].descriptorWord2[phonographIndex];
    return halfwords * 2u;
}

static s32 ComputeDialogLoadDelayFrames(u32 sectors) {
    if (sectors == 0) {
        return 1;
    }

    // PSX CD reads are asynchronous and sector-based (75 sectors/sec).
    // At 30 Hz update: delayFrames ~= ceil(sectors * 30 / 75) = ceil(sectors * 2 / 5).
    const u32 frames = (sectors * 2u + 4u) / 5u;
    return (frames == 0u) ? 1 : static_cast<s32>(frames);
}

static u8 DialogHeaderU8(u32 offset) {
    if (offset >= g_dialogHeader.size()) {
        return 0;
    }
    return g_dialogHeader[offset];
}

static u16 DialogHeaderU16(u32 offset) {
    if (offset + 1 >= g_dialogHeader.size()) {
        return 0;
    }
    return (u16)((u16)g_dialogHeader[offset] | ((u16)g_dialogHeader[offset + 1] << 8));
}

static void ResetDialogHeaderState() {
    if (g_dialogHeaderSize == 0 || g_dialogFileData.size() < g_dialogHeaderSize) {
        g_dialogHeader.clear();
        return;
    }

    g_dialogHeader.assign(g_dialogFileData.begin(), g_dialogFileData.begin() + g_dialogHeaderSize);
    g_lastDialogChoiceOffset = 0;
}

static bool EnsureDialogDataLoaded() {
    if (!g_dialogFileData.empty() && !g_dialogHeader.empty()) {
        return true;
    }

    u8* raw = nullptr;
    u32 size = 0;
    if (!xcReadFileLow("SOUND/DIALOG/RSDIALOG.DLG", &raw, &size) || !raw || size < 6) {
        if (raw) {
            delete[] raw;
        }
        return false;
    }

    g_dialogFileData.assign(raw, raw + size);
    delete[] raw;

    g_dialogHeaderSize = (u32)((u16)g_dialogFileData[0] | ((u16)g_dialogFileData[1] << 8));
    if (g_dialogHeaderSize == 0 || g_dialogHeaderSize > g_dialogFileData.size()) {
        g_dialogFileData.clear();
        g_dialogHeader.clear();
        g_dialogHeaderSize = 0;
        return false;
    }

    g_dialogDataBaseOffset = (g_dialogHeaderSize + (DIALOG_SECTOR_SIZE - 1)) & ~(DIALOG_SECTOR_SIZE - 1);
    if (g_dialogDataBaseOffset >= g_dialogFileData.size()) {
        g_dialogFileData.clear();
        g_dialogHeader.clear();
        g_dialogHeaderSize = 0;
        g_dialogDataBaseOffset = 0;
        return false;
    }

    ResetDialogHeaderState();
    return !g_dialogHeader.empty();
}

static bool SelectDialogClipOffset(s32 character, s32 dialogID, u32* outStart, u32* outEnd,
                                   u16* outChoiceOffset, u32* outSectors, u32* outVariant) {
    if (!outStart || !outEnd || !outChoiceOffset || !outSectors || !outVariant) {
        return false;
    }

    if (!EnsureDialogDataLoaded()) {
        return false;
    }

    const u32 charTableOffset = 4;
    const u32 charEntryOffset = charTableOffset + ((u32)(u16)character * 2);
    if (charEntryOffset + 1 >= g_dialogHeaderSize) {
        return false;
    }

    const u16 charInfoOffset = DialogHeaderU16(charEntryOffset);
    if (charInfoOffset >= g_dialogHeaderSize) {
        return false;
    }

    const u8 dialogCount = DialogHeaderU8(charInfoOffset);
    if (dialogCount == 0) {
        SetDlgStatus(8);
        return false;
    }

    const u32 dialogListOffset = (u32)charInfoOffset + 1;
    const u32 streamTableOffset = dialogListOffset + (u32)dialogCount;
    const u32 clipInfoTableOffset = streamTableOffset + ((u32)dialogCount * 2);
    if (clipInfoTableOffset + ((u32)dialogCount * 2) > g_dialogHeaderSize) {
        return false;
    }

    s32 dialogIndex = -1;
    for (u32 i = 0; i < (u32)dialogCount; ++i) {
        if ((s32)DialogHeaderU8(dialogListOffset + i) == dialogID) {
            dialogIndex = (s32)i;
            break;
        }
    }
    if (dialogIndex < 0) {
        SetDlgStatus(8);
        return false;
    }

    const u32 streamSectors = (u32)DialogHeaderU16(streamTableOffset + ((u32)dialogIndex * 2));
    const u32 streamStartBase = g_dialogDataBaseOffset + streamSectors * DIALOG_SECTOR_SIZE;

    const u16 clipInfoOffset = DialogHeaderU16(clipInfoTableOffset + ((u32)dialogIndex * 2));
    if (clipInfoOffset >= g_dialogHeaderSize) {
        return false;
    }

    const u8 clipCount = DialogHeaderU8(clipInfoOffset);
    if (clipCount == 0) {
        SetDlgStatus(8);
        return false;
    }

    const u32 clipSizeTableOffset = (u32)clipInfoOffset + 1;
    if (clipSizeTableOffset + clipCount > g_dialogHeaderSize) {
        return false;
    }

    const u32 transferLimitBytes = GetDialogTransferLimitBytes();

    while (true) {
        u32 selected = 0;
        if (g_forcedDialogVariant >= 0) {
            selected = static_cast<u32>(g_forcedDialogVariant);
            if (selected >= clipCount) return false;
        }
        else {
            u8 available[256] = {};
            u32 availableCount = 0;
            for (u32 i = 0; i < (u32)clipCount; ++i) {
                const u8 flags = DialogHeaderU8(clipSizeTableOffset + i);
                if ((flags & 0xC0) == 0) {
                    available[availableCount++] = (u8)i;
                }
            }

            if (availableCount == 0) {
                for (u32 i = 0; i < (u32)clipCount; ++i) {
                    const u32 off = clipSizeTableOffset + i;
                    const u8 flags = DialogHeaderU8(off);
                    if ((flags & 0x80) != 0 && (flags & 0x40) == 0) {
                        g_dialogHeader[off] = (u8)(flags & 0x3F);
                    }
                }

                for (u32 i = 0; i < (u32)clipCount; ++i) {
                    const u8 flags = DialogHeaderU8(clipSizeTableOffset + i);
                    if ((flags & 0xC0) == 0) {
                        available[availableCount++] = (u8)i;
                    }
                }
            }

            if (availableCount == 0) {
                SetDlgStatus(9);
                return false;
            }

            const u32 selectedSlot = (u32)rmRangedRandom((s32)availableCount);
            selected = (u32)available[selectedSlot];
        }
        const u16 choiceOffset = (u16)(clipSizeTableOffset + selected);

        // PSX SelectDialog rejects immediate repeat selection instead of rerolling.
        if (g_forcedDialogVariant < 0 && choiceOffset == g_lastDialogChoiceOffset) {
            SetDlgStatus(9);
            return false;
        }

        u32 streamStart = streamStartBase;
        for (u32 i = 0; i < selected; ++i) {
            const u32 clipSectors = (u32)(DialogHeaderU8(clipSizeTableOffset + i) & 0x3F);
            streamStart += clipSectors * DIALOG_SECTOR_SIZE;
        }

        const u32 selectedSectors = (u32)(DialogHeaderU8(choiceOffset) & 0x3F);
        const u32 clipSize = selectedSectors * DIALOG_SECTOR_SIZE;
        if (clipSize == 0) {
            return false;
        }

        if (transferLimitBytes < clipSize) {
            SetDlgStatus(10);
            if ((u32)choiceOffset < g_dialogHeader.size()) {
                g_dialogHeader[choiceOffset] = (u8)(DialogHeaderU8(choiceOffset) | 0x40);
            }
            continue;
        }

        const u32 clipEnd = streamStart + clipSize;
        if (streamStart >= g_dialogFileData.size() || clipEnd > g_dialogFileData.size()) {
            return false;
        }

        *outStart = streamStart;
        *outEnd = clipEnd;
        *outChoiceOffset = choiceOffset;
        *outSectors = selectedSectors;
        *outVariant = selected;
        return true;
    }
}

static AudioSample LoadDialogSample(s32 character, s32 dialogID, u16* outChoiceOffset, u32* outSectors) {
    if (!outChoiceOffset || !outSectors) {
        return AUDIO_SAMPLE_INVALID;
    }

    u32 start = 0;
    u32 end = 0;
    u16 choiceOffset = 0;
    u32 selectedSectors = 0;
    u32 variant = 0;
    if (!SelectDialogClipOffset(character, dialogID, &start, &end, &choiceOffset, &selectedSectors, &variant)) {
        return AUDIO_SAMPLE_INVALID;
    }

#ifdef MOD_LOADER
    char overrideName[64];
    std::snprintf(overrideName, sizeof(overrideName), "c%02d_d%03d_v%02u", character, dialogID, variant);
    std::vector<s16> overridePcm;
    u32 overrideRate = 0, overrideChannels = 0;
    if (ModLoader::Instance().GetSoundOverridePCM(
            "dialog", overrideName, overridePcm, overrideRate, overrideChannels)
        && !overridePcm.empty() && overrideRate > 0 && overrideChannels > 0) {
        AudioSample overrideSample = AudioEngine::LoadSample(
            overridePcm.data(), static_cast<u32>(overridePcm.size()), overrideRate, overrideChannels);
        if (overrideSample != AUDIO_SAMPLE_INVALID) {
            *outChoiceOffset = choiceOffset;
            *outSectors = selectedSectors;
            LOG("[Dialog] Override: dialog/%s", overrideName);
            return overrideSample;
        }
    }
#endif

    const u8* clipData = &g_dialogFileData[start];
    const u32 clipSize = end - start;
    std::vector<s16> pcm = SpuAdpcm::Decode(clipData, clipSize, true);
    if (pcm.empty()) {
        return AUDIO_SAMPLE_INVALID;
    }

    AudioSample sample = AudioEngine::LoadSample(pcm.data(), (u32)pcm.size(), DIALOG_RATE, 1);
    if (sample == AUDIO_SAMPLE_INVALID) {
        return AUDIO_SAMPLE_INVALID;
    }

    *outChoiceOffset = choiceOffset;
    *outSectors = selectedSectors;
    return sample;
}

struct DialogEntry {
    s32 handle;
    s32 character;
    s32 dialogId;
    s32 priority;
    AudioSample sample;
    AudioVoice voice;
    bool valid;
};

static constexpr s32 DIALOG_ENTRY_COUNT = 64;
static DialogEntry g_dialogEntries[DIALOG_ENTRY_COUNT] = {};
static s32 g_nextDialogHandle = 1;
// PSX: dword_800D8808 - default dialog handle used by jcsIsPlaying__Fv.
static s32 g_primaryDialogHandle = 0;
// PSX: dword_800D880C - secondary dialog handle used by handle-role checks.
static s32 g_secondaryDialogHandle = 0;
// PSX: g_Dlg[0] and dword_800D8824 - primary/secondary dialog priorities.
static s32 g_primaryDialogPriority = -1;
static s32 g_secondaryDialogPriority = -1;
// PSX: gp+1212 - JCSDLG state machine status used by handle validation/playability.
static s32 g_dialogStatus = 0;
// PSX: gp+1208 - dialog system enabled by jcsStartDialog / disabled by jcsStopDialog.
static s32 g_dialogSystemStarted = 0;
// Host bookkeeping for PSX-like async load state transition (state 1 -> 5).
static s32 g_dialogLoadPending = 0;
static s32 g_dialogLoadRequestFrame = -1;
static s32 g_dialogLoadReadyFrame = -1;
// PSX: g_Dlg queue entries (8 words per queue, primary + secondary).
struct DialogQueueInfo {
    s32 priority = 0;      // +0
    s32 handle = 0;        // +4
    s32 sectors = 0;       // +8
    s32 timeoutFrame = 0;  // +12
    s32 playPosition = 0;  // +16 (tagLVector pointer/value in PSX ABI)
    u16 clipEntry = 0;     // +20
    u16 pad = 0;           // +22
    s32 flushFrame = 0;    // +24
    s32 dialogId = 112;    // +28
};
static_assert(sizeof(DialogQueueInfo) == 32, "DialogQueueInfo must match PSX g_Dlg row size");
static DialogQueueInfo g_dialogQueues[2] = {};
// PSX: gp+1236 - SetDlgStatus output value.
static s32 g_dialogResultStatus = 0;
static bool g_dialogStatusUpdateInProgress = false;

enum DialogStatus : s32 {
    DialogStatus_Idle = 0,
    DialogStatus_LoadRequestedPrimary = 1,
    DialogStatus_LoadDeferredPrimary = 2,
    DialogStatus_KillRequestedPrimary = 3,
    DialogStatus_LoadDeferredSecondary = 4,
    DialogStatus_LoadedPrimary = 5,
    DialogStatus_LoadedPrimaryWithDeferred = 6,
    DialogStatus_LoadedPrimaryWithDeferredFromState2 = 7,
    DialogStatus_LoadedPrimaryWithDeferredFromState3 = 8,
    DialogStatus_LoadedPrimaryAndSecondary = 9,
    DialogStatus_PlayingPrimary = 10,
    DialogStatus_PlayingPrimaryWithSecondary = 11,
    DialogStatus_PlayingPrimaryState12 = 12,
    DialogStatus_PlayingPrimaryState13 = 13,
    DialogStatus_PlayingPrimaryState14 = 14,
};

static s32 GetDialogFrameCounter() {
    return g_time ? static_cast<s32>(g_time->GetFrameCounter()) : 0;
}

s32 jcsValidateHandle(s32 handle);
static DialogEntry* FindDialogEntry(s32 handle);
static void StopDialogEntryVoice(DialogEntry* entry);
static void ReleaseDialogEntry(DialogEntry* entry);
static void ClearDialogPendingLoadState();
static void PromotePendingDialogLoadState();
static void UpdateDialogStatusFromSlots();
static s32 IsPrimaryHandle(s32 handle);
static s32 PlayDialogHandle(DialogEntry* entry);
static s32 PlayLoadedState(s32 playPosition, s32 nextState);
static s32 PlayDialogRequest(s32 handleParam, s32 playPosition, u32 timeout);

// PSX: SetDlgStatus__F9DlgStatus
static void SetDlgStatus(s32 status) {
    g_dialogResultStatus = status;
}

// PSX: ResetDialogInfo__F5Queue5State queue field reset (without state write).
static void ResetDialogQueueInfo(s32 queue) {
    if (queue < 0 || queue >= 2) {
        return;
    }

    g_dialogQueues[queue] = {};
    g_dialogQueues[queue].dialogId = 112;
}

// PSX: UpdateTimeOut__FUlPC10tagLVector5Queue
static s32 UpdateDialogTimeout(u32 timeout, s32 playPosition, s32 queue) {
    if (timeout == 0u) {
        SetDlgStatus(7);
        return 0;
    }

    if (queue < 0 || queue >= 2) {
        return 0;
    }

    DialogQueueInfo& q = g_dialogQueues[queue];
    q.playPosition = playPosition;
    q.timeoutFrame = GetDialogFrameCounter() + static_cast<s32>(timeout);
    return 1;
}

// PSX: PauseTimeOut__F5Queue
static void PauseDialogTimeout(s32 queue) {
    if (queue < 0 || queue >= 2) {
        return;
    }

    DialogQueueInfo& q = g_dialogQueues[queue];
    if (q.timeoutFrame == 0 || !jcsValidateHandle(q.handle)) {
        return;
    }

    const s32 frameCounter = GetDialogFrameCounter();
    if (frameCounter < q.timeoutFrame) {
        q.timeoutFrame = q.timeoutFrame - frameCounter;
    }
    else {
        q.timeoutFrame = 1;
    }
}

// PSX: ResumeTimeOut__F5Queue
static void ResumeDialogTimeout(s32 queue) {
    if (queue < 0 || queue >= 2) {
        return;
    }

    DialogQueueInfo& q = g_dialogQueues[queue];
    if (q.timeoutFrame == 0 || !jcsValidateHandle(q.handle)) {
        return;
    }

    q.timeoutFrame = q.timeoutFrame + GetDialogFrameCounter();
}

static s32 ResolveDialogHandleParam(s32 handle) {
    if (handle != 0) {
        return handle;
    }
    return g_primaryDialogHandle;
}

static void UpdateDialogState(s32 state) {
    g_dialogStatus = state;
}

static void ResetDialogInfoAndState(s32 queue, s32 state) {
    if (queue == 0) {
        DialogEntry* primaryEntry = FindDialogEntry(g_primaryDialogHandle);
        ReleaseDialogEntry(primaryEntry);
        g_primaryDialogHandle = 0;
        g_primaryDialogPriority = -1;
    }
    else if (queue == 1) {
        DialogEntry* secondaryEntry = FindDialogEntry(g_secondaryDialogHandle);
        ReleaseDialogEntry(secondaryEntry);
        g_secondaryDialogHandle = 0;
        g_secondaryDialogPriority = -1;
    }

    ResetDialogQueueInfo(queue);
    UpdateDialogState(state);
    ClearDialogPendingLoadState();
}

static void UpgradeDialogInfoAndState(s32 state) {
    // PSX UpgradeDlgInfo copies queue1 -> queue0 and resets queue1.
    DialogEntry* primaryEntry = FindDialogEntry(g_primaryDialogHandle);
    DialogEntry* secondaryEntry = FindDialogEntry(g_secondaryDialogHandle);

    if (primaryEntry && (!secondaryEntry || primaryEntry->handle != secondaryEntry->handle)) {
        ReleaseDialogEntry(primaryEntry);
    }

    if (secondaryEntry) {
        g_dialogQueues[0] = g_dialogQueues[1];
        g_primaryDialogHandle = g_secondaryDialogHandle;
        g_primaryDialogPriority = g_secondaryDialogPriority;
    }
    else {
        g_primaryDialogHandle = 0;
        g_primaryDialogPriority = -1;
        ResetDialogQueueInfo(0);
    }

    g_secondaryDialogHandle = 0;
    g_secondaryDialogPriority = -1;
    ResetDialogQueueInfo(1);
    UpdateDialogState(state);
    ClearDialogPendingLoadState();
}

static void UpgradeToPrimaryState() {
    // Host path has decoded samples ready in slot metadata; promotion maps to PSX state 5.
    UpgradeDialogInfoAndState(DialogStatus_LoadedPrimary);
}

static s32 KillDialogByHandleInternal(s32 handleParam) {
    PromotePendingDialogLoadState();
    UpdateDialogStatusFromSlots();

    const s32 handle = ResolveDialogHandleParam(handleParam);
    if (!jcsValidateHandle(handle)) {
        return 0;
    }

    switch (g_dialogStatus) {
        case DialogStatus_Idle:
        case DialogStatus_KillRequestedPrimary:
            return 0;

        case DialogStatus_LoadRequestedPrimary:
        case DialogStatus_LoadDeferredPrimary:
            if (!IsPrimaryHandle(handle)) {
                return 0;
            }
            UpdateDialogState(DialogStatus_KillRequestedPrimary);
            return 1;

        case DialogStatus_LoadDeferredSecondary:
            if (IsPrimaryHandle(handle)) {
                return 0;
            }
            ResetDialogInfoAndState(1, DialogStatus_KillRequestedPrimary);
            return 1;

        case DialogStatus_LoadedPrimary:
            if (!IsPrimaryHandle(handle)) {
                return 0;
            }
            ResetDialogInfoAndState(0, DialogStatus_Idle);
            return 1;

        case DialogStatus_LoadedPrimaryWithDeferred:
            if (IsPrimaryHandle(handle)) {
                UpgradeDialogInfoAndState(DialogStatus_LoadRequestedPrimary);
            }
            else {
                UpdateDialogState(DialogStatus_LoadedPrimaryWithDeferredFromState3);
            }
            return 1;

        case DialogStatus_LoadedPrimaryWithDeferredFromState2:
            if (IsPrimaryHandle(handle)) {
                UpgradeDialogInfoAndState(DialogStatus_LoadDeferredPrimary);
            }
            else {
                UpdateDialogState(DialogStatus_LoadedPrimaryWithDeferredFromState3);
            }
            return 1;

        case DialogStatus_LoadedPrimaryWithDeferredFromState3:
            if (!IsPrimaryHandle(handle)) {
                return 0;
            }
            UpgradeDialogInfoAndState(DialogStatus_KillRequestedPrimary);
            return 1;

        case DialogStatus_LoadedPrimaryAndSecondary:
            if (IsPrimaryHandle(handle)) {
                UpgradeToPrimaryState();
            }
            else {
                ResetDialogInfoAndState(1, DialogStatus_LoadedPrimary);
            }
            return 1;

        case DialogStatus_PlayingPrimary:
            if (!IsPrimaryHandle(handle)) {
                return 0;
            }
            StopDialogEntryVoice(FindDialogEntry(g_primaryDialogHandle));
            ResetDialogInfoAndState(0, DialogStatus_Idle);
            return 1;

        case DialogStatus_PlayingPrimaryWithSecondary:
            if (IsPrimaryHandle(handle)) {
                StopDialogEntryVoice(FindDialogEntry(g_primaryDialogHandle));
                UpgradeDialogInfoAndState(DialogStatus_LoadRequestedPrimary);
            }
            else {
                UpdateDialogState(DialogStatus_PlayingPrimaryState13);
            }
            return 1;

        case DialogStatus_PlayingPrimaryState12:
            if (IsPrimaryHandle(handle)) {
                StopDialogEntryVoice(FindDialogEntry(g_primaryDialogHandle));
                UpgradeDialogInfoAndState(DialogStatus_LoadDeferredPrimary);
            }
            else {
                UpdateDialogState(DialogStatus_PlayingPrimaryState13);
            }
            return 1;

        case DialogStatus_PlayingPrimaryState13:
            if (!IsPrimaryHandle(handle)) {
                return 0;
            }
            StopDialogEntryVoice(FindDialogEntry(g_primaryDialogHandle));
            UpgradeDialogInfoAndState(DialogStatus_KillRequestedPrimary);
            return 1;

        case DialogStatus_PlayingPrimaryState14:
            if (IsPrimaryHandle(handle)) {
                StopDialogEntryVoice(FindDialogEntry(g_primaryDialogHandle));
                UpgradeToPrimaryState();
            }
            else {
                ResetDialogInfoAndState(1, DialogStatus_PlayingPrimary);
            }
            return 1;

        default:
            return 1;
    }
}

static DialogEntry* FindDialogEntry(s32 handle) {
    if (handle == 0) {
        return nullptr;
    }

    for (s32 i = 0; i < DIALOG_ENTRY_COUNT; ++i) {
        if (g_dialogEntries[i].valid && g_dialogEntries[i].handle == handle) {
            return &g_dialogEntries[i];
        }
    }

    return nullptr;
}

static bool IsDialogEntryPlaying(const DialogEntry* entry) {
    return entry
        && entry->voice != AUDIO_VOICE_INVALID
        && AudioEngine::IsVoicePlaying(entry->voice);
}

static void StopDialogEntryVoice(DialogEntry* entry) {
    if (!entry || entry->voice == AUDIO_VOICE_INVALID) {
        return;
    }

    AudioEngine::StopVoice(entry->voice);
    entry->voice = AUDIO_VOICE_INVALID;
}

static void ReleaseDialogEntry(DialogEntry* entry) {
    if (!entry) {
        return;
    }

    StopDialogEntryVoice(entry);
    if (entry->sample != AUDIO_SAMPLE_INVALID) {
        AudioEngine::UnloadSample(entry->sample);
        entry->sample = AUDIO_SAMPLE_INVALID;
    }

    entry->valid = false;
}

static void ClearDialogPendingLoadState() {
    g_dialogLoadPending = 0;
    g_dialogLoadRequestFrame = -1;
    g_dialogLoadReadyFrame = -1;
}

static void PromoteSecondaryToPrimarySlot() {
    if (g_primaryDialogHandle != 0 || g_secondaryDialogHandle == 0) {
        return;
    }

    g_dialogQueues[0] = g_dialogQueues[1];
    ResetDialogQueueInfo(1);
    g_primaryDialogHandle = g_secondaryDialogHandle;
    g_primaryDialogPriority = g_secondaryDialogPriority;
    g_secondaryDialogHandle = 0;
    g_secondaryDialogPriority = -1;
}

// PSX: CheckFlushCount__F5Queue (JCSDLG.CPP:1859, 0x80043700)
static void CheckDialogFlushCount(s32 queue) {
    if (queue < 0 || queue >= 2) {
        return;
    }

    DialogQueueInfo& q = g_dialogQueues[queue];
    if (q.flushFrame == 0) {
        return;
    }

    const s32 frameCounter = GetDialogFrameCounter();
    if (q.flushFrame > frameCounter - 1200) {
        return;
    }

    q.flushFrame = frameCounter;
    if (queue == 0 && g_dialogQueues[0].priority >= 0x3FFFFFFF) {
        KillDialogByHandleInternal(g_primaryDialogHandle);
    }
}

// PSX: DialogTask states 5..9 (JCSDLG.CPP:1977..2014, 0x80043808)
static void ProcessDialogLoadedStates() {
    if (g_dialogStatus < DialogStatus_LoadedPrimary || g_dialogStatus > DialogStatus_LoadedPrimaryAndSecondary) {
        return;
    }

    DialogQueueInfo& q0 = g_dialogQueues[0];
    if (q0.timeoutFrame != 0) {
        const s32 frameCounter = GetDialogFrameCounter();
        if (q0.timeoutFrame < frameCounter) {
            KillDialogByHandleInternal(g_primaryDialogHandle);
            SetDlgStatus(12);
        }
        else {
            PlayDialogRequest(g_primaryDialogHandle, q0.playPosition, 0u);
        }
    }
    else {
        CheckDialogFlushCount(0);
    }

    if (g_dialogStatus == DialogStatus_LoadedPrimaryAndSecondary) {
        CheckDialogFlushCount(1);
    }
}

static void ReleaseNonSlotDialogEntries() {
    for (s32 i = 0; i < DIALOG_ENTRY_COUNT; ++i) {
        DialogEntry& entry = g_dialogEntries[i];
        if (!entry.valid) {
            continue;
        }

        if (entry.handle == g_primaryDialogHandle || entry.handle == g_secondaryDialogHandle) {
            continue;
        }

        ReleaseDialogEntry(&entry);
    }
}

static void UpdateDialogStatusFromSlots() {
    if (g_dialogStatusUpdateInProgress) {
        return;
    }

    g_dialogStatusUpdateInProgress = true;

    DialogEntry* primaryEntry = FindDialogEntry(g_primaryDialogHandle);
    DialogEntry* secondaryEntry = FindDialogEntry(g_secondaryDialogHandle);

    ProcessDialogLoadedStates();
    primaryEntry = FindDialogEntry(g_primaryDialogHandle);
    secondaryEntry = FindDialogEntry(g_secondaryDialogHandle);

    // PSX DialogTask states 10..14: transition when voice stops.
    if (primaryEntry
        && !IsDialogEntryPlaying(primaryEntry)
        && g_dialogStatus >= DialogStatus_PlayingPrimary
        && g_dialogStatus <= DialogStatus_PlayingPrimaryState14) {
        switch (g_dialogStatus) {
            case DialogStatus_PlayingPrimary:
                ReleaseDialogEntry(primaryEntry);
                g_primaryDialogHandle = 0;
                g_primaryDialogPriority = -1;
                ResetDialogQueueInfo(0);
                g_dialogStatus = DialogStatus_Idle;
                ClearDialogPendingLoadState();
                break;

            case DialogStatus_PlayingPrimaryWithSecondary:
            case DialogStatus_PlayingPrimaryState12:
            case DialogStatus_PlayingPrimaryState13:
            case DialogStatus_PlayingPrimaryState14:
            {
                const s32 priorPlayingStatus = g_dialogStatus;
                ReleaseDialogEntry(primaryEntry);
                g_primaryDialogHandle = 0;
                g_primaryDialogPriority = -1;

                if (secondaryEntry) {
                    g_dialogQueues[0] = g_dialogQueues[1];
                    ResetDialogQueueInfo(1);
                    g_primaryDialogHandle = g_secondaryDialogHandle;
                    g_primaryDialogPriority = g_secondaryDialogPriority;
                    g_secondaryDialogHandle = 0;
                    g_secondaryDialogPriority = -1;

                    if (priorPlayingStatus == DialogStatus_PlayingPrimaryWithSecondary) {
                        g_dialogStatus = DialogStatus_LoadRequestedPrimary;
                        g_dialogLoadPending = 1;
                        g_dialogLoadRequestFrame = GetDialogFrameCounter();
                        g_dialogLoadReadyFrame = g_dialogLoadRequestFrame
                            + ComputeDialogLoadDelayFrames(static_cast<u32>(g_dialogQueues[0].sectors));
                    }
                    else if (priorPlayingStatus == DialogStatus_PlayingPrimaryState12) {
                        g_dialogStatus = DialogStatus_LoadDeferredPrimary;
                        ClearDialogPendingLoadState();
                    }
                    else if (priorPlayingStatus == DialogStatus_PlayingPrimaryState13) {
                        g_dialogStatus = DialogStatus_KillRequestedPrimary;
                        ClearDialogPendingLoadState();
                    }
                    else {
                        g_dialogStatus = DialogStatus_LoadedPrimary;
                        ClearDialogPendingLoadState();
                    }
                }
                else {
                    ResetDialogQueueInfo(0);
                    ResetDialogQueueInfo(1);
                    g_dialogStatus = DialogStatus_Idle;
                    ClearDialogPendingLoadState();
                }
                break;
            }

            default:
                break;
        }

        primaryEntry = FindDialogEntry(g_primaryDialogHandle);
        secondaryEntry = FindDialogEntry(g_secondaryDialogHandle);
    }

    if (!primaryEntry) {
        g_primaryDialogHandle = 0;
        g_primaryDialogPriority = -1;
        ResetDialogQueueInfo(0);
    }

    if (!secondaryEntry) {
        g_secondaryDialogHandle = 0;
        g_secondaryDialogPriority = -1;
        ResetDialogQueueInfo(1);
    }

    PromoteSecondaryToPrimarySlot();
    ReleaseNonSlotDialogEntries();
    primaryEntry = FindDialogEntry(g_primaryDialogHandle);
    secondaryEntry = FindDialogEntry(g_secondaryDialogHandle);

    if (!primaryEntry) {
        g_dialogStatus = DialogStatus_Idle;
        ClearDialogPendingLoadState();
        g_dialogStatusUpdateInProgress = false;
        return;
    }

    if (IsDialogEntryPlaying(primaryEntry)) {
        if (g_dialogStatus < DialogStatus_PlayingPrimary || g_dialogStatus > DialogStatus_PlayingPrimaryState14) {
            g_dialogStatus = secondaryEntry ? DialogStatus_PlayingPrimaryWithSecondary : DialogStatus_PlayingPrimary;
        }
        g_dialogStatusUpdateInProgress = false;
        return;
    }

    if (g_dialogStatus >= DialogStatus_PlayingPrimary && g_dialogStatus <= DialogStatus_PlayingPrimaryState14) {
        g_dialogStatus = secondaryEntry ? DialogStatus_LoadedPrimaryAndSecondary : DialogStatus_LoadedPrimary;
    }

    g_dialogStatusUpdateInProgress = false;
}

// PSX load path is asynchronous (state 1 while loading, then state 5 once data is ready).
static void PromotePendingDialogLoadState() {
    if (g_dialogLoadPending == 0) {
        return;
    }

    if (g_dialogStatus != DialogStatus_LoadRequestedPrimary) {
        g_dialogLoadPending = 0;
        g_dialogLoadRequestFrame = -1;
        g_dialogLoadReadyFrame = -1;
        return;
    }

    const s32 frameCounter = GetDialogFrameCounter();
    if (frameCounter < g_dialogLoadReadyFrame) {
        return;
    }

    DialogEntry* entry = FindDialogEntry(g_primaryDialogHandle);
    if (!entry || entry->sample == AUDIO_SAMPLE_INVALID) {
        g_primaryDialogHandle = 0;
        g_primaryDialogPriority = -1;
        g_dialogStatus = DialogStatus_Idle;
        ClearDialogPendingLoadState();
        return;
    }

    g_dialogStatus = DialogStatus_LoadedPrimary;
    ClearDialogPendingLoadState();
}

// PSX: IsPrimaryHandle__Fl
static s32 IsPrimaryHandle(s32 handle) {
    if (handle <= 0 || handle != g_primaryDialogHandle) {
        return 0;
    }
    return (handle != g_secondaryDialogHandle) ? 1 : 0;
}

// PSX: IsSecondaryHandle__Fl
static s32 IsSecondaryHandle(s32 handle) {
    if (handle <= 0 || handle == g_primaryDialogHandle) {
        return 0;
    }
    return (handle == g_secondaryDialogHandle) ? 1 : 0;
}

// PSX: IsEitherHandle__Fl
static s32 IsEitherHandle(s32 handle) {
    if (IsPrimaryHandle(handle)) {
        return 1;
    }
    return IsSecondaryHandle(handle);
}

// PSX: IsCurrentHandle__Fl
static s32 IsCurrentHandle(s32 handle) {
    if (handle <= 0) {
        return 0;
    }

    switch (g_dialogStatus) {
        case 1:
        case 2:
        case 3:
        case 5:
        case 10:
            return IsPrimaryHandle(handle);

        case 4:
        case 6:
        case 7:
        case 8:
        case 9:
        case 11:
        case 12:
        case 13:
        case 14:
            return IsEitherHandle(handle);

        default:
            return 0;
    }
}

static DialogEntry* AllocateDialogEntry() {
    for (s32 i = 0; i < DIALOG_ENTRY_COUNT; ++i) {
        if (!g_dialogEntries[i].valid) {
            g_dialogEntries[i].handle = g_nextDialogHandle++;
            if (g_nextDialogHandle <= 0) {
                g_nextDialogHandle = 1;
            }
            g_dialogEntries[i].sample = AUDIO_SAMPLE_INVALID;
            g_dialogEntries[i].voice = AUDIO_VOICE_INVALID;
            g_dialogEntries[i].valid = true;
            return &g_dialogEntries[i];
        }
    }

    DialogEntry* entry = &g_dialogEntries[0];
    if (entry->voice != AUDIO_VOICE_INVALID) {
        AudioEngine::StopVoice(entry->voice);
    }
    if (entry->sample != AUDIO_SAMPLE_INVALID) {
        AudioEngine::UnloadSample(entry->sample);
    }
    entry->handle = g_nextDialogHandle++;
    if (g_nextDialogHandle <= 0) {
        g_nextDialogHandle = 1;
    }
    entry->sample = AUDIO_SAMPLE_INVALID;
    entry->voice = AUDIO_VOICE_INVALID;
    entry->valid = true;
    return entry;
}

static s32 PlayDialogHandle(DialogEntry* entry) {
    if (!entry || !entry->valid || !g_sound) {
        return 0;
    }

    if (entry->sample == AUDIO_SAMPLE_INVALID) {
        return 0;
    }

    if (entry->voice != AUDIO_VOICE_INVALID) {
        if (AudioEngine::IsVoicePlaying(entry->voice)) {
            return 1;
        }

        AudioEngine::StopVoice(entry->voice);
        entry->voice = AUDIO_VOICE_INVALID;
    }

    entry->voice = AudioEngine::PlaySample(entry->sample, g_sound->dialogVolume, 0.0f, false);
    if (entry->voice == AUDIO_VOICE_INVALID) {
        return 0;
    }

    return 1;
}

// PSX: PlayLoaded__FPC10tagLVector5State (JCSDLG.CPP:2664, 0x8004406C)
static s32 PlayLoadedState(s32 playPosition, s32 nextState) {
    DialogEntry* primaryEntry = FindDialogEntry(g_primaryDialogHandle);
    if (!primaryEntry || !IsPrimaryHandle(primaryEntry->handle)) {
        return 0;
    }

    const u16 clipEntry = g_dialogQueues[0].clipEntry;
    if (clipEntry == 0) {
        KillDialogByHandleInternal(0);
        return 0;
    }

    const bool clipAlreadyUsed = (DialogHeaderU8((u32)clipEntry) & 0x80) != 0;
    if (clipEntry == g_lastDialogChoiceOffset || clipAlreadyUsed) {
        KillDialogByHandleInternal(0);
        return 0;
    }

    // JCSDLG keeps only one active dialog voice at a time.
    for (s32 i = 0; i < DIALOG_ENTRY_COUNT; ++i) {
        DialogEntry& other = g_dialogEntries[i];
        if (!other.valid || other.handle == primaryEntry->handle) {
            continue;
        }
        StopDialogEntryVoice(&other);
    }

    const s32 started = PlayDialogHandle(primaryEntry);
    if (started == 0) {
        KillDialogByHandleInternal(0);
        return 0;
    }

    g_dialogStatus = nextState;
    if ((u32)clipEntry < g_dialogHeader.size()) {
        g_dialogHeader[clipEntry] = (u8)(g_dialogHeader[clipEntry] | 0x80);
    }
    g_lastDialogChoiceOffset = clipEntry;
    g_dialogQueues[0].clipEntry = 0;
    g_dialogQueues[0].playPosition = playPosition;
    g_dialogQueues[0].timeoutFrame = 0;
    ClearDialogPendingLoadState();
    return 1;
}

// PSX: jcsPlayDialog__FlPC10tagLVectorUl (JCSDLG.CPP:1121, 0x80043044)
static s32 PlayDialogRequest(s32 handleParam, s32 playPosition, u32 timeout) {
    SetDlgStatus(0);

    if (g_dialogSystemStarted == 0 || !g_sound) {
        SetDlgStatus(1);
        return 0;
    }

    const s32 handle = ResolveDialogHandleParam(handleParam);
    if (!jcsValidateHandle(handle)) {
        SetDlgStatus(6);
        return 0;
    }

    switch (g_dialogStatus) {
        case DialogStatus_Idle:
        case DialogStatus_KillRequestedPrimary:
        case DialogStatus_LoadDeferredSecondary:
            SetDlgStatus(7);
            return 0;

        case DialogStatus_LoadRequestedPrimary:
        case DialogStatus_LoadDeferredPrimary:
            return UpdateDialogTimeout(timeout, playPosition, 0);

        case DialogStatus_LoadedPrimary:
            return PlayLoadedState(playPosition, DialogStatus_PlayingPrimary);

        case DialogStatus_LoadedPrimaryWithDeferred:
            if (!IsPrimaryHandle(handle)) {
                return UpdateDialogTimeout(timeout, playPosition, 1);
            }
            return PlayLoadedState(playPosition, DialogStatus_PlayingPrimaryWithSecondary);

        case DialogStatus_LoadedPrimaryWithDeferredFromState2:
            if (!IsPrimaryHandle(handle)) {
                return UpdateDialogTimeout(timeout, playPosition, 1);
            }
            return PlayLoadedState(playPosition, DialogStatus_PlayingPrimaryState12);

        case DialogStatus_LoadedPrimaryWithDeferredFromState3:
            if (IsSecondaryHandle(handle)) {
                SetDlgStatus(7);
                return 0;
            }
            return PlayLoadedState(playPosition, DialogStatus_PlayingPrimaryState13);

        case DialogStatus_LoadedPrimaryAndSecondary:
            if (IsPrimaryHandle(handle)) {
                return PlayLoadedState(playPosition, DialogStatus_PlayingPrimaryState14);
            }
            if (g_primaryDialogPriority < g_secondaryDialogPriority) {
                return UpdateDialogTimeout(timeout, playPosition, 1);
            }
            KillDialogByHandleInternal(0);
            return PlayLoadedState(playPosition, DialogStatus_PlayingPrimary);

        case DialogStatus_PlayingPrimaryWithSecondary:
        case DialogStatus_PlayingPrimaryState12:
        case DialogStatus_PlayingPrimaryState14:
            if (!IsSecondaryHandle(handle)) {
                return 1;
            }
            return UpdateDialogTimeout(timeout, playPosition, 1);

        case DialogStatus_PlayingPrimaryState13:
            if (IsSecondaryHandle(handle)) {
                SetDlgStatus(7);
                return 0;
            }
            return 1;

        default:
            return 1;
    }
}

// PSX: rsDialogEvent (RSEVENT.CPP:82, 0x8003470C)
static s32 rsDialogEvent(s32 event, s32 param1, s32 param2, s32 param3) {
    switch (event) {
        case RS_LOAD_DIALOG:
        {
            SetDlgStatus(0);
            if (g_dialogSystemStarted == 0 || !g_sound) {
                SetDlgStatus(1);
                return 0;
            }

            PromotePendingDialogLoadState();
            UpdateDialogStatusFromSlots();

            DialogEntry* entry = AllocateDialogEntry();
            if (!entry) {
                return 0;
            }

            entry->character = param1;
            entry->dialogId = param2;
            entry->priority = param3;

            u16 clipChoiceOffset = 0;
            u32 loadSectors = 0;
            entry->sample = LoadDialogSample(param1, param2, &clipChoiceOffset, &loadSectors);
            if (entry->sample == AUDIO_SAMPLE_INVALID) {
                entry->valid = false;
                return 0;
            }

            DialogEntry* primaryEntry = FindDialogEntry(g_primaryDialogHandle);
            const bool primaryPlaying = IsDialogEntryPlaying(primaryEntry);

            if (!primaryEntry) {
                g_primaryDialogHandle = entry->handle;
                g_primaryDialogPriority = entry->priority;
                g_secondaryDialogHandle = 0;
                g_secondaryDialogPriority = -1;
                ResetDialogQueueInfo(0);
                g_dialogQueues[0].priority = entry->priority;
                g_dialogQueues[0].handle = entry->handle;
                g_dialogQueues[0].sectors = static_cast<s32>(loadSectors);
                g_dialogQueues[0].clipEntry = clipChoiceOffset;
                g_dialogQueues[0].dialogId = entry->dialogId;
                ResetDialogQueueInfo(1);
                g_dialogStatus = DialogStatus_LoadRequestedPrimary;
                g_dialogLoadPending = 1;
                g_dialogLoadRequestFrame = GetDialogFrameCounter();
                g_dialogLoadReadyFrame = g_dialogLoadRequestFrame + ComputeDialogLoadDelayFrames(loadSectors);
                return entry->handle;
            }

            // PSX JCSDLG manages two slots (primary + secondary). New loads become secondary while primary exists.
            if (g_secondaryDialogHandle != 0) {
                DialogEntry* oldSecondary = FindDialogEntry(g_secondaryDialogHandle);
                ReleaseDialogEntry(oldSecondary);
                g_secondaryDialogHandle = 0;
                g_secondaryDialogPriority = -1;
                ResetDialogQueueInfo(1);
            }

            g_secondaryDialogHandle = entry->handle;
            g_secondaryDialogPriority = entry->priority;
            ResetDialogQueueInfo(1);
            g_dialogQueues[1].priority = entry->priority;
            g_dialogQueues[1].handle = entry->handle;
            g_dialogQueues[1].sectors = static_cast<s32>(loadSectors);
            g_dialogQueues[1].clipEntry = clipChoiceOffset;
            g_dialogQueues[1].dialogId = entry->dialogId;
            ClearDialogPendingLoadState();
            g_dialogStatus = primaryPlaying ? DialogStatus_PlayingPrimaryWithSecondary : DialogStatus_LoadedPrimaryAndSecondary;
            return entry->handle;
        }

        case RS_PLAY_DIALOG:
            return PlayDialogRequest(param1, param2, static_cast<u32>(param3));

        case RS_STOP_DIALOG:
        {
            // PSX jcsStopDialog -> KillAllDialog: kill current primary twice to flush primary+secondary slots.
            KillDialogByHandleInternal(0);
            KillDialogByHandleInternal(0);
            g_dialogSystemStarted = 0;
            return 0;
        }

        case RS_KILL_DIALOG:
            return KillDialogByHandleInternal(param1);

        case RS_LOAD_AND_PLAY_DIALOG:
        {
            const s32 handle = rsDialogEvent(RS_LOAD_DIALOG, param1, param2, 255);
            if (!handle) {
                return 0;
            }

            g_dialogLoadReadyFrame = GetDialogFrameCounter();
            PromotePendingDialogLoadState();
            return rsDialogEvent(RS_PLAY_DIALOG, handle, param3, 360);
        }

        case RS_QUERY_DIALOG_PRIORITY:
            return jcsQueryDialogPriority();

        default:
            break;
    }

    return 0;
}

// PSX: gp+544 - global sound enabled flag (0 = enabled, nonzero = disabled)
static s32 g_soundDisabled = 0;

// PSX: gp+572 - pause flag
static s32 g_pauseFlag = 0;

// PSX: gp+732 - mute flag
static s32 g_muteFlag = 0;

// PSX: Sound location to music file mapping (gp-relative at 0x800ED6CC)
// Event 4 (RS_SET_LOCATION) with param1 sets the "current sound location"
// which determines which music track plays when RS_LEVEL_BEGIN fires.
s32 g_currentSoundLocation = 0;

bool g_deferLevelBeginMusic = false;

// PSX: gp[0x2E8] - current ambience space (set by jcsSetAmbienceSpace)
static s32 g_currentAmbienceSpace = 0;

// PSX music file table indexed by sound location (LocInfo at 0x800D6588)
// Matches PSX LocInfo[n][1] ordering exactly
static const char* s_musicTable[] = {
    "SOUND/MUSIC/CHINA1.FAG",    //  0: Chinatown petal 1
    "SOUND/MUSIC/CHINA2.FAG",    //  1: Chinatown petal 2
    "SOUND/MUSIC/CHINA3.FAG",    //  2: Chinatown petal 3
    "SOUND/MUSIC/CHINAB.FAG",    //  3: Chinatown boss
    "SOUND/MUSIC/WATER1.FAG",    //  4: Waterfront petal 1
    "SOUND/MUSIC/WATER2.FAG",    //  5: Waterfront petal 2
    "SOUND/MUSIC/WATER3.FAG",    //  6: Waterfront petal 3
    "SOUND/MUSIC/WATERB.FAG",    //  7: Waterfront boss
    "SOUND/MUSIC/SEWER1.FAG",    //  8: Sewer petal 1
    "SOUND/MUSIC/SEWER2.FAG",    //  9: Sewer petal 2
    "SOUND/MUSIC/SEWER3.FAG",    // 10: Sewer petal 3
    "SOUND/MUSIC/SEWERB.FAG",    // 11: Sewer boss
    "SOUND/MUSIC/ROOF1.FAG",     // 12: Rooftop petal 1
    "SOUND/MUSIC/ROOF2.FAG",     // 13: Rooftop petal 2
    "SOUND/MUSIC/ROOF3.FAG",     // 14: Rooftop petal 3
    "SOUND/MUSIC/ROOFB.FAG",     // 15: Rooftop boss
    "SOUND/MUSIC/FACTORY1.FAG",  // 16: Factory petal 1
    "SOUND/MUSIC/FACTORY2.FAG",  // 17: Factory petal 2
    "SOUND/MUSIC/FACTORY3.FAG",  // 18: Factory petal 3
    "SOUND/MUSIC/FACTORYB.FAG",  // 19: Factory boss
    "SOUND/MUSIC/TEMPLE.FAG",    // 20: Temple
    "SOUND/MUSIC/DESTSEL.FAG",   // 21: Destination select
    "SOUND/MUSIC/TITLE.FAG",     // 22: Title screen
    "SOUND/MUSIC/GAMEOVER.FAG",  // 23: Game over
    nullptr,                      // 24: movie (no FAG)
};
static constexpr s32 MUSIC_TABLE_COUNT = 25;

// PSX: CInteractiveMusicController::Think (MSCCTRLR.CPP:56, 0x80082960)
// Block-based FAG song switching. Decoded from PSX binary: location 9 (SEWER2)
// uses blocks 4-34 for song 1 (action), everything else song 0 (calm).
// Other locations with interactive music decoded similarly.
void InteractiveMusicControllerThink() {
    if (!g_sound || !g_blockManager || !Player::s_player) return;
    if (g_deferLevelBeginMusic) return;

    const s32 loc = g_currentSoundLocation;
    if (loc < 0 || loc >= MUSIC_TABLE_COUNT) return;
    const char* path = s_musicTable[loc];
    if (!path) return;

    const s32 block = static_cast<s32>(
        g_blockManager->GetBlockNumber(Player::s_player->pos));
    if (block < 0 || block == 0x1000) return;

    // Determine desired song index from PSX MSCCTRLR.CPP block thresholds
    s32 desired = 0;
    if (loc == 9) {
        // SEWER2: blocks 4-34 → song 1 (train action), others → song 0
        desired = (block >= 4 && block < 35) ? 1 : 0;
    }
    else if (loc == 8) {
        // SEWER1: blocks 24+ → song 1
        desired = (block >= 24) ? 1 : 0;
    }
    else if (loc == 10) {
        // SEWER3: blocks 22+ → song 1
        desired = (block >= 22) ? 1 : 0;
    }
    else if (loc == 15) {
        // ROOFB: blocks 3+ → song 1
        desired = (block >= 3) ? 1 : 0;
    }
    else {
        return; // No interactive music for this location
    }

    static s32 s_lastBlock = -1;
    static s32 s_currentSong = 0;
    static s32 s_lastLocation = -1;

    if (loc != s_lastLocation) {
        s_lastBlock = -1;
        s_currentSong = 0;
        s_lastLocation = loc;
    }

    if (block == s_lastBlock) return;
    s_lastBlock = block;

    if (desired == s_currentSong) return;
    s_currentSong = desired;

    g_sound->PlayMusicTrackSong(path, static_cast<u32>(desired), g_sound->musicVolume);
}

// PSX: rsEvent (RSEVENT.CPP:48, 0x800346B0)
s32 rsEvent(s32 event, s32 param1, s32 param2, s32 param3) {
    MARKFUNCTION(0x800346B0);

    // PSX: reads gp+544 flag, returns 0 if sound disabled
    if (g_soundDisabled != 0) return 0;

    // PSX dispatch:
    // Events 26-31 -> rsDialogEvent
    // Events 1-23  -> jcsHandleControlEvent
    if (event >= 26 && event <= 31) {
        return rsDialogEvent(event, param1, param2, param3);
    }

    if (event >= 1 && event <= 23) {
        return jcsHandleControlEvent(event, param1, param2, param3);
    }

    return 0;
}

// PSX: jcsHandleControlEvent (JCSOUND.CPP:947, 0x80035B00)
s32 jcsHandleControlEvent(s32 event, s32 param1, s32 param2, s32 param3) {
    MARKFUNCTION(0x80035B00);

    if (!g_sound) return 0;

    switch (event) {
        case RS_INITIALIZE: // 1
            LOG("[rsEvent] Initialize");
            break;

        case RS_TERMINATE: // 2
            LOG("[rsEvent] Terminate");
            rsdWorld::StopAllPersistentSounds();
            g_sound->StopMusic();
            break;

        case RS_UNLOAD_LEVEL: // 3 - stop all sounds
            LOG("[rsEvent] UnloadLevel - stop all");
            rsdWorld::StopAllPersistentSounds();
            g_sound->StopMusic();
            break;

        case RS_SET_LOCATION: // 4 - set sound location (music + SFX bank)
            LOG("[rsEvent] SetSoundLocation(%d)", param1);
            g_currentSoundLocation = param1;
            // PSX: LocInfo[loc][0] -> GAME.WAP location table -> bank index.
            // Mapping: location 0-20 -> bank 0-20, location 21+ -> bank 21.
            if (g_sound) {
                g_sound->activeSfxBank = (param1 < 21) ? param1 : 21;
            }
            // PSX resets dialog header selection flags when switching level/location.
            if (EnsureDialogDataLoaded()) {
                ResetDialogHeaderState();
            }
            break;

        case RS_LEVEL_BEGIN: // 5 - start music for current location
        {
            g_pauseFlag = 0;
            g_muteFlag = 0;
            if (g_deferLevelBeginMusic) {
                // Decode now, while still hidden behind the loading screen -
                // only the (cheap) actual playback start is deferred to the
                // fade-in via Sound::StartPreloadedMusic(), so the fade
                // doesn't stall on file I/O/decode.
                LOG("[rsEvent] LevelBegin - preloading for deferred start (location=%d)", g_currentSoundLocation);
                if (g_currentSoundLocation >= 0 && g_currentSoundLocation < MUSIC_TABLE_COUNT) {
                    const char* path = s_musicTable[g_currentSoundLocation];
                    if (path) {
                        g_sound->PreloadMusicTrack(path, g_sound->musicVolume);
                    }
                }
                break;
            }
            LOG("[rsEvent] LevelBegin - start music (location=%d, musicVol=%.2f)", g_currentSoundLocation, g_sound->musicVolume);
            if (g_currentSoundLocation >= 0 && g_currentSoundLocation < MUSIC_TABLE_COUNT) {
                const char* path = s_musicTable[g_currentSoundLocation];
                if (path) {
                    g_sound->PlayMusicTrack(path, g_sound->musicVolume);
                }
            }
            break;
        }

        case RS_STOP_MUSIC: // 6 - fade/stop music
            LOG("[rsEvent] StopMusic");
            g_pauseFlag = 0;
            jcsFadeOutEngine(14);
            break;

        case RS_FADE_OUT: // 7 - fade out
            LOG("[rsEvent] FadeOut(%d)", param1);
            jcsFadeOutEngine((u32)param1);
            break;

        case RS_FADE_OUT_2: // 8 - fade in
            LOG("[rsEvent] FadeIn(%d)", param1);
            if (!g_muteFlag) {
                jcsFadeInEngine((u32)param1);
            }
            break;

        case RS_PAUSE: // 9
            LOG("[rsEvent] Pause");
            g_pauseFlag = 1;
            break;

        case RS_MUTE: // 10
            LOG("[rsEvent] Mute");
            if (!g_muteFlag) {
                jcsFadeOutEngine(15);
                g_muteFlag = 1;
            }
            break;

        case RS_UNMUTE: // 11
            LOG("[rsEvent] Unmute");
            if (g_muteFlag) {
                if (!g_pauseFlag) {
                    jcsFadeInEngine(15);
                }
                g_muteFlag = 0;
            }
            break;

        case RS_CD_YIELD: // 12
            break;

        case RS_CD_ACCESS: // 13
            break;

        case RS_SET_MUSIC_VOL: // 16
        {
            // PSX: 80 * param / 100 -> SPU vol (0-100 range out of 127)
            f32 vol = (f32)param1 * 0.8f / 100.0f;
            LOG("[rsEvent] SetMusicVol(%d -> %.2f)", param1, vol);
            g_sound->SetMusicVolume(vol);
            break;
        }

        case RS_SET_EFFECTS_VOL_AUX: // 17
        {
            // PSX: no case handler - falls through to default (no-op)
            LOG("[rsEvent] SetEffectsVolAux(%d)", param1);
            break;
        }

        case RS_SET_EFFECTS_VOL: // 18
        {
            // PSX: stores raw value as effects vol
            f32 vol = (f32)param1 / 100.0f;
            LOG("[rsEvent] SetEffectsVol(%d -> %.2f)", param1, vol);
            g_sound->SetEffectsVolume(vol);
            break;
        }

        case RS_SET_DIALOG_VOL: // 19
        {
            // PSX: 83 * param / 100 -> SPU vol
            f32 vol = (f32)param1 * 0.83f / 100.0f;
            LOG("[rsEvent] SetDialogVol(%d -> %.2f)", param1, vol);
            g_sound->SetDialogVolume(vol);
            break;
        }

        case RS_SET_STEREO: // 14
            LOG("[rsEvent] SetStereo");
            AudioEngine::SetOutputMono(false);
            break;

        case RS_SET_MONO: // 15
            LOG("[rsEvent] SetMono");
            AudioEngine::SetOutputMono(true);
            break;

        case 20: // jcsSetAmbienceSpace + optional crossfade
            LOG("[rsEvent] SetAmbienceSpace(%d, crossfade=%d)", param1, param2);
            // PSX: stores ambienceSpace to gp[0x2E8], then calls
            // rsdAmbiance::SetSpace if sound state==2 and ambiance object exists.
            // PC: store space for use by InteractiveMusicControllerThink.
            g_currentAmbienceSpace = param1;
            break;

        case 21: // jcsSetListener(playerPos, cameraData) - 3D audio listener
            rsdWorld::UpdateSpatialAudioState();
            break;

        case 22: // jcsFadeOutEngine(-1) - fade out all
            LOG("[rsEvent] FadeOutAll");
            jcsFadeOutEngine(0xFFFFFFFF);
            break;

        case 23: // jcsFadeInEngine(-1) - fade in all
            LOG("[rsEvent] FadeInAll");
            if (!g_muteFlag) {
                jcsFadeInEngine(0xFFFFFFFF);
            }
            break;

        default:
            LOG("[rsEvent] Unhandled control event %d (p1=%d, p2=%d, p3=%d)",
                event, param1, param2, param3);
            break;
    }

    return 1;
}

// PSX: jcsFadeInEngine__FUl (JCSOUND.CPP:764, 0x8003582C)
// Flags bitmask:
//   bit 1 (0x02): fade in music player
//   bit 2 (0x04): fade in ambiance
//   bit 0 (0x01): resume dialog
//   bit 3 (0x08): fade in persistent sounds + set reverb
void jcsFadeInEngine(u32 flags) {
    MARKFUNCTION(0x8003582C);

    if (!g_sound) return;

    if (flags & 0x02) {
        g_sound->UnmuteMusic();
    }

    // PSX: bit 2 -> rsdAmbiance::FadeIn (1500ms) -- requires rsdAmbiance subsystem

    if (flags & 0x01) {
        ResumeDialogTimeout(0);
        ResumeDialogTimeout(1);
    }

    // PSX: bit 3 -> rsdPersistent::FadeInAll(1500) + rsdSetReverb
    if (flags & 0x08) {
        rsdWorld::UnmuteAllPersistentSounds();
    }
}

// PSX: jcsFadeOutEngine__FUl (JCSOUND.CPP:721, 0x80035760)
// Flags bitmask:
//   bit 1 (0x02): fade out music player
//   bit 2 (0x04): fade out ambiance
//   bit 0 (0x01): pause dialog
//   bit 3 (0x08): fade out persistent sounds + save/clear reverb
// PC: music fade-out stops the music track.
void jcsFadeOutEngine(u32 flags) {
    MARKFUNCTION(0x80035760);

    if (!g_sound) return;

    if (flags & 0x02) {
        g_sound->MuteMusic();
    }

    // PSX: bit 2 -> rsdAmbiance::FadeOut(1500) -- requires rsdAmbiance subsystem

    if (flags & 0x01) {
        if (g_dialogStatus >= DialogStatus_PlayingPrimary
            && g_dialogStatus <= DialogStatus_PlayingPrimaryState14) {
            KillDialogByHandleInternal(0);
        }
        PauseDialogTimeout(0);
        PauseDialogTimeout(1);
    }

    // PSX: bit 3 -> rsdPersistent::FadeOutAll(1500) + save reverb + set reverb(0)
    if (flags & 0x08) {
        rsdWorld::MuteAllPersistentSounds();
    }
}

// PSX: jcsValidateHandle (JCSDLG.CPP:1639, 0x800434D8)
s32 jcsValidateHandle(s32 handle) {
    MARKFUNCTION(0x800434D8);

    PromotePendingDialogLoadState();
    UpdateDialogStatusFromSlots();

    if (!IsCurrentHandle(handle)) {
        return 0;
    }

    switch (g_dialogStatus) {
        case 1:
        case 2:
        case 5:
        case 8:
        case 10:
        case 13:
            return IsPrimaryHandle(handle);

        case 4:
            return IsSecondaryHandle(handle);

        case 6:
        case 7:
        case 9:
        case 11:
        case 12:
        case 14:
            return IsEitherHandle(handle);

        default:
            return 0;
    }
}

// PSX: jcsIsPlayable (JCSDLG.CPP:428, 0x80042908)
s32 jcsIsPlayable(s32 handle) {
    MARKFUNCTION(0x80042908);

    PromotePendingDialogLoadState();

    if (!jcsValidateHandle(handle)) {
        return 0;
    }

    switch (g_dialogStatus) {
        case 5:
        case 6:
        case 7:
        case 8:
            return IsPrimaryHandle(handle);

        case 9:
            if (IsSecondaryHandle(handle)) {
                return 1;
            }
            return IsPrimaryHandle(handle);

        default:
            return 0;
    }
}

// PSX: jcsIsPlaying__Fv (JCSDLG.CPP:488, 0x800429A4)
s32 jcsIsPlaying() {
    MARKFUNCTION(0x800429A4);
    return jcsIsPlaying(g_primaryDialogHandle);
}

// PSX: jcsIsPlaying(handle) (JCSDLG.CPP:506, 0x800429CC)
s32 jcsIsPlaying(s32 handle) {
    MARKFUNCTION(0x800429CC);

    PromotePendingDialogLoadState();
    UpdateDialogStatusFromSlots();

    s32 result = 0;
    if (jcsValidateHandle(handle) && g_dialogStatus >= 10 && g_dialogStatus < 15) {
        result = IsPrimaryHandle(handle);
    }

    return result;
}

// PSX: jcsQueryDialogPriority() (JCSDLG.CPP:1285, 0x80043218)
s32 jcsQueryDialogPriority() {
    MARKFUNCTION(0x80043218);

    PromotePendingDialogLoadState();
    UpdateDialogStatusFromSlots();

    if (g_primaryDialogHandle == 0) {
        return -1;
    }

    s32 bestPriority = g_primaryDialogPriority;
    if (bestPriority < g_secondaryDialogPriority) {
        bestPriority = g_secondaryDialogPriority;
    }

    return bestPriority;
}

// PSX: jcsQueryDialogPriority(handle) (JCSDLG.CPP:1312, 0x80043254)
s32 jcsQueryDialogPriority(s32 handle) {
    MARKFUNCTION(0x80043254);

    PromotePendingDialogLoadState();

    if (!jcsValidateHandle(handle)) {
        return -1;
    }

    if (IsPrimaryHandle(handle)) {
        return g_primaryDialogPriority;
    }

    return g_secondaryDialogPriority;
}

// PSX: jcsStartDialog__Fv (JCSDLG.CPP:1648, 0x800434F0)
void jcsStartDialog() {
    MARKFUNCTION(0x800434F0);

    for (s32 i = 0; i < DIALOG_ENTRY_COUNT; ++i) {
        DialogEntry& entry = g_dialogEntries[i];
        if (entry.voice != AUDIO_VOICE_INVALID) {
            AudioEngine::StopVoice(entry.voice);
        }
        if (entry.sample != AUDIO_SAMPLE_INVALID) {
            AudioEngine::UnloadSample(entry.sample);
        }
        entry = {};
    }

    g_nextDialogHandle = 1;
    g_primaryDialogHandle = 0;
    g_secondaryDialogHandle = 0;
    g_primaryDialogPriority = -1;
    g_secondaryDialogPriority = -1;
    g_dialogStatus = DialogStatus_Idle;
    g_dialogSystemStarted = 1;
    ClearDialogPendingLoadState();
    ResetDialogQueueInfo(0);
    ResetDialogQueueInfo(1);
    g_dialogResultStatus = 0;
}

s32 jcsPlaySpecificDialog(s32 character, s32 dialogId, u32 variant) {
    rsEvent(RS_STOP_DIALOG, 0, 0, 0);
    jcsStartDialog();
    g_forcedDialogVariant = static_cast<s32>(variant);
    const s32 handle = rsEvent(RS_LOAD_DIALOG, character, dialogId, 255);
    g_forcedDialogVariant = -1;
    if (handle != 0) {
        // Loading the decoded PC sample above is synchronous, although the
        // emulated PSX dialog state normally waits a few frames before making
        // it playable.  Callers usually poll the dialog system during gameplay
        // and advance that state.  The quitting menu deliberately processes no
        // more input/gameplay, so promote this one-shot request immediately.
        g_dialogLoadReadyFrame = GetDialogFrameCounter();
        PromotePendingDialogLoadState();
        if (rsEvent(RS_PLAY_DIALOG, handle, 0, 360) == 0) {
            return 0;
        }
    }
    return handle;
}
