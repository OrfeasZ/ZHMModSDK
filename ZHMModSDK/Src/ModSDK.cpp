#include "ModSDK.h"

#include "ModLoader.h"
#include "Globals.h"
#include "HookImpl.h"
#include "Hooks.h"
#include "Logging.h"
#include "Util/ProcessUtils.h"
#include "Glacier/ZString.h"
#include "Rendering/D3D12Renderer.h"

#if _DEBUG
#include "DebugConsole.h"
#endif

extern void SetupLogging(spdlog::level::level_enum p_LogLevel);
extern void FlushLoggers();
extern void ClearLoggers();

ZHMSDK_API IModSDK* SDK()
{
	return ModSDK::GetInstance();
}

ModSDK* ModSDK::g_Instance = nullptr;

ModSDK* ModSDK::GetInstance()
{
	if (g_Instance == nullptr)
		g_Instance = new ModSDK();

	return g_Instance;
}

void ModSDK::DestroyInstance()
{
	delete g_Instance;
	g_Instance = nullptr;
}

ModSDK::ModSDK()
{
	g_Instance = this;

#if _DEBUG
	m_DebugConsole = new DebugConsole();
	SetupLogging(spdlog::level::trace);
#else
	SetupLogging(spdlog::level::info);
#endif

	m_ModLoader = new ModLoader();

	HMODULE s_Module = GetModuleHandleA(nullptr);

	m_ModuleBase = reinterpret_cast<uintptr_t>(s_Module) + Util::ProcessUtils::GetBaseOfCode(s_Module);
	m_SizeOfCode = Util::ProcessUtils::GetSizeOfCode(s_Module);
	m_ImageSize = Util::ProcessUtils::GetSizeOfImage(s_Module);
}

ModSDK::~ModSDK()
{
	delete m_ModLoader;

	Rendering::D3D12Renderer::Shutdown();

	HookRegistry::ClearAllDetours();

	Trampolines::ClearTrampolines();

#if _DEBUG
	FlushLoggers();
	ClearLoggers();

	delete m_DebugConsole;
#endif
}

bool ModSDK::Startup()
{
	Util::ProcessUtils::ResumeSuspendedThreads();

#if _DEBUG
	m_DebugConsole->StartRedirecting();
#endif

	Rendering::D3D12Renderer::Init();

	m_ModLoader->Startup();

	Hooks::ZApplicationEngineWin32_OnDebugInfo->AddDetour(this, [](void*, auto p_Hook, ZApplicationEngineWin32*, const ZString& p_Info, const ZString& p_Details)
		{
			//Logger::Debug("Debug info '{}': {}", p_Info.c_str(), p_Details.c_str());

			return HookResult<void>(HookAction::Continue());
		});

	return true;
}
