#include "snd/basesnd.h"
#include "snd/sndfact.h"
#include "snd/trnssnd.h"
#include "snd/prstsnd.h"

// PSX: __6CSound (BASESND.CPP:31, 0x800A1E18)
CSound::CSound() {
    MARKFUNCTION(0x800A1E18);
    refCount = 1;
    posPtr = nullptr;
    flags = 0;
    pad12 = 0;
}

CSound::~CSound() {}

// PSX: Initialize__6CSoundPC10tagLVector (BASESND.CPP:152, 0x800A1F48)
s32 CSound::Initialize(void* pos) {
    MARKFUNCTION(0x800A1F48);
    posPtr = pos;
    return 0;
}

// PSX: GetPosPtr__C6CSound (BASESND.CPP:202, 0x800A1FAC)
void* CSound::GetPosPtr() const {
    MARKFUNCTION(0x800A1FAC);
    return posPtr;
}

// PSX: Release__6CSound (BASESND.CPP:174, 0x800A1F54)
// PSX: refCount -= 2; if (refCount <= 0 && this != null) vtable[12]+12 -> destroy(3)
void CSound::Release() {
    MARKFUNCTION(0x800A1F54);
    refCount -= 2;
    if ((s32)refCount <= 0) {
        delete this;
    }
}

// PSX: PlayTransient__6CSoundUsUlUs (BASESND.CPP:309, 0x800A2088)
// PSX flow:
//   CreateObject(10070, &tmp, soundId) -> Initialize(tmp, posPtr, flags)
//   if (triggerFlags & 8): Trigger(tmp, pan)
//   elif (triggerFlags & 1): TriggerDialogWorld(tmp, pan)
//   destroy(tmp)
s32 CSound::PlayTransient(u16 soundId, u32 triggerFlags, u16 pan) {
    MARKFUNCTION(0x800A2088);

    CSound* tmp = nullptr;
    s32 result = CSoundFactory::CreateObject(10070, &tmp, soundId);
    if (result < 0) {
        return result;
    }

    CGenericTransientSound* trans = static_cast<CGenericTransientSound*>(tmp);
    s32 err = trans->Initialize(posPtr, flags);
    if (err >= 0) {
        if (triggerFlags & 0x08) {
            err = trans->Trigger(pan);
        }
        else {
            err = -1000;
            if (triggerFlags & 0x01) {
                err = trans->TriggerDialogWorld(pan);
            }
        }
    }

    // PSX: destroy via vtable
    delete trans;
    return err;
}

// PSX: PlayTransientStereo__6CSoundUsUs (BASESND.CPP:360, 0x800A2164)
// PSX flow:
//   CreateObject(10070, &objL, sndL)
//   CreateObject(10070, &objR, sndR)
//   InitializeStereo(objL, 100, 0) - full left
//   InitializeStereo(objR, 0, 100) - full right
//   Trigger(objL, 0), Trigger(objR, 0)
//   destroy both
s32 CSound::PlayTransientStereo(u16 sndL, u16 sndR) {
    MARKFUNCTION(0x800A2164);

    CSound* tmpL = nullptr;
    CSound* tmpR = nullptr;
    const s32 resL = CSoundFactory::CreateObject(10070, &tmpL, sndL);
    const s32 resR = CSoundFactory::CreateObject(10070, &tmpR, sndR);

    if (tmpL && tmpR) {
        CGenericTransientSound* transL = static_cast<CGenericTransientSound*>(tmpL);
        CGenericTransientSound* transR = static_cast<CGenericTransientSound*>(tmpR);

        transL->InitializeStereo(100, 0);
        transR->InitializeStereo(0, 100);
        transL->Trigger(0);
        transR->Trigger(0);
    }
    else if (tmpL) {
        CGenericTransientSound* transL = static_cast<CGenericTransientSound*>(tmpL);
        transL->InitializeStereo(100, 100);
        transL->Trigger(0);
    }
    else if (tmpR) {
        CGenericTransientSound* transR = static_cast<CGenericTransientSound*>(tmpR);
        transR->InitializeStereo(100, 100);
        transR->Trigger(0);
    }

    if (tmpL) {
        delete tmpL;
    }
    if (tmpR) {
        delete tmpR;
    }

    if (!tmpL && !tmpR) {
        return (resL < 0) ? resL : resR;
    }
    return 0;
}

// PSX: BeginPersistent__6CSoundUcPP23CGenericPersistentSound (BASESND.CPP:229, 0x800A1FB8)
s32 CSound::BeginPersistent(u8 soundId, CGenericPersistentSound** outObj) {
    MARKFUNCTION(0x800A1FB8);

    if (*outObj) {
        return -3000;
    }

    CSound* tmp = nullptr;
    s32 result = CSoundFactory::CreateObject(10080, &tmp, soundId);
    if (result < 0) {
        return result;
    }

    *outObj = static_cast<CGenericPersistentSound*>(tmp);
    (*outObj)->Initialize(posPtr, flags);
    (*outObj)->Begin();
    return 0;
}

// PSX: EndPersistent__6CSoundPP23CGenericPersistentSound (BASESND.CPP:275, 0x800A2038)
s32 CSound::EndPersistent(CGenericPersistentSound** obj) {
    MARKFUNCTION(0x800A2038);

    if (!*obj) {
        return -3001;
    }

    // PSX: vtable[12]+8 = End virtual call
    (*obj)->End();
    delete* obj;
    *obj = nullptr;
    return 0;
}
