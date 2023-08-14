#ifndef PILLARASYLUM_H
#define PILLARASYLUM_H

#include "ultra64.h"
#include "global.h"

typedef struct ThePillar {
    DynaPolyActor dyna;
    ColliderCylinder colCylinder;
    u8 secu;
    u16 LeTimer;
} ThePillar;

#endif
