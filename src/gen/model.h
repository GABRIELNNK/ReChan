// model.h - Model class hierarchy reversed from PSX MODEL.CPP / MHUMAN.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP
//
// Original hierarchy (data containers stored in LevelManager):
//   OriginalBasic -> OriginalTree -> OriginalSTree / OriginalGeo / OriginalETree
//
// Per-entity model instances:
//   Model (96 bytes) -> SModel -> HumanoidModel (136 bytes) -> PlayerModel (136 bytes)
//   Model (96 bytes) -> GModel (96 bytes)
//
// Drawable wrappers:
//   DrawableTree -> DrawableSTree / DrawableGeo
#pragma once

#include "core.h"
#include "p3d/p3dmath.h"
#include "gen/cclist.h"

class pddiPrimBuffer;
class Thing;
class AnimStructure;
struct AnimationMatrices;
struct STreeData;

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

// DrawableSTree - wraps OriginalSTree for per-entity rendering (36 bytes on PSX)
// PSX layout:
//   +0..11:  DrawableTree base
//   +16:     u16 (display flag)
//   +24:     OriginalSTree* original
//   +28:     OriginalSTree* alternate (for suit changes)
//   +32:     (reserved)
struct DrawableSTree {
    OriginalSTree* original = nullptr;   // +24 on PSX
    OriginalSTree* alternate = nullptr;  // +28 on PSX
    u16 displayFlag = 1;                 // +16 on PSX

    DrawableSTree(OriginalSTree* orig);
    ~DrawableSTree();

    // PSX: Display__13DrawableSTree (calls OriginalSTree::Draw -> tPrimGeom::Display)
    // PC: draws the pddiPrimBuffer
    void Display(u32 flags);
};

// Model - base class for per-entity 3D models (96 bytes on PSX)
// PSX layout (24 bytes ccNode base + 72 bytes Model-specific):
//   +24: DrawableSTree* drawable
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
    DrawableSTree* drawable = nullptr;
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

    // PSX: _5Model (MODEL.CPP:671)
    Model();
    // PSX: _._5Model (MODEL.CPP:697)
    ~Model() override;

    // PSX: Reset__5Model (MODEL.CPP:777)
    void Reset();

    // PSX: Show__5Model (virtual, overridden by SModel/GModel)
    virtual void Show(u32 flags);

    // PSX: Animate__5Model (virtual)
    virtual void Animate();

    // PSX: ApplyAnimToModel__5Model (virtual)
    virtual void ApplyAnimToModel(s32 thingType, s32 animEnum, s32 p3, s32 p4, s32 p5);

    // PSX: SetAnim__5Modelllil (virtual, overridden by HumanoidModel/PlayerModel)
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

    // PSX: Show__6SModelUl (MODEL.CPP:1454)
    // PC: sets world matrix and draws the pddiPrimBuffer
    void Show(u32 flags) override;

    // PSX: Animate__6SModel (MODEL.CPP:1416)
    void Animate() override;

    // PSX: ApplyAnimToModel__6SModellllll (MODEL.CPP:1098)
    void ApplyAnimToModel(s32 thingType, s32 animEnum, s32 loopType, s32 p4, s32 p5) override;

    // PSX: SetOriginalSTree__6SModelP13OriginalSTreeP10tAnimation (MODEL.CPP:1026)
    void SetOriginalSTree(OriginalSTree* original);

    // PSX: InitSemiTransMode__6SModel (MODEL.CPP:1045)
    void InitSemiTransMode();
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

    // PSX: SetAnim__13HumanoidModelllil (MHUMAN.CPP:166)
    void SetAnim(s32 animEnum, s32 a3, s32 force, s32 extra) override;

    // PSX: _Loop__13HumanoidModelP13AnimStructure (MHUMAN.CPP:196)
    // Only loops when mode == 0 (normal animation playback).
    void HandleLoop(AnimStructure* anim) override;
};

// PlayerModel - player-specific model (136 bytes, same as HumanoidModel)
// PSX: _11PlayerModel
class PlayerModel : public HumanoidModel {
public:
    PlayerModel();
    ~PlayerModel() override;

    // PSX: SetAnim__11PlayerModelllil (MPLAYER.CPP:293)
    void SetAnim(s32 animEnum, s32 a3, s32 force, s32 extra) override;

    // PSX: _Loop__11PlayerModelP13AnimStructure (MPLAYER.CPP:573)
    void HandleLoop(AnimStructure* anim) override;
    // PSX: _RunToLast__11PlayerModelP13AnimStructure (MPLAYER.CPP:374)
    void HandleRunToLast(AnimStructure* anim) override;
    // PSX: _IncFrame__11PlayerModelP13AnimStructure (MPLAYER.CPP:578)
    void HandleIncFrame(AnimStructure* anim) override;
};
