#include "gen/common.h"
#include "ai/platform.h"
#include "ai/activezn.h"
#include "ai/humanoid.h"
#include "ai/obstacle_shared.h"
#include "ai/player.h"
#include "gen/ai.h"
#include "gen/colsect.h"
#include "gen/control.h"
#include "gen/database.h"
#include "gen/game.h"
#include "gen/blockmgr.h"
#include "gen/levelmgr.h"
#include "gen/model.h"
#include "gen/path.h"
#include "snd/platsnd.h"
#include "snd/sndfact.h"
#include "p3d/p3dmath.h"

static inline u32 PtrToken32(const void* p) {
    return static_cast<u32>(reinterpret_cast<u64>(p));
}

static inline s32 MulShift16Signed(s32 a, s32 b) {
    return (s32)(((s64)a * (s64)b) >> 16);
}

static inline s32 AbsDiffS32(s32 a, s32 b) {
    const s32 d = a - b;
    return (d < 0) ? -d : d;
}

static void RotateAroundY(s32 centreX, s32 centreZ, s32 angle, s32& ioX, s32& ioZ) {
    const s32 dx = (ioX - centreX) << 16;
    const s32 dz = (ioZ - centreZ) << 16;

    const s32 cosA = rmSin16(0x4000 - angle);
    const s32 sinNegA = rmSin16(-angle);

    const s32 xFix = MulShift16Signed(dx, cosA) - MulShift16Signed(dz, sinNegA);
    const s32 zFix = MulShift16Signed(dx, sinNegA) + MulShift16Signed(dz, cosA);

    ioX = (xFix >> 16) + centreX;
    ioZ = (zFix >> 16) + centreZ;
}

static s32 ClampDeltaToSpeed(s32 delta, s32 maxAbs) {
    if (delta > maxAbs) {
        return maxAbs;
    }
    if (delta < -maxAbs) {
        return -maxAbs;
    }
    return delta;
}

static s32 ScaleAttribToPlatformFixed(s32 value) {
    const s32 shifted = value << 16;
    const s64 prod = (s64)shifted * (s64)0xB60B60B7;
    s32 out = ((s32)(prod >> 32) + shifted) >> 8;
    out -= (shifted >> 31);
    return out;
}

static s32 PathPitchFromNormY(s32 normY16) {
    f32 y = (f32)normY16 * FIX16_INV;
    if (y < -1.0f) y = -1.0f;
    if (y > 1.0f) y = 1.0f;
    return RAD2ANGLE(std::asin(y));
}

static s32 PathYawFromNormXZ(s32 normX16, s32 normZ16) {
    return (s32)rmATan216((f32)normX16, (f32)normZ16);
}

static const LVector& GetPathDirection(const Path* path) {
    static const LVector kZeroDir = {};
    if (path == nullptr) {
        return kZeroDir;
    }

    if (const LinearPath* linear = dynamic_cast<const LinearPath*>(path)) {
        return linear->direction;
    }
    if (const SplinePath* spline = dynamic_cast<const SplinePath*>(path)) {
        return spline->direction;
    }

    return kZeroDir;
}

static s32 MovePath(Path* path, s32 speed) {
    if (path == nullptr) {
        return 0;
    }

    if (LinearPath* linear = dynamic_cast<LinearPath*>(path)) {
        return linear->Move(speed);
    }

    if (SplinePath* spline = dynamic_cast<SplinePath*>(path)) {
        return spline->Move(speed);
    }

    return 0;
}

static s32 GetCurrentNodeAttrib(const Path* path, s32 attrId) {
    static constexpr s32 ATTR_SENTINEL = (s32)0xABCDABCD;

    if (path == nullptr || path->nodeAttribs == nullptr || path->numPoints <= 0) {
        return ATTR_SENTINEL;
    }

    const s32 idx = path->currentSegment;
    if (idx < 0 || idx >= path->numPoints) {
        return ATTR_SENTINEL;
    }

    return path->nodeAttribs[idx].GetAttrib(attrId);
}

static void SetHumanoidBehaviourCallback(Humanoid* humanoid, Behaviour::AIHandler callback) {
    if (humanoid == nullptr || humanoid->behaviour == nullptr || callback == nullptr) {
        return;
    }

    humanoid->behaviour->handlerThisOffset = 0;
    humanoid->behaviour->handlerDispatch = -1;
    humanoid->behaviour->handler = callback;
}

static Humanoid* FindHumanoidByCRC(u32 crc) {
    if (g_ai == nullptr) {
        return nullptr;
    }

    ccNode* node = g_ai->humanoidList.FindNodeCRC(crc);
    if (node == nullptr) {
        return nullptr;
    }

    return dynamic_cast<Humanoid*>(static_cast<Thing*>(node));
}

// Platform (type 201)
// PSX: PLATFORM.CPP, overlay 6 (NBOL_REL.BIN)
// ============================================================================

Platform::Platform(const LVector* pos, u16 type) : Obstacle(pos, type) {
    MARKFUNCTION(0x800208E4);
    localCollBox = INVALID_COLLISION_BOX;
    velocity = {};
    pathSignal = 0;
}

Platform::~Platform() {
    MARKFUNCTION(0x800208E4);
    delete splinePath;
    splinePath = nullptr;
    delete[] field104;
    field104 = nullptr;
}

void Platform::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80020A24);
    if (!root) return;

    // PSX: seed initial/current orientation from DBRoot +0x28/+0x2C/+0x30
    // (field40/field44/field48) before obstacle mesh analysis.
    initialRot.x = root->field40;
    initialRot.y = root->field44;
    initialRot.z = root->field48;
    orientation = initialRot;
    initialPos = pos;

    Obstacle::AnalyzeMesh(root);

    // PSX: read collision box from root (attrib 5 or volume geometry)
    bool geoFound = ObstacleFillCollisionBox(localCollBox, root, 5);
    if (geoFound) {
        SetCollisionBox(localCollBox);
    }
    else {
        DBVolume* vol = dynamic_cast<DBVolume*>(root);
        if (vol) {
            tagCollisionBox box = {};
            FillCollisionBox(box, *vol);
            localCollBox = box;
            SetCollisionBox(box);
        }
    }

    const DBAttrib* a6 = root->FindAttrib(6);
    if (a6) {
        s32 val = a6->value;
        if (val == 1) platformFlags |= 0x80;
        else if (val == 2) platformFlags |= 0x100;
        else platformFlags |= 0x40;
    }
    else {
        platformFlags |= 0x40;
    }

    const DBAttrib* a7 = root->FindAttrib(7);
    if (a7 && a7->value != 0) {
        platformFlags |= 0x400;
    }

    const DBAttrib* a8 = root->FindAttrib(8);
    if (a8) {
        s32 val = a8->value;
        if (val != 0) {
            platformFlags |= 0x1000;
            loopCount = val;
        }
    }

    const DBAttrib* a9 = root->FindAttrib(9);
    if (a9) {
        s32 val = a9->value;
        if (val != 0) {
            platformFlags |= 0x2000;
            loopCount = val;
        }
    }

    const DBAttrib* a10 = root->FindAttrib(10);
    if (a10 && a10->value != 0) {
        platformFlags |= 0x4000;
    }

    const DBAttrib* a11 = root->FindAttrib(11);
    if (a11) {
        const s32 val = a11->value;
        if (val != 0) {
            platformFlags |= 0x8000;
            field8C = val;
        }
    }

    const DBAttrib* a12 = root->FindAttrib(12);
    if (a12) {
        s32 val = a12->value;
        if (val == 1) platformFlags |= 0x10000;
        else if (val == 2) platformFlags |= 0x20000;
    }

    const DBAttrib* a13 = root->FindAttrib(13);
    if (a13) triggerDelay = a13->value;

    const DBAttrib* a14 = root->FindAttrib(14);
    if (a14) {
        s32 val = a14->value;
        if (val > 0) platformFlags |= 0x8;
        else if (val < 0) platformFlags |= 0x20;
    }

    const DBAttrib* a16 = root->FindAttrib(16);
    speed = a16 ? a16->value : 0;

    const DBAttrib* a17 = root->FindAttrib(17);
    field_188 = a17 ? ScaleAttribToPlatformFixed(a17->value) : 0;

    const DBAttrib* a18 = root->FindAttrib(18);
    killThingsCRC = (a18 && a18->strValue) ? (s32)p3dHash(a18->strValue) : 0;

    const DBAttrib* a20 = root->FindAttrib(20);
    field9C = (a20 && a20->strValue) ? (s32)p3dHash(a20->strValue) : 0;

    const DBAttrib* a29 = root->FindAttrib(29);
    if (a29 && a29->value != 0) {
        platformFlags |= 0x10;
    }

    const DBAttrib* a30 = root->FindAttrib(30);
    if (a30 && a30->value != 0) {
        drawDistSq = a30->value;
        platformFlags |= 0x40000000;
    }

    const DBAttrib* a31 = root->FindAttrib(31);
    if (a31 && a31->value != 0) {
        platformFlags |= 0x800;
    }

    const DBAttrib* a32 = root->FindAttrib(32);
    if (a32 && a32->value != 0) {
        platformFlags |= 0x80000000;
    }

    const DBAttrib* a34 = root->FindAttrib(34);
    childThingCRC = (a34 && a34->strValue) ? (s32)p3dHash(a34->strValue) : 0;

    // PSX: attrib 35 enables split collision boxes (bit 18 / 0x40000).
    const DBAttrib* a35 = root->FindAttrib(35);
    if (a35 && a35->value != 0) {
        platformFlags |= 0x40000;
        field104 = new tagCollisionBox[2];
        field104[0] = INVALID_COLLISION_BOX;
        field104[1] = INVALID_COLLISION_BOX;

        const s16 yThreshold = (s16)((s32)localCollBox.maxY - 0x80);
        FillVehicleCollisionBoxes(field104[1], field104[0], *root, 5, yThreshold);
    }

    const DBAttrib* a37 = root->FindAttrib(37);
    if (a37 && a37->value != 0) {
        platformFlags |= 0x200;
    }

    const DBAttrib* a38 = root->FindAttrib(38);
    if (a38 && a38->value != 0) {
        platformFlags |= 0x80000;

        // PSX allocates 0x1C bytes for this state block (7 x s32 words).
        field12C = new s32[7];
        if (field12C != 0) {
            field12C[0] = Div2TowardZero((s32)localCollBox.minX + (s32)localCollBox.maxX);
            field12C[1] = Div2TowardZero((s32)localCollBox.minY + (s32)localCollBox.maxY);
            field12C[2] = Div2TowardZero((s32)localCollBox.minZ + (s32)localCollBox.maxZ);

            const DBAttrib* a39 = root->FindAttrib(39);
            field12C[6] = a39 ? ScaleAttribToPlatformFixed(a39->value) : 0x16C;
        }
    }

    const DBAttrib* a40 = root->FindAttrib(40);
    if (a40 && a40->value != 0) {
        // PSX allocates 0x1C bytes for this state block (7 x s32 words).
        field130 = new s32[7];
        platformFlags |= 0x100000;

        if (field130 != 0) {
            field130[6] = a40->value;

            const DBAttrib* a41 = root->FindAttrib(41);
            field130[1] = a41 ? (a41->value << 16) : 0x3C0000;

            const DBAttrib* a42 = root->FindAttrib(42);
            field130[0] = a42 ? ScaleAttribToPlatformFixed(a42->value) : 0;
        }
    }

    const DBAttrib* a43 = root->FindAttrib(43);
    if (a43 && a43->value != 0) {
        platformFlags |= 0x1000000;
    }

    const DBAttrib* a44 = root->FindAttrib(44);
    initialCountdown = a44 ? (s32)a44->value : 0;

    // PSX: attrib 4 binds path/line movement data into splinePath (+0xD0).
    delete splinePath;
    splinePath = nullptr;

    const DBAttrib* a4 = root->FindAttrib(4);
    if (a4 && a4->strValue && a4->strValue[0] != '\0' && g_database) {
        const char* pathName = a4->strValue;

        if (DBPath* dbPath = g_database->FindPath(pathName)) {
            if (platformFlags & 0x800000) {
                SplinePath* p = new SplinePath();
                p->Init(dbPath);
                splinePath = p;
            }
            else {
                LinearPath* p = new LinearPath();
                p->Init(dbPath);
                splinePath = p;
            }
        }
        else if (DBLine* dbLine = g_database->FindLine(pathName)) {
            if (platformFlags & 0x800000) {
                SplinePath* p = new SplinePath();
                p->Init(dbLine);
                splinePath = p;
            }
            else {
                LinearPath* p = new LinearPath();
                p->Init(dbLine);
                splinePath = p;
            }
        }
    }

    if (splinePath) {
        pos = splinePath->current;
    }

    // PSX: if bit 11 (0x800) set, clear bit 26 (0x04000000).
    // Otherwise: set 0x400000 only when a path exists, else clear bit 26.
    if (platformFlags & 0x800) {
        platformFlags &= ~0x04000000;
    }
    else if (splinePath) {
        platformFlags |= 0x400000;
    }
    else {
        platformFlags &= ~0x04000000;
    }

    // PSX: attrib 46 controls bit 21 (0x200000): present+nonzero clears it.
    const DBAttrib* a46 = root->FindAttrib(46);
    if (a46 && a46->value != 0) {
        platformFlags &= ~0x200000;
    }
    else {
        platformFlags |= 0x200000;
    }

    // PSX: attrib 47 also sets bit 9 (0x200) when nonzero.
    const DBAttrib* a47 = root->FindAttrib(47);
    if (a47 && a47->value != 0) {
        platformFlags |= 0x200;
    }

    LOG("[Platform::AnalyzeMesh] name=%s pos=(%d,%d,%d) flags=0x%X box=(%d,%d,%d,%d,%d,%d) model=%p",
        GetName() ? GetName() : "null", pos.x, pos.y, pos.z, platformFlags,
        localCollBox.minX, localCollBox.minY, localCollBox.minZ,
        localCollBox.maxX, localCollBox.maxY, localCollBox.maxZ, model);
}

void Platform::CreateModel(const char* name) {
    MARKFUNCTION(0x80021578);

    Thing::CreateModel(name);

    if (platformSound == nullptr) {
        CSound* snd = nullptr;
        if (CSoundFactory::CreateObject(10040, &snd, modelHash) >= 0) {
            platformSound = static_cast<CPlatformSound*>(snd);
            platformSound->Initialize(&pos);
        }
    }
}

void Platform::DeleteModel() {
    MARKFUNCTION(0x80021698);

    Thing::DeleteModel();

    if (platformSound) {
        delete platformSound;
        platformSound = nullptr;
    }
}

void Platform::Reset() {
    MARKFUNCTION(0x800216E8);

    platformFlags = (platformFlags & ~0x2) | 0x4;
    if (platformFlags & 0x800) {
        platformFlags &= ~0x400000;
    }
    else {
        platformFlags |= 0x400000;
    }

    isActive = 0;
    if ((platformFlags & 0x400) == 0 && (platformFlags & 0x800) == 0) {
        isActive = (killThingsCRC == 0) ? 1 : 0;
    }

    if (isActive && (platformFlags & (0x1000 | 0x2000))) {
        moveState = 1;
        deathCountdown = loopCount;
    }
    else {
        moveState = 0;
        deathCountdown = 3;
    }

    countdown2 = initialCountdown;
    moveDone = 0;

    pos = initialPos;
    orientation = initialRot;
    velocity = {};
    fieldCC = 0;
    pathSignal = (s32)0xABCDABCD;

    SetCollisionBox(localCollBox);

    if (childThing) {
        childThing->pos = childInitialRot;
    }

    if (splinePath != nullptr) {
        if (pathInstance != 0) {
            splinePath->Flip();
            pathInstance = 0;
        }
        splinePath->Reset();
    }

    if (field12C != 0) {
        field12C[5] = 0;
        field12C[4] = 0;
        field12C[3] = 0;
    }

    if (field130 != 0) {
        field130[2] = 0;
        field130[5] = 0;
        field130[4] = 0;
        field130[3] = 0;
    }

    OnXorZRot();

    if (platformSound) {
        platformSound->EndMove();
    }

    field134 = 0;

    // PSX: Reset updates block from current pos when valid (0x800218E8..0x80021908)
    if (g_blockManager) {
        const u16 newBlock = g_blockManager->GetBlockNumber(pos);
        if (newBlock != BLOCK_UNASSIGNED) {
            blockNum = newBlock;
        }
    }

    LOG("[Platform::Reset] name=%s pos=(%d,%d,%d) isActive=%d moveState=%d flags=0x%X model=%p",
        GetName() ? GetName() : "null", pos.x, pos.y, pos.z,
        isActive, moveState, platformFlags, model);
}

void Platform::Think() {
    MARKFUNCTION(0x80021918);

    if (platformSound) {
        platformSound->Think();
    }

    // PSX: init invalid collision box on stack
    tagCollisionBox invalidBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };

    // PSX: childThing lookup
    if (childThingCRC != 0 && childThing == nullptr && g_ai) {
        ccNode* node = g_ai->moveList.FindNodeCRC((u32)childThingCRC);
        if (node) {
            childThing = static_cast<Thing*>(node);
            childInitialRot.x = childThing->pos.x;
            childInitialRot.y = childThing->pos.y;
            childInitialRot.z = childThing->pos.z;
        }
    }

    if (isActive) {
        // PSX ACTIVE PATH (0x800219CC)
        velocity = {};

        if (moveState != 0) {
            if (deathCountdown != 0) {
                // PSX: just decrement (0x80021A94)
                deathCountdown--;
            }
            else {
                // PSX: countdown expired (0x800219F0)
                moveState = 0;
                isActive = 0;

                if (platformFlags & 0x2000) {
                    // PSX: bit 13 - drop platform (0x80021A08)
                    velocity.y = -38;
                    velocity.z = 0;
                    velocity.x = 0;
                    moveDone = 1;
                    LOG("[Platform::Think] name=%s countdown expired -> DROP (bit 0x2000)",
                        GetName() ? GetName() : "null");
                }
                else {
                    // PSX: invalidate collision box (0x80021A24)
                    deathCountdown = triggerDelay;
                    // PSX: 0x800A9D60 = memset-like clear of invalidBox, then SetCollisionBox
                    SetCollisionBox(invalidBox);
                    platformFlags = (platformFlags | 0x2) & ~0x4;
                    LOG("[Platform::Think] name=%s countdown expired -> VANISH (bit 0x1000, flags now 0x%X)",
                        GetName() ? GetName() : "null", platformFlags);
                }
            }

            // PSX: sound check after countdown (0x80021A54)
            if (platformSound) {
                if (velocity.x != 0 || velocity.y != 0 || velocity.z != 0) {
                    platformSound->BeginMove();
                }
            }
        }

        // PSX: countdown2 processing (0x80021A98)
        if (countdown2 != 0) {
            countdown2--;
            if (moveState == 0) {
                if (platformSound) {
                    platformSound->EndMove();
                }
            }
        }
        else {
            // PSX: call Move() via vtable (0x80021AD8)
            Move();

            // PSX: sound check after Move (0x80021AF0)
            if (platformSound) {
                if (velocity.x != 0 || velocity.y != 0 || velocity.z != 0) {
                    platformSound->BeginMove();
                }
            }
        }

        // PSX: childThing position update (0x80021B28)
        if (childThing) {
            childThing->pos.x += velocity.x;
            childThing->pos.y += velocity.y;
            childThing->pos.z += velocity.z;
        }
    }
    else {
        // PSX INACTIVE PATH (0x80021BB0)
        if (moveDone) {
            // PSX: falling platform (0x80021BC0)
            LVector savedPos = pos;
            savedPos.y += velocity.y;

            // PSX: gravity from player (0x80021BE0)
            // velocity.y -= (Player::s_player->gravity * 3) / 2
            if (Player::s_player) {
                s32 g = ((DynamicThing*)Player::s_player)->gravity;
                s32 gravComponent = (g * 3 + (g * 3 < 0 ? 1 : 0)) >> 1;
                velocity.y -= gravComponent;
            }

            // PSX: HandleEnvironmentCollision(0x80023D38) controls end-of-fall
            // and humanoid impact handling for falling platforms.
            const bool reachedEnd = HandleEnvironmentCollision(savedPos);

            if (reachedEnd) {
                // PSX: done falling (0x80021C24)
                deathCountdown = triggerDelay;
                SetCollisionBox(invalidBox);
                moveDone = 0;
                platformFlags = (platformFlags | 0x2) & ~0x4;

                // PSX: trigger notification if field9C and bit 13 (0x80021C54)
                if (field9C != 0 && (platformFlags & 0x2000)) {
                    // TODO: trigger by name notification
                }

                if (platformSound) {
                    platformSound->EndMove();
                }
            }
            else {
                // PSX: continue falling (0x80021CC0)
                pos = savedPos;
            }
        }
        else if (field8C != 0 && (platformFlags & 0x2)) {
            if (deathCountdown != 0) {
                deathCountdown--;
            }
            else {
                Reset();
            }
        }
    }

    // PSX COMMON_END (0x80021D3C)
    // Bit 19 (0x80000) check - update draw visibility
    if ((platformFlags & 0x80000) && !(platformFlags & 0x2)) {
        Teeter();
    }

    // PSX: copy velocity to prevVelocity (0x80021D68)
    prevVelocity = velocity;

    // PSX: orientation/collision box update (0x80021D80)
    if (!moveDone) {
        // PSX: bit 20 (0x100000) - orientation interpolation (0x80021DA8)
        if (platformFlags & 0x100000) {
            Bob();
        }

        // PSX: recompute rotated collision box if oriented (0x80021DB0)
        if (orientation.x != 0 || orientation.z != 0) {
            OnXorZRot();
        }
        else if (!(platformFlags & 0x2)) {
            // PSX: reset collision box from localCollBox (0x80021DF8)
            SetCollisionBox(localCollBox);
        }

        MovePassengers();
    }

    if (killThingsCRC != 0 && !isActive && !AtEndOfPath()) {
        DeathCheck();
    }
}

void Platform::Move() {
    MARKFUNCTION(0x80022788);

    if (splinePath == nullptr) {
        return;
    }

    const LVector oldPos = pos;

    if (MovePath(splinePath, speed) != 0) {
        HandlePathNodeTransition();
    }

    pos = splinePath->current;

    if (isActive != 0) {
        velocity.x = ClampDeltaToSpeed(pos.x - oldPos.x, speed);
        velocity.y = ClampDeltaToSpeed(pos.y - oldPos.y, speed);
        velocity.z = ClampDeltaToSpeed(pos.z - oldPos.z, speed);

        if ((velocity.x != 0 || velocity.z != 0) && !(platformFlags & 0x200) && !(platformFlags & 0x8)) {
            constexpr s32 SIGNAL_SENTINEL = (s32)0xABCDABCD;

            const LVector& dir = GetPathDirection(splinePath);

            const s32 mag3 = (s32)rmMag3((f32)dir.x, (f32)dir.y, (f32)dir.z);
            const s32 magXZ = (s32)rmMag2((f32)dir.x, (f32)dir.z);

            const s32 normY = rmDiv16i(dir.y, mag3);
            const s32 normX = rmDiv16i(dir.x, magXZ);
            const s32 normZ = rmDiv16i(dir.z, magXZ);

            const s32 pitchSignal = PathPitchFromNormY(normY);
            const s32 yawSignal = PathYawFromNormXZ(normX, normZ);

            if (pathSignal == SIGNAL_SENTINEL) {
                field_192 = pitchSignal;
                pathSignal = yawSignal;
            }
            else {
                const s32 oldRotY = orientation.y;

                orientation.x = (initialRot.x + (pitchSignal - field_192)) & 0xFFFF;
                orientation.y = initialRot.y + (yawSignal - pathSignal);
                orientation.z = initialRot.z;

                fieldCC = orientation.y - oldRotY;
            }
        }

        if (childThing != nullptr) {
            childThing->pos.y = pos.y;
        }

        if (platformFlags & 0x800) {
            flags |= 0x30;
        }

        if (g_blockManager) {
            const u16 newBlock = g_blockManager->GetBlockNumber(pos);
            if (newBlock != BLOCK_UNASSIGNED) {
                blockNum = newBlock;
            }
        }
    }
    else {
        velocity = {};
    }
}

void Platform::HandlePathNodeTransition() {
    MARKFUNCTION(0x80021E54);

    if (splinePath == nullptr) {
        return;
    }

    static constexpr s32 ATTR_SENTINEL = (s32)0xABCDABCD;

    const s32 a6 = GetCurrentNodeAttrib(splinePath, 6);
    if (a6 != ATTR_SENTINEL) {
        countdown2 = a6;
    }

    const s32 a7 = GetCurrentNodeAttrib(splinePath, 7);
    if (a7 != ATTR_SENTINEL) {
        speed = a7;
    }

    const s32 a8 = GetCurrentNodeAttrib(splinePath, 8);
    if (a8 != ATTR_SENTINEL) {
        isActive = 0;

        tagCollisionBox invalidBox = INVALID_COLLISION_BOX;
        SetCollisionBox(invalidBox);

        // PSX: platformFlags = (platformFlags | 0x2) & ~0x4
        platformFlags = (platformFlags | 0x2) & ~0x4;
    }

    const s32 a9 = GetCurrentNodeAttrib(splinePath, 9);
    if (platformSound != nullptr) {
        platformSound->HitPathNode(a9, countdown2, speed);
    }

    const s32 a10 = GetCurrentNodeAttrib(splinePath, 10);
    if (a10 != ATTR_SENTINEL && g_ai != nullptr) {
        ccNode* node = g_ai->moveList.FindNodeCRC((u32)a10);
        if (node != nullptr) {
            Obstacle* target = dynamic_cast<Obstacle*>(static_cast<Thing*>(node));
            if (target != nullptr) {
                target->TriggerByName(this, nullptr, nullptr);
            }
        }
    }

    const s32 a11 = GetCurrentNodeAttrib(splinePath, 11);
    if (a11 != ATTR_SENTINEL) {
        // TODO: PSX resolves a11 through theWorldMgr +0x6C (World switch list)
        // then calls vtable +0x0C with this platform as argument (switch accept/trigger path).
        // World switch-list structures are not yet represented in PC code with matching layout.
    }

    const s32 a13 = GetCurrentNodeAttrib(splinePath, 13);
    if (a13 != ATTR_SENTINEL) {
        Humanoid* targetHumanoid = FindHumanoidByCRC((u32)a13);
        SetHumanoidBehaviourCallback(targetHumanoid, Behaviour::NDMS);
    }

    Humanoid* subwayHumanoid = nullptr;

    const s32 a20 = GetCurrentNodeAttrib(splinePath, 20);
    if (a20 != ATTR_SENTINEL) {
        subwayHumanoid = FindHumanoidByCRC((u32)a20);
        SetHumanoidBehaviourCallback(subwayHumanoid, Behaviour::NDMS);
    }

    const s32 a21 = GetCurrentNodeAttrib(splinePath, 21);
    if (subwayHumanoid != nullptr && a21 != ATTR_SENTINEL) {
        const s32 rightHash = (s32)p3dHash("RIGHT");
        const s32 leftHash = (s32)p3dHash("LEFT");
        const s32 jumpHash = (s32)p3dHash("JUMP");

        if (a21 == rightHash) {
            SetHumanoidBehaviourCallback(subwayHumanoid, Behaviour::SubwayDodgeRight);
        }
        else if (a21 == leftHash) {
            SetHumanoidBehaviourCallback(subwayHumanoid, Behaviour::SubwayDodgeLeft);
        }
        else if (a21 == jumpHash) {
            SetHumanoidBehaviourCallback(subwayHumanoid, Behaviour::SubwayDodgeJump);
        }
    }

    const s32 a12 = GetCurrentNodeAttrib(splinePath, 12);
    if (a12 != ATTR_SENTINEL) {
        if (platformFlags & 0x4) {
            platformFlags &= ~0x4;
        }
        else {
            platformFlags |= 0x4;
        }
    }

    if (splinePath->EndOfPath() == 0) {
        return;
    }

    if (platformFlags & 0x40) {
        isActive = 0;
        // PSX: platformFlags = (platformFlags | 0x2) & ~0x04000000
        platformFlags = (platformFlags | 0x2) & ~0x04000000;

        if (platformSound != nullptr) {
            platformSound->EndMove();
        }
        return;
    }

    if (platformFlags & 0x100) {
        splinePath->Flip();
        pathInstance = (pathInstance < 1) ? 1 : 0;
    }

    splinePath->Reset();
}

void Platform::OnXorZRot() {
    MARKFUNCTION(0x800222F4);

    tagCollisionBox rotated = INVALID_COLLISION_BOX;
    bool first = true;

    const s16 xs[2] = { localCollBox.minX, localCollBox.maxX };
    const s16 ys[2] = { localCollBox.minY, localCollBox.maxY };
    const s16 zs[2] = { localCollBox.minZ, localCollBox.maxZ };

    for (s32 ix = 0; ix < 2; ix++) {
        for (s32 iy = 0; iy < 2; iy++) {
            for (s32 iz = 0; iz < 2; iz++) {
                SVector local = {};
                local.x = xs[ix];
                local.y = ys[iy];
                local.z = zs[iz];

                SVector world = {};
                GetObjectToWorldSpaceVector(local, world);

                if (first) {
                    rotated.minX = rotated.maxX = world.x;
                    rotated.minY = rotated.maxY = world.y;
                    rotated.minZ = rotated.maxZ = world.z;
                    first = false;
                }
                else {
                    if (world.x < rotated.minX) rotated.minX = world.x;
                    if (world.x > rotated.maxX) rotated.maxX = world.x;
                    if (world.y < rotated.minY) rotated.minY = world.y;
                    if (world.y > rotated.maxY) rotated.maxY = world.y;
                    if (world.z < rotated.minZ) rotated.minZ = world.z;
                    if (world.z > rotated.maxZ) rotated.maxZ = world.z;
                }
            }
        }
    }

    SetCollisionBox(rotated);
}

void Platform::Teeter() {
    MARKFUNCTION(0x80022B90);

    if (field12C == nullptr) {
        return;
    }

    s32* tiltData = field12C;

    s32 tiltX = orientation.x;
    const s32 tiltY = orientation.y;
    s32 tiltZ = orientation.z;

    s32 deltaX = 0;
    s32 deltaZ = 0;
    bool hadPassenger = false;

    for (ccMinNode* node = subNode.next; node; node = node->next) {
        Ticket* ticket = static_cast<Ticket*>(node);
        DynamicThing* passenger = ticket ? ticket->passenger : nullptr;
        if (!passenger) {
            continue;
        }

        const s32 centreX = pos.x + tiltData[0];
        const s32 centreZ = pos.z + tiltData[2];

        const s32 relX = passenger->pos.x - centreX;
        const s32 relZ = passenger->pos.z - centreZ;

        const s32 cosY = rmSin16(0x4000 - tiltY);
        const s32 sinNegY = rmSin16(-tiltY);

        const s32 rotXWorld = MulShift16Signed(relX, cosY) + MulShift16Signed(relZ, sinNegY) + centreX;
        const s32 rotZWorld = MulShift16Signed(relX, -sinNegY) + MulShift16Signed(relZ, cosY) + centreZ;

        const s32 passengerMass = passenger->stateCounter * passenger->maxFallDivisor;
        const s32 denom = stateCounter;
        if (denom != 0) {
            const s32 torqueX = MulShift16Signed(passengerMass, rmSin16(tiltX + 0x4000));
            const s32 torqueZ = MulShift16Signed(passengerMass, rmSin16(tiltZ + 0x4000));

            deltaX += MulShift16Signed(torqueX, rotZWorld - tiltData[2] - pos.z) / denom;
            deltaZ += -MulShift16Signed(torqueZ, rotXWorld - tiltData[0] - pos.x) / denom;
        }

        hadPassenger = true;
    }

    if (platformFlags & 0x100000) {
        tiltData[3] = deltaX;
        tiltData[5] = deltaZ;
    }
    else {
        tiltData[3] += deltaX;
        tiltData[5] += deltaZ;
    }

    tiltX += tiltData[3];
    const s32 maxTilt = tiltData[6];
    if (tiltX < -maxTilt || tiltX > maxTilt) {
        tiltX = (tiltX > maxTilt) ? maxTilt : -maxTilt;
        tiltData[3] = 0;
    }

    tiltZ += tiltData[5];
    if (tiltZ < -maxTilt || tiltZ > maxTilt) {
        tiltZ = (tiltZ > maxTilt) ? maxTilt : -maxTilt;
        tiltData[5] = 0;
    }

    tiltX += MulShift16Signed(-tiltX, 0x11EB);
    tiltZ += MulShift16Signed(-tiltZ, 0x11EB);

    orientation.x = tiltX;
    orientation.z = tiltZ;

    if (!hadPassenger) {
        return;
    }

    const s32 tiltMeasure = MulShift16Signed(rmSin16(tiltZ + 0x4000), rmSin16(tiltX + 0x4000));

    if (platformFlags & 0x08000000) {
        if (!(platformFlags & 0x10000000)) {
            if (platformSound) {
                platformSound->Tilt();
            }
            platformFlags |= 0x10000000;
        }
    }

    if (!(platformFlags & 0x08000000)) {
        if (tiltMeasure <= 0xFFDB) {
            platformFlags |= 0x08000000;
        }
    }

    if (tiltMeasure > 0xFFDC) {
        platformFlags &= ~0x18000000;
    }
}

void Platform::Bob() {
    MARKFUNCTION(0x80023190);

    if (field130 == nullptr) {
        return;
    }

    s32* bobData = field130;
    if (bobData[1] == 0) {
        return;
    }

    const LVector oldPos = pos;
    const LVector oldRot = orientation;

    const s16 phase = (s16)rmDiv16i(bobData[2], bobData[1]);
    const s32 newY = initialRot.y + MulShift16Signed(bobData[6], rmSin16(phase));

    prevVelocity.y += newY - oldPos.y;

    pos = oldPos;
    pos.y = newY;

    const s32 oldTiltVelX = bobData[3];
    const s32 oldTiltVelZ = bobData[5];

    bobData[3] = MulShift16Signed(bobData[0], rmSin16(phase));
    bobData[5] = MulShift16Signed(bobData[0], rmSin16(phase + 0x2AAA));

    orientation = oldRot;
    orientation.x += oldTiltVelX - bobData[3];
    orientation.z += oldTiltVelZ - bobData[5];

    bobData[2] += 0x10000;
}

void Platform::MovePassengers() {
    MARKFUNCTION(0x8002337C);

    if (model) {
        Model* m = static_cast<Model*>(model);
        if (subNode.next) {
            m->modelFlags |= 0x8;
        }
        else {
            m->modelFlags &= ~0x8u;
        }
    }

    for (ccMinNode* node = subNode.next; node; node = node->next) {
        Ticket* ticket = static_cast<Ticket*>(node);
        DynamicThing* passenger = ticket ? ticket->passenger : nullptr;
        if (!passenger) {
            continue;
        }

        LVector savedVel = passenger->velocity;
        LVector savedHome = passenger->homePos;

        savedVel.x += velocity.x;
        savedVel.y += velocity.y;
        savedVel.z += velocity.z;

        if (savedVel.y <= 0) {
            passenger->Land();
            savedVel = passenger->velocity;
            savedVel.y = 0;
        }

        if (isActive) {
            if (platformFlags & 0x1) {
                RotateAroundY(pos.x, pos.z, field_188, savedHome.x, savedHome.z);
                passenger->orientation.y += field_188;
            }

            if (fieldCC) {
                RotateAroundY(pos.x, pos.z, fieldCC, savedHome.x, savedHome.z);
                passenger->orientation.y += fieldCC;
            }
        }

        passenger->homePos = savedHome;
        passenger->velocity = savedVel;
        passenger->SetFloorHeight(savedHome.y);
    }
}

bool Platform::HandleEnvironmentCollision(LVector& nextPos) {
    MARKFUNCTION(0x80023D38);

    LVector boxCentre = {};
    {
        SVector localCentre = {};
        localCentre.x = (s16)Div2TowardZero((s32)collBox.minX + (s32)collBox.maxX);
        localCentre.y = (s16)Div2TowardZero((s32)collBox.minY + (s32)collBox.maxY);
        localCentre.z = (s16)Div2TowardZero((s32)collBox.minZ + (s32)collBox.maxZ);

        SVector rotatedCentre = {};
        GetObjectToWorldSpaceVector(localCentre, rotatedCentre);

        boxCentre.x = pos.x + rotatedCentre.x;
        boxCentre.y = pos.y + rotatedCentre.y;
        boxCentre.z = pos.z + rotatedCentre.z;
    }

    const s32 xSpan = (s32)collBox.maxX - (s32)collBox.minX;
    const s32 zSpan = (s32)collBox.maxZ - (s32)collBox.minZ;
    const s32 span = (xSpan < zSpan) ? zSpan : xSpan;
    const s32 radius = Div2TowardZero(span);

    s32 floorHeight = 0;
    s32 ceilingHeight = 0;
    LVector floorNormal = {};
    LVector ceilingNormal = {};
    CollisionSector::GetWorldFloorAndCeilingHeight(
        floorHeight, ceilingHeight, floorNormal, ceilingNormal, boxCentre, radius);

    const s32 nextBottom = nextPos.y + (s32)collBox.minY;
    if (floorHeight <= (s32)0x80000001) {
        return false;
    }

    if (nextBottom < floorHeight) {
        if (platformSound) {
            platformSound->Impact();
        }
        return true;
    }

    bool hitSomething = false;
    if (!g_ai) {
        return false;
    }

    for (ccMinNode* node = g_ai->humanoidList.head; node; node = node->next) {
        Humanoid* hum = static_cast<Humanoid*>(static_cast<ccNode*>(node));
        const s32 reach = hum->collBboxMin.x + radius;

        if (AbsDiffS32(nextPos.x, hum->pos.x) > reach) {
            continue;
        }
        if (AbsDiffS32(nextPos.z, hum->pos.z) > reach) {
            continue;
        }

        const s32 humTop = hum->pos.y + hum->collBboxMin.z;
        const s32 humBottom = hum->pos.y - hum->collBboxMin.y;
        if (!(nextBottom < humTop && humBottom < nextBottom)) {
            continue;
        }

        hum->HandleCollision(this, 17);
        hitSomething = true;

        if (platformSound) {
            platformSound->HitHumanoid();
            platformSound->Impact();
        }
    }

    return hitSomething;
}

void Platform::Draw() {
    MARKFUNCTION(0x800209BC);

    if (!model) return;

    // PSX: check platformFlags bit 2 (visible) must be set
    if (!(platformFlags & 0x4)) return;
    // PSX: check platformFlags bit 3 (invisible from attrib 14) must be clear
    if (platformFlags & 0x8) return;

    // PSX: distance culling when bit 22 (0x400000) is set
    // (not implemented - would need camera distance)

    Obstacle::Draw();
}

void Platform::UpdatePosition() {
    MARKFUNCTION(0x8007CDDC);
}

void Platform::FillSphere(tSphere& sphere) const {
    MARKFUNCTION(0x80024C70);
}

void Platform::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x80023CE4);
    if (pickup) {
        pickup->Kill();
    }
}

void Platform::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x800240C4);

    if (!hum) return;
    if (platformFlags & 0x20) return;
    if (hum->actionState == 73) return;
    if ((platformFlags & 0x10) && hum == (Humanoid*)Player::s_player) return;

    tagCollisionBox box = collBox;

    LVector basisA = {
        pos.x - velocity.x,
        pos.y - velocity.y,
        pos.z - velocity.z
    };

    LVector humVel = hum->velocity;

    LVector humPrevPos;
    humPrevPos.x = hum->pos.x - humVel.x;
    humPrevPos.y = hum->pos.y - humVel.y;
    humPrevPos.z = hum->pos.z - humVel.z;

    bool onTop = false;
    s32 setFloor = 0;

    if (moveDone != 0) {
        goto NEAR_END;
    }

    {
        // PSX: outPos/outNormal/outPushedPos shared between static and field104 paths
        LVector outPos = {};
        LVector outNormal = {};
        LVector outPushedPos = {};

        if (field104 != nullptr) {
            // PSX: dual collision box path (PLATFORM.CPP:2140-2185)
            // First CorrectThingPosition with field104[0]
            LVector outNormal1 = {};

            CorrectThingPositionObstacle(
                basisA, pos, orientation.y, orientation.y,
                field104[0], humPrevPos, hum->homePos,
                hum->collBboxMin.x, hum->collBboxMin.y, hum->collBboxMin.z,
                outPos, outNormal1, outPushedPos);

            // PSX: on-top check includes orientation test for field104 path
            if (orientation.x != 0 || orientation.z != 0 || outNormal1.y > 0) {
                if (humVel.y <= 0) {
                    onTop = true;
                    setFloor = 1;
                    hum->SetFloorHeight(pos.y + (s32)field104[0].maxY);
                }
            }

            // PSX: save first outPos, use as pointB for second call (PLATFORM.CPP:2153)
            LVector firstOutPos = outPos;

            // Second CorrectThingPosition with field104[1]
            LVector outNormal2 = {};

            bool corrected2 = CorrectThingPositionObstacle(
                basisA, pos, orientation.y, orientation.y,
                field104[1], humPrevPos, firstOutPos,
                hum->collBboxMin.x, hum->collBboxMin.y, hum->collBboxMin.z,
                outPos, outNormal2, outPushedPos);

            // PSX: on-top check for second box (PLATFORM.CPP:2171-2176)
            if (orientation.x != 0 || orientation.z != 0 || outNormal2.y > 0) {
                if (humVel.y <= 0) {
                    onTop = true;
                    setFloor = 1;
                    hum->SetFloorHeight(pos.y + (s32)field104[1].maxY);
                }
            }

            // PSX: merge normals - use outNormal1 unless 2nd CTP corrected (PLATFORM.CPP:2182-2185)
            outNormal = outNormal1;
            if (corrected2) {
                outNormal = outNormal2;
            }
        } else {
            // PSX: static platform path (single collision box)
            // Slope pre-adjustment (bit 23 + orientation.x)
            if ((platformFlags & 0x800000) && orientation.x != 0) {
                LVector slopeDelta = {
                    pos.x - hum->pos.x,
                    pos.y - hum->pos.y,
                    pos.z - hum->pos.z
                };

                s32 sinY = rmSin16(orientation.y);
                s32 cosY = rmSin16(orientation.y + 0x4000);
                s32 localX = (s32)(((s64)cosY * slopeDelta.x + (s64)(-sinY) * slopeDelta.z) >> 16);
                s32 localZ = (s32)(((s64)sinY * slopeDelta.x + (s64)cosY * slopeDelta.z) >> 16);

                s32 sinX = rmSin16(orientation.x);
                s32 cosX = rmSin16(orientation.x + 0x4000);
                s32 heightAdj = (s32)(((s64)sinX * (s64)localX) >> 16);
                if (cosX != 0) {
                    heightAdj = rmDiv16i(heightAdj, cosX);
                }

                // PSX: loads localCollBox into local box, adjusts maxY and humPrevPos.y
                box = localCollBox;
                box.maxY = (s16)((s32)box.maxY + 0x80);
                humPrevPos.y += 0x100;

                s32 tiltZ = (s32)(((s64)cosX * (s64)(s32)((s16)box.maxZ << 16 >> 16)) >> 16);
                box.minY = (s16)((s32)box.minY + (s16)tiltZ - (s16)box.maxZ);
            }

            bool corrected = CorrectThingPositionObstacle(
                basisA, pos, orientation.y, orientation.y,
                box, humPrevPos, hum->homePos,
                hum->collBboxMin.x, hum->collBboxMin.y, hum->collBboxMin.z,
                outPos, outNormal, outPushedPos);

            // PSX: slope post-adjust (only if bit 23 + orientX + corrected)
            if ((platformFlags & 0x800000) && orientation.x != 0 && corrected) {
                box.maxY = (s16)((s32)box.maxY - 0x80);
                outPos.y -= 0x80;
            }

            // PSX: on-top detection from correction normal
            if (outNormal.y > 0) {
                if (humVel.y <= 0) {
                    onTop = true;
                    setFloor = 1;
                    hum->SetFloorHeight(pos.y + (s32)box.maxY);
                }
            }
        }

        // PSX: shared wall/squash/apply path (loc_8002472C)

        // PSX: bit 18 -> skip wall check, go to squash check
        if (platformFlags & 0x40000) {
            goto DO_SQUASH_CHECK;
        }

        // PSX: bit 21 -> LedgeCheck (OBSTACLE.CPP:1124)
        if (platformFlags & 0x200000) {
            if (LedgeCheck(box, outNormal, outPushedPos, hum)) {
                if (hum == (Humanoid*)Player::s_player) {
                    if (hum->actionState != (s32)AS_LEDGE_LATCH) {
                        hum->SetActionState(AS_LEDGE_LATCH, 0);
                        hum->PrepareLedgeLatch(outPushedPos, outNormal);
                        if (hum->humanoidSound) {
                            hum->humanoidSound->Grab((CSoundMaterial)GetFloorMaterial());
                        }
                    }

                    onTop = true;
                    setFloor = 0;
                }
                goto NEAR_END;
            }
            goto DO_SQUASH_CHECK;
        }

    DO_SQUASH_CHECK:
        // PSX: CheckForSquash (0x80023FC4)
        if (CheckForSquash(outPos, outNormal, hum->collBboxMin)) {
            // PSX: normalize velocity direction for damage (PLATFORM.CPP:2214)
            LVector damageDir = {
                velocity.x << 16,
                velocity.y << 16,
                velocity.z << 16
            };
            rmV3Normalize(&damageDir, &damageDir);

            // PSX: clear humanoid contact force (PLATFORM.CPP:2223)
            hum->contactForce = {};

            // PSX: if health > 0, call HandleCollision + dialog/shock (PLATFORM.CPP:2230-2237)
            if (hum->health != 0) {
                hum->HandleCollision(this, 1);
                if (hum == (Humanoid*)Player::s_player) {
                    hum->LoadDialog(0x36, 0xFF);
                    Shock(SHOCK_9);
                }
            }

            // PSX: revert outPos to humPrevPos (PLATFORM.CPP:2243)
            outPos = humPrevPos;

            // PSX: set hum velocity = platform velocity * 2 (PLATFORM.CPP:2245)
            hum->velocity.x = velocity.x * 2;
            hum->velocity.y = velocity.y * 2;
            hum->velocity.z = velocity.z * 2;

            // PSX: HitHumanoid sound (PLATFORM.CPP:2248-2250)
            if (platformSound) {
                platformSound->HitHumanoid();
            }

            // PSX: check bit 14 (0x4000) - if not set, deactivate (PLATFORM.CPP:2253-2265)
            if (!(platformFlags & 0x4000)) {
                onTop = false;
                isActive = 0;
                platformFlags |= 0x2;
                if (platformSound) {
                    platformSound->EndMove();
                }
            }
        }

        // PSX: APPLY_POS (0x80024A20) - copy corrected pos unless ledge-climbing
        if (hum->actionState >= 25 && hum->actionState <= 27) {
            onTop = false;
        } else {
            hum->homePos = outPos;
        }
    }

    if (setFloor != 0) {
        if (!(platformFlags & 0x2)) {
            if (platformFlags & 0x400) {
                isActive = 1;
                if (moveState == 0) {
                    if ((platformFlags & 0x1000) || (platformFlags & 0x2000)) {
                        moveState = 1;
                        if (hum->actionState == 23 && loopCount >= 6 && loopCount < 27) {
                            deathCountdown = 27;
                        } else {
                            deathCountdown = loopCount;
                        }
                    }
                }
            }
        }
    }

NEAR_END:
    if (moveState != 0) {
        if (deathCountdown < 25) {
            hum->field368 |= 0x80;
        }
    }

    // PSX: if onTop, add as passenger (virtual call via vtable+0x38)
    if (onTop) {
        AddPassenger(hum);
    }
}

// PSX: CheckForSquash (0x80023FC4, PLATFORM.CPP:1995)
// Checks if humanoid is being squashed by the platform
// Saves and restores this->pos (in case earlier code modified it)
bool Platform::CheckForSquash(const LVector& outPos, const LVector& outNormal, const LVector& collBbox) {
    MARKFUNCTION(0x80023FC4);

    // PSX saves pos to stack, may overwrite only Y, then restores from that stack copy.
    LVector savedPos = pos;
    s32 squash = 0;

    // PSX branch at 0x80023FEC..0x80024034:
    // downward contact + platform moving down + active -> update temporary Y only.
    // This path does NOT count as squash and skips lateral dot test.
    if (outNormal.y < 0 && velocity.y < 0 && isActive != 0) {
        savedPos.y = outPos.y + collBbox.z - (s32)localCollBox.minY - velocity.y;
    }
    else {
        // PSX branch at 0x80024038..0x80024084:
        // only lateral squash test contributes to return value.
        if (platformFlags & 0x40000) {
            s32 dot = (s32)((s64)outNormal.x * (s64)velocity.x)
                + (s32)((s64)outNormal.z * (s64)velocity.z);
            squash = (dot > 0) ? 1 : 0;
        }
    }

    pos = savedPos;

    if (squash != 0) {
        platformFlags |= 0x2;
    }

    return squash != 0;
}

// PSX: FillVehicleCollisionBoxes__8ObstacleR15tagCollisionBoxT1RC6DBRootUll (0x8007B068, OBSTACLE.CPP:610)
// Splits geometry vertices into two collision boxes based on Y threshold.
// box1 receives vertices with y < threshold, box2 receives vertices with y >= threshold.
// After filling, box2 gets X extents from box1, Y boundary shared, Z expanded by skin.
void Platform::FillVehicleCollisionBoxes(
    tagCollisionBox& box1, tagCollisionBox& box2,
    const DBRoot& dbRoot, u32 attribNum, s16 yThreshold)
{
    const DBAttrib* attrib = dbRoot.FindAttrib(attribNum);
    if (!attrib || !attrib->strValue) return;

    s32 hash = (s32)p3dHash(attrib->strValue);
    if (!g_levelManager) return;

    OriginalBasic* geo = g_levelManager->FindGeo(hash);
    if (!geo) return;

    OriginalGeo* ogeo = static_cast<OriginalGeo*>(geo);

    // PSX iterates actual vertices and calls IncludeVertexInBox on each.
    // PC: use bounding box as approximation - split the geo's bbox at yThreshold.
    s16 geoMinX = (s16)ogeo->bboxMin[0];
    s16 geoMinY = (s16)ogeo->bboxMin[1];
    s16 geoMinZ = (s16)ogeo->bboxMin[2];
    s16 geoMaxX = (s16)ogeo->bboxMax[0];
    s16 geoMaxY = (s16)ogeo->bboxMax[1];
    s16 geoMaxZ = (s16)ogeo->bboxMax[2];

    // box1: upper portion (y < threshold)
    box1.minX = geoMinX;
    box1.minY = geoMinY;
    box1.minZ = geoMinZ;
    box1.maxX = geoMaxX;
    box1.maxY = (yThreshold < geoMaxY) ? yThreshold : geoMaxY;
    box1.maxZ = geoMaxZ;
    SetCollisionBoxExtent(box1);

    // box2: lower portion (y >= threshold)
    // PSX post-loop: copy X from box1, set minY = box1.maxY, expand Z by skin
    static constexpr s16 COLLISION_SKIN = 0x40; // PSX: gp+0x83C
    box2.minX = box1.minX;
    box2.minY = box1.maxY;
    box2.minZ = geoMinZ - COLLISION_SKIN;
    box2.maxX = box1.maxX;
    box2.maxY = geoMaxY;
    box2.maxZ = geoMaxZ + COLLISION_SKIN;
    SetCollisionBoxExtent(box2);
}

void Platform::SetPlatformToPathNode(const char* name) {
    MARKFUNCTION(0x80023830);

    if (!splinePath || !name) {
        return;
    }

    const s32 targetHash = (s32)p3dHash(name);

    s32 nodeIndex = 0;
    for (; nodeIndex < splinePath->numPoints; nodeIndex++) {
        if (splinePath->nodeAttribs[nodeIndex].GetAttrib(14) == targetHash) {
            break;
        }
    }

    if (nodeIndex >= splinePath->numPoints) {
        return;
    }

    // PSX does two virtual +0x10 calls here before rebasing to the target node.
    Move();
    Move();

    splinePath->currentSegment = nodeIndex;
    splinePath->current = splinePath->positions[nodeIndex];

    pos = splinePath->current;

    if (Player::s_player) {
        blockNum = Player::s_player->blockNum;
    }

    HandlePathNodeTransition();
    UpdatePosition();

    const LVector& dir = GetPathDirection(splinePath);

    const s32 mag3 = (s32)rmMag3((f32)dir.x, (f32)dir.y, (f32)dir.z);
    const s32 magXZ = (s32)rmMag2((f32)dir.x, (f32)dir.z);

    const s32 normY = rmDiv16i(dir.y, mag3);
    const s32 normX = rmDiv16i(dir.x, magXZ);
    const s32 normZ = rmDiv16i(dir.z, magXZ);

    const s32 pitchSignal = PathPitchFromNormY(normY);
    const s32 yawSignal = PathYawFromNormXZ(normX, normZ);

    const s32 oldRotY = orientation.y;

    orientation.x = (initialRot.x + (pitchSignal - field_192)) & 0xFFFF;
    orientation.y = initialRot.y + (yawSignal - pathSignal);
    orientation.z = initialRot.z;

    fieldCC = orientation.y - oldRotY;
}

void Platform::TriggerByName(Thing* source, const char* name, const char* param) {
    MARKFUNCTION(0x80023A80);

    const u16 sourceType = source->thingType;

    // PSX 0x80023AB8: for non-zero source type, require platformFlags bit 0.
    if (sourceType != 0 && !(platformFlags & 0x1)) {
        return;
    }

    if (name != nullptr) {
        if (name[0] == '*') {
            if (isActive != 0) {
                return;
            }

            SetPlatformToPathNode(name + 1);
            isActive = 1;

            // PSX virtual dispatch: vtable+20 (Think), then vtable+56 (AddPassenger)
            Think();
            AddPassenger(static_cast<DynamicThing*>(source));

            LVector boxCentre = pos;
            SVector localCentre = {};
            localCentre.x = (s16)Div2TowardZero((s32)localCollBox.minX + (s32)localCollBox.maxX);
            localCentre.y = (s16)Div2TowardZero((s32)localCollBox.minY + (s32)localCollBox.maxY);
            localCentre.z = (s16)Div2TowardZero((s32)localCollBox.minZ + (s32)localCollBox.maxZ);

            SVector rotatedCentre = {};
            GetObjectToWorldSpaceVector(localCentre, rotatedCentre);
            boxCentre.x += rotatedCentre.x;
            boxCentre.y += rotatedCentre.y;
            boxCentre.z += rotatedCentre.z;
            boxCentre.y += (s32)localCollBox.maxY;

            if (Player::s_player) {
                Player::s_player->pos = boxCentre;
                Player::s_player->homePos = boxCentre;
            }

            flags |= TF_ACTIVATED;
            return;
        }

        if (sourceType == 0 && ::_stricmp(name, "reset") == 0) {
            // PSX virtual dispatch: vtable+12 (Reset)
            Reset();
            return;
        }

        if (sourceType != 0) {
            return;
        }

        if (PtrToken32(param) == field134) {
            return;
        }

        if (platformFlags & 0x2) {
            platformFlags &= ~0x2;

            if (splinePath) {
                splinePath->Flip();
                pathInstance = (pathInstance < 1) ? 1 : 0;
                splinePath->Reset();
            }
        }

        isActive = 1;
        field134 = PtrToken32(param);
        return;
    }

    // PSX: null-name trigger path
    LOG("[Platform::TriggerByName NULL] name=%s pos=(%d,%d,%d) flags=0x%X isActive=%d moveState=%d loopCount=%d splinePath=%p",
        GetName() ? GetName() : "null", pos.x, pos.y, pos.z,
        platformFlags, isActive, moveState, loopCount, splinePath);

    if (platformFlags & 0x2) {
        LOG("[Platform::TriggerByName NULL] aborting: platformFlags bit 1 (0x2) already set");
        return;
    }

    flags |= 0x30;
    isActive = 1;
    // PSX: null-name trigger sets bit 26 (0x04000000).
    platformFlags |= 0x04000000;

    if (moveState != 0) {
        LOG("[Platform::TriggerByName NULL] moveState=%d already running, skipping countdown init", moveState);
        return;
    }

    if (platformFlags & (0x1000 | 0x2000)) {
        moveState = 1;
        deathCountdown = loopCount;
        LOG("[Platform::TriggerByName NULL] started moveState=1 deathCountdown=%d", deathCountdown);
    }
    else {
        LOG("[Platform::TriggerByName NULL] flags bit 12/13 (0x1000/0x2000) not set -> platform will not self-countdown");
    }
}

const LVector* Platform::GetDeltaVelocity() const {
    MARKFUNCTION(0x80024B7C);
    // PSX: bit 24 (0x1000000) = platform has movement, return prevVelocity
    if (platformFlags & 0x1000000) {
        return &prevVelocity;
    }
    return &ZERO_DELTA_VELOCITY;
}

s32 Platform::AtEndOfPath() const {
    MARKFUNCTION(0x80024BA0);
    // PSX: if no spline, treat as "at end". Otherwise defer to Path::EndOfPath (vtable+0x20).
    if (splinePath == nullptr) {
        return 1;
    }
    return splinePath->EndOfPath();
}

const LVector* Platform::GetInitialPos() const {
    MARKFUNCTION(0x80024D04);
    return &initialPos;
}

s32 Platform::GetFloorMaterial() const {
    MARKFUNCTION(0x80024CD0);
    return 0;
}

void Platform::DeathCheck() {
    MARKFUNCTION(0x80024BE0);

    if (killTarget) {
        // PSX: check ActiveZone member count (like Door::DeathCheck)
        ActiveZone* az = static_cast<ActiveZone*>(static_cast<ccNode*>(killTarget));
        s32 thinkingCount = 0;
        for (s32 i = 0; i < az->memberCount; i++) {
            if (az->members[i] != nullptr) {
                thinkingCount++;
            }
        }
        if (thinkingCount > 0) {
            deathCountdown = 3;
        }
        else {
            deathCountdown--;
            if (deathCountdown <= 0) {
                isActive = 1;
            }
        }
    }
    else {
        if (killThingsCRC != 0 && g_ai) {
            killTarget = static_cast<Thing*>(
                g_ai->activeZoneList.FindNodeCRC((u32)killThingsCRC));
        }
        if (!killTarget) {
            isActive = 1;
        }
    }
}
