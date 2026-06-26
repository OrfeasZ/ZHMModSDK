#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

class MaxPatchLevel : public IPluginInterface {
public:
    void Init() override;

private:
    DECLARE_PLUGIN_DETOUR(
        MaxPatchLevel, void*, IPackageManager_SPartitionInfo_SPartitionInfo, IPackageManager::SPartitionInfo* th, int32_t index,
        ZString partitionID, IPackageManager::EPartitionType type, int32_t patchLevel
    );
};

DECLARE_ZHM_PLUGIN(MaxPatchLevel)
