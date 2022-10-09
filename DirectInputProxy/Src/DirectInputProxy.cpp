#include <thread>
#include <unordered_set>
#include <Windows.h>
#include <TlHelp32.h>
#include <filesystem>

static HMODULE g_OriginalDirectInput = nullptr;
static HMODULE g_ZHMModSDK = nullptr;

typedef HRESULT (__stdcall* DirectInput8Create_t)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
static DirectInput8Create_t o_DirectInput8Create = nullptr;

static std::unordered_set<HANDLE>* g_SuspendedThreads = nullptr;

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

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	static wchar_t s_PathBuffer[8192];

	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		if (!GetSystemDirectoryW(s_PathBuffer, sizeof(s_PathBuffer) / sizeof(wchar_t)))
			return false;

		std::filesystem::path s_Dinput8Path = s_PathBuffer;
		s_Dinput8Path += "/dinput8.dll";

		g_OriginalDirectInput = LoadLibraryW(canonical(s_Dinput8Path).c_str());

		if (g_OriginalDirectInput == nullptr)
			return false;

		o_DirectInput8Create = reinterpret_cast<DirectInput8Create_t>(GetProcAddress(g_OriginalDirectInput, "DirectInput8Create"));

		if (o_DirectInput8Create == nullptr)
			return false;

		// If this isn't the HITMAN3 executable then don't load the SDK.
		if (!GetModuleFileNameW(nullptr, s_PathBuffer, sizeof(s_PathBuffer) / sizeof(wchar_t)))
			return false;

		std::filesystem::path s_ModuleFilePath = s_PathBuffer;
		std::string s_ExecutableName = s_ModuleFilePath.filename().string();
		std::transform(s_ExecutableName.begin(), s_ExecutableName.end(), s_ExecutableName.begin(), [](unsigned char c) { return std::tolower(c); });

		// We return true here because if we return false then whatever app is loading this will crash.
		if (s_ExecutableName != "hitman3.exe")
			return true;

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
