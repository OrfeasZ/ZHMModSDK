#pragma once

#include "IPluginInterface.h"

class SkipIntro : public IPluginInterface {
public:
    void Init() override;

private:
    DECLARE_PLUGIN_DETOUR(SkipIntro, ZString*, ZEngineAppCommon_GetBootScene, ZEngineAppCommon* th, ZString& result);
};

DECLARE_ZHM_PLUGIN(SkipIntro)
