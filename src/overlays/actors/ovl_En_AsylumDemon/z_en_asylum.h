#ifndef Z_EN_ASYLUM_H
#define Z_EN_ASYLUM_H

#include "ultra64.h"
#include "global.h"

struct EnAsylum;

typedef void (*EnAsylumActionFunc)(struct EnAsylum*, PlayState*);

typedef struct EnAsylum {
    Actor actor;
    SkelAnime skelAnime;
    Vec3s bodyPartsPos[10];
    ColliderCylinder AssCollider;
    ColliderJntSph colliderSpheres;
    ColliderJntSphElement colliderSpheresElements[10];
    ColliderTris shieldCollider;
    ColliderTrisElement shieldColliderItems[4];
    u16 timerone;
    u8 ParticleBool;
    u8 IsAlive;

    u8 DemonState;
    Vec3f LinkNewPos;
    Vec3s LinkNewRot;

    EnAsylumActionFunc actionFunc;
} EnAsylum;

void PlayerCall_Draw(Actor* thisx, PlayState* play);

#endif
