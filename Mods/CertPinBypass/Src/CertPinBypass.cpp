#include "CertPinBypass.h"

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZScene.h>

void CertPinBypass::Init()
{
    Hooks::Check_SSL_Cert->AddDetour(this, &CertPinBypass::On_Check_SSL_Cert);
}

DECLARE_PLUGIN_DETOUR(CertPinBypass, bool, On_Check_SSL_Cert, void* unk1, void* unk2)
{
    return HookResult<bool>(HookAction::Return(), true);
}

DECLARE_ZHM_PLUGIN(CertPinBypass);
