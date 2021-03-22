#include "ModSDK.h"

#include "ModLoader.h"
#include "Globals.h"
#include "HookImpl.h"
#include "Hooks.h"
#include "Logging.h"
#include "Rendering/ImguiRenderer.h"
#include "Util/ProcessUtils.h"

#include "Glacier/ZString.h"

#if _DEBUG
#include "DebugConsole.h"
#endif

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

	Rendering::ImguiRenderer::GetInstance()->Shutdown();
	
#if _DEBUG
	delete m_DebugConsole;
#endif

	Trampolines::ClearTrampolines();
}

bool ModSDK::Startup()
{
	Util::ProcessUtils::ResumeSuspendedThreads();

#if _DEBUG
	m_DebugConsole->StartRedirecting();
#endif

	Rendering::ImguiRenderer::GetInstance()->Init();
	
	m_ModLoader->Startup();

	return true;
}
