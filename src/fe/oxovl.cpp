#include "fe/oxovl.h"
#include "xclib/xclib.h"

// PSX: _5oxOvl (OXOVL.CPP:98, 0x80091050)
oxOvl::oxOvl() {
    MARKFUNCTION(0x80091050);
}

// PSX: __5oxOvl (OXOVL.CPP:103, 0x80091064)
oxOvl::~oxOvl() {
    MARKFUNCTION(0x80091064);
}

// PSX: Init__5oxOvlP9xcOverlay (OXOVL.CPP:107, 0x80091098)
void oxOvl::Init(xcOverlayData* ovl) {
    MARKFUNCTION(0x80091098);
    overlay = ovl;
    SelfInit();
}

// PSX: SetVisible__5oxOvls (OXOVL.CPP:125, 0x800910D0)
void oxOvl::SetVisible(s16 vis) {
    MARKFUNCTION(0x800910D0);
    if (overlay) {
        overlay->visibility = vis ? 1 : 0;
    }
}

// PSX: IsVisible__5oxOvl (OXOVL.CPP:130, 0x800910F8)
bool oxOvl::IsVisible() const {
    MARKFUNCTION(0x800910F8);
    return overlay && overlay->visibility != 0;
}

// PSX: GetPrimPos__5oxOvlP9xcPrimObjRsT2 (OXOVL.CPP:137, 0x80091138)
bool oxOvl::GetPrimPos(u8* prim, s16& x, s16& y) const {
    MARKFUNCTION(0x80091138);
    if (!prim) {
        return false;
    }

    const u8 type = prim[0];
    if (type == XC_PRIM_POLYF4 || type == XC_PRIM_POLYG4) {
        const s16* p = reinterpret_cast<const s16*>(prim);
        x = p[4];
        y = p[5];
        return true;
    }

    if (type == XC_PRIM_SPRITE || type == XC_PRIM_TEXT) {
        const s16* p = reinterpret_cast<const s16*>(prim);
        x = p[7];
        y = p[13];
        return true;
    }

    return false;
}

// PSX: SetPrimPos__5oxOvlP9xcPrimObjss (OXOVL.CPP:142, 0x80091144)
bool oxOvl::SetPrimPos(u8* prim, s16 x, s16 y) const {
    MARKFUNCTION(0x80091144);
    if (!prim) {
        return false;
    }

    const u8 type = prim[0];
    if (type == XC_PRIM_SPRITE || type == XC_PRIM_TEXT) {
        s32* p = reinterpret_cast<s32*>(prim);
        p[3] = x << 16;
        p[6] = y << 16;
        return true;
    }

    if (type == XC_PRIM_POLYF4) {
        s16* p = reinterpret_cast<s16*>(prim);
        const s16 oldX0 = p[4];
        const s16 oldY0 = p[5];
        const s16 oldX1 = p[6];
        const s16 oldY1 = p[7];
        const s16 oldX2 = p[8];

        p[5] = y;
        p[9] = y;
        p[4] = oldX1;

        const s16 nx = (s16)(x + oldX2 - oldX0);
        p[8] = nx;
        p[10] = nx;

        const s16 ny = (s16)(y + oldY1 - oldY0);
        p[7] = ny;
        p[11] = ny;
        return true;
    }

    if (type == XC_PRIM_POLYG4) {
        s16* p = reinterpret_cast<s16*>(prim);
        const s16 oldX0 = p[4];
        const s16 oldY0 = p[5];
        const s16 oldX1 = p[8];
        const s16 oldY1 = p[9];
        const s16 oldX2 = p[12];

        p[5] = y;
        p[13] = y;
        p[4] = oldX1;

        const s16 nx = (s16)(x + oldX2 - oldX0);
        p[12] = nx;
        p[16] = nx;

        const s16 ny = (s16)(y + oldY1 - oldY0);
        p[9] = ny;
        p[17] = ny;
        return true;
    }

    return false;
}
