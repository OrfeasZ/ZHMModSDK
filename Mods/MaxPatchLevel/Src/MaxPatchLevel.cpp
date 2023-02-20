#include "MaxPatchLevel.h"

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZScene.h>

void MaxPatchLevel::Init()
{
    Hooks::ZPackageManagerPackage_ZPackageManagerPackage->AddDetour(this, &MaxPatchLevel::ZPackageManagerPackage_ZPackageManagerPackage);
}

DECLARE_PLUGIN_DETOUR(MaxPatchLevel, void*, ZPackageManagerPackage_ZPackageManagerPackage, ZPackageManagerPackage* th, void* a2, const ZString& a3, int a4, int patchLevel)
{
    // 1000 should be enough. Higher numbers just make the game freeze up when going into mission planning.
    Logger::Debug("Changing patch level for '{}' package from {} to {}.", a3.c_str(), patchLevel, 1000);
    return HookResult<void*>(HookAction::Return(), p_Hook->CallOriginal(th, a2, a3, a4, 1000));
}

DECLARE_ZHM_PLUGIN(MaxPatchLevel);
