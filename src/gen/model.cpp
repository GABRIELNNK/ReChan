// model.cpp - Model class hierarchy implementation
// Reversed from PSX MODEL.CPP / MHUMAN.CPP
#include "gen/model.h"
#include "gen/animmat.h"
#include "gen/animstruct.h"
#include "gen/skeleton.h"
#include "p3d/context.h"
#include "p3d/p3dmath.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include <cstdlib>

// OriginalSTree
OriginalSTree::OriginalSTree() {
    SetType(1); // STree type
}

OriginalSTree::~OriginalSTree() {
    if (meshBuffer) {
        meshBuffer->Release();
        meshBuffer = nullptr;
    }
    if (skeleton) {
        delete skeleton;
        skeleton = nullptr;
    }
}

// DrawableSTree
DrawableSTree::DrawableSTree(OriginalSTree* orig) {
    original = orig;
    alternate = nullptr;
    displayFlag = 1;
}

DrawableSTree::~DrawableSTree() {
    original = nullptr;
    alternate = nullptr;
}

// PSX: Display dispatches through vtable to OriginalSTree::Draw -> tPrimGeom::Display
// PC: draws the skeleton mesh (per-joint transforms baked in) or flat fallback
void DrawableSTree::Display(u32 /*flags*/) { 
    if (!original)
        return;

    // Prefer skeleton-built mesh (rest-pose transformed vertices)
    if (original->skeleton && original->skeleton->joints &&
        original->skeleton->joints[0].meshBuffer) {
        p3d::context->DrawPrimBuffer(original->skeleton->joints[0].meshBuffer);
        return;
    }

    // Fallback to flat mesh
    if (original->meshBuffer) {
        p3d::context->DrawPrimBuffer(original->meshBuffer);
    }
}

// Model
// PSX: _5Model (MODEL.CPP:671, 0x8006E654)
Model::Model() {
    MARKFUNCTION(0x8006E654);
    drawable = nullptr;
    drawableType = 0;
    animStructure = nullptr;
    field36 = nullptr;
    ambientLight = nullptr;
    hwLights = nullptr;
    hwLightCount = 0;
    rotX = 0;
    rotY = 0;
    rotZ = 0;
    posX = 0;
    posY = 0;
    posZ = 0;
    backPtr = nullptr;
    modelFlags = 0;
    Reset();
}

// PSX: __5Model (MODEL.CPP:697, 0x8006E6CC)
Model::~Model() {
    MARKFUNCTION(0x8006E6CC);
    DeleteDrawable();
}

// PSX: Reset__5Model (MODEL.CPP:777, 0x8006E86C)
void Model::Reset() {
    MARKFUNCTION(0x8006E86C);
    shadowAngle = 0;
}

void Model::Show(u32 /*flags*/) {
    // Base does nothing - overridden by SModel/GModel
}

void Model::Animate() {
    // Base does nothing - overridden by SModel
}

void Model::ApplyAnimToModel(s32 /*thingType*/, s32 /*animEnum*/, s32 /*p3*/, s32 /*p4*/, s32 /*p5*/) {
    // Base does nothing - overridden by SModel
}

void Model::DeleteDrawable() {
    if (drawable) {
        delete drawable;
        drawable = nullptr;
    }
    drawableType = 0;
}

// SModel
// PSX: _6SModel (MODEL.CPP:1013, 0x8006ED68)
SModel::SModel() {
    MARKFUNCTION(0x8006ED68);
    scale = FIX16_ONE;
    field92 = 0;
}

SModel::~SModel() {
    MARKFUNCTION(0x8006EDAC);
}

// PSX: Show__6SModelUl (MODEL.CPP:1454, 0x8006F68C)
// PC adaptation: sets world matrix from position/rotation/scale, then draws mesh
void SModel::Show(u32 flags) {
    MARKFUNCTION(0x8006F68C);

    // PSX: clear visible/drawn bits
    modelFlags &= ~0x30;

    if (!drawable || !backPtr)
        return;

    // Mark as visible + drawn
    modelFlags |= 0x50;

    // Build world matrix from position + rotation + scale
    // PSX: TransMatrix, RotMatrixZYXAndLights, ScaleMatrix (from MIPS LST)
    Mat4 world;
    p3dBuildRotMatrixZYX(rotX, rotY, rotZ, world);

    // Scale: PSX FIX16_ONE = 1.0
    f32 s = FIX16_TO_FLOAT(scale);
    world.ScaleRotation(s);

    // Translation
    world.SetTranslation((f32)posX, (f32)posY, (f32)posZ);

    p3d::context->SetWorldMatrix(world);

    // PSX: calls drawable->Display(flags) through vtable
    drawable->Display(flags);
}

// PSX: Animate__6SModel (MODEL.CPP:1416, 0x8006F640)
void SModel::Animate() {
    MARKFUNCTION(0x8006F640);
    if (animStructure) {
        AnimStructure* anim = (AnimStructure*)animStructure;
        anim->ExecuteHandler(1);
    }
}

// PSX: SetOriginalSTree__6SModelP13OriginalSTreeP10tAnimation (MODEL.CPP:1026, 0x8006EDD4)
void SModel::SetOriginalSTree(OriginalSTree* original) {
    MARKFUNCTION(0x8006EDD4);
    DeleteDrawable();
    drawable = new DrawableSTree(original);
    drawableType = 2; // STree type
}

// PSX: InitSemiTransMode__6SModel (MODEL.CPP:1045, 0x8006EE20)
void SModel::InitSemiTransMode() {
    MARKFUNCTION(0x8006EE20);
    // PSX: calls SetSemiMode on the OriginalSTree - no-op on PC for now
}

// HumanoidModel
// PSX: _13HumanoidModel (MHUMAN.CPP:45, 0x8006E020)
HumanoidModel::HumanoidModel() {
    MARKFUNCTION(0x8006E020);
    // PSX allocates AnimationMatrices (660 bytes on PSX layout).
    animMatrices = new AnimationMatrices();
    attackHandRadius = 100;
    attackFootRadius = 100;
    field108 = 400;
    field112 = 0;
    field116 = 0;
    field120 = 0;
    field124 = 0;
    field128 = 0;
    field132 = INVALID_HANDLE;
}

// PSX: __13HumanoidModel (MHUMAN.CPP:56, 0x8006E0C8)
HumanoidModel::~HumanoidModel() {
    MARKFUNCTION(0x8006E0C8);
    if (animMatrices) {
        delete animMatrices;
        animMatrices = nullptr;
    }
}

// PlayerModel
PlayerModel::PlayerModel() {
    // Same as HumanoidModel on PSX
}

PlayerModel::~PlayerModel() {
}
