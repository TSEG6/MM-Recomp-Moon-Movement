#include "modding.h"
#include "global.h"
#include "math.h"
#include "overlays/actors/ovl_En_Fall/z_en_fall.h"
#include "z64item.h"
#include "recompconfig.h"
#include "recomputils.h"

Actor* EnFall_MoonsTear_GetTerminaFieldMoon(PlayState* play);

struct {
    EnFall* this;
    PlayState* play;
} EnFall_MoonPerform_Args;

struct {
    EnFall* this;
    PlayState* play;
} EnFall_MoonAdjust_Args;

struct {
    EnFall* this;
    PlayState* play;
} EnFall_MoonSetup_Args;

float moonStartY = 1000.0f;
float moonBaseScale = 1.0f;
Vec3f moonStartPos = { 0.0f, 0.0f, 0.0f }; // I had the moon going in a circle at one point because it was funny (maybe I'll add it as a feature later)
bool moonOverrideActive = false;
bool titlescreen = true;
bool moonFirstFrame = false;

RECOMP_HOOK ("FileSelect_FadeOut")
void hasenteredfileselect() {

    PlayState* play = EnFall_MoonSetup_Args.play;

    moonOverrideActive = true;
    titlescreen = false;
}

RECOMP_HOOK("Sram_SetFlashPagesOwlSave")
void hasenteredtitlescreenagain() {

    PlayState* play = EnFall_MoonSetup_Args.play;

    moonOverrideActive = false;
    titlescreen = true;
}

bool EnFall_CrashingMoon_IsMoonType(EnFall* this) {
    switch (EN_FALL_TYPE(&this->actor)) {
    case EN_FALL_TYPE_CRASH_FIRE_BALL:
    case EN_FALL_TYPE_CRASH_RISING_DEBRIS:
    case EN_FALL_TYPE_MOONS_TEAR:
    case EN_FALL_TYPE_CRASH_FIRE_RING:
        return false;
    default:
        return true;
    }
}


void EnFall_CrashingMoon_StoreScaleHook(EnFall* this, PlayState* play) {
    if (Object_IsLoaded(&play->objectCtx, this->objectSlot) &&
        EnFall_CrashingMoon_IsMoonType(this)) {

        moonStartPos.x = this->actor.home.pos.x;
        moonStartPos.z = this->actor.home.pos.z;

        moonStartY = this->actor.home.pos.y;
        moonBaseScale = this->scale;
    }
}

void EnFall_CrashingMoon_PerformActionsCommonHook(EnFall* this, PlayState* play) {
    if (!EnFall_CrashingMoon_IsMoonType(this)) return;

    bool hasOcarina = (INV_CONTENT(ITEM_OCARINA_OF_TIME) == ITEM_OCARINA_OF_TIME);
    bool isInverted = (EN_FALL_TYPE(&this->actor) == EN_FALL_TYPE_LODMOON_INVERTED_STONE_TOWER);

    double firstCycleUseC = recomp_get_config_double("first_cycle_use");
    double moonScaleC = recomp_get_config_double("moon_scale");
    double moonHeightC = recomp_get_config_double("moon_hstart");
    double curvedMovementC = recomp_get_config_double("curved_growth");
    double moonRotationC = recomp_get_config_double("moon_rotation");
    double moonRotationEndC = recomp_get_config_double("moon_rotate_end");
    double facingPlayerC = recomp_get_config_double("player_rotation");

    u16 currentTime = CURRENT_TIME;
    s16 pitchOffset = 0;
    s32 targetPitch = isInverted ? 0x9000 : -0x9000;

    if (play->sceneId == SCENE_OKUJOU || CURRENT_DAY == 0 || CURRENT_DAY == 4) {
        return;
    }

    if (play->csCtx.state != 0) {
        if (!(play->sceneId == SCENE_10YUKIYAMANOMURA2 || play->sceneId == SCENE_21MITURINMAE || play->sceneId == SCENE_IKANAMAE)) {
            return;
        }
    }

    if (!moonOverrideActive) {
        return;
    }

    if (!hasOcarina && firstCycleUseC == 1.0 && (CURRENT_DAY == 1 || CURRENT_DAY == 2)) {
        return;
    }

    if (AudioSeq_GetActiveSeqId(SEQ_PLAYER_BGM_MAIN) == NA_BGM_GATHERING_GIANTS) {
        return;
    }

    u16 dayStartTime = CLOCK_TIME(6, 1);
    u16 dayEndTime = CLOCK_TIME(5, 59);

    float dayLength;
    float timeIntoDay;

    if (currentTime >= dayStartTime) {
        timeIntoDay = currentTime - dayStartTime;
    }
    else {
        timeIntoDay = (0x10000 - dayStartTime) + currentTime;
    }

    dayLength = (dayEndTime >= dayStartTime)
        ? (dayEndTime - dayStartTime)
        : ((0x10000 - dayStartTime) + dayEndTime);

    float dayProgress = timeIntoDay / dayLength;
    dayProgress = CLAMP(dayProgress, 0.0f, 1.0f);

    float totalProgress = ((CURRENT_DAY - 1) + dayProgress) / 3.0f;
    totalProgress = CLAMP(totalProgress, 0.0f, 1.0f);

    float easedProgress = totalProgress;

    // "Curved" based movement real???
    if (curvedMovementC == 0.0) {
        easedProgress = totalProgress * totalProgress;
    }

    float vanillaStartScale = moonBaseScale * 1.2f;
    float vanillaEndScale = moonBaseScale * 3.6f;

    float customStartScale = vanillaStartScale * (float)moonScaleC;

    float scaleFactor = customStartScale + (vanillaEndScale - customStartScale) * easedProgress;
    Actor_SetScale(&this->actor, scaleFactor);

    float vanillaTotalDistance = 6700.0f * moonBaseScale * 6.25f;
    float vanillaEndY;

    if (EN_FALL_TYPE(&this->actor) == EN_FALL_TYPE_LODMOON_INVERTED_STONE_TOWER) {
        vanillaEndY = moonStartY + vanillaTotalDistance;
    }
    else {
        vanillaEndY = moonStartY - vanillaTotalDistance;
    }

    float customStartY = moonStartY + (float)moonHeightC;

    this->actor.world.pos.y = customStartY + (vanillaEndY - customStartY) * easedProgress;

    this->actor.world.pos.x = moonStartPos.x;
    this->actor.world.pos.z = moonStartPos.z;

    // the horrors
    if (facingPlayerC == 0.0) {
        Player* player = GET_PLAYER(play);

        s16 targetYaw = Math_Vec3f_Yaw(&this->actor.world.pos, &player->actor.world.pos);
        s16 targetPitch = Math_Vec3f_Pitch(&this->actor.world.pos, &player->actor.world.pos);

        if (moonFirstFrame) {
            this->actor.shape.rot.y = targetYaw;
            this->actor.shape.rot.x = targetPitch;
        }
        else {
            Math_SmoothStepToS(&this->actor.shape.rot.y, targetYaw, 10, 0x1000, 0x10);
            Math_SmoothStepToS(&this->actor.shape.rot.x, targetPitch, 10, 0x1000, 0x10);
        }
    }
    else {
        if (moonRotationC != 1.0) {
            float rotationProgress = 1.0f;
            int targetDay = (int)moonRotationEndC;
            float currentOverallProgress = (CURRENT_DAY - 1) + dayProgress;
            float targetOverallProgress = (targetDay - 1) + 0.5f;

            if (currentOverallProgress <= 0.0f) {
                rotationProgress = 0.0f;
            }
            else if (currentOverallProgress >= targetOverallProgress) {
                rotationProgress = 1.0f;
            }
            else {
                rotationProgress = currentOverallProgress / targetOverallProgress;
            }

            pitchOffset = (s16)(targetPitch * (1.0f - rotationProgress));
        }

        s16 defaultPitch = this->actor.home.rot.x + pitchOffset;

        if (moonFirstFrame) {
            this->actor.shape.rot.x = defaultPitch;
            this->actor.shape.rot.y = this->actor.home.rot.y;
        }
        else {
            Math_SmoothStepToS(&this->actor.shape.rot.x, defaultPitch, 10, 0x1000, 0x10);
            Math_SmoothStepToS(&this->actor.shape.rot.y, this->actor.home.rot.y, 10, 0x1000, 0x10);
        }
    }
    moonFirstFrame = false;
}

// Darío's Crazy Moon hooks below (thanks btw)

RECOMP_HOOK("EnFall_Setup")
void EnFall_SetupHook(EnFall* this, PlayState* play) {
    

    EnFall_MoonSetup_Args.this = this;
    EnFall_MoonSetup_Args.play = play;
}

RECOMP_HOOK_RETURN("EnFall_Setup")
void EnFall_SetupHookReturn() {
    EnFall* this = EnFall_MoonSetup_Args.this;
    PlayState* play = EnFall_MoonSetup_Args.play;

    moonFirstFrame = true;
    EnFall_CrashingMoon_StoreScaleHook(this, play);
    EnFall_CrashingMoon_PerformActionsCommonHook(this, play);
}

RECOMP_HOOK("EnFall_CrashingMoon_PerformCutsceneActions")
void EnFall_CrashingMoon_PerformCutsceneActionsHook(
    EnFall* this, PlayState* play) {


    EnFall_MoonPerform_Args.this = this;
    EnFall_MoonPerform_Args.play = play;
}

RECOMP_HOOK_RETURN("EnFall_CrashingMoon_PerformCutsceneActions")
void EnFall_CrashingMoon_PerformCutsceneActionsHookReturn() {

    EnFall_CrashingMoon_PerformActionsCommonHook(
        EnFall_MoonPerform_Args.this,
        EnFall_MoonPerform_Args.play
    );
}


RECOMP_HOOK("EnFall_StoppedOpenMouthMoon_PerformCutsceneActions")
void EnFall_StoppedOpenMouthMoon_PerformCutsceneActionsHook(
    EnFall* this, PlayState* play) {


    EnFall_MoonPerform_Args.this = this;
    EnFall_MoonPerform_Args.play = play;
}

RECOMP_HOOK_RETURN("EnFall_StoppedOpenMouthMoon_PerformCutsceneActions")
void EnFall_StoppedOpenMouthMoon_PerformCutsceneActionsHookReturn() {
    EnFall_CrashingMoon_PerformActionsCommonHook(
        EnFall_MoonPerform_Args.this,
        EnFall_MoonPerform_Args.play
    );
}

RECOMP_HOOK("EnFall_StoppedClosedMouthMoon_PerformCutsceneActions")
void EnFall_StoppedClosedMouthMoon_PerformCutsceneActionsHook(
    EnFall* this, PlayState* play) {

    EnFall_MoonPerform_Args.this = this;
    EnFall_MoonPerform_Args.play = play;
}

RECOMP_HOOK_RETURN("EnFall_StoppedClosedMouthMoon_PerformCutsceneActions")
void EnFall_StoppedClosedMouthMoon_PerformCutsceneActionsHookReturn() {
    EnFall_CrashingMoon_PerformActionsCommonHook(
        EnFall_MoonPerform_Args.this,
        EnFall_MoonPerform_Args.play
    );
}

RECOMP_HOOK("EnFall_ClockTowerOrTitleScreenMoon_PerformCutsceneActions")
void EnFall_ClockTowerOrTitleScreenMoon_PerformCutsceneActionsHook(
    EnFall* this, PlayState* play) {

    EnFall_MoonPerform_Args.this = this;
    EnFall_MoonPerform_Args.play = play;
}

RECOMP_HOOK_RETURN("EnFall_ClockTowerOrTitleScreenMoon_PerformCutsceneActions")
void EnFall_ClockTowerOrTitleScreenMoon_PerformCutsceneActionsHookReturn() {
    EnFall_CrashingMoon_PerformActionsCommonHook(
        EnFall_MoonPerform_Args.this,
        EnFall_MoonPerform_Args.play
    );
}

RECOMP_HOOK("EnFall_Moon_PerformDefaultActions")
void EnFall_Moon_PerformDefaultActionsHook(
    EnFall* this, PlayState* play) {

    EnFall_MoonPerform_Args.this = this;
    EnFall_MoonPerform_Args.play = play;
}

RECOMP_HOOK_RETURN("EnFall_Moon_PerformDefaultActions")
void EnFall_Moon_PerformDefaultActionsHookReturn() {
    EnFall_CrashingMoon_PerformActionsCommonHook(
        EnFall_MoonPerform_Args.this,
        EnFall_MoonPerform_Args.play
    );
}

RECOMP_HOOK("EnFall_Moon_AdjustScaleAndPosition")
void EnFall_Moon_AdjustScaleAndPositionHook(
    EnFall* this, PlayState* play) {

    EnFall_MoonAdjust_Args.this = this;
    EnFall_MoonAdjust_Args.play = play;
}

RECOMP_HOOK_RETURN("EnFall_Moon_AdjustScaleAndPosition")
void EnFall_Moon_AdjustScaleAndPositionHookReturn() {
    EnFall_CrashingMoon_StoreScaleHook(
        EnFall_MoonAdjust_Args.this,
        EnFall_MoonAdjust_Args.play
    );
}

static f32 sMoonEyeGlow = 0.0f;

RECOMP_HOOK("EnFall_Update") // Glowing Eyes Stuff
void MoonEyeGlow(EnFall* moon, PlayState* play) {

    double glowingEyes = recomp_get_config_double("glowing_eyes");
    double glowingEyesStrength = recomp_get_config_double("glowing_eyes_intensity"); 

    if (play->sceneId == SCENE_OKUJOU || play->csCtx.state != CS_STATE_IDLE) {

        return;
        moon->eyeGlowIntensity = 0.0f;
    }

    if (glowingEyes == 1.0) {
        moon->eyeGlowIntensity = 0.0f;
        return;
    }

    u16 time = CURRENT_TIME;
    f32 glow = 0.0f;
    f32 t;

    if ((time >= CLOCK_TIME(17, 0)) && (time < CLOCK_TIME(19, 0))) {
        t = (float)(time - CLOCK_TIME(17, 0)) / (CLOCK_TIME(19, 0) - CLOCK_TIME(17, 0));
        t = CLAMP(t, 0.0f, 1.0f);
        glow = t;  
    }
    
    else if ((time >= CLOCK_TIME(19, 0)) || (time < CLOCK_TIME(4, 0))) {
        glow = 1.0f;
    }

    else if ((time >= CLOCK_TIME(4, 0)) && (time < CLOCK_TIME(6, 0))) {
        t = (float)(time - CLOCK_TIME(4, 0)) / (CLOCK_TIME(6, 0) - CLOCK_TIME(4, 0));
        t = CLAMP(t, 0.0f, 1.0f);
        glow = 1.0f - t;  
    }
    
    else {
        glow = 0.0f;
    }
    
    glow *= (float)glowingEyesStrength;

    sMoonEyeGlow += (glow - sMoonEyeGlow) * 0.1f;
    moon->eyeGlowIntensity = sMoonEyeGlow;
}

// Moon day transition fix (I hate that this works to fix the issue) (actually fixed now lol)

RECOMP_HOOK("Sram_IncrementDay")
void jankfix() {

    moonOverrideActive = false;
}

RECOMP_HOOK("Player_Update")
void jankfixpart2() {

    if (!titlescreen) {
        moonOverrideActive = true;
    }
}