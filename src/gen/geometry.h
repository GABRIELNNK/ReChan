#pragma once
#include "core.h"

// Reads PSX tPrimGeom data from BLK stream entries and produces
// pddiPrimBuffer objects for rendering through the pddi abstraction.

class pddiPrimBuffer;

// Parse tPrimGeom data into a pddiPrimBuffer for PC rendering.
// pg points to the tPrimGeom base (BLK entry + 24), pgSize is bytes remaining.
// PC equivalent of LoadPrim__5BlockPv pointer fixups + RP render pipeline.
pddiPrimBuffer* ParseBLKPrims(const u8* pg, u32 pgSize);
