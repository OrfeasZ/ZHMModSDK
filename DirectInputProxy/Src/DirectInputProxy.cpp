#include <thread>
#include <Windows.h>

static HMODULE g_OriginalDirectInput = nullptr;
static HMODULE g_ZHMModSDK = nullptr;

typedef HRESULT (__stdcall* DirectInput8Create_t)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
static DirectInput8Create_t o_DirectInput8Create = nullptr;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		char s_Dinput8Path[MAX_PATH];

		if (!GetSystemDirectoryA(s_Dinput8Path, sizeof(s_Dinput8Path)))
			return false;

		strcat_s(s_Dinput8Path, sizeof(s_Dinput8Path), "\\dinput8.dll");

		g_OriginalDirectInput = LoadLibraryA(s_Dinput8Path);

		if (g_OriginalDirectInput == nullptr)
			return false;

		o_DirectInput8Create = reinterpret_cast<DirectInput8Create_t>(GetProcAddress(g_OriginalDirectInput, "DirectInput8Create"));

		if (o_DirectInput8Create == nullptr)
			return false;

		g_ZHMModSDK = LoadLibraryA("ZHMModSDK");

		if (g_ZHMModSDK == nullptr)
			return false;

		// These threads allow an external process to cause the SDK to unload and reload.
		// This is useful for hot-reloading scenarios, where we don't want to restart the entire
		// game to test a set of changes.
#if _DEBUG
		static std::thread s_UnloadThread([&]()
		{
			const auto s_UnloadEvent = CreateEventA(nullptr, true, false, "Global_ZHMSDK_Unload_Signal");

			if (s_UnloadEvent == nullptr)
				return;
			
			while (true)
			{
				WaitForSingleObject(s_UnloadEvent, INFINITE);
				ResetEvent(s_UnloadEvent);

				if (g_ZHMModSDK != nullptr)
				{
					GetProcAddress(g_ZHMModSDK, "Unload")();

					while (FreeLibrary(g_ZHMModSDK));

					g_ZHMModSDK = nullptr;
				}

				const auto s_UnloadedEvent = OpenEventA(EVENT_MODIFY_STATE, false, "GLOBAL_ZHMSDK_Unloaded_Signal");

				if (s_UnloadedEvent != nullptr)
				{
					SetEvent(s_UnloadedEvent);
					CloseHandle(s_UnloadedEvent);
				}
			}
		});

		static std::thread s_ReloadThread([&]()
		{
			const auto s_ReloadEvent = CreateEventA(nullptr, true, false, "Global_ZHMSDK_Reload_Signal");

			if (s_ReloadEvent == nullptr)
				return;

			while (true)
			{
				WaitForSingleObject(s_ReloadEvent, INFINITE);
				ResetEvent(s_ReloadEvent);

				if (g_ZHMModSDK != nullptr)
				{
					GetProcAddress(g_ZHMModSDK, "Unload")();

					while (FreeLibrary(g_ZHMModSDK));

					g_ZHMModSDK = nullptr;
				}

				const auto s_UnloadedEvent = OpenEventA(EVENT_MODIFY_STATE, false, "GLOBAL_ZHMSDK_Unloaded_Signal");

				if (s_UnloadedEvent != nullptr)
				{
					SetEvent(s_UnloadedEvent);
					CloseHandle(s_UnloadedEvent);
				}

				g_ZHMModSDK = LoadLibraryA("ZHMModSDK");

				const auto s_LoadedEvent = OpenEventA(EVENT_MODIFY_STATE, false, "GLOBAL_ZHMSDK_Loaded_Signal");

				if (s_LoadedEvent != nullptr)
				{
					SetEvent(s_LoadedEvent);
					CloseHandle(s_LoadedEvent);
				}
			}
		});
#endif
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		if (g_OriginalDirectInput != nullptr)
		{
			o_DirectInput8Create = nullptr;
			
			FreeLibrary(g_OriginalDirectInput);
			g_OriginalDirectInput = nullptr;
		}

		if (g_ZHMModSDK != nullptr)
		{
			FreeLibrary(g_ZHMModSDK);
			g_ZHMModSDK = nullptr;
		}
	}

	return true;
}

extern "C" __declspec(dllexport) HRESULT DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter)
{
	if (o_DirectInput8Create == nullptr)
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_READY); // DIERR_NOTINITIALIZED

	return o_DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}