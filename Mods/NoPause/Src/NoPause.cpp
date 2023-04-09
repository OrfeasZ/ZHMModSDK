#include "NoPause.h"
#include "Logging.h"

#include <Glacier/ZString.h>

void NoPause::Init()
{
    Hooks::GetApplicationOptionBool->AddDetour(this, &NoPause::GetOption);
    Hooks::ZApplicationEngineWin32_MainWindowProc->AddDetour(this, &NoPause::WndProc);
}

DEFINE_PLUGIN_DETOUR(NoPause, bool, GetOption, const ZString& p_OptionName, bool p_Default)
{
    if (p_OptionName == "PauseOnFocusLoss")
        return HookResult<bool>(HookAction::Return(), false);

    if (p_OptionName == "NO_MINIMIZE_FOCUSLOSS")
        return HookResult<bool>(HookAction::Return(), true);

    return HookResult<bool>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(NoPause, LRESULT, WndProc, ZApplicationEngineWin32* th, HWND p_Hwnd, UINT p_Message, WPARAM p_Wparam, LPARAM p_Lparam)
{
    if (p_Message == WM_ACTIVATEAPP)
        return HookResult<LRESULT>(HookAction::Return(), DefWindowProcW(p_Hwnd, p_Message, p_Wparam, p_Lparam));

    return HookResult<LRESULT>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(NoPause);
