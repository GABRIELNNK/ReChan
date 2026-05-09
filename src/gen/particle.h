#pragma once

#include "core.h"
#include "gen/cclist.h"
#include "p3d/lvector.h"

struct ParticleStats;
class ParticleSystem;

class ParticleSystemMgr {
public:
    ParticleSystemMgr();
    explicit ParticleSystemMgr(ParticleSystem* system);
    ~ParticleSystemMgr();

    void InitMgr(ParticleSystem* system);
    s32 CreateParticles(const LVector& origin, ParticleStats* statsOverride = nullptr);
    s32 SetParticleDirection(const LVector* direction);
    s32 ResetParticleDirection();
    void SetDisplayOffset(const LVector* offset);
    s32 Update();
    void Display();
    s32 ActiveParticles();
    s32 PurgeParticles();
    u32 GetSystemHash() const;

    ParticleSystem* GetSystem() const {
        return system;
    }

private:
    void BindSystemList();

    ParticleSystem* system = nullptr;
    LVector direction = {};
    ccMinList particles;
};

// PSX: Load__14ParticleSystemR10tReadChunkPPv (PARTICLE.CPP:205, 0x80095494)
s32 ParticleSystem_LoadChunk(const u8* body, u32 bodySize);

// PSX: InitParticleInfoMemory__14ParticleSystem (PARTICLE.CPP:312, 0x80095610)
s32 ParticleSystem_InitParticleInfoMemory();

// PSX: Unload__14ParticleSystem (PARTICLE.CPP:346, 0x800956D0)
void ParticleSystem_Unload();

// PSX: UnloadLevel__14ParticleSystem (PARTICLE.CPP:370, 0x80095740)
void ParticleSystem_UnloadLevel();

// PSX: CommonParticles__14ParticleSystemi (PARTICLE.CPP:408, 0x80095854)
void ParticleSystem_CommonParticles(s32 count);

// PSX: Find__14ParticleSystemUl (PARTICLE.CPP:908, 0x80096318)
ParticleSystem* ParticleSystem_Find(u32 hash);
