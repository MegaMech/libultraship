#pragma once

#include "pi.h"

typedef struct {
    /* 0x00 */ u32 initialized;
    /* 0x04 */ OSThread* mgrThread;
    /* 0x08 */ OSMesgQueue* cmdQueue;
    /* 0x0C */ OSMesgQueue* eventQueue;
    /* 0x10 */ OSMesgQueue* accessQueue;
    /* 0x14 */ s32 (*piDmaCallback)(s32, u32, void*, size_t);
    /* 0x18 */ s32 (*epiDmaCallback)(OSPiHandle*, s32, u32, void*, size_t);
} OSMgrArgs; // size = 0x1C

typedef struct {
    /* 0x00 */ OSMesgQueue* queue;
    /* 0x04 */ OSMesg msg;
} __OSEventState; // size = 0x08

#ifdef __cplusplus
extern "C" {
#endif
extern OSMgrArgs __osPiDevMgr;
extern __OSEventState __osEventStateTab[];
#ifdef __cplusplus
}
#endif
