#include "ai/simplbox.h"
#include "gen/database.h"

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
