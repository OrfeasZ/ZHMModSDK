#define WIN32_LEAN_AND_MEAN 
#include <thread>
#include <unordered_set>
#include <Windows.h>
#include <TlHelp32.h>
#include <filesystem>

static HMODULE g_ZHMModSDK = nullptr;


HMODULE g_thisModule;
bool o_loaded = false;
HMODULE o_dll = NULL;

static std::unordered_set<HANDLE>* g_SuspendedThreads = nullptr;

#pragma comment(linker, "/export:DirectInput8Create=DirectInput8Create")
#pragma comment(linker, "/export:DllCanUnloadNow=DllCanUnloadNow,PRIVATE")
#pragma comment(linker, "/export:DllGetClassObject=DllGetClassObject,PRIVATE")
#pragma comment(linker, "/export:DllRegisterServer=DllRegisterServer,PRIVATE")
#pragma comment(linker, "/export:DllUnregisterServer=DllUnregisterServer,PRIVATE")
#pragma comment(linker, "/export:GetdfDIJoystick=GetdfDIJoystick")

typedef HRESULT(APIENTRY* o_DirectInput8Create )(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, void* punkOuter);

o_DirectInput8Create  DirectInput8Create_t = NULL;
FARPROC DllCanUnloadNow_t;
FARPROC DllGetClassObject_t;
FARPROC DllRegisterServer_t;
FARPROC DllUnregisterServer_t;
FARPROC GetdfDIJoystick_t;

void SuspendAllThreadsButCurrent()
{
	if (g_SuspendedThreads != nullptr)
		return;

	g_SuspendedThreads = new std::unordered_set<HANDLE>();

	HANDLE s_Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	if (s_Snapshot == INVALID_HANDLE_VALUE)
		return;

	auto s_CurrentThread = GetCurrentThreadId();

	THREADENTRY32 s_ThreadEntry{};
	s_ThreadEntry.dwSize = sizeof(s_ThreadEntry);

	if (Thread32First(s_Snapshot, &s_ThreadEntry))
	{
		do
		{
			if (s_ThreadEntry.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(s_ThreadEntry.th32OwnerProcessID) &&
				s_ThreadEntry.th32ThreadID != s_CurrentThread &&
				s_ThreadEntry.th32OwnerProcessID == GetCurrentProcessId())
			{
				HANDLE s_Thread = OpenThread(THREAD_ALL_ACCESS, false, s_ThreadEntry.th32ThreadID);

				if (s_Thread != nullptr)
					g_SuspendedThreads->insert(s_Thread);
			}

			s_ThreadEntry.dwSize = sizeof(s_ThreadEntry);
		} while (Thread32Next(s_Snapshot, &s_ThreadEntry));
	}

	CloseHandle(s_Snapshot);

	for (auto* s_Thread : *g_SuspendedThreads)
		SuspendThread(s_Thread);
}

void ResumeSuspendedThreads()
{
	if (g_SuspendedThreads == nullptr)
		return;

	for (auto* s_Thread : *g_SuspendedThreads)
	{
		ResumeThread(s_Thread);
		CloseHandle(s_Thread);
	}

	delete g_SuspendedThreads;
	g_SuspendedThreads = nullptr;
}


std::wstring GetModuleFormSysdir()
{
	// get the filename of our DLL and try loading the DLL with the same name from system32
	WCHAR modulePath[MAX_PATH] = { 0 };
	if (!GetSystemDirectoryW(modulePath, _countof(modulePath)))
	{
		MessageBoxW(nullptr, L"GetSystemDirectoryW failed", L"Error", MB_ICONERROR);
	}

	// get filename of this DLL, which should be the original DLLs filename too
	WCHAR ourModulePath[MAX_PATH] = { 0 };
	GetModuleFileNameW(g_thisModule, ourModulePath, _countof(ourModulePath));

	WCHAR exeName[MAX_PATH] = { 0 };
	WCHAR extName[MAX_PATH] = { 0 };
	_wsplitpath_s(ourModulePath, NULL, NULL, NULL, NULL, exeName, MAX_PATH, extName, MAX_PATH);

	swprintf_s(modulePath, MAX_PATH, L"%ws\\%ws%ws", modulePath, exeName, extName);

	std::wstring path = std::wstring(modulePath);

	return path;
};
bool LoadProxiedDll()
{
	if (o_loaded)
		return true;

	o_dll = LoadLibraryW(GetModuleFormSysdir().c_str());
	if (!o_dll)
	{
		MessageBoxW(nullptr, L"Could not load originial module", L"Error", MB_ICONERROR);
		return false;
	}

	DirectInput8Create_t = (o_DirectInput8Create )GetProcAddress(o_dll, "DirectInput8Create");
	DllCanUnloadNow_t = GetProcAddress(o_dll, "DllCanUnloadNow");
	DllGetClassObject_t = GetProcAddress(o_dll, "DllGetClassObject");
	DllRegisterServer_t = GetProcAddress(o_dll, "DllRegisterServer");
	DllUnregisterServer_t = GetProcAddress(o_dll, "DllUnregisterServer");

	o_loaded = true;
	return true;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	static wchar_t s_PathBuffer[8192];

	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hinstDLL);

		g_thisModule = hinstDLL;

		// If this isn't the HITMAN3 executable then don't load the SDK.
		if (!GetModuleFileNameW(nullptr, s_PathBuffer, sizeof(s_PathBuffer) / sizeof(wchar_t)))
			return false;

		std::filesystem::path s_ModuleFilePath = s_PathBuffer;
		std::string s_ExecutableName = s_ModuleFilePath.filename().string();
		std::transform(s_ExecutableName.begin(), s_ExecutableName.end(), s_ExecutableName.begin(), [](unsigned char c) { return std::tolower(c); });

		// We return true here because if we return false then whatever app is loading this will crash.
		if (s_ExecutableName != "hitman3.exe")
			return true;

		g_ZHMModSDK = LoadLibraryA("ZHMModSDK.dll");

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

					SuspendAllThreadsButCurrent();

					while (FreeLibrary(g_ZHMModSDK));

					ResumeSuspendedThreads();
					
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

					SuspendAllThreadsButCurrent();
					
					while (FreeLibrary(g_ZHMModSDK));

					ResumeSuspendedThreads();

					g_ZHMModSDK = nullptr;
				}

				const auto s_UnloadedEvent = OpenEventA(EVENT_MODIFY_STATE, false, "GLOBAL_ZHMSDK_Unloaded_Signal");

				if (s_UnloadedEvent != nullptr)
				{
					SetEvent(s_UnloadedEvent);
					CloseHandle(s_UnloadedEvent);
				}

				g_ZHMModSDK = LoadLibraryA("ZHMModSDK.dll");

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
		if (o_dll)
		{
			FreeLibrary(o_dll);
		}

		if (g_ZHMModSDK != nullptr)
		{
			FreeLibrary(g_ZHMModSDK);
			g_ZHMModSDK = nullptr;
		}
	}

	return true;
}

extern "C" __declspec(dllexport) HRESULT DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, void* punkOuter)
{

	if (!DirectInput8Create_t)
		LoadProxiedDll();

	return DirectInput8Create_t(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}

extern "C" __declspec(dllexport) void APIENTRY DllCanUnloadNow() { DllCanUnloadNow_t(); }
extern "C" __declspec(dllexport) void APIENTRY DllGetClassObject() { DllGetClassObject_t(); }
extern "C" __declspec(dllexport) void APIENTRY DllRegisterServer() { DllRegisterServer_t(); }
extern "C" __declspec(dllexport) void APIENTRY DllUnregisterServer() { DllUnregisterServer_t(); }
extern "C" __declspec(dllexport) void APIENTRY GetdfDIJoystick() { GetdfDIJoystick_t(); }
