#pragma once
#include "core.h"
#include "p3d/p3dmath.h"
#include "gen/cclist.h"

// Original hierarchy (data containers stored in LevelManager):
//   OriginalBasic -> OriginalTree -> OriginalSTree / OriginalGeo / OriginalETree
//
// Per-entity model instances:
//   Model (96 bytes) -> SModel -> HumanoidModel (136 bytes) -> PlayerModel (136 bytes)
//   Model (96 bytes) -> GModel (96 bytes)
//
// Drawable wrappers:
//   DrawableTree -> DrawableSTree / DrawableGeo / DrawableETree

class pddiPrimBuffer;
class Thing;
class AnimStructure;
struct AnimationMatrices;
struct STreeData;
struct TransformAnim;

struct ModelFloorHeightState {
    s32 current = (s32)0x80000001;
    s32 previous = (s32)0x80000001;
};

// Pre-parsed skinning data for CPU vertex skinning each frame
struct SkinVertex {
    f32 lx, ly, lz;          // joint-local position
    f32 r, g, b;             // vertex color
    f32 u, v;                // texture coords
    f32 tpage, cba;          // PSX texture info
    u32 jointIdx;            // owning joint index
};

struct SkinData {
    SkinVertex* verts = nullptr;
    u16* indices = nullptr;
    u32 numVerts = 0;
    u32 numIndices = 0;

    ~SkinData() { delete[] verts; delete[] indices; }
};

// OriginalBasic - base for model data stored in LevelManager lists.
// PSX: ccNode-derived with type and storeID fields.
// PSX layout:
//   +0..23:  ccNode base
//   +16 (u16): type (0=Geo, 1=STree, 2=ETree) - overlaps ccNode.flags
//   +19 (s8):  storeID (0 or 2, for LevelManager purge)
//   +20 (u32): nameCRC (from ccNode)
struct OriginalBasic : public ccNode {
    // type is stored at +16 which IS ccNode.flags
    // storeID at +19 which IS ccNode.pri
    // nameCRC at +20 which IS ccNode.nameCRC

    u16 GetType() const { return (u16)flags; }
    void SetType(u16 t) { flags = (s16)t; }
    s8 GetStoreID() const { return pri; }
    void SetStoreID(s8 id) { pri = id; }
};

// OriginalSTree - skeleton tree model data (60 bytes on PSX)
// Stored in LevelManager::streeList, looked up by nameCRC.
// PSX layout:
//   +0..23:  OriginalBasic (ccNode)
//   +24..35: (OriginalTree fields, unused on PC)
//   +36:     tSTree* (PSX P3D STree pointer)
//   +52:     tCompositeAnim* (PSX composite animation)
//   +56:     animation data pointer
//
// PC: instead of PSX-specific tSTree/tPrimGeom, we store PC-ready mesh data.
struct OriginalSTree : public OriginalBasic {
    // PC mesh data (replaces PSX tSTree + tPrimGeom + GTE rendering)
    pddiPrimBuffer* meshBuffer = nullptr;
    u32 meshVertCount = 0;
    u32 meshTriCount = 0;

    // Skeleton data (parsed from P3D 0x6122/0x6121 chunks)
    STreeData* skeleton = nullptr;

    // CPU skinning data (local-space verts + joint indices)
    SkinData* skinData = nullptr;

    OriginalSTree();
    ~OriginalSTree() override;
};

// OriginalGeo - static/dynamic geometry model data (40 bytes on PSX)
// Stored in LevelManager geo lists, looked up by nameCRC.
// PC: stores a PC-ready prim buffer built from tDynGeom polygon data.
struct OriginalGeo : public OriginalBasic {
    pddiPrimBuffer* meshBuffer = nullptr;
    s32 bboxMin[3] = {};
    s32 bboxMax[3] = {};

    OriginalGeo();
    ~OriginalGeo() override;
};

// OriginalETree - export-tree model data (52 bytes on PSX).
// Current PC port stores a single renderable mesh buffer, matching the
// existing Geo path until dedicated ETree data decoding is implemented.
struct OriginalETree : public OriginalBasic {
    pddiPrimBuffer* meshBuffer = nullptr;

    OriginalETree();
    ~OriginalETree() override;
};

// DrawableBasic - base drawable wrapper used by Model.
// STree models expose skeleton accessors for animation code; Geo models return null.
struct DrawableBasic {
    u16 displayFlag = 1;

    virtual ~DrawableBasic();
    virtual void Display(u32 flags) = 0;
    virtual OriginalSTree* GetOriginalSTree() const { return nullptr; }
    virtual OriginalSTree* GetAlternateSTree() const { return nullptr; }
};

// DrawableSTree - wraps OriginalSTree for per-entity rendering (36 bytes on PSX)
// PSX layout:
//   +0..11:  DrawableTree base
//   +16:     u16 (display flag)
//   +24:     OriginalSTree* original
//   +28:     OriginalSTree* alternate (for suit changes)
//   +32:     (reserved)
struct DrawableSTree : public DrawableBasic {
    OriginalSTree* original = nullptr;   // +24 on PSX
    OriginalSTree* alternate = nullptr;  // +28 on PSX

    DrawableSTree(OriginalSTree* orig);
    ~DrawableSTree() override;

    // PSX: Display__13DrawableSTree (calls OriginalSTree::Draw -> tPrimGeom::Display)
    // PC: draws the pddiPrimBuffer
    void Display(u32 flags) override;
    OriginalSTree* GetOriginalSTree() const override { return original; }
    OriginalSTree* GetAlternateSTree() const override { return alternate; }
};

// DrawableGeo - wraps OriginalGeo for per-entity rendering.
struct DrawableGeo : public DrawableBasic {
    OriginalGeo* original = nullptr;

    DrawableGeo(OriginalGeo* orig);
    ~DrawableGeo() override;

    void Display(u32 flags) override;
};

// DrawableETree - wraps OriginalETree for EModel rendering.
struct DrawableETree : public DrawableBasic {
    OriginalETree* original = nullptr;

    DrawableETree(OriginalETree* orig);
    ~DrawableETree() override;

    void Display(u32 flags) override;
};

// Model - base class for per-entity 3D models (96 bytes on PSX)
// PSX layout (24 bytes ccNode base + 72 bytes Model-specific):
//   +24: DrawableBasic* drawable
//   +28: s32 drawableType (0=none, 1=Geo, 2=STree)
//   +32: void* animStructure (AnimStructure*)
//   +36: (reserved)
//   +40: void* ambientLight
//   +44: void* hwLights
//   +48: s32 hwLightCount
//   +52: u16 rotX (binary angle)
//   +54: u16 rotY
//   +56: u16 rotZ
//   +58: (padding)
//   +60: (padding)
//   +64: s32 posX
//   +68: s32 posY
//   +72: s32 posZ
//   +76: Thing* backPtr
//   +80: u32 modelFlags
//   +84: u16 shadowAngle
//   +86: (padding)
//   +88: s32 scale (SModel extension, 0x10000=1.0)
//   +92: s32 (reserved)
class Model : public ccNode {
public:
    // +24
    DrawableBasic* drawable = nullptr;
    // +28
    s32 drawableType = 0;
    // +32
    void* animStructure = nullptr;
    // +36
    void* field36 = nullptr;
    // +40
    void* ambientLight = nullptr;
    // +44
    void* hwLights = nullptr;
    // +48
    s32 hwLightCount = 0;
    // +52,+54,+56 rotation (binary angle units, 0-65535)
    u16 rotX = 0;
    u16 rotY = 0;
    u16 rotZ = 0;
    u16 pad58 = 0;
    // +60
    s32 pad60 = 0;
    // +64,+68,+72 position (PSX fixed-point)
    s32 posX = 0;
    s32 posY = 0;
    s32 posZ = 0;
    // +76
    Thing* backPtr = nullptr;
    // +80
    u32 modelFlags = 0;
    // +84
    u16 shadowAngle = 0;
    u16 pad86 = 0;

    Model();
    ~Model() override;
    void Reset();
    virtual void Show(u32 flags);

    virtual void Animate();
    virtual void ApplyAnimToModel(s32 thingType, s32 animEnum, s32 p3, s32 p4, s32 p5);
    virtual void SetAnim(s32 animEnum, s32 loopType, s32 flag, s32 extra);

    // PSX: animation boundary handlers dispatched via vtable from ExecuteHandler.
    // Base Model implementations are trampolines to AnimStructure methods.
    // Overridden by HumanoidModel (_Loop) and PlayerModel (_Loop, _RunToLast, _IncFrame).
    // PSX signature: void handler(Model* this_adjusted, AnimStructure* anim)
    virtual void HandleLoop(AnimStructure* anim);
    virtual void HandleLoopReverse(AnimStructure* anim);
    virtual void HandleHoldFirst(AnimStructure* anim);
    virtual void HandleHoldLast(AnimStructure* anim);
    virtual void HandleRunToLast(AnimStructure* anim);
    virtual void HandleHoldFrame(AnimStructure* anim);
    virtual void HandleRunToFrame(AnimStructure* anim);
    virtual void HandleIncFrame(AnimStructure* anim);
    virtual void HandleDecFrame(AnimStructure* anim);
    virtual void HandleLoopDesired(AnimStructure* anim);
    virtual void HandleRunToLastBlend(AnimStructure* anim);

    // PSX: DeleteDrawable (called from destructor)
    void DeleteDrawable();
};

// SModel - skeleton-tree model (96 bytes on PSX, same allocation as Model)
// Extends Model with scale field for proportional scaling.
// PSX: _6SModel / Show__6SModelUl (MODEL.CPP:1013, 1454)
class SModel : public Model {
public:
    // +88. Scale fixed-point: 0x10000 = 1.0
    s32 scale = FIX16_ONE;
    // +92
    s32 field92 = 0;

    SModel();
    ~SModel() override;

    // PC: sets world matrix and draws the pddiPrimBuffer
    void Show(u32 flags) override;

    void Animate() override;
    void ApplyAnimToModel(s32 thingType, s32 animEnum, s32 loopType, s32 p4, s32 p5) override;
    void SetOriginalSTree(OriginalSTree* original);
    void InitSemiTransMode();
    void PlayDynamicAnim(s32 animEnum);
};

// GModel - geometry model (96 bytes on PSX, same allocation as Model)
// Used by props loaded through OriginalGeo/tDynGeom.
class GModel : public Model {
public:
    GModel();
    ~GModel() override;

    void Show(u32 flags) override;
    void SetOriginalGeo(OriginalGeo* original);
};

// EModel - export-tree model (120 bytes on PSX).
// Uses drawableType=3 so launcher logic can select the EModel anim path.
class EModel : public Model {
public:
    EModel();
    ~EModel() override;

    void Show(u32 flags) override;
    void Animate() override;
    void ApplyAnimToModel(s32 thingType, s32 animEnum, s32 loopType, s32 p4, s32 p5) override;
    void SetOriginalETree(OriginalETree* original, TransformAnim* animation = nullptr);
};

// HumanoidModel - character model with animation matrices (136 bytes on PSX)
// PSX: _13HumanoidModel (MHUMAN.CPP:45)
class HumanoidModel : public SModel {
public:
    // +96: AnimationMatrices* (660 bytes, 10 joint slots, double-buffered)
    AnimationMatrices* animMatrices = nullptr;
    // +100: attack hand radius
    s32 attackHandRadius = 100;
    // +104: attack foot radius
    s32 attackFootRadius = 100;
    // +108
    s32 field108 = 400;
    // +112
    s32 field112 = 0;
    // +116
    s32 field116 = 0;
    // +120
    s32 field120 = 0;
    // +124
    s32 field124 = 0;
    // +128
    s32 field128 = 0;
    // +132
    s32 field132 = 0xFFFF;

    HumanoidModel();
    ~HumanoidModel() override;

    void Animate() override;
    void SetAnim(s32 animEnum, s32 a3, s32 force, s32 extra) override;

    // Only loops when mode == 0 (normal animation playback).
    void HandleLoop(AnimStructure* anim) override;
};

// PlayerModel - player-specific model (136 bytes, same as HumanoidModel)
class PlayerModel : public HumanoidModel {
public:
    PlayerModel();
    ~PlayerModel() override;

    void ApplyAnimToModel(s32 thingType, s32 animEnum, s32 loopType, s32 p4, s32 p5) override;
    void SetAnim(s32 animEnum, s32 a3, s32 force, s32 extra) override;
    void HandleLoop(AnimStructure* anim) override;
    void HandleRunToLast(AnimStructure* anim) override;
    void HandleIncFrame(AnimStructure* anim) override;
};
