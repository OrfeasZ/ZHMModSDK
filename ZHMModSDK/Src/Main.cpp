#include <Windows.h>

#include "Logging.h"
#include "ModSDK.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		if (!ModSDK::GetInstance()->Startup())
			return false;
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