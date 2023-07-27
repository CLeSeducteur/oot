/*
 * File: z_en_asylum.c
 * Overlay: ovl_En_Asylum
 * Description: Asylum
 */


#include "z_en_asylum.h"
#include "terminal.h"
#include "assets/objects/object_asylum/ArmatureAsylumDemon.h"

#define FLAGS (ACTOR_FLAG_0 | ACTOR_FLAG_2 | ACTOR_FLAG_4)


void En_Asylum_Init(EnAsylum* this, PlayState* play);
void En_Asylum_Destroy(EnAsylum* this, PlayState* play);
void En_Asylum_Update(EnAsylum* this, PlayState* play);
void En_Asylum_Draw(EnAsylum* this, PlayState* play);

//Behaviour
void En_Asylum_Walking(EnAsylum* this, PlayState* play);
void En_Asylum_ButtSlam(EnAsylum* this, PlayState* play);
void En_Asylum_Bonk(EnAsylum* this, PlayState* play);
void En_Asylum_BackBonk(EnAsylum* this, PlayState* play);
void En_Asylum_BonkSmash(EnAsylum* this, PlayState* play);
void En_Asylum_Defeated(EnAsylum* this, PlayState* play);
//Event
void En_Asylum_JumpFromRoof(EnAsylum* this, PlayState* play);
void En_Asylum_WatchPlayerUp(EnAsylum* this, PlayState* play);
void En_Asylum_CriticalHit(EnAsylum* this, PlayState* play);

//Others
Actor* DetectPillar(PlayState* play, Actor* actor);
void En_Asylum_CheckDeath(EnAsylum* this);
void En_Asylum_CheckColliding(EnAsylum* this, PlayState* play);
s32 En_Asylum_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, void* thisx); 
void En_Asylum_PostLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, void* thisx);


const ActorInit En_AsylumDemon_InitVars = {
    ACTOR_EN_ASYLUMDEMON, //ActorNumber Change it if neccessary.
    ACTORCAT_ENEMY,
    FLAGS,
    OBJECT_ASYLUMDEMON,
    sizeof(EnAsylum),
    (ActorFunc)En_Asylum_Init,
    (ActorFunc)En_Asylum_Destroy,
    (ActorFunc)En_Asylum_Update,
    (ActorFunc)En_Asylum_Draw,
};

void En_Asylum_SetupAction(EnAsylum* this, EnAsylumActionFunc actionFunc) {
    this->actionFunc = actionFunc;
}

LinkAnimationHeader* myPlayerAnim = (LinkAnimationHeader*) 0x04002720; //stab animetion
typedef enum {
    DONTPLAY = 1,
    PLAY = 2,
    PLAYING = 3
} MyPlayerAnimState;
MyPlayerAnimState myPlayerAnimState = DONTPLAY;

void myPlayerDraw(Actor* player, PlayState* play) {

    Player* pp = GET_PLAYER(play);

    if (myPlayerAnimState == PLAY) {
        // initialize the animation
        LinkAnimation_Change(play, &pp->skelAnime, myPlayerAnim, 0.0f, Animation_GetLastFrame(myPlayerAnim) , Animation_GetLastFrame(myPlayerAnim), ANIMMODE_LOOP, 0.0f); //STUCK ON FRAME
        myPlayerAnimState = PLAYING;
    } else if (myPlayerAnimState == PLAYING) {
        // play the animation
        if (LinkAnimation_Update(play, &pp->skelAnime)) {
            // animation is finished
            //myPlayerAnimState = DONTPLAY;
        }
    }
    // call vanilla player draw function
    PlayerCall_Draw(player, play);
}

#define THIS ((EnAsylum*)thisx)
#define PillarID 0x6 //PillarActorNumber Change it if neccessary.

//If you need to change the damage, ctrl+f -> ".info.toucher.damage"
//Changing hp is in "init"

static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_METAL,
        AT_ON | AT_TYPE_ENEMY,
        AC_NONE,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0xFFCFFFFF, 0x04, 0x38 },
        { 0x00000000, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_NONE,
        OCELEM_ON,
    },
    { 200, 80, 30, { 0, 0, 0 } },
};

static ColliderJntSphElementInit sJntSphItemsInit[10] = {
    {
        {
            ELEMTYPE_UNK1,
            { 0xFFCFFFFF, 0x00, 0x08 },
            { 0x00000000, 0x00, 0x00 },
            TOUCH_ON | TOUCH_SFX_NORMAL,
            BUMP_NONE,
            OCELEM_ON,
        },
        { ARMATUREASYLUMDEMON_CUISSER_LIMB, { { 0, 0, 0 }, 50 }, 100 }, //1ere valeur = rayon/diametre 2e valeur = probablement une valeur qui se fait overwrite par update
    },
    {
        {
            ELEMTYPE_UNK1,
            { 0x00000000, 0x00, 0x00 },
            { 0xFFC1FFFF, 0x00, 0x00 },
            TOUCH_NONE,
            TOUCH_NONE,
            OCELEM_ON,
        },
        { ARMATUREASYLUMDEMON_CUISSEL_LIMB, { { 0, 0, 0 }, 50 }, 100 },
    },
    {
        {
            ELEMTYPE_UNK1,
            { 0x00000000, 0x00, 0x00 },
            { 0xFFC1FFFF, 0x00, 0x00 },
            TOUCH_NONE,
            BUMP_ON | BUMP_HOOKABLE,
            OCELEM_ON,
        },
        { ARMATUREASYLUMDEMON_WAIST_LIMB, { { 0, 0, 0 }, 78 }, 100 },
    },
    {
        {
            ELEMTYPE_UNK1,
            { 0x00000000, 0x00, 0x00 },
            { 0xFFC1FFFF, 0x00, 0x00 },
            TOUCH_NONE,
            BUMP_ON | BUMP_HOOKABLE,
            OCELEM_ON,
        },
        { ARMATUREASYLUMDEMON_COLLAR_LIMB, { { 0, 0, 0 }, 50 }, 100 },
    },
    {
        {
            ELEMTYPE_UNK1,
            { 0x00000000, 0x00, 0x00 },
            { 0xFFC1FFFF, 0x00, 0x00 },
            TOUCH_NONE,
            BUMP_ON | BUMP_HOOKABLE,
            OCELEM_ON,
        },
        { ARMATUREASYLUMDEMON_FOOTL_LIMB, { { 0, 0, 0 }, 30 }, 100 },
    },
    {
        {
            ELEMTYPE_UNK1,
            { 0x00000000, 0x00, 0x00 },
            { 0xFFC1FFFF, 0x00, 0x00 },
            TOUCH_NONE,
            BUMP_ON | BUMP_HOOKABLE,
            OCELEM_ON,
        },
        { ARMATUREASYLUMDEMON_FOOTR_LIMB, { { 0, 0, 0 }, 30 }, 100 },
    },
    {
        {
            ELEMTYPE_UNK1,
            { 0x00000000, 0x00, 0x00 },
            { 0xFFC1FFFF, 0x00, 0x00 },
            TOUCH_NONE,
            BUMP_ON | BUMP_HOOKABLE,
            OCELEM_ON,
        },
        { ARMATUREASYLUMDEMON_ARML_LIMB, { { 0, 0, 0 }, 35 }, 100 },
    },
    {
        {
            ELEMTYPE_UNK1,
            { 0x00000000, 0x00, 0x00 },
            { 0xFFC1FFFF, 0x00, 0x00 },
            TOUCH_NONE,
            BUMP_ON | BUMP_HOOKABLE,
            OCELEM_ON,
        },
        { ARMATUREASYLUMDEMON_WRISTL_LIMB, { { 0, 0, 0 }, 25 }, 100 },
    },
    {
        {
            ELEMTYPE_UNK1,
            { 0x00000000, 0x00, 0x00 },
            { 0xFFC1FFFF, 0x00, 0x00 },
            TOUCH_NONE,
            BUMP_ON | BUMP_HOOKABLE,
            OCELEM_ON,
        },
        { ARMATUREASYLUMDEMON_ARMR_LIMB, { { 0, 0, 0 }, 35 }, 100 },
    },
    {
        {
            ELEMTYPE_UNK1,
            { 0x00000000, 0x00, 0x00 },
            { 0xFFC1FFFF, 0x00, 0x00 },
            TOUCH_NONE,
            BUMP_ON | BUMP_HOOKABLE,
            OCELEM_ON,
        },
        { ARMATUREASYLUMDEMON_WRISTR_LIMB, { { 0, 0, 0 }, 25 }, 100 },
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

static ColliderTrisElementInit sTrisElementsInit[4] = {
    {
        {
            ELEMTYPE_UNK2,
            { 0xFFCFFFFF, 0x04, 0x8 },
            { 0x00000000, 0x00, 0x00 },
            TOUCH_ON | TOUCH_SFX_NORMAL,
            BUMP_NONE,
            OCELEM_NONE,
        },
        {{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } } },
    },
    {
        {
            ELEMTYPE_UNK2,
            { 0xFFCFFFFF, 0x04, 0x8 },
            { 0x00000000, 0x00, 0x00 },
            TOUCH_ON | TOUCH_SFX_NORMAL,
            BUMP_NONE,
            OCELEM_NONE,
        },
        { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -20.0f }, { 0.0f, 0.0f, 0.0f } } },
    },
    {
        {
            ELEMTYPE_UNK2,
            { 0xFFCFFFFF, 0x04, 0x8 },
            { 0x00000000, 0x00, 0x00 },
            TOUCH_ON | TOUCH_SFX_NORMAL,
            BUMP_NONE,
            OCELEM_NONE,
        },
        {{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } } },
    },
    {
        {
            ELEMTYPE_UNK2,
            { 0xFFCFFFFF, 0x04, 0x8 },
            { 0x00000000, 0x00, 0x00 },
            TOUCH_ON | TOUCH_SFX_NORMAL,
            BUMP_NONE,
            OCELEM_NONE,
        },
        {{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } } },
    },
};

static ColliderTrisInit sTrisInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_ENEMY,
        AC_NONE,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_TRIS,
    },
    ARRAY_COUNT(sTrisElementsInit),
    sTrisElementsInit,
};

static DamageTable sDamageTable = { //changement à faire ici
    /* Deku nut      */ DMG_ENTRY(0, 0xD),
    /* Deku stick    */ DMG_ENTRY(2, 0xF),
    /* Slingshot     */ DMG_ENTRY(1, 0xE),
    /* Explosive     */ DMG_ENTRY(2, 0xF),
    /* Boomerang     */ DMG_ENTRY(0, 0xD),
    /* Normal arrow  */ DMG_ENTRY(30, 0xE),
    /* Hammer swing  */ DMG_ENTRY(2, 0xF),
    /* Hookshot      */ DMG_ENTRY(0, 0xD),
    /* Kokiri sword  */ DMG_ENTRY(2, 0xF),
    /* Master sword  */ DMG_ENTRY(2, 0xF),
    /* Giant's Knife */ DMG_ENTRY(4, 0xF),
    /* Fire arrow    */ DMG_ENTRY(2, 0xE),
    /* Ice arrow     */ DMG_ENTRY(2, 0xE),
    /* Light arrow   */ DMG_ENTRY(2, 0xE),
    /* Unk arrow 1   */ DMG_ENTRY(2, 0xE),
    /* Unk arrow 2   */ DMG_ENTRY(2, 0xE),
    /* Unk arrow 3   */ DMG_ENTRY(15, 0xE),
    /* Fire magic    */ DMG_ENTRY(0, 0x6),
    /* Ice magic     */ DMG_ENTRY(0, 0x6),
    /* Light magic   */ DMG_ENTRY(0, 0x6),
    /* Shield        */ DMG_ENTRY(0, 0x0),
    /* Mirror Ray    */ DMG_ENTRY(0, 0x0),
    /* Kokiri spin   */ DMG_ENTRY(2, 0xF),
    /* Giant spin    */ DMG_ENTRY(4, 0xF),
    /* Master spin   */ DMG_ENTRY(2, 0xF),
    /* Kokiri jump   */ DMG_ENTRY(4, 0xF),
    /* Giant jump    */ DMG_ENTRY(8, 0xF),
    /* Master jump   */ DMG_ENTRY(4, 0xF),
    /* Unknown 1     */ DMG_ENTRY(10, 0xF),
    /* Unblockable   */ DMG_ENTRY(0, 0x0),
    /* Hammer jump   */ DMG_ENTRY(4, 0xF),
    /* Unknown 2     */ DMG_ENTRY(0, 0x0),
};

void En_Asylum_Init(EnAsylum* this, PlayState* play) {

    Player* player = GET_PLAYER(play);

    player->actor.draw = myPlayerDraw;

    this->actor.naviEnemyId = 0x01; 

    this->IsAlive = 1;

    SkelAnime_InitFlex(play, &this->skelAnime, &ArmatureAsylumDemon, &ArmatureAsylumDemonWalkAnim, NULL, NULL, 0);

    Animation_Change(&this->skelAnime, &ArmatureAsylumDemonWalkAnim, 1.0f, 0, Animation_GetLastFrame(&ArmatureAsylumDemonWalkAnim),ANIMMODE_LOOP, 0.0f);

    Actor_SetScale(&this->actor, 0.014f);
    Actor_SetFocus(&this->actor, 0.0f);

    this->actor.targetMode = 9;

    this->timerone = 0;
    this->DemonState = 2;
    this->ParticleBool = 0;
    Rand_Seed(50);


    ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawCircle, 150.0f);

    Collider_InitCylinder(play, &this->AssCollider);
    Collider_SetCylinder(play, &this->AssCollider, &this->actor, &sCylinderInit);

    Collider_InitJntSph(play, &this->colliderSpheres);
    Collider_SetJntSph(play, &this->colliderSpheres, &this->actor, &sJntSphInit, this->colliderSpheresElements); 

    Collider_InitTris(play, &this->shieldCollider);
    Collider_SetTris(play, &this->shieldCollider, &this->actor, &sTrisInit, this->shieldColliderItems);

    this->actor.colChkInfo.damageTable = &sDamageTable;
    this->actor.colChkInfo.mass = MASS_HEAVY;
    this->actor.colChkInfo.health = 35; //52 hp //CHANGE HP IF NECESSARY
    this->actor.shape.rot.y = this->actor.world.rot.y + 0x3ffe;

    if ( Flags_GetSwitch(play, this->actor.params & 0x00FF )) { // if condition is true: is already dead
        Actor_Kill(&this->actor);
    }

    Flags_SetSwitch(play, (this->actor.params & 0xFF00) >> 8 ); // hop ça vire //

    if ( !Flags_GetSwitch(play, (this->actor.params & 0xFF00) >> 8 )) { // if condition is true: hasn't jumped from roof already
        //jump from roof
        this->actor.world.pos.y += 800.0f;
        Animation_Change(&this->skelAnime, &ArmatureAsylumDemonButtslamAnim, -1.0f, 20, 1,ANIMMODE_ONCE, 0.0f);
        this->timerone = 20;
        En_Asylum_SetupAction(this, En_Asylum_JumpFromRoof);
    } else { //normal
        SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 0, 0, NA_BGM_BOSS);
        En_Asylum_SetupAction(this, En_Asylum_Walking);
    }
}

void En_Asylum_Destroy(EnAsylum* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 1);
    player->actor.draw = PlayerCall_Draw;
    SkelAnime_Free(&this->skelAnime, play);
    Collider_DestroyCylinder(play,&this->AssCollider);
    Collider_DestroyTris(play,&this->shieldCollider);
    Collider_DestroyJntSph(play,&this->colliderSpheres);   
}

Actor* DetectPillar(PlayState* play, Actor* actor) {

    Actor* prop = play->actorCtx.actorLists[ACTORCAT_PROP].head;

    while (prop != NULL) {
        if ((prop == actor) || (prop->id != PillarID)) {
            prop = prop->next;
            continue;
        } else if (Actor_ActorAIsFacingAndNearActorB(actor, prop, 110.0f, 0x5A52)) {
            return prop;
        }

        prop = prop->next;
    }

    return NULL;
}

void En_Asylum_JumpFromRoof(EnAsylum* this, PlayState* play) {

    Math_SmoothStepToF(&this->actor.world.pos.y, this->actor.home.pos.y, 1.0f , 80.0f , 0.0f );

    if ( this->actor.world.pos.y <= this->actor.home.pos.y + 120 ) {
        SkelAnime_Update(&this->skelAnime);
        this->timerone--;
    }

    if (this->timerone == 0) {
        Flags_SetSwitch(play, (this->actor.params & 0xFF00) >> 8 );
        En_Asylum_SetupAction(this, En_Asylum_Walking);
    } else if (this->timerone == 17){
        Actor_PlaySfx(&this->actor, NA_SE_EN_KDOOR_BREAK);
        Camera_RequestQuake(&play->mainCamera, 2, 35, 5);
        SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 0, 0, NA_BGM_BOSS);
        this->ParticleBool = 1;
    } else {
        this->ParticleBool = 0;
    }
}

void En_Asylum_WatchPlayerUp(EnAsylum* this, PlayState* play) {

    Player* player = GET_PLAYER(play);

    SkelAnime_Update(&this->skelAnime);

    if (this->DemonState == 1) {
        this->DemonState = 3;
        Animation_Change(&this->skelAnime, &ArmatureAsylumDemonCriticalAnim, 1.0f, 0, Animation_GetLastFrame(&ArmatureAsylumDemonCriticalAnim),ANIMMODE_ONCE_INTERP, 4.0f);
        player->actor.draw = myPlayerDraw;
        myPlayerAnimState = PLAY;
        play->grabPlayer(play, player);
        player->actor.parent = &this->actor;
        Actor_SetColorFilter(&this->actor, 1, 50, 0, 15);
        Actor_PlaySfx(&this->actor, NA_SE_EN_LAST_DAMAGE);
        this->actor.colChkInfo.health-=10;
        En_Asylum_SetupAction(this, En_Asylum_CriticalHit); //stun
    } else if (this->actor.yDistToPlayer <= 15.0f ) {
        Animation_Change(&this->skelAnime, &ArmatureAsylumDemonWalkAnim, 1.0f, 0, Animation_GetLastFrame(&ArmatureAsylumDemonWalkAnim),ANIMMODE_LOOP, 0.0f);
        this->DemonState = 3;
        En_Asylum_SetupAction(this, En_Asylum_Walking);
    }
}

void En_Asylum_CriticalHit(EnAsylum* this, PlayState* play) {

    Player* player = GET_PLAYER(play);
    Input* sControlInput = &play->state.input[0];
    s32 curFrame = this->skelAnime.curFrame;

    if (curFrame <= 32 ) {
        player->actor.world.rot = this->LinkNewRot;
        player->actor.world.pos = this->LinkNewPos;
        player->actor.world.pos.y += 15.0f;
        player->unk_850 = 0 ;
    } else {
        player->actor.parent = NULL;
        myPlayerAnimState = DONTPLAY;
    }

    if ( curFrame == Animation_GetLastFrame(&ArmatureAsylumDemonCriticalAnim)) {
        player->stateFlags2 &= ~PLAYER_STATE2_7;
        En_Asylum_SetupAction(this, En_Asylum_Walking);
    }

    SkelAnime_Update(&this->skelAnime);
}

void En_Asylum_Walking(EnAsylum* this, PlayState* play) {

    f32 PlayerDistance = this->actor.xzDistToPlayer;
    s16 PlayerAngle = this->actor.yawTowardsPlayer;
    s16 yawdiff = (ABS((s16)(this->actor.yawTowardsPlayer - this->actor.shape.rot.y)));

    this->timerone++;

    CollisionCheck_SetAC(play, &play->colChkCtx, &this->shieldCollider.base);

    Actor_MoveXZGravity(&this->actor);
    this->actor.speed = 1.2f;
    Math_SmoothStepToS(&this->actor.world.rot.y, PlayerAngle , 1, 0x17A, 0); //valeurs à tweaker
    //Rotation -0x7FFF < 0 < x7FFF
    this->actor.shape.rot.y = this->actor.world.rot.y + 0x3ffe;
    
    if (this->timerone == 1){
        Animation_Change(&this->skelAnime, &ArmatureAsylumDemonWalkAnim, 1.0f, 0x0, Animation_GetLastFrame(&ArmatureAsylumDemonWalkAnim),ANIMMODE_LOOP_INTERP, 0.0f);

        if (this->actor.yDistToPlayer >= 600.0f ) { //if player is above
            //LOOK UP
            Animation_Change(&this->skelAnime, &ArmatureAsylumDemonWaitingAnim, 1.0f, 0 , Animation_GetLastFrame(&ArmatureAsylumDemonWaitingAnim) ,ANIMMODE_LOOP_INTERP, 0.0f);
            this->actor.world.rot.y += 0x7fff;
            this->actor.shape.rot.y = this->actor.world.rot.y + 0x3ffe;
            this->DemonState = 0;
            SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 0, 0, NA_BGM_BOSS);
            En_Asylum_SetupAction(this, En_Asylum_WatchPlayerUp);
        }

    } else if ((this->timerone == 19) || (this->timerone == 37)) {  
        Actor_PlaySfx(&this->actor, NA_SE_EN_DODO_J_WALK );
    } else if (this->timerone == Animation_GetLastFrame(&ArmatureAsylumDemonWalkAnim)) {
       this->timerone = 0;
    }
    
    if  ( ( ( ((s16)(this->actor.yawTowardsPlayer - this->actor.world.rot.y)) <= 0x5552 ) && ( ((s16)(this->actor.yawTowardsPlayer - this->actor.world.rot.y)) >= 0x0 ) )
    && ((0 <= PlayerDistance) && (PlayerDistance <= 130)) )  { //GO TO Bonk BckSwing

        if (Rand_Centered() <= 0.0) {
            this->timerone = 0;
            En_Asylum_SetupAction(this, En_Asylum_ButtSlam);
        }else{
            this->timerone = 0;
            En_Asylum_SetupAction(this, En_Asylum_BackBonk);
            this->shieldCollider.elements[0].info.toucher.damage = this->shieldCollider.elements[1].info.toucher.damage = this->shieldCollider.elements[2].info.toucher.damage = this->shieldCollider.elements[3].info.toucher.damage = 32;
        }
    } else if  ( ( ( ((s16)(this->actor.yawTowardsPlayer - this->actor.world.rot.y)) >= -0x5552 ) && ( ((s16)(this->actor.yawTowardsPlayer - this->actor.world.rot.y)) < 0x0 ) )
    && ((0 <= PlayerDistance) && (PlayerDistance <= 130)) ) { //GO TO Bonk Swing 

        if (Rand_Centered() <= 0.0) {
            this->timerone = 0;
            En_Asylum_SetupAction(this, En_Asylum_ButtSlam);
        }else{
            this->timerone = 0;
            En_Asylum_SetupAction(this, En_Asylum_Bonk);
            this->shieldCollider.elements[0].info.toucher.damage = this->shieldCollider.elements[1].info.toucher.damage = this->shieldCollider.elements[2].info.toucher.damage = this->shieldCollider.elements[3].info.toucher.damage = 32;
        }
    } else if ( (ABS((s16)(this->actor.yawTowardsPlayer - this->actor.world.rot.y)) < 0xFFF) && (( 130 < PlayerDistance) && (PlayerDistance < 270)) ) { //GO TO Smash Bonk

        if (Rand_CenteredFloat(this->actor.xzDistToPlayer) >= 90.0) {
            this->timerone = 0;
            En_Asylum_SetupAction(this, En_Asylum_BonkSmash);
            this->shieldCollider.elements[0].info.toucher.damage = this->shieldCollider.elements[1].info.toucher.damage = this->shieldCollider.elements[2].info.toucher.damage = this->shieldCollider.elements[3].info.toucher.damage = 48;
        }
    } else if (DetectPillar(play, &this->actor) != NULL) {
        this->timerone = 0;
        En_Asylum_SetupAction(this, En_Asylum_Bonk);
        this->shieldCollider.elements[0].info.toucher.damage = this->shieldCollider.elements[1].info.toucher.damage = this->shieldCollider.elements[2].info.toucher.damage = this->shieldCollider.elements[3].info.toucher.damage = 32;
    } else if ( (ABS((s16)(this->actor.yawTowardsPlayer - this->actor.world.rot.y)) > 0x5552) && (PlayerDistance < 200) ){ // GO TO buttslam 

        this->timerone = 0;
        En_Asylum_SetupAction(this, En_Asylum_ButtSlam);
    }
    SkelAnime_Update(&this->skelAnime);
    En_Asylum_CheckDeath(this);
}

void En_Asylum_ButtSlam(EnAsylum* this, PlayState* play) {

    this->timerone++;

    if (this->timerone == 1){
        
        Animation_Change(&this->skelAnime, &ArmatureAsylumDemonButtslamAnim, 1.0f, 0x0, Animation_GetLastFrame(&ArmatureAsylumDemonButtslamAnim),ANIMMODE_ONCE_INTERP, 0.0f);
    } else if ( 20 < this->timerone && this->timerone < 51){

        Actor_MoveXZGravity(&this->actor);
        this->actor.speed = 0.1f;
        Math_SmoothStepToS(&this->actor.world.rot.y, this->actor.yawTowardsPlayer , 1, 0x2EE, 0); 
        this->actor.shape.rot.y = this->actor.world.rot.y + 0x3ffe;
    } else if (this->timerone == 57) {

        Vec3f effPos = this->actor.world.pos;
        Vec3f effVelocity = { 0.0f, 5.0f, 0.0f };
        Vec3f effAccel = { 0.0f, -0.5f, 0.0f };
        effPos.x += 0.0f;
        effPos.y += 350.0f;

        EffectSsBlast_SpawnWhiteShockwave(play, &effPos, &effVelocity, &effAccel);
        Actor_PlaySfx(&this->actor, NA_SE_EN_KDOOR_BREAK);
        Camera_RequestQuake(&play->mainCamera, 2, 55, 5);
        this->ParticleBool = 2;

    } else if (this->timerone == 58){
        this->ParticleBool = 0;
    } else if (this->timerone == 62){
        CollisionCheck_SetOC(play, &play->colChkCtx, &this->AssCollider.base);
    } else if ((this->timerone == 78) || (this->timerone == 89)){
        Actor_PlaySfx(&this->actor, NA_SE_EN_DODO_J_WALK );
    } else if (this->timerone == Animation_GetLastFrame(&ArmatureAsylumDemonButtslamAnim)) {
        this->timerone = 0;
        En_Asylum_SetupAction(this, En_Asylum_Walking);
    }

    if ( (58 < this->timerone) && (this->timerone < 62) ){
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->AssCollider.base);
    } else {
        CollisionCheck_SetAC(play, &play->colChkCtx, &this->shieldCollider.base);
    }
    SkelAnime_Update(&this->skelAnime);
    En_Asylum_CheckDeath(this);
}

void En_Asylum_Bonk(EnAsylum* this, PlayState* play) {

    this->timerone++;

    if (this->timerone == 1){
        Animation_Change(&this->skelAnime, &ArmatureAsylumDemonBonkswingAnim, 1.0f, 0x0, Animation_GetLastFrame(&ArmatureAsylumDemonBonkswingAnim),ANIMMODE_ONCE_INTERP, 0.0f);
    } else if ( ( (this->timerone == 9) || (this->timerone == 29) ) || ( (this->timerone == 45) || (this->timerone == 53) ) ) {
        Actor_PlaySfx(&this->actor, NA_SE_EN_DODO_J_WALK );
    } else if (this->timerone == 19){
        this->ParticleBool = 3;
    } else if (this->timerone == 23){
        this->ParticleBool = 0;
    } else if (this->timerone >= Animation_GetLastFrame(&ArmatureAsylumDemonBonkswingAnim)) {
       this->timerone = 0;
       En_Asylum_SetupAction(this, En_Asylum_Walking);
    }

    if (this->timerone <= 20){
        Math_SmoothStepToS(&this->actor.world.rot.y, this->actor.yawTowardsPlayer , 1, 0x100, 0); 
        this->actor.shape.rot.y = this->actor.world.rot.y + 0x3ffe; 
    }

    if ( ( 10 < this->timerone ) && (this->timerone < 46) ) {
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->shieldCollider.base);
    } else {
        CollisionCheck_SetAC(play, &play->colChkCtx, &this->shieldCollider.base);
    }

    SkelAnime_Update(&this->skelAnime);
    En_Asylum_CheckDeath(this);
}

void En_Asylum_BackBonk(EnAsylum* this, PlayState* play){

    this->timerone++;

    if (this->timerone == 1){
        Animation_Change(&this->skelAnime, &ArmatureAsylumDemonBonkbckswingAnim, 1.0f, 0x0, Animation_GetLastFrame(&ArmatureAsylumDemonBonkbckswingAnim),ANIMMODE_ONCE_INTERP, 0.0f);
    } else if ( ( (this->timerone == 19) || (this->timerone == 28) ) || ( (this->timerone == 39) || (this->timerone == 49) ) ) {
        Actor_PlaySfx(&this->actor, NA_SE_EN_DODO_J_WALK );
    } else if (this->timerone == 17){
        this->ParticleBool = 4;
    } else if (this->timerone == 21){
        this->ParticleBool = 0;
    } else if (this->timerone >= Animation_GetLastFrame(&ArmatureAsylumDemonBonkbckswingAnim)) {
       this->timerone = 0;
       En_Asylum_SetupAction(this, En_Asylum_Walking);
    }

    if ( ( 12 < this->timerone ) && (this->timerone < 40) ) {
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->shieldCollider.base);
    } else {
        CollisionCheck_SetAC(play, &play->colChkCtx, &this->shieldCollider.base);
    }

    if (this->timerone < 13){
        Math_SmoothStepToS(&this->actor.world.rot.y, this->actor.yawTowardsPlayer , 1, 0x100, 0); 
        this->actor.shape.rot.y = this->actor.world.rot.y + 0x3ffe;  
    }
    SkelAnime_Update(&this->skelAnime);
    En_Asylum_CheckDeath(this);
}

void En_Asylum_BonkSmash(EnAsylum* this, PlayState* play) {

    this->timerone++;

    if (this->timerone == 1) {
        Animation_Change(&this->skelAnime, &ArmatureAsylumDemonBonksmashAnim, 1.0f, 0x0, Animation_GetLastFrame(&ArmatureAsylumDemonBonksmashAnim),ANIMMODE_ONCE_INTERP, 0.0f);
    } else if ( (this->timerone == 20) || (this->timerone == 45) ) {
        Actor_PlaySfx(&this->actor, NA_SE_EN_DODO_J_WALK );
        if (this->timerone == 45){
            this->ParticleBool = 0;
        }   
    } else if (this->timerone == 44){
        this->ParticleBool = 5;
    } else if (this->timerone == 46){
         Camera_RequestQuake(&play->mainCamera, 2, 55, 5);
         Actor_PlaySfx( &this->actor,NA_SE_EN_IRONNACK_BREAK_PILLAR2 );
    } else if (this->timerone == Animation_GetLastFrame(&ArmatureAsylumDemonBonksmashAnim)) {
       this->timerone = 0;
       En_Asylum_SetupAction(this, En_Asylum_Walking);
    }

    if ( ( 20 < this->timerone ) && (this->timerone < 70) ) {
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->shieldCollider.base);
    } else {
        CollisionCheck_SetAC(play, &play->colChkCtx, &this->shieldCollider.base);
    }

    if (this->timerone < 40){
        Math_SmoothStepToS(&this->actor.world.rot.y, this->actor.yawTowardsPlayer , 1, 0xFA, 0); 
        this->actor.shape.rot.y = this->actor.world.rot.y + 0x3ffe;
    }
    SkelAnime_Update(&this->skelAnime);
    En_Asylum_CheckDeath(this);
}

void En_Asylum_CheckDeath(EnAsylum* this) {

    if ( (this->actor.colChkInfo.health == 0) && (this->IsAlive == 1) ){ // Thank you i'm die forever.
        this->IsAlive = 0;
        this->timerone = 0;
        SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 1);
        this->actor.flags &= ~ACTOR_FLAG_0; //no target
        Animation_Change(&this->skelAnime, &ArmatureAsylumDemonDyingAnim, 1.0f, 0x0, Animation_GetLastFrame(&ArmatureAsylumDemonDyingAnim),ANIMMODE_ONCE_INTERP, 0.0f);
        Actor_PlaySfx(&this->actor,NA_SE_IT_SWORD_SWING_HARD); 
        En_Asylum_SetupAction(this, En_Asylum_Defeated);
    }
}

void En_Asylum_Defeated(EnAsylum* this, PlayState* play) {

    Vec3f pos;
    Vec3f sp7C = { 0.0f, 0.5f, 0.0f };
    s32 i;
    Vec3f prepos = this->actor.world.pos;
    prepos.y = this->actor.world.pos.y + 60;
    this->actor.speed = 0.0f;

    this->timerone++;


    if (this->timerone == 18){
        Message_StartTextbox(play, 0x4010, NULL); //spawn text
        Flags_SetSwitch(play, this->actor.params & 0x00FF);
        SfxSource_PlaySfxAtFixedWorldPos(play, &this->actor.world.pos, 120 , NA_SE_IT_EXPLOSION_FRAME); //wooooosh t'as gagné bruit
    } else if ( (this->timerone == 45) || (this->timerone == 47)) {
        Actor_PlaySfx(&this->actor, NA_SE_EN_DODO_J_WALK );
    } else if  (this->timerone == 68) {
        Actor_PlaySfx(&this->actor, NA_SE_EN_DODO_K_WALK );
    }else if (this->timerone == 92) {
        Actor_Kill(&this->actor);
    }

    if ((38 < this->timerone) && (this->timerone < 88)){
        for (i = 0x2A ; i >= 0; i--) {
            pos.x = prepos.x + Rand_CenteredFloat(150.0f);
            pos.z = prepos.z + Rand_CenteredFloat(150.0f);
            pos.y = prepos.y + Rand_CenteredFloat(150.0f);
            EffectSsDeadDb_Spawn(play, &pos, &sp7C, &sp7C, 100, 0, 255, 255, 255, 255, 0, 0, 255, 1, 9, true);
        }
    }
    SkelAnime_Update(&this->skelAnime);
}

void En_Asylum_CheckColliding(EnAsylum* this, PlayState* play){

    Player* player = GET_PLAYER(play);

    if ( (this->colliderSpheres.base.acFlags & AC_HIT) && (this->actor.colorFilterTimer == 0) ){

        if (this->actor.colChkInfo.health != 0) {
            if (this->DemonState == 0) {
                this->DemonState = 1;
            } else {
                Actor_SetColorFilter(&this->actor, 0, 30, 0, 15);
                Actor_ApplyDamage(&this->actor);
            }
        }
    }

    CollisionCheck_SetOC(play, &play->colChkCtx, &this->colliderSpheres.base); // pushing collision

    CollisionCheck_SetAC(play, &play->colChkCtx, &this->colliderSpheres.base); // getting hurt collision 

    Collider_UpdateCylinder(&this->actor, &this->AssCollider); //update location of ass collider
}

void En_Asylum_Update(EnAsylum* this, PlayState* play) {

    Actor_UpdateBgCheckInfo(play, &this->actor, 70.0f, 100.0f, 190.0f, 0x1D);

    En_Asylum_CheckColliding(this,play);

    Math_SmoothStepToF(&this->actor.world.pos.y , this->actor.floorHeight , 1.0f , 2.0f , 0.0f);

    //this->actor.world.pos.y = this->actor.floorHeight;

    //AI_Behaviour(this,play);
    this->actionFunc(this, play);
}

s32 En_Asylum_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, void* thisx) {

    static Vec3f D_80A78514[] = {
        { -18000.0, -1700.0, -1200.0 }, //0
        { 0.0, -2500.0, 0.0 }, //1
        { -5200.0, -1700.0, -2000.0 },{ -4900.0, -1700.0, -4000.0 },{ -4600.0, -1700.0, -6000.0 },{ -4300.0, -1700.0, -8000.0 }, //2-5
        { -5200.0, -1700.0, 2000.0 },{ -4900.0, -1700.0, 4000.0 },{ -4600.0, -1700.0, 6000.0 },{ -4300.0, -1700.0, 8000.0 } //6-9
    };

    s32 i;
    s32 j;
    EnAsylum* this = THIS;

    if (limbIndex == 1) {

        Vec3f effVelocity = { 0.0f, 10.0f, 0.0f };
        Vec3f effAccel = { 0.0f, -0.5f, 0.0f };
        Color_RGBA8 primColor = { 0, 0, 0, 125 };
        Color_RGBA8 envColor = { 165, 146, 119, 0 };

        switch (this->ParticleBool) {

            case 1 : //STOMP I'M HERE BITCHES

                for (i = 40 ; i >= 0; i--) {

                    Vec3f effectPos;
                    effVelocity.y = 11.0f;
                    Matrix_MultVec3f(&D_80A78514[1], &effectPos);
                    effectPos.x += Rand_CenteredFloat(230.0f);
                    effectPos.z += Rand_CenteredFloat(230.0f);
                    effectPos.y -= 15.0f;
                    effAccel.x += Rand_CenteredFloat(2.0f);
                    effAccel.z += Rand_CenteredFloat(2.0f);
                    EffectSsDust_Spawn(play, 1, &effectPos, &effVelocity, & effAccel,
                                &primColor, &envColor, 75, 100, 75, 0 );
                }
            break;

            case 2 : // Big Juicy Ass Slam

                for (i = 40 ; i >= 0; i--) {

                    Vec3f effectPos;
                    effVelocity.y = 11.0f;
                    Matrix_MultVec3f(&D_80A78514[1], &effectPos);
                    effectPos.x += Rand_CenteredFloat(230.0f);
                    effectPos.z += Rand_CenteredFloat(230.0f);
                    effAccel.x += Rand_CenteredFloat(2.0f);
                    effAccel.z += Rand_CenteredFloat(2.0f);
                    EffectSsDust_Spawn(play, 1, &effectPos, &effVelocity, & effAccel,
                                &primColor, &envColor, 75, 100, 75, 0 );
                }
            break;

            case 3 : //Bonk Swing

                switch (this->timerone) {

                    case 19 :

                        for (i = 3 ; i >= 0; i--) {
                            Vec3f effectPos;
                            Matrix_MultVec3f(&D_80A78514[2], &effectPos);
                            effectPos.x += Rand_CenteredFloat(50.0f);
                            effectPos.z += Rand_CenteredFloat(50.0f);
                            EffectSsDust_Spawn(play, 1, &effectPos, &effVelocity, & effAccel,
                                    &primColor, &envColor, 75, 100, 75, 0 );
                        }

                    break;

                    case 20 :

                        for (i = 3 ; i >= 0; i--) {
                            Vec3f effectPos;
                            effVelocity.y = 11.0f;
                            Matrix_MultVec3f(&D_80A78514[3], &effectPos);
                            effectPos.x += Rand_CenteredFloat(50.0f);
                            effectPos.z += Rand_CenteredFloat(50.0f);
                            EffectSsDust_Spawn(play, 1, &effectPos, &effVelocity, & effAccel,
                                    &primColor, &envColor, 75, 100, 75, 0 );
                        }
                    break;

                    case 21 :

                        for (i = 3 ; i >= 0; i--) {
                            Vec3f effectPos;
                            effVelocity.y = 11.5f;
                            Matrix_MultVec3f(&D_80A78514[4], &effectPos);
                            effectPos.x += Rand_CenteredFloat(50.0f);
                            effectPos.z += Rand_CenteredFloat(50.0f);
                            EffectSsDust_Spawn(play, 1, &effectPos, &effVelocity, & effAccel,
                                    &primColor, &envColor, 75, 100, 75, 0 );
                        }

                    break;

                    case 22:

                        for (i = 3 ; i >= 0; i--) {
                            Vec3f effectPos;
                            effVelocity.y = 12.0f;
                            Matrix_MultVec3f(&D_80A78514[5], &effectPos);
                            effectPos.x += Rand_CenteredFloat(50.0f);
                            effectPos.z += Rand_CenteredFloat(50.0f);
                            EffectSsDust_Spawn(play, 1, &effectPos, &effVelocity, & effAccel,
                                    &primColor, &envColor, 75, 100, 75, 0 );
                        }
                        Actor_PlaySfx(&this->actor, NA_SE_EV_WALL_BROKEN );
                        Camera_RequestQuake(&play->mainCamera, 2, 15, 5);

                    break;
  
                }
            break;

            case 4 : //Bonk BackSwing

                switch (this->timerone) {

                    case 17 :

                        for (i = 3 ; i >= 0; i--) {
                            Vec3f effectPos;
                            Matrix_MultVec3f(&D_80A78514[6], &effectPos);
                            effectPos.x += Rand_CenteredFloat(50.0f);
                            effectPos.z += Rand_CenteredFloat(50.0f);
                            EffectSsDust_Spawn(play, 1, &effectPos, &effVelocity, & effAccel,
                                    &primColor, &envColor, 75, 100, 75, 0 );
                        }

                    break;

                    case 18 :

                        for (i = 3 ; i >= 0; i--) {
                            Vec3f effectPos;
                            effVelocity.y = 11.0f;
                            Matrix_MultVec3f(&D_80A78514[7], &effectPos);
                            effectPos.x += Rand_CenteredFloat(50.0f);
                            effectPos.z += Rand_CenteredFloat(50.0f);
                            EffectSsDust_Spawn(play, 1, &effectPos, &effVelocity, & effAccel,
                                    &primColor, &envColor, 75, 100, 75, 0 );
                        }
                    break;

                    case 19 :

                        for (i = 3 ; i >= 0; i--) {
                            Vec3f effectPos;
                            effVelocity.y = 11.5f;
                            Matrix_MultVec3f(&D_80A78514[8], &effectPos);
                            effectPos.x += Rand_CenteredFloat(50.0f);
                            effectPos.z += Rand_CenteredFloat(50.0f);
                            EffectSsDust_Spawn(play, 1, &effectPos, &effVelocity, & effAccel,
                                    &primColor, &envColor, 75, 100, 75, 0 );
                        }

                    break;

                    case 20:

                        for (i = 3 ; i >= 0; i--) {
                            Vec3f effectPos;
                            effVelocity.y = 12.0f;
                            Matrix_MultVec3f(&D_80A78514[9], &effectPos);
                            effectPos.x += Rand_CenteredFloat(50.0f);
                            effectPos.z += Rand_CenteredFloat(50.0f);
                            EffectSsDust_Spawn(play, 1, &effectPos, &effVelocity, & effAccel,
                                    &primColor, &envColor, 75, 100, 75, 0 );
                        }
                        Actor_PlaySfx(&this->actor, NA_SE_EV_WALL_BROKEN );
                        Camera_RequestQuake(&play->mainCamera, 2, 15, 5);

                    break;
  
                }
            break;

            case 5 : //Bonk Smash
                for (i = 10 ; i >= 0; i--) {

                    Vec3f effectPos;
                    Matrix_MultVec3f(&D_80A78514[0], &effectPos);
                    effectPos.x += Rand_CenteredFloat(50.0f);
                    effectPos.z += Rand_CenteredFloat(140.0f);
                    EffectSsDust_Spawn(play, 1, &effectPos, &effVelocity, & effAccel,
                                &primColor, &envColor, 75, 100, 75, 0 );
                }
            break;
        }
    } else if (limbIndex == 10) {
        this->LinkNewRot.x = rot->x;
        this->LinkNewRot.y = rot->y;
        this->LinkNewRot.z = rot->z;
    }

    return 0;
}

void En_Asylum_PostLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, void* thisx){

    static Vec3f frontShieldingTriModel0[] = {
        { -1600.0f, 2500.0f, 0.0f },
        { -1600.0f, 15500.0f, -2800.0f },
        { 1200.0f, 15500.0f, 1000.0f },
    };
    static Vec3f frontShieldingTriModel1[] = {
        { -1600.0f, 2500.0f, 0.0f },
        { -1600.0f, 15500.0f, -2800.0f },
        { -4800.0f, 15500.0f, -300.0f },
    };
    static Vec3f frontShieldingTriModel2[] = {
        { -1600.0f, 2500.0f, 0.0f },
        { -1500.0f, 15500.0f, 3100.0f },
        { -4800.0f, 15500.0f, -300.0f },
    };
    static Vec3f frontShieldingTriModel3[] = {
        { -1600.0f, 2500.0f, 0.0f },
        { -1500.0f, 15500.0f, 3100.0f },
        { 1200.0f, 15500.0f, 1000.0f },
    };
    static Vec3f LinkPos = { 0.0f, 0.0f, 0.0f };
    u8 i;
    Vec3f sp2C;

    Vec3f frontShieldingTri0[3];
    Vec3f frontShieldingTri1[3];
    Vec3f frontShieldingTri2[3];
    Vec3f frontShieldingTri3[3];

    EnAsylum* this = THIS;
    s32 bodyPartIndex = -1;

    Collider_UpdateSpheres(limbIndex, &this->colliderSpheres);

    //Actor_SetFeetPos(&this->actor, limbIndex, ARMATUREASYLUMDEMON_FOOTL, &feetPos, ARMATUREASYLUMDEMON_FOOTR, &feetPos);

    /*
    Actor_SetFeetPos(&this, &this->actor.shape.feetPos[ARMATUREASYLUMDEMON_FOOTL], &sp2C,
                      this->actor.floorHeight - this->actor.shape.feetPos[ARMATUREASYLUMDEMON_FOOTL].y, 7.0f, 5.0f);
    Actor_SetFeetPos(&this, &this->actor.shape.feetPos[ARMATUREASYLUMDEMON_FOOTR], &sp2C,
                      this->actor.floorHeight - this->actor.shape.feetPos[ARMATUREASYLUMDEMON_FOOTR].y, 7.0f, 5.0f);*/


    if (limbIndex == 9){ //torso
        ColliderJntSph* coco = &this->colliderSpheres;
        Actor_SetFocus(&this->actor, coco->elements[9].dim.worldSphere.center.y + 120.0f);
    }

    if (limbIndex == 10 ){//collar
        if (this->DemonState == 3) {
            Matrix_MultVec3f(&LinkPos, &this->LinkNewPos);
        }
    } 

    if (limbIndex == 17) { //handR

        for (i = 0; i < 3; i++) {
            Matrix_MultVec3f(&frontShieldingTriModel0[i], &frontShieldingTri0[i]);
            Matrix_MultVec3f(&frontShieldingTriModel1[i], &frontShieldingTri1[i]);
            Matrix_MultVec3f(&frontShieldingTriModel2[i], &frontShieldingTri2[i]);
            Matrix_MultVec3f(&frontShieldingTriModel3[i], &frontShieldingTri3[i]);
        }

        Collider_SetTrisVertices(&this->shieldCollider, 0, &frontShieldingTri0[0], &frontShieldingTri0[1],
                                 &frontShieldingTri0[2]);
        Collider_SetTrisVertices(&this->shieldCollider, 1, &frontShieldingTri1[0], &frontShieldingTri1[1],
                                 &frontShieldingTri1[2]);
        Collider_SetTrisVertices(&this->shieldCollider, 0, &frontShieldingTri2[0], &frontShieldingTri2[1],
                                 &frontShieldingTri2[2]);
        Collider_SetTrisVertices(&this->shieldCollider, 1, &frontShieldingTri3[0], &frontShieldingTri3[1],
                                 &frontShieldingTri3[2]);
    }   
}

void En_Asylum_Draw(EnAsylum* this, PlayState* play) {

    Player* player = GET_PLAYER(play); 

    s32 i;
    Vec3f frontShieldingTri0[3];
    Vec3f frontShieldingTri1[3];

    GfxPrint printer;
    Gfx* gfx = play->state.gfxCtx->polyOpa.p + 1;

    gSPDisplayList(play->state.gfxCtx->overlay.p++, gfx);

    GfxPrint_Init(&printer);
    GfxPrint_Open(&printer, gfx);

    GfxPrint_SetColor(&printer, 255, 255, 0, 255); 
    GfxPrint_SetPos(&printer, 1, 23);
    GfxPrint_Printf(&printer, "Player_curFrame: %f",player->skelAnime.curFrame);

    GfxPrint_SetColor(&printer, 255, 255, 0, 255); 
    GfxPrint_SetPos(&printer, 1, 24);
    GfxPrint_Printf(&printer, "Player_lastFrame: %f",player->skelAnime.endFrame);

    GfxPrint_SetColor(&printer, 0, 255, 0, 255); 
    GfxPrint_SetPos(&printer, 1, 25);
    GfxPrint_Printf(&printer, "Boss Health :");

    GfxPrint_SetColor(&printer, 0, 255, 0, 255); 
    GfxPrint_SetPos(&printer, 1, 26);
    GfxPrint_Printf(&printer, "%d", this->actor.colChkInfo.health); 

    gfx = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);

    gSPEndDisplayList(gfx++);
    gSPBranchList(play->state.gfxCtx->polyOpa.p, gfx);
    play->state.gfxCtx->polyOpa.p = gfx;

    SkelAnime_DrawFlexOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable,this->skelAnime.dListCount, En_Asylum_OverrideLimbDraw , En_Asylum_PostLimbDraw, this);

}