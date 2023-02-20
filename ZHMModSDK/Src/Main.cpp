#include <Windows.h>

#include "Logging.h"
#include "ModSDK.h"

DWORD WINAPI StartupProc(LPVOID)
{
    ModSDK::GetInstance()->ThreadedStartup();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        if (!ModSDK::GetInstance()->Startup())
        {
            return false;
        }

        // We start up the SDK in a different thread because of the weirdness that
        // is DllMain. That way we are free to load Dlls, create threads, and do whatever
        // else without having to worry about weird initialization order nonsense.
        CreateThread(nullptr, 0, StartupProc, nullptr, 0, nullptr);
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        ModSDK::DestroyInstance();
    }

    return true;
}

#if _DEBUG
extern "C" __declspec(dllexport) void Unload()
{
    Logger::Debug("Unload requested. Destroying Mod SDK instance.");
    ModSDK::DestroyInstance();
}
#endif
