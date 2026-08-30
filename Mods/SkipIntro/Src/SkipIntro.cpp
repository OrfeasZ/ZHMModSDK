#include "SkipIntro.h"

#include "Glacier/ZComponent.h"

#include "Hooks.h"
#include "Logging.h"
#include "Functions.h"

void SkipIntro::Init() {
    Hooks::ZEngineAppCommon_GetBootScene->AddDetour(this, &SkipIntro::ZEngineAppCommon_GetBootScene);
}

DEFINE_PLUGIN_DETOUR(SkipIntro, ZString*, ZEngineAppCommon_GetBootScene, ZEngineAppCommon* th, ZString& result) {
    if (Functions::GetApplicationOptionBool->Call("START_BENCHMARK", false)) {
        return HookResult<ZString*>(HookAction::Continue());
    }

    ZString s_Result = (*Globals::ComponentManager)->m_pApplication->GetOption("SCENE_FILE");

    if (s_Result == "assembly:/_PRO/Scenes/Frontend/Boot.entity") {
        s_Result = "assembly:/_PRO/Scenes/Frontend/MainMenu.entity";
    }

    std::construct_at(&result, std::move(s_Result));

    return HookResult<ZString*>(HookAction::Return(), &result);
}

DEFINE_ZHM_PLUGIN(SkipIntro);
