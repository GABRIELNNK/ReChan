#include "gen/common.h"
#include "ai/thing.h"
#include "gen/blockmgr.h"
#include "gen/database.h"
#include "gen/model.h"
#include "gen/levelmgr.h"
#include "gen/time.h"
#include "p3d/hash.h"
#include "p3d/p3dmath.h"
#include "p3d/context.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include "pc/log.h"
#include <vector>

// Global Thing unique ID counter (PSX: gp+3868)
u16 Thing::s_nextUniqueID = 0;

static s32 LerpS32(s32 a, s32 b, f32 alpha) {
    f64 af = (f64)a;
    f64 bf = (f64)b;
    f64 t = (f64)alpha;
    return (s32)(af + (bf - af) * t);
}

#if INTERPOLATED_RENDERING
static s32 LerpAngle16(s32 a, s32 b, f32 alpha) {
    s32 a16 = (s16)(a & 0xFFFF);
    s32 b16 = (s16)(b & 0xFFFF);
    s32 delta = b16 - a16;

    if (delta > 32767) {
        delta -= 65536;
    }
    else if (delta < -32768) {
        delta += 65536;
    }

    return a16 + (s32)((f64)delta * (f64)alpha);
}

static bool IsRenderTeleport(const LVector& prevPos, const LVector& currPos) {
    constexpr s32 teleportThreshold = 4096;

    s32 dx = currPos.x - prevPos.x;
    s32 dy = currPos.y - prevPos.y;
    s32 dz = currPos.z - prevPos.z;

    if (dx < 0) {
        dx = -dx;
    }
    if (dy < 0) {
        dy = -dy;
    }
    if (dz < 0) {
        dz = -dz;
    }

    return (dx > teleportThreshold) || (dy > teleportThreshold) || (dz > teleportThreshold);
}
#endif

// ThingHandle - lazy-allocated safe reference (8 bytes on PSX)
struct ThingHandle {
    Thing* owner;
    u16 refCount;
};

// PSX: __6TicketP5ThingP12DynamicThing (THING.CPP:1312)
Ticket::Ticket(Thing* iss, DynamicThing* pass) {
    issuer = iss;
    passenger = pass;
}

// PSX: _._6Ticket (THING.CPP:1318)
Ticket::~Ticket() {
    issuer = nullptr;
    passenger = nullptr;
}

// PSX: __5ThingPC10tagLVectorUs (THING.CPP:428)
Thing::Thing(const LVector* initialPos, u16 type) {
    MARKFUNCTION(0x80061558);

    thingType = type;
    collisionRadius = INVALID_HANDLE;

    pos = *initialPos;
    orientation = {};

    stateCounter = 1;
    activeRadius = 1;
    initialActiveRadius = 1;

    thingHandle = nullptr;
    field76 = nullptr;
    model = nullptr;
    blockNum = BLOCK_UNASSIGNED;

    // Assign unique ID from global counter
    uniqueID = s_nextUniqueID;
    s_nextUniqueID++;

    // Initialize flags: set needs activation, clear activated and model created
    flags = TF_NEEDS_ACTIVATION;
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
// Sets model position/orientation from Thing fields, then calls model->Show
void Thing::Draw() {
    MARKFUNCTION(0x800616EC);

    LVector drawPos = pos;
    LVector drawOrient = orientation;

    if (model) {
        // PSX: copies pos/orientation to model, then calls Show
        Model* m = static_cast<Model*>(model);
        m->posX = drawPos.x;
        m->posY = drawPos.y;
        m->posZ = drawPos.z;
        m->rotX = (u16)(drawOrient.x & 0xFFFF);
        m->rotY = (u16)(drawOrient.y & 0xFFFF);
        m->rotZ = (u16)(drawOrient.z & 0xFFFF);
        m->Show(0);
        return;
    }

    // PC debug: draw wireframe box at thing position when no model is loaded
    // Box size: 300 wide, 768 tall (approximate humanoid collision box)
    f32 hw = 300.0f;  // half-width
    f32 hh = 768.0f;  // full height
    f32 hd = 300.0f;  // half-depth

    f32 cx = (f32)drawPos.x;
    f32 cy = (f32)drawPos.y;
    f32 cz = (f32)drawPos.z;

    f32 x0 = cx - hw, x1 = cx + hw;
    f32 y0 = cy, y1 = cy + hh;
    f32 z0 = cz - hd, z1 = cz + hd;

    // 12 edges of a box = 24 line vertices, 24 indices
    struct DV { f32 x, y, z, r, g, b; };
    DV verts[24];
    u16 indices[24];
    u32 vi = 0;

    // Color: yellow for player (type 1), red for others
    f32 cr = (thingType == AITypes::TT_PLAYER) ? 1.0f : 1.0f;
    f32 cg = (thingType == AITypes::TT_PLAYER) ? 1.0f : 0.3f;
    f32 cb = (thingType == AITypes::TT_PLAYER) ? 0.0f : 0.3f;

    // Helper macro to push a line
#define PUSHLINE(ax,ay,az,bx,by,bz) \
        indices[vi] = (u16)vi; verts[vi] = {ax,ay,az,cr,cg,cb}; vi++; \
        indices[vi] = (u16)vi; verts[vi] = {bx,by,bz,cr,cg,cb}; vi++;

    // Bottom face
    PUSHLINE(x0, y0, z0, x1, y0, z0);
    PUSHLINE(x1, y0, z0, x1, y0, z1);
    PUSHLINE(x1, y0, z1, x0, y0, z1);
    PUSHLINE(x0, y0, z1, x0, y0, z0);
    // Top face
    PUSHLINE(x0, y1, z0, x1, y1, z0);
    PUSHLINE(x1, y1, z0, x1, y1, z1);
    PUSHLINE(x1, y1, z1, x0, y1, z1);
    PUSHLINE(x0, y1, z1, x0, y1, z0);
    // Verticals
    PUSHLINE(x0, y0, z0, x0, y1, z0);
    PUSHLINE(x1, y0, z0, x1, y1, z0);
    PUSHLINE(x1, y0, z1, x1, y1, z1);
    PUSHLINE(x0, y0, z1, x0, y1, z1);
#undef PUSHLINE

    pddiPrimBufferDesc desc(PDDI_PRIM_LINES,
                            PDDI_V_POSITION | PDDI_V_COLOUR,
                            vi, vi);
    pddiPrimBuffer* buf = p3d::device->NewPrimBuffer(desc);
    buf->SetVertexData(verts, vi);
    buf->SetIndices(indices, vi);

    Mat4 identity;
    p3d::context->SetWorldMatrix(identity);
    p3d::context->SetVRAMHandle(0);
    p3d::context->DrawPrimBuffer(buf);
    buf->Release();
}

// PSX: Reset__5Thing (THING.CPP:502)
void Thing::Reset() {
    MARKFUNCTION(0x80061760);
    // PSX: flags |= 4 (needs activation)
    flags |= TF_NEEDS_ACTIVATION;
    // PSX: clear orientation
    orientation = {};
    // PSX: restore activeRadius from initialActiveRadius
    activeRadius = initialActiveRadius;
    // PSX: flags2 &= 1 (keep only bit 0)
    flags2 &= TF2_KILLED;

#if INTERPOLATED_RENDERING
    renderSnapshotInit = false;
#endif
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
        flags |= TF_ACTIVATED;
        // If model not yet created, create it
        if (!(flags & TF_MODEL_CREATED)) {
            CreateModel(nullptr);
        }
    }
}

// PSX: Deactivate__5Thing (THING.CPP:546)
void Thing::Deactivate() {
    MARKFUNCTION(0x8006182C);
    // PSX: clear activated flag, delete model
    flags &= ~TF_ACTIVATED;
    DeleteModel();
}

// PSX: Move__5Thing (THING.CPP:835)
// Base implementation does nothing (pure virtual-like)
void Thing::Move() {
    MARKFUNCTION(0x80061D60);
}

// PSX: CreateModel__5ThingPCc (THING.CPP:585, 0x800618E0)
// PSX: looks up model by hash in LevelManager, creates SModel/GModel/EModel
// based on type, links drawable to the OriginalSTree/OriginalGeo data.
void Thing::CreateModel(const char* name) {
    MARKFUNCTION(0x800618E0);

    if (!g_levelManager)
        return;

    // PSX: if name provided, hash it; else use field76 (nameHash from AnalyzeMesh)
    s32 modelHash;
    if (name) {
        modelHash = (s32)p3dHash(name);
    }
    else {
        modelHash = (s32)(uintptr_t)field76;
        if (!modelHash)
            return;
    }

    // PSX: FindModel__12LevelManagerl(theLevelMgr, hash)
    OriginalBasic* found = g_levelManager->FindModel(modelHash);
    if (!found) {
        LOG("[Thing::CreateModel] Model not found for hash 0x%08X", (u32)modelHash);
        flags |= TF_MODEL_CREATED;
        return;
    }

    // PSX: check type at OriginalBasic+16 (0=Geo, 1=STree, 2=ETree)
    u16 modelType = found->GetType();
    SModel* sm = static_cast<SModel*>(static_cast<Model*>(model));

    if (sm) {
        // Model already exists - just update the drawable
        if (modelType == 1) {
            sm->SetOriginalSTree(static_cast<OriginalSTree*>(found));
        }
    }
    else if (modelType == 1) {
        // Create new SModel for STree type
        sm = new SModel();
        sm->SetOriginalSTree(static_cast<OriginalSTree*>(found));
        model = sm;
    }

    if (model) {
        // PSX: copy the loaded model's nameHash to model+20
        Model* m = static_cast<Model*>(model);
        m->nameCRC = found->nameCRC;
        // PSX: set backPtr to this Thing
        m->backPtr = this;
    }

    // PSX: flags |= 0x50 (TF_MODEL_CREATED | TF_ACTIVATED)
    flags |= (TF_MODEL_CREATED | TF_ACTIVATED);
}

// PSX: DeleteModel__5Thing (THING.CPP:689)
void Thing::DeleteModel() {
    MARKFUNCTION(0x80061AAC);
    // PSX: calls model destructor through vtable: (*(model+8+8))(model, 3)
    if (model) {
        Model* m = static_cast<Model*>(model);
        delete m;
        model = nullptr;
    }
    flags &= ~TF_MODEL_CREATED;
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
    // PSX: *(a1+88) |= 1 - sets bit 0 of flags (offset 88)
    flags |= TF_DEAD;
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
    // Allocate ticket and link into subNode list
    Ticket* t = new Ticket(this, passenger);
    subNode.next = (ccMinNode*)t; // simplified: single-link for now
    passenger->ticket = t;
}

// PSX: RemPassenger__5ThingP6Ticket (THING.CPP:1104)
void Thing::RemPassenger(Ticket* t) {
    MARKFUNCTION(0x8006247C);
    if (!t) return;
    // Clear the passenger's ticket pointer
    if (t->passenger) {
        t->passenger->ticket = nullptr;
    }
    // Unlink from list
    t->Remove();
    delete t;
}

// PSX: RemAllPassengers__5Thing (THING.CPP:1144)
void Thing::RemAllPassengers() {
    MARKFUNCTION(0x80062504);
    // Iterate subNode list, remove each ticket
    ccMinNode* node = subNode.next;
    while (node) {
        ccMinNode* next = node->next;
        Ticket* t = static_cast<Ticket*>(node);
        if (t->passenger) {
            t->passenger->ticket = nullptr;
        }
        delete t;
        node = next;
    }
    subNode.next = nullptr;
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
    if (!model) {
        return;
    }

    Model* m = static_cast<Model*>(model);
    if (!m->field36) {
        return;
    }

    ModelFloorHeightState* floorState = static_cast<ModelFloorHeightState*>(m->field36);
    floorState->previous = floorState->current;
    floorState->current = (s32)0x80000001;
}

// PSX: SetFloorHeight__5Thingl (THING.CPP:777)
void Thing::SetFloorHeight(s32 height) {
    MARKFUNCTION(0x80061C38);
    if (!model) {
        return;
    }

    Model* m = static_cast<Model*>(model);
    if (!m->field36) {
        return;
    }

    ModelFloorHeightState* floorState = static_cast<ModelFloorHeightState*>(m->field36);
    if (height > floorState->current) {
        floorState->current = height;
    }
}

// PSX: GetObjectToWorldSpaceVector__5Thing (THING.CPP:1352)
void Thing::GetObjectToWorldSpaceVector(const SVector& in, SVector& out) {
    MARKFUNCTION(0x80062874);
    // PSX: reads orientation as u16 angles, builds YZX rotation matrix, transforms
    Mat4 rot;
    p3dBuildRotMatrixYZX(orientation.x, orientation.y, orientation.z, rot);

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
    flags |= TF_DYNAMIC;

    // PSX: homePos = initialPos
    homePos = *initialPos;

    // PSX: groundStandHeight = 0
    groundStandHeight = 0;
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
    gravity = FIX16_HALF;

    // Clear all movement vectors
    velocity = {};
    force = {};
    contactForce = {};

    // Reset home position to current position
    homePos = pos;

    maxFallDivisor = 10;

    // PSX: groundStandHeight = 0
    groundStandHeight = 0;
    Disembark();
}

// PSX: Move__12DynamicThing (THING.CPP:891)
// PSX: 0x80061EC4, 1340 bytes.
// Full physics step: subtracts force from velocity, applies gravity friction,
// distributes contact forces over stateCounter frames, half-step integration,
// clamps XZ speed to health, damps XZ force by g_dampingFactor.
void DynamicThing::Move() {
    MARKFUNCTION(0x80061EC4);

    LVector localForce = {};
    LVector movement = {};

    // Step 1: subtract accumulated force from velocity
    velocity.x -= force.x;
    velocity.y -= force.y;
    velocity.z -= force.z;

    // Step 2: compute signs for velocity and contact force directions
    s32 sign_vx = (velocity.x >= 0) ? 1 : -1;
    s32 sign_vz = (velocity.z >= 0) ? 1 : -1;
    s32 sign_cfx = (contactForce.x >= 0) ? 1 : -1;
    s32 sign_cfz = (contactForce.z >= 0) ? 1 : -1;

    // Step 3: gravity friction (drag opposing velocity)
    // PSX: abs(vel) * gravity → 64-bit, extract bits 16..47, negate sign
    s32 abs_vx = velocity.x * sign_vx;
    s32 abs_vz = velocity.z * sign_vz;

    s32 drag_x = (s32)(((s64)abs_vx * (s64)gravity) >> 16);
    s32 drag_z = (s32)(((s64)abs_vz * (s64)gravity) >> 16);

    s32 friction_x = (-sign_vx) * drag_x;
    s32 friction_z = (-sign_vz) * drag_z;

    // Reset gravity to default (overridden each frame by ground contact)
    gravity = FIX16_HALF;

    // Step 4: contact force handling (if stateCounter != 0)
    if (stateCounter != 0) {
        // X axis
        if (contactForce.x != 0) {
            s32 abs_cfx = contactForce.x * sign_cfx;
            s32 divided = rmDiv16i(abs_cfx, stateCounter) >> 16;
            localForce.x = (divided * sign_cfx) + friction_x;
        }
        else {
            s32 av = (velocity.x >= 0) ? velocity.x : -velocity.x;
            if (av < 2) {
                velocity.x = 0;
            }
            else if (friction_x != 0) {
                localForce.x = friction_x;
            }
        }

        // Y axis (plain integer division, no rmDiv16i)
        if (contactForce.y != 0) {
            localForce.y = contactForce.y / stateCounter;
        }

        // Z axis
        if (contactForce.z != 0) {
            s32 abs_cfz = contactForce.z * sign_cfz;
            s32 divided = rmDiv16i(abs_cfz, stateCounter) >> 16;
            localForce.z = (divided * sign_cfz) + friction_z;
        }
        else {
            s32 av = (velocity.z >= 0) ? velocity.z : -velocity.z;
            if (av < 2) {
                velocity.z = 0;
            }
            else if (friction_z != 0) {
                localForce.z = friction_z;
            }
        }

        // Y gravity fall (maxFallDivisor accumulates downward acceleration)
        if (maxFallDivisor != 0) {
            localForce.y -= maxFallDivisor;
            if (localForce.y < -g_maxFallSpeed) {
                localForce.y = -g_maxFallSpeed;
            }
        }
        maxFallDivisor = 18;
    }

    // Step 5: half-step integration
    // PSX pattern: make positive, (n + (unsigned(n) >> 31)) >> 1, restore sign
    s32 sign_lfx = (localForce.x >= 0) ? 1 : -1;
    s32 sign_lfz = (localForce.z >= 0) ? 1 : -1;

    s32 abs_lfx = localForce.x * sign_lfx;
    s32 abs_lfz = localForce.z * sign_lfz;

    s32 half_lfx = (s32)(((u32)abs_lfx + ((u32)abs_lfx >> 31)) >> 1) * sign_lfx;
    s32 half_lfy = (localForce.y + (s32)((u32)localForce.y >> 31)) >> 1;
    s32 half_lfz = (s32)(((u32)abs_lfz + ((u32)abs_lfz >> 31)) >> 1) * sign_lfz;

    movement.x = velocity.x + half_lfx;
    movement.y = velocity.y + half_lfy;
    movement.z = velocity.z + half_lfz;

    // Step 6: clamp movement magnitude to health (XZ only, Y untouched)
    s32 mag = (s32)rmMag3((f32)movement.x, (f32)movement.y, (f32)movement.z);
    if (health < mag) {
        LVector normalized;
        rmV3Normalize(&normalized, &movement);
        movement.x = (s32)(((s64)normalized.x * health) >> 16);
        // movement.y intentionally NOT clamped
        movement.z = (s32)(((s64)normalized.z * health) >> 16);
    }

    // Step 7: add accumulated force back to movement
    movement.x += force.x;
    movement.y += force.y;
    movement.z += force.z;

    // Step 8: update homePos
    homePos.x += movement.x;
    homePos.y += movement.y;
    homePos.z += movement.z;

    // Step 9: update velocity with localForce
    velocity.x += localForce.x;
    velocity.y += localForce.y;
    velocity.z += localForce.z;

    // Step 10: damp XZ force and add to velocity
    // PSX: (g_dampingFactor * force) >> 16, stored back to force, then added to velocity
    s32 damped_fx = (s32)(((s64)g_dampingFactor * (s64)force.x) >> 16);
    s32 damped_fz = (s32)(((s64)g_dampingFactor * (s64)force.z) >> 16);

    force.x = damped_fx;
    force.z = damped_fz;

    velocity.x += damped_fx;
    velocity.y += force.y;  // Y force is NOT damped
    velocity.z += damped_fz;

    // Step 11: save contactForce to field148[0..2], clear contactForce
    field148[0] = contactForce.x;
    field148[1] = contactForce.y;
    field148[2] = contactForce.z;
    contactForce = {};
}

// PSX: UpdatePosition__12DynamicThing (THING.CPP:1280)
// PSX: 196 bytes. Computes displacement, updates block membership,
// then commits homePos to pos.
void DynamicThing::UpdatePosition() {
    MARKFUNCTION(0x8006272C);

    // Compute displacement (homePos - pos)
    field148[3] = homePos.x - pos.x;
    field148[4] = homePos.y - pos.y;
    field148[5] = homePos.z - pos.z;

    // Update block membership if activated
    if (flags & TF_ACTIVATED) {
        if (g_blockManager) {
            u16 newBlock = g_blockManager->GetBlockNumber(homePos);
            if (newBlock != BLOCK_UNASSIGNED) {
                blockNum = newBlock;
            }
            else if (!(flags & TF_BIT5)) {
                // Left all blocks and bit5 not set: deactivate
                flags &= ~TF_ACTIVATED;
            }
        }
    }

    // Commit homePos to pos
    pos = homePos;
}

// PSX: AddForce__12DynamicThinglPC9_RMVECT16 (THING.CPP:753, 0x80061B44)
// PSX: builds rotation matrix from SVector (Euler angles), rotates {0,0,magnitude}
// to get world-space force, adds to contactForce.
// direction is packed as int32[3] on PSX stack; as s16*: [rotX, 0, rotY, 0, rotZ, 0]
// p3dBuildRotMatrixZYX uses s16[0]=rotX, s16[2]=rotY, s16[4]=rotZ
// For all known callers, only rotY (stored at SVector.z) is non-zero.
void DynamicThing::AddForce(s32 magnitude, const SVector* direction) {
    MARKFUNCTION(0x80061B44);
    if (!direction) return;

    // PSX: p3dBuildRotMatrixZYX(*a3, a3[2], a3[4], matrix)
    // a3[0] = SVector.x = rotX around X axis
    // a3[2] = SVector.z = rotY around Y axis
    // a3[4] = beyond SVector = rotZ around Z axis (typically 0)
    // Then rotates {0, 0, magnitude} by this matrix and adds to contactForce.
    s16 rotY = direction->z;
    f32 angle = ANGLE2RAD(rotY);
    f32 sinA = std::sin(angle);
    f32 cosA = std::cos(angle);

    // {0, 0, magnitude} rotated by Y-axis: x = sin * mag, z = cos * mag
    contactForce.x += (s32)(sinA * (f32)magnitude);
    contactForce.z += (s32)(cosA * (f32)magnitude);
}

// PSX: Land__12DynamicThing (THING.CPP:794)
void DynamicThing::Land() {
    MARKFUNCTION(0x80061C78);
    flags |= TF_ON_GROUND;
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
    if (ticket) {
        if (ticket->issuer) {
            ticket->issuer->RemPassenger(ticket);
        }
    }
    ticket = nullptr;
}

// PSX: GetTicketIssuer__12DynamicThing (THING.CPP:1162)
Thing* DynamicThing::GetTicketIssuer() {
    MARKFUNCTION(0x80062550);
    if (ticket) {
        return ticket->issuer;
    }
    return nullptr;
}

// PSX: HandleLand__12DynamicThingl (THING.CPP:1360)
void DynamicThing::HandleLand(s32 /*height*/) {
    MARKFUNCTION(0x800628C8);
}

// PSX: DistanceFromPointXZ__C5ThingRC10tagLVector (THING.CPP:1180, 0x800625C0)
s32 Thing::DistanceFromPointXZ(const LVector& point) const {
    MARKFUNCTION(0x800625C0);
    return rmMag2ff(pos.x - point.x, pos.z - point.z);
}

// PSX: DistanceFromPoint__C5ThingRC10tagLVector (THING.CPP:1189, 0x800625F4)
s32 Thing::DistanceFromPoint(const LVector& point) const {
    MARKFUNCTION(0x800625F4);
    return rmMag3ff(pos.x - point.x, pos.y - point.y, pos.z - point.z);
}
