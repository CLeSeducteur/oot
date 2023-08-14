#include "OscarNPC.h"
#include "assets/objects/object_oscar/OscarArmature.h"

void EnOscar_Init(EnOscar* this, PlayState* play);
void EnOscar_Destroy(EnOscar* this, PlayState* play);
void EnOscar_Update(EnOscar* this, PlayState* play);
void EnOscar_Draw(EnOscar* this, PlayState* play);

void EnOscar_CheckCollision(EnOscar* this, PlayState* play);

//Dying Behaviour
void EnOscar_Wait(EnOscar* this, PlayState* play);
void EnOscar_FirstTalk(EnOscar* this, PlayState* play);
void EnOscar_TalkYes1(EnOscar* this, PlayState* play);
void EnOscar_WaitEstus(EnOscar* this, PlayState* play);
void EnOscar_DisplayEstus(EnOscar* this, PlayState* play);
void EnOscar_TalkPreYes2(EnOscar* this, PlayState* play);
void EnOscar_TalkYes2(EnOscar* this, PlayState* play);
void EnOscar_WaitKey(EnOscar* this, PlayState* play);
void EnOscar_DisplayKey(EnOscar* this, PlayState* play);
void EnOscar_TalkPreAfter(EnOscar* this, PlayState* play);
void EnOscar_TalkAfter(EnOscar* this, PlayState* play);
void EnOscar_TalkNo(EnOscar* this, PlayState* play);
void EnOscar_Death(EnOscar* this, PlayState* play);

//You Murderer
void EnOscar_Killed(EnOscar* this, PlayState* play);
void EnOscar_KilledWait(EnOscar* this, PlayState* play);
void EnOscar_KilledDeath(EnOscar* this, PlayState* play);
void EnOscar_TalkPostKill(EnOscar* this, PlayState* play);
void EnOscar_WaitPostKill1(EnOscar* this, PlayState* play);
void EnOscar_DisplayPostKill1(EnOscar* this, PlayState* play);
void EnOscar_WaitPostKill2(EnOscar* this, PlayState* play);
void EnOscar_DisplayPostKill2(EnOscar* this, PlayState* play);

//CS_behaviour
void EnOscar_WaitInCutscene(EnOscar* this, PlayState* play);
void EnOscar_LeaveCutscene(EnOscar* this, PlayState* play);

#define FLAGS (ACTOR_FLAG_0 | ACTOR_FLAG_3) //can target
#define THIS ((EnOscar*)thisx)

const ActorInit En_Oscar_InitVars = {
    ACTOR_EN_OSCAR,
    ACTORCAT_NPC, 
    FLAGS,
    OBJECT_OSCAR,
    sizeof(EnOscar),
    (ActorFunc)EnOscar_Init,
    (ActorFunc)EnOscar_Destroy,
    (ActorFunc)EnOscar_Update,
    (ActorFunc)EnOscar_Draw,
};

static ColliderJntSphElementInit sJntSphItemsInit[1] = {
    {
        {
            ELEMTYPE_UNK1,
            { 0x00000000, 0x00, 0x00 },
            { 0xFFC1FFFF, 0x00, 0x00 },
            TOUCH_NONE,
            BUMP_ON | BUMP_HOOKABLE,
            OCELEM_ON,
        },
        { OSCARARMATURE_TORSO_LIMB, { { 0, 0, 0 }, 60 }, 100 },
    },
};

static ColliderJntSphInit sJntSphInit = {
    {
        COLTYPE_HIT7, //Types : 0->JabuJabu(BlueBlood), 1->Dirt, 2->Impact+GreenBlood, 3->WeirdImpact, 4->Bubbles, 5->NormalHit(Link), 6->Impact+GreenBlood, 7->Impact+RedBlood, 8->Impact+BlueBlood
        AT_ON | AT_TYPE_ENEMY,
        AC_ON | AC_TYPE_PLAYER,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_1,
        COLSHAPE_JNTSPH,
    },
    ARRAY_COUNT(sJntSphItemsInit),
    sJntSphItemsInit,
};

Mtx* mtx;

void EnOscar_SetupAction(EnOscar* this, EnOscarActionFunc actionFunc) {
    this->actionFunc = actionFunc;
}

void EnOscar_Init(EnOscar* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Actor_SetScale(&this->actor, 0.0172f);

    if (this->actor.params >> 8 & 0x0F) {
        if (play->csCtx.state != CS_STATE_IDLE) { // Play CS
            Actor_ChangeCategory(play, &play->actorCtx, &this->actor, ACTORCAT_PROP); //necessary for CS i think...
            SkelAnime_InitFlex(play, &this->skelAnime, &OscarArmature, &OscarArmatureLookingdownAnim, NULL, NULL, 0);
            EnOscar_SetupAction(this, EnOscar_WaitInCutscene);
            this->actor.colChkInfo.mass = MASS_IMMOVABLE;
            this->actor.flags &= ~ACTOR_FLAG_0; //no target
            this->actor.flags |= ACTOR_FLAG_4; //keep updating
            //this->actor.flags |= ACTOR_FLAG_5; //keep DRAW
            this->VariableUsedToTestThings = 0;
            Actor_SetScale(&this->actor, 0.02f);
            this->TalkState = 0;
        } else { //CS already played
            Actor_Kill(&this->actor);
        }
    } else {
        SkelAnime_InitFlex(play, &this->skelAnime, &OscarArmature, &OscarArmatureDyingbreathAnim, NULL, NULL, 0);
        EnOscar_SetupAction(this, EnOscar_Wait);
        this->actor.colChkInfo.mass = MASS_IMMOVABLE;
        this->actor.targetMode = 1;
        //this->actor.flags &= ~ACTOR_FLAG_0;
        this->VariableUsedToTestThings = 0;
        this->TalkState = 0;
        this->BigBrain = 0;
        Actor_SetFocus(&this->actor, 30.0f);

        this->actor.colChkInfo.mass = MASS_HEAVY;
        this->actor.colChkInfo.health = 1;

        Collider_InitJntSph(play, &this->colliderSpheres);
        Collider_SetJntSph(play, &this->colliderSpheres, &this->actor, &sJntSphInit, this->colliderSpheresElements); 

        if ( Flags_GetSwitch(play, this->actor.params & 0x00FF )) { // if condition is true: is already dead
            Actor_Kill(&this->actor);
        }
    }
}

//Dying Behaviour

void EnOscar_CheckCollision(EnOscar* this, PlayState* play) {

if ( (this->colliderSpheres.base.acFlags & AC_HIT) && (this->actor.colorFilterTimer == 0) ){
        EnOscar_SetupAction(this, EnOscar_Killed);
    }

    CollisionCheck_SetOC(play, &play->colChkCtx, &this->colliderSpheres.base); // pushing collision
    CollisionCheck_SetAC(play, &play->colChkCtx, &this->colliderSpheres.base); // getting hurt collision

}

void EnOscar_Wait(EnOscar* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    //do dialogue things
    this->VariableUsedToTestThings = 1;

    if (Actor_ProcessTalkRequest(&this->actor, play)) {
        if (this->TalkState == 0) {
            player->actor.textId = 0x606C;
            EnOscar_SetupAction(this, EnOscar_FirstTalk);
        }else {
            EnOscar_SetupAction(this, EnOscar_TalkAfter);
        }
    } else {
        func_8002F298(&this->actor, play, 100.0f, EXCH_ITEM_NONE);//it just works
        this->VariableUsedToTestThings = 0;
    }

    if ( (this->TalkState == 1) && (this->actor.xzDistToPlayer >= 600.0f) ) {
        EnOscar_SetupAction(this, EnOscar_Death);
        this->VariableUsedToTestThings = 0;
        Actor_PlaySfx(&this->actor, NA_SE_EN_LAST_DAMAGE);
        Animation_Change(&this->skelAnime, &OscarArmatureDeadAnim, 1.0f, 0, Animation_GetLastFrame(&OscarArmatureDeadAnim),ANIMMODE_LOOP, 0.0f);
    }
}

void EnOscar_FirstTalk(EnOscar* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    this->VariableUsedToTestThings = 2;

    if ((Message_GetState(&play->msgCtx) == TEXT_STATE_CHOICE) && Message_ShouldAdvance(play)) {
         switch (play->msgCtx.choiceIndex) {
            case 0: // yes
                this->actionFunc = EnOscar_TalkYes1;
                Flags_SetSwitch(play, this->actor.params & 0x00FF );
                Message_ContinueTextbox(play, 0x606D);
                break;
            case 1: // no
                Message_ContinueTextbox(play, 0x6070);
                EnOscar_SetupAction(this, EnOscar_TalkNo);
        }
    }
}

void EnOscar_TalkYes1(EnOscar* this, PlayState* play) {
    this->VariableUsedToTestThings = 3;
    if ((Message_GetState(&play->msgCtx) == TEXT_STATE_EVENT) && Message_ShouldAdvance(play)) {
        Message_CloseTextbox(play);
        this->actionFunc = EnOscar_WaitEstus;
        Actor_OfferGetItem(&this->actor, play, GI_RUPEE_GREEN, 10000.0f, 50.0f);
    }
}

void EnOscar_WaitEstus(EnOscar* this, PlayState* play) {
    this->VariableUsedToTestThings = 4;
    if (Actor_HasParent(&this->actor, play)) {
        this->actor.parent = NULL;
        this->actionFunc = EnOscar_DisplayEstus;
        gSaveContext.subTimerState = SUBTIMER_STATE_OFF;
    } else {
        Actor_OfferGetItem(&this->actor, play, GI_RUPEE_GREEN, 10000.0f, 50.0f);
    }
}

void EnOscar_DisplayEstus(EnOscar* this, PlayState* play) {
    this->VariableUsedToTestThings = 5;
     if (Actor_TextboxIsClosing(&this->actor, play)) {
        this->actor.textId = 0x606E;
        this->actionFunc = EnOscar_TalkPreYes2;
        this->actor.flags &= ~ACTOR_FLAG_8;
        SET_ITEMGETINF(ITEMGETINF_30);
    }
}

void EnOscar_TalkPreYes2(EnOscar* this, PlayState* play) {
    if (Actor_ProcessTalkRequest(&this->actor, play)) {
        this->actionFunc = EnOscar_TalkYes2;
    } else {
        this->actor.flags |= ACTOR_FLAG_16;
        func_8002F2CC(&this->actor, play, 1000.0f);
    }
}

void EnOscar_TalkYes2(EnOscar* this, PlayState* play){
    this->VariableUsedToTestThings = 6;
    if ((Message_GetState(&play->msgCtx) == TEXT_STATE_EVENT) && Message_ShouldAdvance(play)) {
            Message_CloseTextbox(play);
            this->actionFunc = EnOscar_WaitKey;
            Actor_OfferGetItem(&this->actor, play, GI_RUPEE_BLUE, 10000.0f, 50.0f);
        }
}

void EnOscar_WaitKey(EnOscar* this, PlayState* play) {
    this->VariableUsedToTestThings = 7;
    if (Actor_HasParent(&this->actor, play)) {
        this->actor.parent = NULL;
        this->actionFunc = EnOscar_DisplayKey;
        gSaveContext.subTimerState = SUBTIMER_STATE_OFF;
    } else {
        Actor_OfferGetItem(&this->actor, play, GI_RUPEE_BLUE, 10000.0f, 50.0f);
    }    
}

void EnOscar_DisplayKey(EnOscar* this, PlayState* play) {
    this->VariableUsedToTestThings = 8;
     if (Actor_TextboxIsClosing(&this->actor, play)) {
        this->actor.textId = 0x606F;
        this->actionFunc = EnOscar_TalkPreAfter;
        this->actor.flags &= ~ACTOR_FLAG_8;
        SET_ITEMGETINF(ITEMGETINF_30);
        this->TalkState = 1;
    }
}

void EnOscar_TalkPreAfter(EnOscar* this, PlayState* play) {
    if (Actor_ProcessTalkRequest(&this->actor, play)) {
        this->actionFunc = EnOscar_TalkAfter;
    } else {
        this->actor.flags |= ACTOR_FLAG_16;
        func_8002F2CC(&this->actor, play, 1000.0f);
    }
}

void EnOscar_TalkAfter(EnOscar* this, PlayState* play) {
    this->VariableUsedToTestThings = 9;
    if (Actor_TextboxIsClosing(&this->actor, play)) {
        this->actionFunc = EnOscar_Wait;
        this->actor.flags &= ~ACTOR_FLAG_16;
    }
}

void EnOscar_TalkNo(EnOscar* this, PlayState* play) {
    this->VariableUsedToTestThings = 10;
    if (Actor_TextboxIsClosing(&this->actor, play)) {
        EnOscar_SetupAction(this, EnOscar_Wait);
        this->actor.flags &= ~ACTOR_FLAG_16;
    }
}

void EnOscar_Death(EnOscar* this, PlayState* play) {
    if (this->VariableUsedToTestThings == 8 ) {
        if (this->TalkState == 1) { 
            Actor_Kill(&this->actor);
        } else {
            //placeholder
        }
    } else {
        this->VariableUsedToTestThings++;
    }
}

//Murdered Behaviour
void EnOscar_Killed(EnOscar* this, PlayState* play) {
    //EnOscar_SetupAction(this, EnOscar_DeathKilled);
    //EnOscar_KilledDeath(this,play);
    this->VariableUsedToTestThings = 0;
    this->actor.flags |= ACTOR_FLAG_4;
    Actor_PlaySfx(&this->actor, NA_SE_EN_LAST_DAMAGE);
    Animation_Change(&this->skelAnime, &OscarArmatureDeadAnim, 1.0f, 0, Animation_GetLastFrame(&OscarArmatureDeadAnim),ANIMMODE_LOOP, 0.0f);
    EnOscar_SetupAction(this, EnOscar_KilledWait);
}

void EnOscar_KilledDeath(EnOscar* this, PlayState* play) {
    Vec3f pos;
    Vec3f sp7C = { 0.0f, 0.5f, 0.0f };
    u8 i;
    Vec3f prepos = this->actor.world.pos;

    if (this->VariableUsedToTestThings < 40 ) {
        for (i = 5 ; i > 0; i--) {
            pos.x = prepos.x + Rand_CenteredFloat(60.0f);
            pos.z = prepos.z + Rand_CenteredFloat(60.0f);
            pos.y = prepos.y + Rand_CenteredFloat(10.0f);
            EffectSsDeadDb_Spawn(play, &pos, &sp7C, &sp7C, 50, 0, 255, 255, 255, 255, 0, 0, 255, 1, 9, true);
        }
        this->VariableUsedToTestThings++;
    } else  {
        if (this->VariableUsedToTestThings == 40 ){
            this->actor.world.pos.y -= 50.0f;
            this->actor.flags &= ~ACTOR_FLAG_0;
            this->VariableUsedToTestThings = 41;
        }
    }
}

void EnOscar_KilledWait(EnOscar* this, PlayState* play) {
    if (Actor_ProcessTalkRequest(&this->actor, play)) {
        EnOscar_SetupAction(this, EnOscar_TalkPostKill);
    } else {
        this->actor.textId = 0x6071;
        this->actor.flags |= ACTOR_FLAG_16;
        func_8002F1C4(&this->actor, play, this->actor.xzDistToPlayer, 500.0f, EXCH_ITEM_NONE);
    }
}

void EnOscar_TalkPostKill(EnOscar* this, PlayState* play) {
    EnOscar_KilledDeath(this,play);
    if ((Message_GetState(&play->msgCtx) == TEXT_STATE_EVENT) && Message_ShouldAdvance(play)) {
        Message_CloseTextbox(play);
        if (this->TalkState == 0) {
            Flags_SetSwitch(play, this->actor.params & 0x00FF );
            Actor_OfferGetItem(&this->actor, play, GI_RUPEE_GREEN, 10000.0f, 50.0f);
            this->actionFunc = EnOscar_WaitPostKill1;
        } else { 
            this->actor.flags &= ~ACTOR_FLAG_8;
            this->actor.flags &= ~ACTOR_FLAG_16;
            Actor_Kill(&this->actor);
        }
    }
}

void EnOscar_WaitPostKill1(EnOscar* this, PlayState* play) {
    EnOscar_KilledDeath(this,play);
    if (Actor_HasParent(&this->actor, play)) {
        this->actor.parent = NULL;
        this->actionFunc = EnOscar_DisplayPostKill1;
        gSaveContext.subTimerState = SUBTIMER_STATE_OFF;
    } else {
        Actor_OfferGetItem(&this->actor, play, GI_RUPEE_GREEN, 10000.0f, 50.0f);
    }
}

void EnOscar_DisplayPostKill1(EnOscar* this, PlayState* play) {
    EnOscar_KilledDeath(this,play);
     if (Actor_TextboxIsClosing(&this->actor, play)) {
        this->actionFunc = EnOscar_WaitPostKill2;
        this->actor.flags &= ~ACTOR_FLAG_8;
        SET_ITEMGETINF(ITEMGETINF_30);
        Actor_OfferGetItem(&this->actor, play, GI_RUPEE_BLUE, 10000.0f, 50.0f);
    }
}

void EnOscar_WaitPostKill2(EnOscar* this, PlayState* play) {
    EnOscar_KilledDeath(this,play);
    if (Actor_HasParent(&this->actor, play)) {
        this->actor.parent = NULL;
        this->actionFunc = EnOscar_DisplayPostKill2;
        gSaveContext.subTimerState = SUBTIMER_STATE_OFF;
    } else {
        Actor_OfferGetItem(&this->actor, play, GI_RUPEE_BLUE, 10000.0f, 50.0f);
    }
}

void EnOscar_DisplayPostKill2(EnOscar* this, PlayState* play) {
     if (Actor_TextboxIsClosing(&this->actor, play)) {
        this->actor.flags &= ~ACTOR_FLAG_8;
        this->actor.flags &= ~ACTOR_FLAG_16;
        SET_ITEMGETINF(ITEMGETINF_30);
        Actor_Kill(&this->actor);
    }
}

//CS Behaviour

void EnOscar_WaitInCutscene(EnOscar* this, PlayState* play) {
    Camera* CsCam = Play_GetCamera(play,play->csCtx.subCamId);//get the cs cam
    Vec3f CamPos = CsCam->at; //get coords
    CamPos.y += 750.0f; //above the roof but doesn't seem to do anything ffs

    if (play->csCtx.curFrame == 1530) { //get the fuck away
        Animation_Change(&this->skelAnime, &OscarArmatureMovingawayAnim, 1.0f, 0, Animation_GetLastFrame(&OscarArmatureMovingawayAnim),ANIMMODE_ONCE_INTERP, 4.0f);
        this->actionFunc = EnOscar_LeaveCutscene;
    } else if (play->csCtx.curFrame == 1340) { //throw chest
        Flags_SetSwitch(play, 0x0005 );
    } else if ((play->csCtx.curFrame == 1217)) {
        Actor_PlaySfx(&this->actor, NA_SE_EV_RONRON_DOOR_CLOSE);
    }

    if (play->csCtx.curFrame <= 1150) {
        if ((play->csCtx.curFrame%43)==0) {
            if (Rand_Centered() <= 0.0) {
                func_80078914(&CamPos,NA_SE_PL_METALEFFECT_ADULT); //play sound
            } else {
                func_80078914(&CamPos,NA_SE_PL_METALEFFECT_KID); //play another sound
            }
        }
    }
}
void EnOscar_LeaveCutscene(EnOscar* this, PlayState* play) {
    s32 curFrame = this->skelAnime.curFrame;

    if (curFrame == Animation_GetLastFrame(&OscarArmatureMovingawayAnim)) {
        Actor_Kill(&this->actor);
    } else if (curFrame == 15) {
        Actor_PlaySfx(&this->actor,NA_SE_PL_METALEFFECT_ADULT);
    }
}


void EnOscar_Destroy(EnOscar* this, PlayState* play) {
    SkelAnime_Free(&this->skelAnime, play);
    Collider_DestroyJntSph(play,&this->colliderSpheres); 
}

void EnOscar_Update(EnOscar* this, PlayState* play) {

    EnOscar_CheckCollision(this,play);

    SkelAnime_Update(&this->skelAnime);

    this->actionFunc(this, play);
    

}

s32 EnOscar_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, void* thisx) {

    //Les DL ont un problème de rot, faut check le limb et rotate le y dans le sens opposé. ex: roll -90 -> 90. Because fuck you. 
    //handR et HandL: -90
    //Torso: 90
    EnOscar* this = THIS;
    Gfx* dAstoraSword;
    Gfx* displayList;
    Gfx* displayListHead;
    s32 curFrame = this->skelAnime.curFrame;

    if (limbIndex == OSCARARMATURE_HAND_R_LIMB) {

        if (!(this->actor.params >> 8 & 0x0F)) {
    
            // allocate a 3-commands long dlist "for the current frame"
            displayList = Graph_Alloc(play->state.gfxCtx, 3 * sizeof(Gfx));
    
            // build new display list jumping to both original dlist and custom dlist
            displayListHead = displayList;
            gSPDisplayList( displayListHead++, *dList);
    
            //gSPMatrix( displayListHead++, mtx, G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            gSPDisplayList( displayListHead++, SwordOfAstora);
            //gSPPopMatrix( displayListHead++, G_MTX_MODELVIEW);
    
            gSPEndDisplayList( displayListHead++);
    
            // set overriden dlist
            *dList = displayList;
        }

    } else if (limbIndex == OSCARARMATURE_FOREARM_L_LIMB) {

        if (!(this->actor.params >> 8 & 0x0F)) {
            //change ROT of shield
            Matrix_Push();
            Matrix_Translate(0.0f, 0.0f, 0.0f, MTXMODE_NEW);
            Matrix_RotateX(BINANG_TO_RAD_ALT(0x7FFF), MTXMODE_APPLY);
            Matrix_ToMtx(mtx,"", 0);
            Matrix_Pop();
            //

            displayList = Graph_Alloc(play->state.gfxCtx, 5 * sizeof(Gfx));

            displayListHead = displayList;
            gSPDisplayList( displayListHead++, *dList);

            gSPMatrix( displayListHead++, mtx, G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            gSPDisplayList( displayListHead++, ShieldOscar);
            gSPPopMatrix( displayListHead++, G_MTX_MODELVIEW);

            gSPEndDisplayList( displayListHead++);

            // set overriden dlist
            *dList = displayList; 
        }
    

    } else if (limbIndex == OSCARARMATURE_TORSO_LIMB) {

        if (this->actor.params >> 8 & 0x0F) { //If CS

            //change ROT and POS of shield
            Matrix_Push();
            Matrix_Translate(600.0f, -650.0f, -700.0f, MTXMODE_NEW); //X->Z Y->X Z->Y
            //Matrix_Translate(0.0f, 0.0f, 0.0f, MTXMODE_NEW);
            //Matrix_RotateZYX(0x93E, -0x3C71, -0x3777, MTXMODE_APPLY); //src : tkt frer blender
            Matrix_RotateZYX(-0x3C71, -0x3777, 0x93E, MTXMODE_APPLY);
            Matrix_ToMtx(mtx,"", 0);
            Matrix_Pop();
            //

            displayList = Graph_Alloc(play->state.gfxCtx, 5 * sizeof(Gfx));

            displayListHead = displayList;
            gSPDisplayList( displayListHead++, *dList);

            gSPMatrix( displayListHead++, mtx, G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            gSPDisplayList( displayListHead++, ShieldOscar);
            gSPPopMatrix( displayListHead++, G_MTX_MODELVIEW);

            gSPEndDisplayList( displayListHead++);

            // set overriden dlist
            *dList = displayList;
        }
    }

    return false;
}

void EnOscar_PostLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, void* thisx) {
    EnOscar* this = THIS;
    Collider_UpdateSpheres(limbIndex, &this->colliderSpheres);
}

void EnOscar_Draw(EnOscar* this, PlayState* play) {
    GfxPrint printer;
    Gfx* gfx;
    Player* player = GET_PLAYER(play);

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    mtx = Graph_Alloc(play->state.gfxCtx, sizeof(Mtx));

    gfx = POLY_OPA_DISP + 1;
    gSPDisplayList(OVERLAY_DISP++, gfx);

    GfxPrint_Init(&printer);
    GfxPrint_Open(&printer, gfx);

    GfxPrint_SetColor(&printer, 0, 255, 255, 255); 
    GfxPrint_SetPos(&printer, 1, 1);
    GfxPrint_Printf(&printer, "CS_frames: %d",play->csCtx.curFrame);

    gfx = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);

    gSPEndDisplayList(gfx++);
    gSPBranchList(POLY_OPA_DISP, gfx);
    POLY_OPA_DISP = gfx;

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    SkelAnime_DrawFlexOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable, this->skelAnime.dListCount, EnOscar_OverrideLimbDraw, EnOscar_PostLimbDraw, this);
}