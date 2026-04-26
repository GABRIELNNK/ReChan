#include "ai/activezn.h"
#include "ai/humanoid.h"
#include "ai/thing.h"
#include "gen/database.h"
#include "gen/path.h"

// SubZoneVolume

// PSX: _13SubZoneVolumeP8DBVolume (ACTIVEZN.CPP:170)
SubZoneVolume::SubZoneVolume(DBVolume* vol) {
    SetName(vol->GetName(), 0);
    box.SetBox(vol);
}

// PSX: IsInSubZoneVolume__13SubZoneVolumeP5Thing (ACTIVEZN.CPP:183)
bool SubZoneVolume::IsInSubZoneVolume(Thing* thing) const {
    return box.IsInside(thing->pos);
}

// ActiveZone

// PSX: _10ActiveZoneP8DBVolumeUl (ACTIVEZN.CPP:143)
ActiveZone::ActiveZone(DBVolume* vol) {
    SetName(vol->GetName(), 0);

    overlordType = 0;
    overlordValue = 0;
    overlordID = 0;
    memberCount = 0;
    members[0] = nullptr;
    members[1] = nullptr;
    members[2] = nullptr;

    // PSX: attrib 6 = overlord type
    const DBAttrib* a6 = vol->FindAttrib(6);
    if (a6) {
        u8 val = (u8)a6->value;
        overlordValue = val;
        if ((u8)(val - 1) < 3) {
            overlordType = 1;
            // PSX: attrib 7 = overlord ID
            const DBAttrib* a7 = vol->FindAttrib(7);
            if (a7)
                overlordID = a7->value;
            else
                overlordID = 0;
        }
        else {
            overlordValue = 0;
        }
    }

    // PSX: attrib 9 = special flag
    specialFlag = 0;
    if (vol->FindAttrib(9))
        specialFlag = 1;

    box.SetBox(vol);
}

// PSX: __10ActiveZone (ACTIVEZN.CPP:380)
ActiveZone::~ActiveZone() {
    // Drain and delete paths
    ccMinNode* n;
    while ((n = pathList.RemHead()) != nullptr) {
        delete n;
    }
    // subZoneList drains via Purge (no delete - ccNode dtor chain)
}

// PSX: AddLinearPath__10ActiveZoneR10LinearPath (ACTIVEZN.CPP:505)
void ActiveZone::AddLinearPath(LinearPath* path) {
    pathList.AddNodeTail(path);
}

// PSX: AddSubZoneVolume__10ActiveZoneR13SubZoneVolume (ACTIVEZN.CPP:512)
void ActiveZone::AddSubZoneVolume(SubZoneVolume* szv) {
    subZoneList.AddNodeTail(szv);
}

// PSX: AddHumanoidToOverlordMembers__10ActiveZoneP8Humanoid (ACTIVEZN.CPP:390)
void ActiveZone::AddHumanoidToOverlordMembers(Humanoid* h) {
    if (memberCount >= 3)
        return;
    memberCount++;
    for (int i = 0; i < 3; i++) {
        if (!members[i]) {
            members[i] = h;
            return;
        }
    }
}

// PSX: RemoveHumanoidFromOverlordMembers__10ActiveZoneP8Humanoid (ACTIVEZN.CPP:417)
void ActiveZone::RemoveHumanoidFromOverlordMembers(Humanoid* h) {
    for (int i = 0; i < 3; i++) {
        if (members[i] == h) {
            members[i] = nullptr;
            memberCount--;
            return;
        }
    }
}

// PSX: GetNumberOfThinkingMembers__10ActiveZone (ACTIVEZN.CPP:433, 0x800A6E30)
s32 ActiveZone::GetNumberOfThinkingMembers() const {
    MARKFUNCTION(0x800A6E30);

    s32 thinkingCount = 0;
    for (s32 index = 0; index < 3; index++) {
        Humanoid* member = members[index];
        if (member && (member->flags & TF_ACTIVATED) != 0) {
            thinkingCount++;
        }
    }

    return thinkingCount;
}

// PSX: IsInActiveZone__10ActiveZoneP5Thing (ACTIVEZN.CPP:680)
bool ActiveZone::IsInActiveZone(Thing* thing) const {
    return box.IsInside(thing->pos);
}

// PSX: IsPathNodeTerminator__10ActiveZoneP10LinearPathl (ACTIVEZN.CPP:1318)
bool ActiveZone::IsPathNodeTerminator(LinearPath* path, s32 nodeIndex) const {
    MARKFUNCTION(0x800A76AC);

    if (!path || !path->nodeAttribs || nodeIndex < 0 || nodeIndex >= path->numPoints) {
        return false;
    }

    const NodeAttribs& attribs = path->nodeAttribs[nodeIndex];
    for (s32 index = 0; index < attribs.count; index++) {
        if (attribs.ids[index] == 'W') {
            return true;
        }
    }

    return false;
}

// PSX: AllowBreakoffOfDestinationNode__10ActiveZoneP10LinearPathl (ACTIVEZN.CPP:1352)
bool ActiveZone::AllowBreakoffOfDestinationNode(LinearPath* path, s32 nodeIndex) const {
    MARKFUNCTION(0x800A7718);

    if (!path || !path->nodeAttribs || nodeIndex < 0 || nodeIndex >= path->numPoints) {
        return false;
    }

    const NodeAttribs& attribs = path->nodeAttribs[nodeIndex];
    for (s32 index = 0; index < attribs.count; index++) {
        if (attribs.ids[index] == 'X') {
            return true;
        }
    }

    return false;
}
