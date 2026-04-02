// thing.cpp - Thing and DynamicThing implementations
// Reversed from PSX C:\CHAN\GAME\SRC\AI\THING.CPP
#include "ai/thing.h"
#include "gen/blockmgr.h"
#include "gen/database.h"
#include "p3d/hash.h"
#include "p3d/p3dmath.h"

// Global Thing unique ID counter (PSX: gp+3868)
u16 Thing::s_nextUniqueID = 0;

// Global BlockManager pointer (defined in world.cpp)
extern BlockManager* g_blockManager;

// ThingHandle - lazy-allocated safe reference (8 bytes on PSX)
struct ThingHandle {
    Thing* owner;
    u16 refCount;
};

// PSX: __5ThingPC10tagLVectorUs (THING.CPP:428)
Thing::Thing(const LVector* initialPos, u16 type) {
    MARKFUNCTION(0x80061558);

    thingType = type;
    collisionRadius = 0xFFFF;

    pos = *initialPos;
    orientation = {};

    stateCounter = 1;
    activeRadius = 1;
    initialActiveRadius = 1;

    thingHandle = nullptr;
    field76 = nullptr;
    model = nullptr;
    blockNum = 0x1000;

    // Assign unique ID from global counter
    uniqueID = s_nextUniqueID;
    s_nextUniqueID++;

    // Initialize flags: set bit 2 (needs activation), clear bit 4 and 6
    flags = 0x0004;   // |= 4, &= ~0x10, &= ~0x40
    flags2 = 0;
}

// PSX: _._5Thing (THING.CPP:458)
Thing::~Thing() {
    MARKFUNCTION(0x80061640);
    DeleteModel();
    RemAllPassengers();
    if (thingHandle) {
        // PSX: Close__11ThingHandle - clear the handle's back-pointer
        thingHandle = nullptr;
    }
}

// PSX: Think__5Thing (THING.CPP:478)
// Calls UpdatePosition() through virtual dispatch
void Thing::Think() {
    MARKFUNCTION(0x800616BC);
    UpdatePosition();
}

// PSX: Draw__5Thing (THING.CPP:487)
// Sets model position/orientation from Thing fields, then calls model->Display
void Thing::Draw() {
    MARKFUNCTION(0x800616EC);
    // PSX: reads model at +80, sets position at model+64/+68/+72
    //       sets orientation at model+52/+56/+60
    //       calls model->Display(0) via vtable
    // PC: model rendering will be implemented when character models are loaded
    if (!model) return;
    // TODO: set model transform from pos/orientation and call Display
}

// PSX: Reset__5Thing (THING.CPP:502)
void Thing::Reset() {
    MARKFUNCTION(0x80061760);
    // PSX: flags |= 4 (needs activation)
    flags |= 0x0004;
    // PSX: clear orientation
    orientation = {};
    // PSX: restore activeRadius from initialActiveRadius
    activeRadius = initialActiveRadius;
    // PSX: flags2 &= 1 (keep only bit 0)
    flags2 &= 0x0001;
}

// PSX: UpdatePosition__5Thing (THING.HPP:440)
// Base implementation does nothing
void Thing::UpdatePosition() {
    MARKFUNCTION(0x800628F4);
}

// PSX: Activate__5Thing (THING.CPP:521)
void Thing::Activate() {
    MARKFUNCTION(0x80061790);
    // PSX: check if block is in active list via BlockManager
    bool inActiveList = false;
    if (g_blockManager) {
        // TODO: InActiveList not yet reversed - use IsValidBlockNumber as fallback
        inActiveList = g_blockManager->IsValidBlockNumber(blockNum);
    }

    if (inActiveList) {
        // Mark as activated
        flags |= 0x0010;
        // If model not yet created (bit 6 not set), create it
        if (!(flags & 0x0040)) {
            CreateModel(nullptr);
        }
    }
}

// PSX: Deactivate__5Thing (THING.CPP:546)
void Thing::Deactivate() {
    MARKFUNCTION(0x8006182C);
    // PSX: clear activated flag, delete model
    flags &= ~0x0010;
    DeleteModel();
}

// PSX: Move__5Thing (THING.CPP:835)
// Base implementation does nothing (pure virtual-like)
void Thing::Move() {
    MARKFUNCTION(0x80061D60);
}

// PSX: CreateModel__5ThingPCc (THING.CPP:585)
void Thing::CreateModel(const char* /*name*/) {
    MARKFUNCTION(0x800618E0);
    // PSX: loads model from CharacterManager / resource system
    // PC: character model loading not yet implemented
    // Mark model as created
    flags |= 0x0040;
}

// PSX: DeleteModel__5Thing (THING.CPP:689)
void Thing::DeleteModel() {
    MARKFUNCTION(0x80061AAC);
    // PSX: releases model reference
    if (model) {
        model = nullptr;
    }
    flags &= ~0x0040;
}

// PSX: HandleCollision__5ThingP5Thingle (THING.CPP:713)
void Thing::HandleCollision(Thing* /*other*/, s32 /*damage*/) {
    MARKFUNCTION(0x80061B08);
    // Base does nothing
}

// PSX: AnalyzeMesh__5ThingP6DBRoot (THING.CPP:1224)
void Thing::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80062680);
    if (!root)
        return;

    // PSX: FindAttrib(root, 5) - attrib 5 = mesh name hash
    const DBAttrib* a5 = root->FindAttrib(5);
    if (a5) {
        if (a5->type == 0) {
            // String attribute - hash it
            const char* str = a5->strValue ? a5->strValue : "";
            field76 = (void*)(uintptr_t)p3dHash(str);
        }
        else if (a5->type == 1) {
            // Numeric attribute - use directly
            field76 = (void*)(uintptr_t)a5->value;
        }
    }

    // PSX: FindAttrib(root, 15) - attrib 15 = block number
    const DBAttrib* a15 = root->FindAttrib(15);
    if (a15) {
        blockNum = (u16)a15->value;
    }
}

// PSX: GetViewSpot__5ThingP10tagLVectorT1 (THING.CPP:1210)
void Thing::GetViewSpot(LVector* outPos, LVector* /*outTarget*/) {
    MARKFUNCTION(0x80062638);
    if (outPos) {
        *outPos = pos;
    }
}

// PSX: Kill__5Thing (THING.HPP:518)
void Thing::Kill() {
    MARKFUNCTION(0x800628D0);
    // Mark for removal
    flags2 |= 0x0001;
}

// PSX: GetSoundPosPtr__5Thing (THING.HPP:516)
LVector* Thing::GetSoundPosPtr() {
    MARKFUNCTION(0x800628E4);
    return &pos;
}

// PSX: GetInitialPos__5Thing (THING.HPP:512)
const LVector* Thing::GetInitialPos() {
    MARKFUNCTION(0x800628EC);
    return &pos;
}

// PSX: AddPassenger__5ThingP12DynamicThing (THING.CPP:1079)
void Thing::AddPassenger(DynamicThing* passenger) {
    MARKFUNCTION(0x80062400);
    if (!passenger)
        return;
    if (passenger->ticket)
        return;
    // PSX: allocates Ticket, links into subNode list, sets passenger->ticket
    // Ticket system not yet reversed - store basic association
    passenger->standingOn = this;
}

// PSX: RemPassenger__5ThingP6Ticket (THING.CPP:1104)
void Thing::RemPassenger(Ticket* /*ticket*/) {
    MARKFUNCTION(0x8006247C);
    // PSX: removes ticket from list, clears passenger->ticket, deletes ticket
}

// PSX: RemAllPassengers__5Thing (THING.CPP:1144)
void Thing::RemAllPassengers() {
    MARKFUNCTION(0x80062504);
    // PSX: iterates passenger list, calls RemPassenger on each
}

// PSX: GetThingHandle__5Thing (THING.CPP:1170)
u32 Thing::GetThingHandle() {
    MARKFUNCTION(0x80062574);
    // PSX: lazily allocates ThingHandle (8 bytes) with owner=this, refCount=1
    if (!thingHandle) {
        ThingHandle* h = new ThingHandle();
        h->owner = this;
        h->refCount = 1;
        thingHandle = h;
    }
    return uniqueID;
}

// PSX: ClearFloorHeight__5Thing (THING.CPP:765)
void Thing::ClearFloorHeight() {
    MARKFUNCTION(0x80061BFC);
    // PSX: navigates model->+36 (floor tracking sub-object)
    // Copies current floor to prev, resets current to min sentinel
    // Requires model type to be fully reversed - no-op until then
}

// PSX: SetFloorHeight__5Thingl (THING.CPP:777)
void Thing::SetFloorHeight(s32 /*height*/) {
    MARKFUNCTION(0x80061C38);
    // PSX: sets floor height on model->+36 sub-object if higher
    // "highest floor wins" for collision
    // Requires model type to be fully reversed - no-op until then
}

// PSX: GetObjectToWorldSpaceVector__5Thing (THING.CPP:1352)
void Thing::GetObjectToWorldSpaceVector(const SVector& in, SVector& out) {
    MARKFUNCTION(0x80062874);
    // PSX: reads orientation as u16 angles, builds YZX rotation matrix, transforms
    Mat4 rot;
    p3dBuildRotMatrixYZX(orientation.x, orientation.z, orientation.y, rot);

    Vec3 v((f32)in.x, (f32)in.y, (f32)in.z);
    Vec3 result = p3dVecTimesRotMatrix(v, rot);
    out.x = (s16)result.x;
    out.y = (s16)result.y;
    out.z = (s16)result.z;
    out.pad = 0;
}

// PSX: __12DynamicThingPC10tagLVectorUs (THING.CPP:840)
DynamicThing::DynamicThing(const LVector* initialPos, u16 type)
    : Thing(initialPos, type) {
    MARKFUNCTION(0x80061D68);

    // PSX: flags |= 0x0800 (mark as dynamic)
    flags |= 0x0800;

    // PSX: homePos = initialPos
    homePos = *initialPos;

    // PSX: standingOn = global default obstacle
    standingOn = nullptr;
    ticket = nullptr;
}

// PSX: _._12DynamicThing (THING.CPP:851)
DynamicThing::~DynamicThing() {
    MARKFUNCTION(0x80061DE0);
    if (ticket) {
        Disembark();
        ticket = nullptr;
    }
}

// PSX: Reset__12DynamicThing (THING.CPP:865)
void DynamicThing::Reset() {
    MARKFUNCTION(0x80061E38);
    Thing::Reset();

    health = 100;
    gravity = 0x8000;

    // Clear all movement vectors
    velocity = {};
    force = {};
    contactForce = {};

    // Reset home position to current position
    homePos = pos;

    maxFallDivisor = 10;

    // PSX: standingOn = global default obstacle
    standingOn = nullptr;
    Disembark();
}

// PSX: Move__12DynamicThing (THING.CPP:891)
// Physics simulation: applies velocity, force, gravity
void DynamicThing::Move() {
    MARKFUNCTION(0x80061EC4);

    // PSX: velocity -= force (drag/friction)
    velocity.x -= force.x;
    velocity.y -= force.y;
    velocity.z -= force.z;

    // PSX: clear force after applying
    force = {};

    // PSX: apply gravity to Y velocity if airborne
    if (!(flags & 0x1000)) { // not on ground
        velocity.y -= gravity;
        // PSX: clamp fall speed
        if (maxFallDivisor > 0) {
            s32 maxFall = -0x8000 / maxFallDivisor;
            if (velocity.y < maxFall)
                velocity.y = maxFall;
        }
    }

    // PSX: apply contact force
    velocity.x += contactForce.x;
    velocity.y += contactForce.y;
    velocity.z += contactForce.z;
    contactForce = {};

    // PSX: update position from velocity
    // PSX uses >>12 fixed-point shift for velocity-to-position
    pos.x += velocity.x;
    pos.y += velocity.y;
    pos.z += velocity.z;

    // PSX: clear on-ground flag (will be re-set by collision)
    flags &= ~0x1000;
}

// PSX: UpdatePosition__12DynamicThing (THING.CPP:1280)
void DynamicThing::UpdatePosition() {
    MARKFUNCTION(0x8006272C);
    Move();
}

// PSX: AddForce__12DynamicThinglPC9_RMVECT16 (THING.CPP:753)
void DynamicThing::AddForce(s32 magnitude, const SVector* direction) {
    MARKFUNCTION(0x80061B44);
    if (!direction) return;
    // PSX: scale direction by magnitude (fixed-point multiply)
    force.x += (direction->x * magnitude) >> 12;
    force.y += (direction->y * magnitude) >> 12;
    force.z += (direction->z * magnitude) >> 12;
}

// PSX: Land__12DynamicThing (THING.CPP:794)
void DynamicThing::Land() {
    MARKFUNCTION(0x80061C78);
    flags |= 0x1000; // on ground
    velocity.y = 0;
}

// PSX: DisembarkObstacle__12DynamicThingRC10tagLVector (THING.CPP:810)
void DynamicThing::DisembarkObstacle(const LVector& newPos) {
    MARKFUNCTION(0x80061CC4);
    pos = newPos;
    Disembark();
}

// PSX: Disembark__12DynamicThing (THING.CPP:1126)
void DynamicThing::Disembark() {
    MARKFUNCTION(0x800624C4);
    if (standingOn && ticket) {
        standingOn->RemPassenger(ticket);
    }
    ticket = nullptr;
}

// PSX: GetTicketIssuer__12DynamicThing (THING.CPP:1162)
Thing* DynamicThing::GetTicketIssuer() {
    MARKFUNCTION(0x80062550);
    return standingOn;
}

// PSX: HandleLand__12DynamicThingl (THING.CPP:1360)
void DynamicThing::HandleLand(s32 /*height*/) {
    MARKFUNCTION(0x800628C8);
    Land();
}
