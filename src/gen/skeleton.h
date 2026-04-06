// skeleton.h - game-side skeleton utilities
#pragma once
#include "p3d/skeleton.h"

// Parse the P3D stream (0xFF04 container) extracting BOTH textures (to VRAM)
// and skeleton data (returned). Textures are uploaded via World::UploadToVRAM.
// Returns the STreeData if found, nullptr otherwise.
STreeData* ParseP3DStreamFull(const u8* data, u32 size);

// Parse the raw tTransformAnim binary blob and apply frame-0 values to skeleton joints.
// PSX: tTranAnimLoader2::Load relocates the blob, then UpdateJoints writes to joints.
// This reads the pre-relocation binary directly and sets rotation/translation on joints.
void ApplyAnimFrame0(STreeData* skeleton, const u8* rawAnimData, u32 rawAnimSize);

// Build a combined pddiPrimBuffer from tPrimGeom data using skeleton joint info.
// Transforms vertices into model space using skeleton joint world matrices.
// Stores combined mesh in skeleton->joints[0].meshBuffer.
void BuildPerJointMeshes(STreeData* skeleton, const u8* primGeomData, u32 primGeomSize);
