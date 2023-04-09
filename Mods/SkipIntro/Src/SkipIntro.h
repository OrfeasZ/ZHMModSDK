#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

class SkipIntro : public IPluginInterface
{
public:
    void Init() override;

private:
    DECLARE_PLUGIN_DETOUR(SkipIntro, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);
};

DECLARE_ZHM_PLUGIN(SkipIntro)
