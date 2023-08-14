/*
 * File: DoorBars.c
 * Overlay: ovl_DoorBars
 * Description: Iron bars that can interact with switches
 */

#include "DoorBars.h"
#include "terminal.h"
#include "assets/objects/object_gnd/object_gnd.h"

#define FLAGS ACTOR_FLAG_4

void DoorBars_Init(Actor* thisx, PlayState* play);
void DoorBars_Destroy(Actor* thisx, PlayState* play);
void DoorBars_Update(Actor* thisx, PlayState* play);
void DoorBars_Draw(Actor* thisx, PlayState* play);

void DoorBars_IsOpen(DoorBars* this, PlayState* play);
void DoorBars_IsClosed(DoorBars* this, PlayState* play);
void DoorBars_CS(DoorBars* this, PlayState* play);


ActorInit DoorBars_InitVars = {
    ACTOR_DOORBARS,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_GND,
    sizeof(DoorBars),
    (ActorFunc)DoorBars_Init,
    (ActorFunc)DoorBars_Destroy,
    (ActorFunc)DoorBars_Update,
    (ActorFunc)DoorBars_Draw,
};

void DoorBars_SetupAction(DoorBars* this, DoorBarsActionFunc actionFunc) {
    this->actionFunc = actionFunc;
}

void DoorBars_Init(Actor* thisx, PlayState* play) {
	DoorBars* this = (DoorBars*)thisx;
	CollisionHeader* colHeader = NULL;

	DynaPolyActor_Init(&this->dyna, 0);
    CollisionHeader_GetVirtual(&gPhantomGanonBarsCol, &colHeader);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, thisx, colHeader);

    thisx->scale.x = 1.0f; //WHY THE FUCK DO YOU NEED THIS REEEEEEEEEEEEEEEEEEEEEEE IT SHOULD BE AT 1 FROM THE START REEEEEEEEEEEE
    thisx->scale.y = 1.0f;
    thisx->scale.z = 1.0f;

    if (Flags_GetSwitch(play, this->dyna.actor.params & 0x3F)) { //isOpen
    	thisx->world.pos.y = thisx->home.pos.y - 125.0f;
    	DoorBars_SetupAction(this,DoorBars_IsOpen);
    } else { 											    //isNOTopen
    	DoorBars_SetupAction(this,DoorBars_IsClosed);
    }

}

void DoorBars_IsOpen(DoorBars* this, PlayState* play) {
	if (!Flags_GetSwitch(play, this->dyna.actor.params & 0x3F)) { //if closed
		Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_METALDOOR_CLOSE);
		this->timer = 35;
		DoorBars_SetupAction(this,DoorBars_CS);
	}
}

void DoorBars_IsClosed(DoorBars* this, PlayState* play) {
	if (Flags_GetSwitch(play, this->dyna.actor.params & 0x3F)) { //if open
		Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_METALDOOR_OPEN);
		this->timer = 35;
		DoorBars_SetupAction(this,DoorBars_CS);
	}
}

void DoorBars_CS(DoorBars* this, PlayState* play) {
/*
	Math_SmoothStepToF(&this->dyna.actor.world.pos.y, this->dyna.actor.home.pos.y - 125.0f , 1.0f , 5.0f , 0.1f );
	if (this->dyna.actor.world.pos.y == this->dyna.actor.home.pos.y - 125.0f )
		DoorBars_SetupAction(this,DoorBars_IsOpen);
*/
	Player* player = GET_PLAYER(play);
	Vec3f targetpos;
	Vec3f camerapos;

	if (this->timer-- == 0) {

        Play_ReturnToMainCam(play, this->cutsceneCamera, 0);
        this->cutsceneCamera = 0;
        Cutscene_StopManual(play, &play->csCtx);
        func_8002DF54(play, &this->dyna.actor, 7);

        if (Flags_GetSwitch(play, this->dyna.actor.params & 0x3F)) {
        	this->dyna.actor.world.pos.y = this->dyna.actor.home.pos.y - 125.0f;
        	DoorBars_SetupAction(this,DoorBars_IsOpen);
        } else {
        	this->dyna.actor.world.pos.y = this->dyna.actor.home.pos.y;
        	DoorBars_SetupAction(this,DoorBars_IsClosed);
        }

    } else {

    	if (Flags_GetSwitch(play, this->dyna.actor.params & 0x3F)) {
    		Math_SmoothStepToF(&this->dyna.actor.world.pos.y, this->dyna.actor.home.pos.y - 125.0f , 1.0f , 5.0f , 0.1f );
    	} else {
    		Math_SmoothStepToF(&this->dyna.actor.world.pos.y, this->dyna.actor.home.pos.y, 1.0f , 5.0f , 0.1f );
    	}

        Cutscene_StartManual(play, &play->csCtx);
        func_8002DF54(play, &this->dyna.actor, 1);
        Play_ClearAllSubCameras(play);
        this->cutsceneCamera = Play_CreateSubCamera(play);
        Play_ChangeCameraStatus(play, 0, 1);
        Play_ChangeCameraStatus(play, this->cutsceneCamera, 7);

        camerapos = player->actor.world.pos;
        camerapos.y = player->actor.world.pos.y + 90.0f;
        targetpos = this->dyna.actor.home.pos;
        targetpos.y = this->dyna.actor.home.pos.y + 20.0f;

        Play_SetCameraAtEye(play, this->cutsceneCamera, &targetpos, &camerapos);

    }



}

void DoorBars_Destroy(Actor* thisx, PlayState* play) {
	DoorBars* this = (DoorBars*)thisx;
    DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
}

void DoorBars_Update(Actor* thisx, PlayState* play) {
	DoorBars* this = (DoorBars*)thisx;
	this->actionFunc(this, play);
}

void DoorBars_Draw(Actor* thisx, PlayState* play) {
	GfxPrint printer;
    Gfx* gfx;
    s32 IsOpen;

    IsOpen = Flags_GetSwitch(play, thisx->params & 0x3F);

	OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

	gfx = POLY_OPA_DISP + 1;
    gSPDisplayList(OVERLAY_DISP++, gfx);

    GfxPrint_Init(&printer);
    GfxPrint_Open(&printer, gfx);

    GfxPrint_SetColor(&printer, 0, 255, 255, 255); 
    GfxPrint_SetPos(&printer, 1, 1);
    GfxPrint_Printf(&printer, "Is Open ? %d",IsOpen);

    gfx = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);

    gSPEndDisplayList(gfx++);
    gSPBranchList(POLY_OPA_DISP, gfx);
    POLY_OPA_DISP = gfx;

    Gfx_SetupDL_25Opa(play->state.gfxCtx);

    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, __FILE__, __LINE__),G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_OPA_DISP++, gPhantomGanonBarsDL);

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

}



