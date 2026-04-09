#include "snd/snddrct.h"
#include "snd/sndfact.h"
#include "snd/trnssnd.h"
#include "snd/prstsnd.h"

// PSX: PlayTransient__12CSoundDirectUsPC10tagLVectorUsUl (SNDDRCT.CPP:18, 0x8008D5B0)
// PSX flow: CreateObject(10070, &tmp, soundId) -> Initialize(tmp, posPtr, flags) -> Trigger(tmp, pan) -> destroy(tmp)
s32 CSoundDirect::PlayTransient(u16 soundId, void* posPtr, u16 pan, u32 flags) {
    MARKFUNCTION(0x8008D5B0);

    CSound* tmp = nullptr;
    s32 result = CSoundFactory::CreateObject(10070, &tmp, soundId);
    if (result < 0) {
        return result;
    }

    CGenericTransientSound* trans = static_cast<CGenericTransientSound*>(tmp);
    trans->Initialize(posPtr, (u16)flags);
    trans->Trigger(pan);

    // PSX: vtable[12]+8 = destructor with delete flag
    delete trans;
    return result;
}

// PSX: BeginPersistent__12CSoundDirectUcPP23CGenericPersistentSoundPC10tagLVector (SNDDRCT.CPP:45, 0x8008D650)
s32 CSoundDirect::BeginPersistent(u8 persistId, CGenericPersistentSound** outObj, void* posPtr) {
    MARKFUNCTION(0x8008D650);

    if (*outObj) {
        return -3000;
    }

    CSound* tmp = nullptr;
    s32 result = CSoundFactory::CreateObject(10080, &tmp, persistId);
    if (result < 0) {
        return 0;
    }

    *outObj = static_cast<CGenericPersistentSound*>(tmp);
    (*outObj)->Initialize(posPtr);
    (*outObj)->Begin();
    return 0;
}

// PSX: EndPersistent__12CSoundDirectPP23CGenericPersistentSound (SNDDRCT.CPP:71, 0x8008D6C4)
s32 CSoundDirect::EndPersistent(CGenericPersistentSound** obj) {
    MARKFUNCTION(0x8008D6C4);

    if (!*obj) {
        return -3001;
    }

    // PSX: vtable[12]+8 = End/destructor
    (*obj)->End();
    delete* obj;
    *obj = nullptr;
    return 0;
}
