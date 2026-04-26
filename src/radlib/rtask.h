#pragma once

#include "core.h"

struct RTASK;
typedef s32 (*RTASKFUNC)(RTASK*);

struct RTASK {
    RTASK* next = nullptr;
    u32 when = 0;
    RTASKFUNC func = nullptr;
    intptr_t param1 = 0;
    intptr_t param2 = 0;
};

struct RTASK_BUCKET {
    RTASK* head = nullptr;
    RTASK** tail = nullptr;
};

struct RTASKLIST {
    u32 time = 0;
    u32 busy = 0;
    RTASK_BUCKET immediate;
    RTASK_BUCKET overflow;
    RTASK_BUCKET buckets[32];
};

extern RTASKLIST rMainTaskList;
extern RTASKLIST rFrameTaskList;
extern s32 rFrameCount;
extern s32 rFrameCount60;

void rTaskInit(s32 taskCount);
bool rInitTaskList(RTASKLIST* taskList);
s32 rTaskSuicide(RTASK* task);
RTASKFUNC rDelTask(RTASKLIST* taskList, RTASK* task);
u32 rDoTaskList(RTASKLIST* taskList, u32 untilTime);
RTASK* rNewTask(RTASKLIST* taskList, RTASKFUNC func, s32 delay);
RTASK* rNewTaskP(RTASKLIST* taskList, RTASKFUNC func, s32 delay, intptr_t param1);
RTASK* rNewTaskPP(RTASKLIST* taskList, RTASKFUNC func, s32 delay, intptr_t param1, intptr_t param2);