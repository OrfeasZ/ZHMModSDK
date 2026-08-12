#include "MaxPatchLevel.h"

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZScene.h>

void MaxPatchLevel::Init() {
    Hooks::IPackageManager_SPartitionInfo_SPartitionInfo->AddDetour(
        this, &MaxPatchLevel::IPackageManager_SPartitionInfo_SPartitionInfo
    );
}

DEFINE_PLUGIN_DETOUR(
    MaxPatchLevel, void*, IPackageManager_SPartitionInfo_SPartitionInfo, IPackageManager::SPartitionInfo* th, int32_t index,
    ZString partitionID, IPackageManager::EPartitionType type, int32_t patchLevel
) {
    // 1000 should be enough. Higher numbers just make the game freeze up when going into mission planning.
    Logger::Debug("Changing patch level for '{}' package from {} to {}.", partitionID.c_str(), patchLevel, 1000);
    return HookResult<void*>(HookAction::Return(), p_Hook->CallOriginal(th, index, partitionID, type, 1000));
}

DEFINE_ZHM_PLUGIN(MaxPatchLevel);
