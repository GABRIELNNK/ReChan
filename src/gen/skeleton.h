#pragma once
#include "p3d/skeleton.h"

struct CompositeAnimData;

// Parse the P3D stream (0xFF04 container) extracting BOTH textures (to VRAM)
// and skeleton data (returned). Textures are uploaded via World::UploadToVRAM.
// When requested, also preserves the raw character composite animation
// definition from chunk 0x4007. textureScope (e.g. the character name) is used
// to look up mod texture overrides nested under "{textureScope}/{name}".
STreeData* ParseP3DStreamFull(const u8* data,
                              u32 size,
                              CompositeAnimData** outCompositeAnim = nullptr,
                              u32 expectedCompositeNameUID = 0,
                              const char* textureScope = nullptr);

// Parse the raw tTransformAnim binary blob and apply frame-0 values to skeleton joints.
// PSX: tTranAnimLoader2::Load relocates the blob, then UpdateJoints writes to joints.
// This reads the pre-relocation binary directly and sets rotation/translation on joints.
void ApplyAnimFrame0(STreeData* skeleton, const u8* rawAnimData, u32 rawAnimSize);

struct OriginalSTree;

// Build a combined pddiPrimBuffer from tPrimGeom data using skeleton joint info.
// Transforms vertices into model space using skeleton joint world matrices.
// Stores combined mesh in skeleton->joints[0].meshBuffer.
// Also stores SkinData on OriginalSTree for per-frame CPU skinning.
void BuildPerJointMeshes(OriginalSTree* original, const u8* primGeomData, u32 primGeomSize);
