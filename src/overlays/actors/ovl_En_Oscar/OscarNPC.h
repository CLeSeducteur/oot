#ifndef OSCARNPC_H
#define OSCARNPC_H

#include "ultra64.h"
#include "global.h"

struct EnOscar;

typedef void (*EnOscarActionFunc)(struct EnOscar*, PlayState*);

typedef struct EnOscar {
    Actor actor;
    SkelAnime skelAnime;
    EnOscarActionFunc actionFunc;

    ColliderJntSph colliderSpheres;
    ColliderJntSphElement colliderSpheresElements[1];

    u16 VariableUsedToTestThings;
    u8 TalkState;
    s32 BigBrain;
} EnOscar;

#endif
