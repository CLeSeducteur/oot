#ifndef DOORBARS_H
#define DOORBARS_H

#include "ultra64.h"
#include "global.h"

struct DoorBars;

typedef void (*DoorBarsActionFunc)(struct DoorBars*, PlayState*);

typedef struct DoorBars {
    DynaPolyActor dyna;
    DoorBarsActionFunc actionFunc;
    s16 timer;
    s16 cutsceneCamera;
} DoorBars;

#endif
