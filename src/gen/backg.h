#pragma once

#include "core.h"

struct tPrimGeom;
class tTexture;
class World;
struct OriginalGeo;

struct BackGWorkParams {
    s32 fieldEC0 = 0;
    s32 fieldEC4 = 0;
    s32 fieldEC8 = 0;
    u16 fieldED0 = 0;
    s32 fieldECC = 0;
    u8 fieldED4 = 0x80;
    u8 fieldED5 = 0x80;
    u8 fieldED6 = 0x80;
    u16 fieldED8 = 0;
    s32 fieldEDC = 0;
    s32 fieldEE0 = 0;
    u8 fieldEE4 = 0;
    u8 fieldEE5 = 0;
    u8 fieldEE6 = 0;
};

struct BackGRuntimeParams {
    s32 field614 = 0;
    s32 field618 = 0;
    s32 field61C = 0;
    u16 field620 = 0;
    s32 field624 = 0;
    u16 field628 = 0;
    s32 field62C = 0;
    u8 field630 = 0;
    u8 field631 = 0;
    u8 field632 = 0;
};

struct BackGGeoEntry {
    tPrimGeom* geo = nullptr;
    OriginalGeo* sourceGeo = nullptr;
    s32 field4 = 0;
    u16 field8 = 0;
    u16 fieldA = 0;
    u16 fieldC = 0;
    u16 fieldE = 0;
    s16 vx[4] = {};
    s16 vy[4] = {};
    u32 color[4] = {};
    u8 uByIndex[4] = {};
    u8 vByIndex[4] = {};
    u16 tpage = 0;
    u16 cba = 0;
    u8 useDynamic = 0;
    u8 drawAsPolyG4 = 0;
};

struct BackGVramCacheEntry {
    const World* world = nullptr;
    u16 tpage = 0;
    u16 cba = 0;
    tTexture* texture = nullptr;
};

class BackG {
public:
	BackG();
	~BackG();

	static void InitBG();
	static void LoadBG();
	static void DeleteBG();
	static void DrawBG();

	s32 GetScrollY(s32 angle);
	s32 Draw();
	s32 DrawSprite(BackGGeoEntry* entry, s16 x, s16 y);
	s32 DrawPolyG4(BackGGeoEntry* entry, s16 x, s16 y);
	void UpdateSplat();
	void UpdateBG();
	void GetCamVect();

	BackGGeoEntry* field0 = nullptr;
	s32 field4 = 0;
	s32 field8 = 0;
	s32 fieldC = 0;
	u32 field10 = 0;
	s32 field14 = 0;
	s32 field18 = 0;
};
