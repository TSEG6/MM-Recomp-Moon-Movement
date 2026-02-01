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


float g_moonStartY = 1000.0f;
float g_moonBaseScale = 1.0f;
Vec3f g_moonStartPos = { 0.0f, 0.0f, 0.0f }; // I had the moon going in a circle at one point because it was funny
bool g_moonOverrideActive = false;

RECOMP_HOOK ("FileSelect_FadeOut")
void hasenteredfileselect() {

    PlayState* play = EnFall_MoonSetup_Args.play;

        g_moonOverrideActive = true;


}



RECOMP_HOOK("Sram_SetFlashPagesOwlSave")
void hasenteredtitlescreenagain() {

    PlayState* play = EnFall_MoonSetup_Args.play;

    g_moonOverrideActive = false;


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

        g_moonStartPos.x = this->actor.home.pos.x;
        g_moonStartPos.z = this->actor.home.pos.z;
        g_moonStartY = this->actor.home.pos.y;
        g_moonBaseScale = this->scale;
    }
}


void EnFall_CrashingMoon_PerformActionsCommonHook(EnFall* this, PlayState* play) {
    if (!EnFall_CrashingMoon_IsMoonType(this)) return;

    bool hasOcarina = (INV_CONTENT(ITEM_OCARINA_OF_TIME) == ITEM_OCARINA_OF_TIME);
    double firstCycleUse = recomp_get_config_double("first_cycle_use");


    if (play->sceneId == SCENE_OKUJOU || play->csCtx.state != CS_STATE_IDLE || CURRENT_DAY == 0 || CURRENT_DAY == 4) {
        
        return;
    }

    if (play->csCtx.state != 0) {

        return;

    }

    if (!g_moonOverrideActive){
    

        return;
    }
    else {


    }


    if (!hasOcarina && firstCycleUse == 1.0 &&(CURRENT_DAY == 1 || CURRENT_DAY == 2)) {
        
        return;

    }


    u16 currentTime = CURRENT_TIME;
    u16 dayStartTime = CLOCK_TIME(6, 0);

    float dayProgress = (currentTime < dayStartTime)
        ? 1.0f - ((float)(dayStartTime - currentTime) / 65536.0f)
        : ((float)(currentTime - dayStartTime) / 65536.0f);

    dayProgress = CLAMP(dayProgress, 0.0f, 1.0f);

    float totalProgress = ((CURRENT_DAY - 1) + dayProgress) / 3.0f;
    totalProgress = CLAMP(totalProgress, 0.0f, 1.0f);

    float scaleFactor = g_moonBaseScale * (1.2f + (3.6f - 1.2f) * totalProgress);
    Actor_SetScale(&this->actor, scaleFactor);

    float yOffset = totalProgress * 6700.0f * g_moonBaseScale * 6.25f;

    if (EN_FALL_TYPE(&this->actor) == EN_FALL_TYPE_LODMOON_INVERTED_STONE_TOWER) {
        this->actor.world.pos.y = g_moonStartY + yOffset;
    }
    else {
        this->actor.world.pos.y = g_moonStartY - yOffset;
    }

    this->actor.world.pos.x = g_moonStartPos.x;
    this->actor.world.pos.z = g_moonStartPos.z;


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

RECOMP_HOOK("EnFall_Update")
void MoonEyeGlow(EnFall* moon, PlayState* play) {

    double glowingEyes = recomp_get_config_double("glowing_eyes");
    double glowingEyesStrength = recomp_get_config_double("glowing_eyes_intensity"); // 0 → 1

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

    /* Dusk: 17:00 → 19:00 */
    if ((time >= CLOCK_TIME(17, 0)) && (time < CLOCK_TIME(19, 0))) {
        t = (float)(time - CLOCK_TIME(17, 0)) / (CLOCK_TIME(19, 0) - CLOCK_TIME(17, 0));
        t = CLAMP(t, 0.0f, 1.0f);
        glow = t;  // 0 → 1
    }
    /* Night: 19:00 → 04:00 (wraps past midnight) */
    else if ((time >= CLOCK_TIME(19, 0)) || (time < CLOCK_TIME(4, 0))) {
        glow = 1.0f;
    }
    /* Dawn: 04:00 → 06:00 */
    else if ((time >= CLOCK_TIME(4, 0)) && (time < CLOCK_TIME(6, 0))) {
        t = (float)(time - CLOCK_TIME(4, 0)) / (CLOCK_TIME(6, 0) - CLOCK_TIME(4, 0));
        t = CLAMP(t, 0.0f, 1.0f);
        glow = 1.0f - t;  // 1 → 0
    }
    /* Daytime: 06:00 → 17:00 */
    else {
        glow = 0.0f;
    }

    /* Apply strength scaling */
    glow *= (float)glowingEyesStrength;

    /* Smooth the transition to avoid bouncing */
    sMoonEyeGlow += (glow - sMoonEyeGlow) * 0.1f;
    moon->eyeGlowIntensity = sMoonEyeGlow;
}