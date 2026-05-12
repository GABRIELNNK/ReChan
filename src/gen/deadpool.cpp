#include "gen/deadpool.h"

DeadPool levelDeadPool;

DeadPool::~DeadPool() {
    MARKFUNCTION(0x8008D4D8);
}

void DeadPool::InternalReset() {
    MARKFUNCTION(0x8008D40C);
    count = 0;
}

s32 DeadPool::AddUID(u32 uid) {
    MARKFUNCTION(0x8008D414);

    uids[count] = uid;
    count = count + 1;
    return count;
}

s32 DeadPool::IsUIDInDeadPool(u32 uid) const {
    MARKFUNCTION(0x8008D43C);

    s32 index = count - 1;
    if (index < 0) {
        return 0;
    }

    for (const u32* slot = &uids[index]; ; --slot) {
        index--;
        if (*slot == uid) {
            break;
        }

        if (index < 0) {
            return 0;
        }
    }

    return 1;
}
