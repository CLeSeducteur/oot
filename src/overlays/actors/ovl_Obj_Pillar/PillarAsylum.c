/*
 * File: ThePillar.c
 * Overlay: ovl_Obj_Pillar
 * Description: Pillar Asylum Destructible
 */

#include "PillarAsylum.h"
#include "assets/objects/object_jya_iron/object_jya_iron.h"

void ThePillar_Init(ThePillar* this, PlayState* play);
void ThePillar_Destroy(ThePillar* this, PlayState* play);
void ThePillar_Update(ThePillar* this, PlayState* play);
void ThePillar_Draw(ThePillar* this, PlayState* play);

#define FLAGS 0

const ActorInit Obj_Pillar_InitVars = {
    ACTOR_OBJ_PILLAR, //actor number
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_JYA_IRON, //object number
    sizeof(ThePillar),
    (ActorFunc)ThePillar_Init,
    (ActorFunc)ThePillar_Destroy,
    (ActorFunc)ThePillar_Update,
    (ActorFunc)ThePillar_Draw,
};


static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_NONE,
        AT_NONE,
        AC_ON | AC_TYPE_ENEMY,
        OC1_NONE,
        OC2_TYPE_2,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0x00000000, 0x00, 0x00 },
        { 0xFFCFFFFF, 0x00, 0x00 },
        TOUCH_NONE,
        BUMP_ON,
        OCELEM_NONE,
    },
    { 30, 150, 0, { 0, 0, 0 } },
};


void Pillar_InitCylinder(ThePillar* this, PlayState* play){

    Collider_InitCylinder(play, &this->colCylinder);
    Collider_SetCylinder(play, &this->colCylinder,&this->dyna.actor, &sCylinderInit);
    Collider_UpdateCylinder(&this->dyna.actor, &this->colCylinder);

}

void ThePillar_Init(ThePillar* this, PlayState* play) {

    CollisionHeader* colHeader = NULL;
    this->LeTimer = 0;
    this->secu = 0;

    Pillar_InitCylinder(this, play);
    CollisionHeader_GetVirtual(&PillarCol_collisionHeader, &colHeader);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);

    Actor_SetScale(&this->dyna.actor, 0.1f);
}

void ThePillar_Destroy(ThePillar* this, PlayState* play) {

    DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
}


void SpawnPillarParticles(ThePillar* this, PlayState* play, Actor* actor) {

    u8 i;
    s16 rotY;

    rotY = Actor_WorldYawTowardActor(&this->dyna.actor, actor);

    if (this->secu == 0){

        SfxSource_PlaySfxAtFixedWorldPos(play, &this->dyna.actor.world.pos, 80, NA_SE_EN_IRONNACK_BREAK_PILLAR2);

        this->secu = 1 ;

        for (i = 0; i < 8; i++ ){

            Actor* actor =
            Actor_Spawn(&play->actorCtx, play, ACTOR_BG_JYA_HAHENIRON, this->dyna.actor.world.pos.x,
                        Rand_ZeroOne() * 130.0f + this->dyna.actor.world.pos.y + 50.0f, this->dyna.actor.world.pos.z, 0,
                        (s16)(Rand_ZeroOne() * 0x4000) + rotY - 0x2000, 0, 0);
            if (actor != NULL) {
                actor->speed = Rand_ZeroOne() * 4.0f + 9.0f;
                actor->velocity.y = Rand_ZeroOne() * 5.0f + 6.0f;
            }
        }

        Actor_Kill(&this->dyna.actor);
    } 
}



void ColliderBehaviour(ThePillar* this, PlayState* play){

    Actor* actor;

     if (this->colCylinder.base.acFlags & AC_HIT) {

        actor = this->colCylinder.base.ac;
        this->colCylinder.base.acFlags &= ~AC_HIT;

        if (actor != NULL && actor->id == ACTOR_EN_ASYLUMDEMON) {

            SpawnPillarParticles(this, play ,actor);

            return;
        }

    } else {

        CollisionCheck_SetAC(play, &play->colChkCtx, &this->colCylinder.base);
    }
}


void ThePillar_Update(ThePillar* this, PlayState* play) {

    ColliderBehaviour(this,play);

    if (this->secu != 0){
        this->LeTimer += 1;
    }

    if (this->LeTimer > 8){
        Actor_Kill(&this->dyna.actor);
    }

}

void ThePillar_Draw(ThePillar* this, PlayState* play) {

    Gfx_DrawDListOpa(play, Pillar);

}