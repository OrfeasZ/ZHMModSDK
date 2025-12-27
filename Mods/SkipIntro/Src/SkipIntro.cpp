#include "SkipIntro.h"

#include "Hooks.h"
#include "Logging.h"

void SkipIntro::Init() {
    Hooks::ZEngineAppCommon_GetBootScene->AddDetour(this, &SkipIntro::ZEngineAppCommon_GetBootScene);
}

DEFINE_PLUGIN_DETOUR(SkipIntro, ZString*, ZEngineAppCommon_GetBootScene, ZEngineAppCommon* th, ZString& result) {
    p_Hook->CallOriginal(th, result);

    if (result == "assembly:/_PRO/Scenes/Frontend/Boot.entity") {
        result = "assembly:/_PRO/Scenes/Frontend/MainMenu.entity";
    }

    return HookResult<ZString*>(HookAction::Return(), &result);
}

DEFINE_ZHM_PLUGIN(SkipIntro);
