#include "mycustomactor.h"
#include "assets/objects/object_blackcrow/blackcrow_skel.h" /*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*/
#include "assets/scenes/test_levels/nua_test/nua_test_scene.h"

#define FLAGS (ACTOR_FLAG_25 | ACTOR_FLAG_4 | ACTOR_FLAG_5 )

void CustomActor_Init(CustomActor* this, PlayState* play);
void CustomActor_Destroy(CustomActor* this, PlayState* play);
void CustomActor_Update(CustomActor* this, PlayState* play);
void CustomActor_Draw(CustomActor* this, PlayState* play);

//Fly Stuff
void CustomActor_PathInit(CustomActor* this, PlayState* play);
void CustomActor_FlySky(CustomActor* this, PlayState* play);
void CustomActor_WaitForC(CustomActor* this, PlayState* play);
void CustomActor_FlyCurve(CustomActor* this, PlayState* play);
void CustomActor_WaitEndCS(CustomActor* this, PlayState* play);

//GroundStuff
void CustomActor_ChangeFlags(CustomActor* this, PlayState* play);
void CustomActor_GroundBehaviour(CustomActor* this, PlayState* play);

//Misc
void CustomActor_ChangeMode(CustomActor* this, SkelAnime* skelAnime, AnimationHeader* animation);

void CustomActor_TestingShit(CustomActor* this, PlayState* play);

//AsylumStuff
void CustomActor_WaitForPlayer(CustomActor* this, PlayState* play);
void CustomActor_CrowAppear(CustomActor* this, PlayState* play);
void CustomActor_CrowDrawsIn(CustomActor* this, PlayState* play);
void CustomActor_CrowGrabsPlayer(CustomActor* this, PlayState* play);
void CustomActor_FlyAway(CustomActor* this, PlayState* play);

const ActorInit MyCustomActor_InitVars = {
    ACTOR_MYCUSTOM,
    ACTORCAT_PROP, //necessary for cutscene
    FLAGS,
    OBJECT_BLACKCROW,
    sizeof(CustomActor),
    (ActorFunc)CustomActor_Init,
    (ActorFunc)CustomActor_Destroy,
    (ActorFunc)CustomActor_Update,
    (ActorFunc)CustomActor_Draw,
};

void CustomActor_SetupAction(CustomActor* this, CustomActorActionFunc actionFunc) {
    this->actionFunc = actionFunc;
}

void CustomActor_Init(CustomActor* this, PlayState* play) {

    Path* path = &play->setupPathList[0x0000];

    SkelAnime_InitFlex(play, &this->skelAnime, &blackcrow_skel, &blackcrow_skelCrow_flyAnim, NULL, NULL, 18); //FlyingSkeleton
    SkelAnime_InitFlex(play, &this->skelAnime2, &blackcrow_skel2, &blackcrow_skel2TestrestAnim , NULL, NULL, 12); //GroundSkeleton

    Actor_SetScale(&this -> actor, 0.04f);

    if (play->sceneId == SCENE_FIRELINKSHRINE) { // if crow in firelink

        if (play->csCtx.state != CS_STATE_IDLE) { // Play CS

            CustomActor_ChangeMode(this, &this->skelAnime, &blackcrow_skelCrow_flyAnim);
    
            this->timer = 0;
            /////////////Init Path
            this->endWaypoint = path->count - 1;
            this->currentWaypoint = 0;
            this->nextWaypoint = 1;
            this->pathDirection = 1;
            this->FrameCount = 45.0f;

            CustomActor_PathInit(this, play);
            this->actor.shape.rot.y = this->actor.world.rot.y;
            this->actor.shape.rot.z = this->actor.world.rot.z + 0x3ffe;
            CustomActor_SetupAction(this, CustomActor_FlySky);

        } else { // CS was already played

            CustomActor_ChangeMode(this, &this->skelAnime2, &blackcrow_skel2TestrestAnim);
         
            CustomActor_ChangeFlags(this,play); 
            this->actor.shape.rot.z = this->actor.world.rot.z;
            this->actor.shape.rot.y = this->actor.world.rot.y + 0x3ffe;
            CustomActor_SetupAction(this, CustomActor_GroundBehaviour);
        }

    } else if (play->sceneId == SCENE_NUA_TEST) { // if crow in asylum
        CustomActor_ChangeMode(this, &this->skelAnime, &blackcrow_skelCrowrevealAnim);
        this->InBounds = 1;
        this->timer = 0;
        /////////////Init Path
        this->endWaypoint = path->count - 1;
        this->currentWaypoint = 0;
        this->nextWaypoint = 1;
        this->pathDirection = 1;
        this->FrameCount = 10.0f;
        CustomActor_SetupAction(this, CustomActor_WaitForPlayer);
    }

}

void CustomActor_ChangeMode(CustomActor* this, SkelAnime* skelAnime, AnimationHeader* animation) 
{
    this->curSkelAnime = skelAnime;
    Animation_Change(this->curSkelAnime, animation, 1.0f, 0.0f, Animation_GetLastFrame(animation), ANIMMODE_LOOP, 0.0f);
}

void CustomActor_ChangeFlags(CustomActor* this, PlayState* play)
{
    this->actor.flags &= ~ACTOR_FLAG_4; //No constant update
    this->actor.flags &= ~ACTOR_FLAG_5; //No constant draw
    this->actor.flags &= ~ACTOR_FLAG_25; // freeze when warp or ocarina
}

void CustomActor_WaitForPlayer(CustomActor* this, PlayState* play) //blackcrow_skelCrowrevealAnim
{
    if (this->actor.xzDistToPlayer < 350.0f) {
        this->InBounds = 2;
        play->csCtx.segment = SEGMENTED_TO_VIRTUAL(LeaveAsylum);
        gSaveContext.cutsceneTrigger = 1;
        CustomActor_SetupAction(this,CustomActor_CrowAppear);
    }
}

void CustomActor_CrowAppear(CustomActor* this, PlayState* play)
{
    this->timer++;
    if (this->timer == 245) {
        CustomActor_PathInit(this, play);
        //this->actor.shape.rot.y = this->actor.world.rot.y + 0x3ffe;
        this->actor.shape.rot.y = this->actor.world.rot.y = this->actor.yawTowardsPlayer;
    } else if (this->timer > 245) {

        SkelAnime_Update(this->curSkelAnime);

        Math_StepToF(&this->actor.world.pos.x, this->nextPosF.x, this->SpeedToPoint);
        Math_StepToF(&this->actor.world.pos.z, this->nextPosF.z, this->SpeedToPoint);
        Math_StepToF(&this->actor.world.pos.y, this->nextPosF.y, this->SpeedToPoint);

    }
    if (this->timer >= 255) { 
        this->currentWaypoint++ ;
        this->nextWaypoint++ ;
        this->FrameCount = 35.0f;
        CustomActor_PathInit(this, play);
        this->actor.shape.rot.y = this->actor.world.rot.y + 0x3ffe;

        Audio_PlayActorSfx2(&this->actor, NA_SE_EN_OWL_FLUTTER);

        Animation_Change(this->curSkelAnime, &blackcrow_skelSlowrevealAnim, 1.0f, 0.0f, Animation_GetLastFrame(&blackcrow_skelSlowrevealAnim), ANIMMODE_ONCE, 0.0f); //35 frames
        CustomActor_SetupAction(this, CustomActor_CrowDrawsIn);
    }

}

void CustomActor_CrowDrawsIn(CustomActor* this, PlayState* play) 
{
    this->timer++;
    if (this->timer >= 290) {
        this->currentWaypoint++ ;
        this->nextWaypoint++ ;
        this->FrameCount = 10.0f;
        CustomActor_PathInit(this, play);
        this->actor.shape.rot.y = this->actor.world.rot.y + 0x3ffe;
        Animation_Change(this->curSkelAnime, &blackcrow_skelCoverplayerAnim, 2.0f, 0.0f, Animation_GetLastFrame(&blackcrow_skelCoverplayerAnim), ANIMMODE_ONCE, 0.0f); //20 frames
        CustomActor_SetupAction(this, CustomActor_CrowGrabsPlayer);
    } else {
        SkelAnime_Update(this->curSkelAnime);

        Math_StepToF(&this->actor.world.pos.x, this->nextPosF.x, this->SpeedToPoint);
        Math_StepToF(&this->actor.world.pos.z, this->nextPosF.z, this->SpeedToPoint);
        Math_StepToF(&this->actor.world.pos.y, this->nextPosF.y, this->SpeedToPoint);
    }

}

void CustomActor_CrowGrabsPlayer(CustomActor* this, PlayState* play) 
{
    if (this->timer >= 300) {
        this->timer++;
        if (this->timer >= 320) {

            this->currentWaypoint+=2 ;
            this->nextWaypoint+=2 ;
            this->FrameCount = 400.0f;

            CustomActor_PathInit(this, play);
            this->actor.shape.rot.y = this->actor.world.rot.y ;
            this->actor.shape.rot.z = this->actor.world.rot.z + 0x3ffe;
            Animation_Change(this->curSkelAnime, &blackcrow_skelFlyupwardsAnim, 1.0f, 0.0f, Animation_GetLastFrame(&blackcrow_skelFlyupwardsAnim), ANIMMODE_ONCE, 0.0f);
            CustomActor_SetupAction(this, CustomActor_FlyAway);

        }
        if (this->timer == 268) {
            Audio_PlayActorSfx2(&this->actor, NA_SE_EN_KAICHO_FLUTTER);
        }
    } else {

        this->timer++;

        SkelAnime_Update(this->curSkelAnime);

        Math_StepToF(&this->actor.world.pos.x, this->nextPosF.x, this->SpeedToPoint);
        Math_StepToF(&this->actor.world.pos.z, this->nextPosF.z, this->SpeedToPoint);
        Math_StepToF(&this->actor.world.pos.y, this->nextPosF.y, this->SpeedToPoint);
    }
}

void CustomActor_FlyAway(CustomActor* this, PlayState* play)
{
    Player* player = GET_PLAYER(play);

    SkelAnime_Update(this->curSkelAnime);

    if (this->timer <= 720) {

        this->timer++;

        Math_StepToF(&this->actor.world.pos.x, this->nextPosF.x, this->SpeedToPoint);
        Math_StepToF(&this->actor.world.pos.z, this->nextPosF.z, this->SpeedToPoint);
        Math_StepToF(&this->actor.world.pos.y, this->nextPosF.y, this->SpeedToPoint);

        player->actor.world.pos.x = this->actor.world.pos.x;
        player->actor.world.pos.y = this->actor.world.pos.y - 30.0f;
        player->actor.world.pos.z = this->actor.world.pos.z;
        player->actor.world.rot.y = this->actor.world.rot.y -0x3ffe;

        if (this->timer == 335) {
            Audio_PlayActorSfx2(&this->actor, NA_SE_EN_OWL_FLUTTER);
        }

    }

    if (this->timer == 350) {
        Animation_MorphToLoop(this->curSkelAnime, &blackcrow_skelCrow_flyAnim, -4.0f);
    }

}

void CustomActor_PathInit(CustomActor* this, PlayState* play)
{
    Path* path = &play->setupPathList[0x0000];
    Vec3s* pointPos;
    Vec3s* nextPos;
    s16 pointAngle;
    this->endWaypoint = path->count - 1;

    ///////////////Go to starting point
    pointPos = (Vec3s*)SEGMENTED_TO_VIRTUAL(path->points) + this->currentWaypoint;

    this->actor.world.pos.x = pointPos->x;
    this->actor.world.pos.y = pointPos->y;
    this->actor.world.pos.z = pointPos->z; 

    //////////////Face nextPoint
    nextPos = (Vec3s*)SEGMENTED_TO_VIRTUAL(path->points) + this->nextWaypoint;

    this->nextPosF.x = nextPos->x;
    this->nextPosF.y = nextPos->y;
    this->nextPosF.z = nextPos->z;

    //////// Prepare the FlySky

    // rotate towards point (yaw)

    pointAngle = Math_Vec3f_Yaw(&this->actor.world.pos, &this->nextPosF) + 0x3ffe;
    this->actor.world.rot.y = pointAngle;

    //(ABS((s16)Math_Vec3f_Yaw(&this->actor.world.pos, &this->nextPosF) - this->actor.world.rot.y)) + 0x7FFC;

    //Speed to point
    this->SpeedToPoint =  Actor_WorldDistXYZToPoint(&this->actor, &this->nextPosF) / this->FrameCount; //frames = 44 (temps) v = d/t
    //

}

void CustomActor_TestingShit(CustomActor* this, PlayState* play)
{
    //creer new path faire follow le path et tester la rot pourquoi elle merde
    Player* player = GET_PLAYER(play);

    //s16 playerAngle = this->actor.yawTowardsPlayer + 0x3ffe;
    s16 playerAngle = Math_Vec3f_Yaw(&this->actor.world.pos, &player->actor.world.pos) + 0x3ffe;

    this->actor.world.rot.z = 0x3ffe;

    Math_SmoothStepToS(&this->actor.world.rot.y, playerAngle , 1, 0x444, 0x1);

    SkelAnime_Update(this->curSkelAnime);

}

void CustomActor_FlySky(CustomActor* this, PlayState* play) 
{

    if (this->timer <= 45 ) {
        SkelAnime_Update(this->curSkelAnime);

        Math_StepToF(&this->actor.world.pos.x, this->nextPosF.x, this->SpeedToPoint);
        Math_StepToF(&this->actor.world.pos.z, this->nextPosF.z, this->SpeedToPoint);
        Math_StepToF(&this->actor.world.pos.y, this->nextPosF.y, this->SpeedToPoint);

        this->timer++;
    }
    else
    {
        this->currentWaypoint++ ; // = 1
        this->nextWaypoint++ ; // = 2
        CustomActor_SetupAction(this, CustomActor_WaitForC);
    }
}

void CustomActor_WaitForC(CustomActor* this, PlayState* play)
{
    Player* player = GET_PLAYER(play);

    if (this->timer >= 246 ){

        this->currentWaypoint++ ; // = 2
        this->nextWaypoint++ ; // = 3
        this->FrameCount = 14.0f;

        CustomActor_PathInit(this, play);

        //Change anim
        Animation_Change(this->curSkelAnime, &blackcrow_skelFlyupwardsAnim, 1.0f, 0.0f, Animation_GetLastFrame(&blackcrow_skelFlyupwardsAnim), ANIMMODE_ONCE, 0.0f); //30 frames
        this->actor.shape.rot.y = this->actor.world.rot.y;
        this->actor.shape.rot.z = this->actor.world.rot.z = 0x3ffe;
        // tp link once
        player->actor.world.pos.x = this->actor.world.pos.x;
        player->actor.world.pos.y = this->actor.world.pos.y - 30.0f;
        player->actor.world.pos.z = this->actor.world.pos.z;

        CustomActor_SetupAction(this, CustomActor_FlyCurve);

    } else {

        this->timer++;

    }

}

void CustomActor_FlyCurve(CustomActor* this, PlayState* play)
{
    Player* player = GET_PLAYER(play);

    SkelAnime_Update(this->curSkelAnime);
    this->timer++;

    if ( ( 246 < this->timer) && (this->timer < 260 ) )
    {
        //fly point 2 à 3
        //tp link à peine en dessous de l'acteur à chaque frame

        player->actor.world.pos.x = this->actor.world.pos.x;
        player->actor.world.pos.y = this->actor.world.pos.y - 30.0f;
        player->actor.world.pos.z = this->actor.world.pos.z;
        player->actor.shape.rot.y = this->actor.world.rot.y - 0x3ffe;

        Math_StepToF(&this->actor.world.pos.x, this->nextPosF.x, this->SpeedToPoint);
        Math_StepToF(&this->actor.world.pos.z, this->nextPosF.z, this->SpeedToPoint);
        Math_StepToF(&this->actor.world.pos.y, this->nextPosF.y, this->SpeedToPoint);

        if (this->timer == 259) //prepare 3 to 4
        {

            this->currentWaypoint++ ; // =3
            this->nextWaypoint++ ; // = 4
            this->FrameCount = 20.0f;
    
            CustomActor_PathInit(this, play);
    
        }


    }
    else if ( ( 260 <= this->timer) && (this->timer <= 280 ) )
    {
        //fly point 3 à 4
        //stop tp link en dessous de l'acteur, le laisser faire ses merdes
        Math_StepToF(&this->actor.world.pos.x, this->nextPosF.x, this->SpeedToPoint);
        Math_StepToF(&this->actor.world.pos.z, this->nextPosF.z, this->SpeedToPoint);
        Math_StepToF(&this->actor.world.pos.y, this->nextPosF.y, this->SpeedToPoint);

        if (this->timer == 260)
        {
            Audio_PlayActorSfx2(&this->actor, NA_SE_EN_OWL_FLUTTER); //not working ?
        }
    }
    else 
    {
        //change action, tp under the map ,wait for cutscene to end
        CustomActor_ChangeMode(this, &this->skelAnime2, &blackcrow_skel2TestrestAnim); //cs done

        this->actor.world.pos = this->actor.home.pos;
        this->actor.world.rot.y = this->actor.home.rot.y;

        //player->actor.world.pos.x = //spawn coordinates
        //player->actor.world.pos.y = //spawn coordinates
        //player->actor.world.pos.z = //spawn coordinates

        CustomActor_ChangeFlags(this,play); 
        this->actor.shape.rot.y = this->actor.world.rot.y + 0x3ffe;
        this->actor.shape.rot.z = this->actor.world.rot.z - 0x3ffe;
        CustomActor_SetupAction(this, CustomActor_GroundBehaviour);
    }

}

void CustomActor_GroundBehaviour(CustomActor* this, PlayState* play)
{
    SkelAnime_Update(this->curSkelAnime);
}


void CustomActor_Destroy(CustomActor* this, PlayState* play) {

}

void CustomActor_Update(CustomActor* this, PlayState* play) {

    //SkelAnime_Update(this->curSkelAnime);

    //this->actor.world.pos.y = this->actor.home.pos.y + Rand_CenteredFloat(10.0f); //YOU CAN MOVE THE ACTOR WITH IT

    //this->actor.shape.rot.y = this->actor.world.rot.y;
    //this->actor.shape.rot.z = this->actor.world.rot.z = 0x3ffe;


    this->actionFunc(this, play);

}

void CustomActor_Draw(CustomActor* this, PlayState* play) {
    GfxPrint printer;
    Gfx* gfx;
    Player* player = GET_PLAYER(play);

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    /*
    gfx = POLY_OPA_DISP + 1;
    gSPDisplayList(OVERLAY_DISP++, gfx);

    GfxPrint_Init(&printer);
    GfxPrint_Open(&printer, gfx);

    GfxPrint_SetColor(&printer, 255, 0, 0, 255);
    GfxPrint_SetPos(&printer, 1, 21);
    GfxPrint_Printf(&printer, "shape rot x y z: %d,%d,%d",this->actor.shape.rot.x,this->actor.shape.rot.y,this->actor.shape.rot.z);

    GfxPrint_SetColor(&printer, 255, 0, 0, 255);
    GfxPrint_SetPos(&printer, 1, 22);
    GfxPrint_Printf(&printer, "world rot x y z: %d,%d,%d",this->actor.world.rot.x,this->actor.world.rot.y,this->actor.world.rot.z);

    GfxPrint_SetColor(&printer, 255, 0, 255, 255);
    GfxPrint_SetPos(&printer, 1, 23);
    GfxPrint_Printf(&printer, "YawToPlayer: %d",this->actor.yawTowardsPlayer);

    // Crow pos
    GfxPrint_SetColor(&printer, 255, 0, 255, 255);
    GfxPrint_SetPos(&printer, 1, 24);
    GfxPrint_Printf(&printer, "Crow Pos xyz:");
    GfxPrint_SetColor(&printer, 255, 0, 255, 255);
    GfxPrint_SetPos(&printer, 1, 25);
    GfxPrint_Printf(&printer, "%f,%f,%f",this->actor.world.pos.x,this->actor.world.pos.y,this->actor.world.pos.z);
    //

    // 
    GfxPrint_SetColor(&printer, 255, 0, 255, 255);
    GfxPrint_SetPos(&printer, 1, 27);
    GfxPrint_Printf(&printer, "Where To Go xyz:");
    GfxPrint_SetColor(&printer, 255, 0, 255, 255);
    GfxPrint_SetPos(&printer, 1, 28);
    GfxPrint_Printf(&printer, "%f,%f,%f", this->nextPosF.x,this->nextPosF.y,this->nextPosF.z);
    //

    GfxPrint_SetColor(&printer, 0, 0, 225, 255);
    GfxPrint_SetPos(&printer, 1, 29);
    GfxPrint_Printf(&printer, "cutscene done: %x", GET_EVENTCHKINF(0x0F) );
    //

    GfxPrint_SetColor(&printer, 0, 0, 225, 255);
    GfxPrint_SetPos(&printer, 1, 1);
    GfxPrint_Printf(&printer, "In bounds: %d", this->InBounds );
    GfxPrint_SetColor(&printer, 0, 0, 225, 255);
    GfxPrint_SetPos(&printer, 1, 2);
    GfxPrint_Printf(&printer, "dist : %f", this->actor.xzDistToPlayer );

    gfx = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);

    gSPEndDisplayList(gfx++);
    gSPBranchList(POLY_OPA_DISP, gfx);
    POLY_OPA_DISP = gfx;
    */

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    SkelAnime_DrawFlexOpa(play, this->curSkelAnime->skeleton, this->curSkelAnime->jointTable, this->curSkelAnime->dListCount, NULL, NULL, this);
}