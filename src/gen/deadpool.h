#pragma once
#include "core.h"

// PSX: DeadPool (DEADPOOL.HPP, 88 bytes)
// Stores destroyed object UIDs for dead-pool-checked spawns (type 424).
class DeadPool {
public:
    s32 count = 0;
    u32 uids[21] = {};

    ~DeadPool();

    void InternalReset();
    s32 AddUID(u32 uid);
    s32 IsUIDInDeadPool(u32 uid) const;
};

static_assert(sizeof(DeadPool) == 88, "DeadPool size must match PSX layout");

extern DeadPool levelDeadPool;
