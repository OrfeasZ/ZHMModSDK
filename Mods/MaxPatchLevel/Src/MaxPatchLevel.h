#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

class MaxPatchLevel : public IPluginInterface {
public:
    void Init() override;

private:
    DECLARE_PLUGIN_DETOUR(
        MaxPatchLevel, void*, IPackageManager_SPartitionInfo_IPackageManager_SPartitionInfo, IPackageManager::SPartitionInfo* th, void* a2,
        const ZString& a3, int a4, int patchLevel
    );
};

DECLARE_ZHM_PLUGIN(MaxPatchLevel)
