// activezn.h - ActiveZone and SubZoneVolume classes
// Reversed from PSX C:\CHAN\GAME\SRC\AI\ACTIVEZN.CPP
#pragma once

#include "gen/cclist.h"
#include "ai/simplbox.h"

class Humanoid;
class LinearPath;
struct DBVolume;

// SubZoneVolume (48 bytes) - ccNode(24) + SimpleBox(24)
// PSX source: C:\CHAN\GAME\SRC\AI\ACTIVEZN.CPP:176
class SubZoneVolume : public ccNode {
public:
    SimpleBox box;  // +24

    // PSX: _13SubZoneVolumeP8DBVolume (ACTIVEZN.CPP:170)
    SubZoneVolume(DBVolume* vol);

    // PSX: _._13SubZoneVolume (ACTIVEZN.CPP:176)
    ~SubZoneVolume() override = default;

    // PSX: IsInSubZoneVolume__13SubZoneVolumeP5Thing (ACTIVEZN.CPP:183)
    bool IsInSubZoneVolume(class Thing* thing) const;
};

// ActiveZone (104 bytes) - manages an area with overlord AI, paths, and sub-zones
// PSX layout:
//   +0:  ccNode base (24 bytes)
//   +24: SimpleBox (24 bytes)
//   +48: pathList (ccList, 12 bytes) - LinearPath entries
//   +60: subZoneList (ccList, 12 bytes) - SubZoneVolume entries
//   +72: overlordType (s32)
//   +76: overlordValue (u8)
//   +80: overlordID (s32)
//   +84: memberCount (u8)
//   +88: members[3] (Humanoid* x3 = 12 bytes)
//   +100: specialFlag (s32)
// PSX source: C:\CHAN\GAME\SRC\AI\ACTIVEZN.CPP
class ActiveZone : public ccNode {
public:
    SimpleBox box;          // +24
    ccList pathList;        // +48: LinearPath nodes
    ccList subZoneList;     // +60: SubZoneVolume nodes
    s32 overlordType = 0;   // +72
    u8 overlordValue = 0;   // +76
    s32 overlordID = 0;     // +80
    u8 memberCount = 0;     // +84
    Humanoid* members[3] = {};  // +88
    s32 specialFlag = 0;    // +100

    // PSX: _10ActiveZoneP8DBVolumeUl (ACTIVEZN.CPP:143)
    ActiveZone(DBVolume* vol);

    // PSX: __10ActiveZone (ACTIVEZN.CPP:380)
    ~ActiveZone() override;

    // PSX: AddLinearPath__10ActiveZoneR10LinearPath (ACTIVEZN.CPP:505)
    void AddLinearPath(LinearPath* path);

    // PSX: AddSubZoneVolume__10ActiveZoneR13SubZoneVolume (ACTIVEZN.CPP:512)
    void AddSubZoneVolume(SubZoneVolume* szv);

    // PSX: AddHumanoidToOverlordMembers__10ActiveZoneP8Humanoid (ACTIVEZN.CPP:390)
    void AddHumanoidToOverlordMembers(Humanoid* h);

    // PSX: RemoveHumanoidFromOverlordMembers__10ActiveZoneP8Humanoid (ACTIVEZN.CPP:417)
    void RemoveHumanoidFromOverlordMembers(Humanoid* h);

    // PSX: IsInActiveZone__10ActiveZoneP5Thing (ACTIVEZN.CPP:680)
    bool IsInActiveZone(class Thing* thing) const;
};
