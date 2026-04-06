// thing.h - Thing and DynamicThing base classes
// Reversed from PSX C:\CHAN\GAME\INC\AI\THING.HPP / C:\CHAN\GAME\SRC\AI\THING.CPP
#pragma once

#include "core.h"
#include "p3d/lvector.h"
#include "gen/cclist.h"

struct DBRoot;
class BlockManager;
struct Ticket;

// Block number sentinel (unassigned)
static constexpr u16 BLOCK_UNASSIGNED = 0x1000;

// PSX AI:: scope enums (namespace renamed to avoid class AI conflict)
namespace AITypes {

    // ThingTypes - entity type IDs dispatched by AI::AddThingNoTagList
    // PSX: enum AI::ThingTypes (used for CharacterManager, etc.)
    // Values match PSX AddThingNoTagList dispatch at 0x80054404
    enum ThingTypes : u16 {
        TT_PLAYER = 0,
        // Humanoid range: 1-28 (generic thugs + named bosses)
        TT_HUMANOID_FIRST = 1,
        TT_THUG1 = 1,
        TT_THUG2 = 2,
        TT_THUG3 = 3,
        TT_THUG4 = 4,
        TT_THUG5 = 5,
        TT_THUG6 = 6,
        TT_THUG7 = 7,
        TT_THUG8 = 8,
        TT_GRONTAR = 10,   // Grontar class (616 bytes)
        TT_PAUL = 12,   // Paul class (616 bytes)
        TT_OSCAR = 13,   // Oscar class (616 bytes)
        TT_DANTE = 15,   // Dante class (684 bytes)
        TT_BUTCH = 17,   // Butch class (620 bytes)
        TT_HUMANOID_LAST = 28,
        // Collectible (Pickup class, goes to pickupList)
        TT_COLLECTIBLE = 101,
        // Platform (goes to moveList)
        TT_PLATFORM = 201,
        // Obstacle range: 301-328 (Pickup class, goes to pickupList)
        TT_OBSTACLE_FIRST = 301,
        TT_OBSTACLE_LAST = 328,
        // Interactive objects (go to moveList)
        TT_LAUNCHER = 402,
        TT_CONVEYOR = 404,
        TT_SLIPPERYFLOOR = 405,
        TT_HORIZONTALPOLE = 407,
        TT_EXPLOSIVE = 410,
        TT_DESTRUCTIBLE = 412,
        TT_CRUSHER = 413,
        TT_KNOCKDOWN = 420,
        TT_STACK = 421,
        TT_KICKNROLL = 422,
        TT_DESTRUCTIBLE_DP = 424,  // with dead pool check
        TT_UNTOUCHABLE = 435,
        TT_COLLECTIBLE_OBJ = 436,
        TT_TRIGGERTHING = 451,
        TT_GENERATOR = 455,
        TT_ENEMYGENERATOR = 456,
        TT_EXPLOSIVE_OBJ = 457,
        TT_BLAST = 459,
        TT_THROWGENERATOR = 460,
        TT_PUSHABLE = 462,
        TT_DOOR = 463,
        TT_TELEPORTER = 464,
        TT_PENDULUM = 466,
        TT_TRAPDOOR = 467,
        TT_TABLE = 468,
        TT_BOSS = 469,
        TT_LADDER = 470,
        TT_CHAIR = 471,
        TT_ARROW = 472,
        // Special analysis types (not real entities)
        TT_LIGHTSPHERE = 65533,
        TT_LIGHTSPHERE2 = 65534,
        TT_MESHANALYSIS = 65535,
    };

} // namespace AITypes

// Thing flags (u32 flags bitmask)
enum ThingFlags : u32 {
    TF_DEAD = 0x0001,  // marked for death transfer
    TF_BIT1 = 0x0002,  // cleared each Think frame
    TF_NEEDS_ACTIVATION = 0x0004,  // needs activation check
    TF_BIT3 = 0x0008,
    TF_ACTIVATED = 0x0010,  // currently activated
    TF_BIT5 = 0x0020,
    TF_MODEL_CREATED = 0x0040,  // model has been created
    TF_BIT8 = 0x0100,
    TF_DYNAMIC = 0x0800,  // is a DynamicThing
    TF_ON_GROUND = 0x1000,  // on ground
};

// Thing secondary flags (u32 flags2 bitmask)
enum ThingFlags2 : u32 {
    TF2_KILLED = 0x0001,  // marked for removal
    TF2_BIT3 = 0x0008,  // cleared each Think frame
    TF2_NIS_ENTER = 0x0010,  // entered NIS control
    TF2_NIS_FROZEN = 0x0020,  // NIS frozen (no movement)
    TF2_NIS_MASK = 0x0070,  // mask for NIS state bits
    TF2_DIRECTOR_ACTIVE = 0x0200,  // director sets on all humanoids during script
};

// Thing - base class for all game entities
// PSX: 96 bytes. Inherits ccNode (intrusive list node with name).
// Every entity (player, enemies, objects, obstacles) derives from Thing.
// Source: C:\CHAN\GAME\SRC\AI\THING.CPP
class Thing : public ccNode {
public:
    // PSX +24 (u16): entity type from AI::ThingTypes
    u16 thingType = 0;
    // PSX +26 (u16): initialized to 0xFFFF
    u16 collisionRadius = 0xFFFF;

    // PSX +28,+32,+36: world position (s32 fixed-point)
    LVector pos = {};

    // PSX +40,+44,+48: orientation/rotation (reset to 0)
    LVector orientation = {};

    // PSX +52 (s32): general-purpose state counter (initialized to 1)
    s32 stateCounter = 1;

    // PSX +56 (u16): active/draw radius
    u16 activeRadius = 1;
    // PSX +58 (u16): initial active radius (restored on Reset)
    u16 initialActiveRadius = 1;

    // PSX +60 (ptr): ThingHandle used for safe cross-references
    void* thingHandle = nullptr;

    // PSX +64: embedded ccNode for secondary list insertion (entity lists in BlockManager)
    ccNode subNode;

    // PSX +76 (ptr): reserved
    void* field76 = nullptr;

    // PSX +80 (ptr): tDrawable* model for rendering
    void* model = nullptr;

    // PSX +84 (u16): block number this Thing is in (BLOCK_UNASSIGNED = unassigned)
    u16 blockNum = BLOCK_UNASSIGNED;

    // PSX +86 (u16): unique ID assigned from global counter
    u16 uniqueID = 0;

    // PSX +88 (u32): bitfield flags (see ThingFlags enum)
    u32 flags = 0;

    // PSX +92 (u32): secondary flags (see ThingFlags2 enum)
    u32 flags2 = 0;

    // Global Thing counter - PSX: gp+3868
    static u16 s_nextUniqueID;


    // PSX: __5ThingPC10tagLVectorUs (THING.CPP:428)
    Thing(const LVector* initialPos, u16 type);

    // PSX: _._5Thing (THING.CPP:458)
    ~Thing() override;


    // PSX: Think__5Thing (THING.CPP:478) - calls UpdatePosition()
    virtual void Think();

    // PSX: Draw__5Thing (THING.CPP:487) - renders model at pos/orientation
    virtual void Draw();

    // PSX: Reset__5Thing (THING.CPP:502) - reset to initial state
    virtual void Reset();

    // PSX: UpdatePosition__5Thing (THING.HPP:440) - base is empty
    virtual void UpdatePosition();

    // PSX: Activate__5Thing (THING.CPP:521) - activate in block's entity list
    virtual void Activate();

    // PSX: Deactivate__5Thing (THING.CPP:546) - deactivate
    virtual void Deactivate();

    // PSX: Move__5Thing (THING.CPP:835) - base does nothing
    virtual void Move();

    // PSX: CreateModel__5ThingPCc (THING.CPP:585) - load/create the 3D model
    virtual void CreateModel(const char* name);

    // PSX: DeleteModel__5Thing (THING.CPP:689) - destroy the model
    virtual void DeleteModel();

    // PSX: HandleCollision__5ThingP5Thingle (THING.CPP:713) - base does nothing
    virtual void HandleCollision(Thing* other, s32 damage);

    // PSX: AnalyzeMesh__5ThingP6DBRoot (THING.CPP:1224) - parse WDB mesh info
    virtual void AnalyzeMesh(DBRoot* root);

    // PSX: GetViewSpot__5ThingP10tagLVectorT1 (THING.CPP:1210) - camera target point
    virtual void GetViewSpot(LVector* outPos, LVector* outTarget);

    // PSX: Kill__5Thing (THING.HPP:518)
    virtual void Kill();

    // PSX: GetSoundPosPtr__5Thing (THING.HPP:516) - returns &pos
    virtual LVector* GetSoundPosPtr();

    // PSX: GetInitialPos__5Thing (THING.HPP:512) - returns &pos
    virtual const LVector* GetInitialPos();


    // PSX: AddPassenger__5ThingP12DynamicThing (THING.CPP:1079)
    void AddPassenger(class DynamicThing* passenger);

    // PSX: RemPassenger__5ThingP6Ticket (THING.CPP:1104)
    void RemPassenger(Ticket* ticket);

    // PSX: RemAllPassengers__5Thing (THING.CPP:1144)
    void RemAllPassengers();

    // PSX: GetThingHandle__5Thing (THING.CPP:1170)
    u32 GetThingHandle();

    // PSX: ClearFloorHeight__5Thing (THING.CPP:765)
    void ClearFloorHeight();

    // PSX: SetFloorHeight__5Thingl (THING.CPP:777)
    void SetFloorHeight(s32 height);

    // PSX: GetObjectToWorldSpaceVector__5Thing (THING.CPP:1352)
    void GetObjectToWorldSpaceVector(const SVector& in, SVector& out);
};

// DynamicThing - Thing with physics (velocity, gravity, forces)
// PSX: extends Thing, ~200 bytes total.
// Source: C:\CHAN\GAME\SRC\AI\THING.CPP
class DynamicThing : public Thing {
public:
    // PSX +96 (s32): hit points (initialized to 100)
    s32 health = 100;

    // PSX +100,+104,+108: velocity vector
    LVector velocity = {};

    // PSX +112,+116,+120: applied force (subtracted from velocity each frame)
    LVector force = {};

    // PSX +124,+128,+132: home/previous position (set from pos on init)
    LVector homePos = {};

    // PSX +136,+140,+144: contact/push force
    LVector contactForce = {};

    // PSX padding/fields +148..+183
    s32 field148[9] = {};

    // PSX +184 (s32): ground standing Y height (set when TF_ON_GROUND)
    s32 groundStandHeight = 0;

    // PSX +188 (ptr): embark ticket
    Ticket* ticket = nullptr;

    // PSX +192 (s32): gravity magnitude (0x8000 = default)
    s32 gravity = 0x8000;

    // PSX +196 (s32): max fall speed divisor (10)
    s32 maxFallDivisor = 10;


    // PSX: __12DynamicThingPC10tagLVectorUs (THING.CPP:840)
    DynamicThing(const LVector* initialPos, u16 type);

    // PSX: _._12DynamicThing (THING.CPP:851)
    ~DynamicThing() override;


    // PSX: Reset__12DynamicThing (THING.CPP:865)
    void Reset() override;

    // PSX: Move__12DynamicThing (THING.CPP:891)
    void Move() override;

    // PSX: UpdatePosition__12DynamicThing (THING.CPP:1280)
    void UpdatePosition() override;


    // PSX: AddForce__12DynamicThinglPC9_RMVECT16 (THING.CPP:753)
    void AddForce(s32 magnitude, const SVector* direction);

    // PSX: Land__12DynamicThing (THING.CPP:794)
    void Land();

    // PSX: DisembarkObstacle__12DynamicThingRC10tagLVector (THING.CPP:810)
    void DisembarkObstacle(const LVector& newPos);

    // PSX: Disembark__12DynamicThing (THING.CPP:1126)
    void Disembark();

    // PSX: GetTicketIssuer__12DynamicThing (THING.CPP:1162)
    Thing* GetTicketIssuer();

    // PSX: HandleLand__12DynamicThingl (THING.CPP:1360)
    virtual void HandleLand(s32 height);
};

// Ticket - embark/passenger link between a Thing and a DynamicThing
// PSX: 32 bytes. Inherits ccNode, stored in Thing's subNode list.
// Source: C:\CHAN\GAME\SRC\AI\THING.CPP:1312
struct Ticket : public ccNode {
    Thing* issuer = nullptr;           // PSX +24: the Thing being stood on
    DynamicThing* passenger = nullptr; // PSX +28: the DynamicThing standing on it

    // PSX: __6TicketP5ThingP12DynamicThing (THING.CPP:1312)
    Ticket(Thing* iss, DynamicThing* pass);
    // PSX: _._6Ticket (THING.CPP:1318)
    ~Ticket() override;
};

// DynamicThing physics constants (PSX: gp+1740, gp+1744, defined in world.cpp)
extern s32 g_maxFallSpeed;
extern s32 g_dampingFactor;
