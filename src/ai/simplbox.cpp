// simplbox.cpp - SimpleBox implementation
// Reversed from PSX C:\CHAN\GAME\SRC\AI\SIMPLBOX.CPP
#include "ai/simplbox.h"
#include "gen/database.h"

// PSX: SetBox__9SimpleBoxP8DBVolume (SIMPLBOX.CPP:31)
void SimpleBox::SetBox(const DBVolume* vol) {
    if (vol) {
        minX = vol->bboxMin.x;
        minY = vol->bboxMin.y;
        minZ = vol->bboxMin.z;
        maxX = vol->bboxMax.x;
        maxY = vol->bboxMax.y;
        maxZ = vol->bboxMax.z;
    }
}
