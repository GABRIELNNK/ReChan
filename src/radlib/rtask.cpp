#include "radlib/rtask.h"

#include <cstdlib>

RTASKLIST rMainTaskList = {};
RTASKLIST rFrameTaskList = {};
// PSX MAIN.CPP uses these as VBlank-updated frame flags.
s32 rFrameCount = 0;
s32 rFrameCount60 = 0;

struct RTASK_BLOCK {
    RTASK_BLOCK* next;
    RTASK* tasks;
    s32 count;
};

static RTASK_BLOCK* s_taskBlocks = nullptr;
static RTASK* s_taskFreeList = nullptr;
static s32 s_taskOverflowSize = 20;

static void InitTaskBucket(RTASK_BUCKET* bucket) {
    bucket->head = reinterpret_cast<RTASK*>(bucket);
    bucket->tail = reinterpret_cast<RTASK**>(bucket);
}

static void ReleaseTaskBlocks() {
    while (s_taskBlocks) {
        RTASK_BLOCK* next = s_taskBlocks->next;
        std::free(s_taskBlocks->tasks);
        std::free(s_taskBlocks);
        s_taskBlocks = next;
    }

    s_taskFreeList = nullptr;
}

static bool AllocateTaskBlock(s32 count) {
    if (count <= 0) {
        return false;
    }

    RTASK_BLOCK* block = static_cast<RTASK_BLOCK*>(std::malloc(sizeof(RTASK_BLOCK)));
    if (!block) {
        return false;
    }

    RTASK* tasks = static_cast<RTASK*>(std::malloc(sizeof(RTASK) * count));
    if (!tasks) {
        std::free(block);
        return false;
    }

    block->next = s_taskBlocks;
    block->tasks = tasks;
    block->count = count;
    s_taskBlocks = block;

    for (s32 i = 0; i < count - 1; i++) {
        tasks[i].next = &tasks[i + 1];
        tasks[i].when = 0;
        tasks[i].func = nullptr;
        tasks[i].param1 = 0;
        tasks[i].param2 = 0;
    }

    tasks[count - 1].next = s_taskFreeList;
    tasks[count - 1].when = 0;
    tasks[count - 1].func = nullptr;
    tasks[count - 1].param1 = 0;
    tasks[count - 1].param2 = 0;
    s_taskFreeList = tasks;
    return true;
}

static RTASK* AllocTask() {
    if (!s_taskFreeList && !AllocateTaskBlock(s_taskOverflowSize)) {
        return nullptr;
    }

    RTASK* task = s_taskFreeList;
    s_taskFreeList = task->next;
    task->next = nullptr;
    task->when = 0;
    task->func = nullptr;
    task->param1 = 0;
    task->param2 = 0;
    return task;
}

static void FreeTask(RTASK* task) {
    if (!task) {
        return;
    }

    task->next = s_taskFreeList;
    s_taskFreeList = task;
}

// PSX: rInsertTask__FP10_RTASKLISTP6_RTASK (RTASK.CPP:72, 0x8002E978)
static RTASK* rInsertTask(RTASKLIST* taskList, RTASK* task) {
    MARKFUNCTION(0x8002E978);

    RTASK_BUCKET* bucket = &taskList->immediate;
    if (task->when >= taskList->time) {
        if (task->when >= taskList->time + 32) {
            bucket = &taskList->overflow;
        }
        else {
            bucket = &taskList->buckets[task->when & 0x1F];
        }
    }

    *bucket->tail = task;
    bucket->tail = &task->next;
    return task;
}

// PSX: rExecuteTaskBucket__FP10_RTASKLISTP13_RTASK_BUCKET (RTASK.CPP:115, 0x8002E9D0)
static RTASK* rExecuteTaskBucket(RTASKLIST* taskList, RTASK_BUCKET* bucket) {
    MARKFUNCTION(0x8002E9D0);

    RTASK** tail = bucket->tail;
    *tail = nullptr;

    RTASK* task = bucket->head;
    InitTaskBucket(bucket);

    RTASK* result = nullptr;
    while (task) {
        RTASK* next = task->next;
        s32 delay = task->func ? task->func(task) : -1;
        task->when += static_cast<u32>(delay);

        if (delay < 0) {
            FreeTask(task);
        }
        else {
            result = rInsertTask(taskList, task);
        }

        task = next;
    }

    return result;
}

// PSX: rReInsertTaskBucket__FP10_RTASKLISTP13_RTASK_BUCKET (RTASK.CPP:149, 0x8002EA6C)
static RTASK* rReInsertTaskBucket(RTASKLIST* taskList, RTASK_BUCKET* bucket) {
    MARKFUNCTION(0x8002EA6C);

    RTASK** tail = bucket->tail;
    *tail = nullptr;

    RTASK* task = bucket->head;
    InitTaskBucket(bucket);

    RTASK* result = nullptr;
    while (task) {
        RTASK* next = task->next;
        if (task->when >= taskList->time + 32) {
            *taskList->overflow.tail = task;
            taskList->overflow.tail = &task->next;
            result = task;
        }
        else {
            RTASK_BUCKET* dst = &taskList->buckets[task->when & 0x1F];
            RTASK** oldTail = dst->tail;
            task->next = dst->head;
            dst->head = task;
            if (oldTail == reinterpret_cast<RTASK**>(dst)) {
                dst->tail = &task->next;
            }
            result = task;
        }

        task = next;
    }

    return result;
}

// PSX: rTaskInit (RTASK.CPP:25, 0x8002E8DC)
void rTaskInit(s32 taskCount) {
    MARKFUNCTION(0x8002E8DC);
    ReleaseTaskBlocks();
    s_taskOverflowSize = 20;
    AllocateTaskBlock(taskCount);
}

// PSX: rInitTaskList (RTASK.CPP:38, 0x8002E910)
bool rInitTaskList(RTASKLIST* taskList) {
    MARKFUNCTION(0x8002E910);

    InitTaskBucket(&taskList->immediate);
    InitTaskBucket(&taskList->overflow);
    for (s32 i = 0; i < 32; i++) {
        InitTaskBucket(&taskList->buckets[i]);
    }

    taskList->busy = 0;
    taskList->time = 0;
    return false;
}

// PSX: rTaskSuicide (RTASK.CPP:62, 0x8002E960)
s32 rTaskSuicide(RTASK* task) {
    MARKFUNCTION(0x8002E960);
    (void)task;
    return -1;
}

// PSX: rDelTask (RTASK.CPP:67, 0x8002E968)
RTASKFUNC rDelTask(RTASKLIST* taskList, RTASK* task) {
    MARKFUNCTION(0x8002E968);
    (void)taskList;
    task->func = rTaskSuicide;
    return rTaskSuicide;
}

// PSX: rDoTaskList (RTASK.CPP:180, 0x8002EAFC)
u32 rDoTaskList(RTASKLIST* taskList, u32 untilTime) {
    MARKFUNCTION(0x8002EAFC);

    u32 result = taskList->busy;
    if (result) {
        return result;
    }

    rExecuteTaskBucket(taskList, &taskList->immediate);
    while (true) {
        u32 currentTime = taskList->time;
        result = untilTime < currentTime;
        if (result) {
            break;
        }

        taskList->time = currentTime + 1;
        rExecuteTaskBucket(taskList, &taskList->buckets[currentTime & 0x1F]);
        if ((taskList->time & 0x1F) == 0) {
            rReInsertTaskBucket(taskList, &taskList->overflow);
        }
    }

    return result;
}

// PSX: rNewTask (RTASK.CPP:210, 0x8002EB90)
RTASK* rNewTask(RTASKLIST* taskList, RTASKFUNC func, s32 delay) {
    MARKFUNCTION(0x8002EB90);

    RTASK* task = AllocTask();
    if (!task) {
        return nullptr;
    }

    task->func = func;
    task->when = taskList->time + static_cast<u32>(delay);
    return rInsertTask(taskList, task);
}

// PSX: rNewTaskP (RTASK.CPP:223, 0x8002EBEC)
RTASK* rNewTaskP(RTASKLIST* taskList, RTASKFUNC func, s32 delay, intptr_t param1) {
    MARKFUNCTION(0x8002EBEC);

    RTASK* task = rNewTask(taskList, func, delay);
    if (task) {
        task->param1 = param1;
    }
    return task;
}

// PSX: rNewTaskPP (RTASK.CPP:231, 0x8002EC14)
RTASK* rNewTaskPP(RTASKLIST* taskList, RTASKFUNC func, s32 delay, intptr_t param1, intptr_t param2) {
    MARKFUNCTION(0x8002EC14);

    RTASK* task = rNewTaskP(taskList, func, delay, param1);
    if (task) {
        task->param2 = param2;
    }
    return task;
}