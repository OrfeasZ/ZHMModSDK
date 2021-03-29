#include "ModSDK.h"

#include "ModLoader.h"
#include "Globals.h"
#include "HookImpl.h"
#include "Hooks.h"
#include "Logging.h"
#include "IPluginInterface.h"
#include "Util/ProcessUtils.h"
#include "Rendering/D3D12Renderer.h"
#include "Rendering/Renderers/ImGuiRenderer.h"
#include "Glacier/ZModule.h"
#include "Glacier/ZScene.h"
#include "Glacier/ZGameUIManager.h"

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
	if (g_Instance == nullptr)
		return;

	// We request to pause the game as that increases our chances of succeeding at unloading
	// all our hooks, since less of them will be called.
	if (Globals::GameUIManager->m_pGameUIManagerEntity.m_pInterfaceRef)
		Hooks::ZGameUIManagerEntity_TryOpenMenu->Call(Globals::GameUIManager->m_pGameUIManagerEntity.m_pInterfaceRef, EGameUIMenu::eUIMenu_PauseMenu, true);

	// We do this in a different thread so the game has time to pause.
	auto* s_ExitThread = CreateThread(nullptr, 0, [](LPVOID) -> DWORD
		{
			Sleep(500);
			delete g_Instance;
			g_Instance = nullptr;
			return 0;
		}, nullptr, 0, nullptr);

	WaitForSingleObject(s_ExitThread, INFINITE);
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
	m_ImGuiInitialized = false;

	Rendering::D3D12Renderer::Shutdown();

	delete m_ModLoader;

	HookRegistry::DestroyHooks();
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

	m_ModLoader->Startup();
	
	// Notify all loaded mods that the engine has intialized once it has.
	Hooks::Engine_Init->AddDetour(this, &ModSDK::Engine_Init);

	Hooks::ZGameLoopManager_RequestPause->AddDetour(this, [](void*, auto p_Hook, ZGameLoopManager* th, const ZString& a2)
		{
			return HookResult<void>(HookAction::Continue());
		});
	
	return true;
}

void ModSDK::ThreadedStartup()
{
	Rendering::D3D12Renderer::Init();

	for (auto& s_Mod : m_ModLoader->GetLoadedMods())
		s_Mod->Init();

	// If the engine is already initialized, inform the mods.
	// TODO: This isn't a good check. Fix it.
	if (*Globals::Hitman5Module != nullptr &&
		(*Globals::Hitman5Module)->m_pEntitySceneContext != nullptr &&
		(*Globals::Hitman5Module)->m_pEntitySceneContext->m_sceneData.m_sceneName.size() > 0)
	{
		OnEngineInit();
	}
}

void ModSDK::OnDrawUI(bool p_HasFocus)
{
	for (auto& s_Mod : m_ModLoader->GetLoadedMods())
		s_Mod->OnDrawUI(p_HasFocus);
}

void ModSDK::OnDraw3D()
{
	for (auto& s_Mod : m_ModLoader->GetLoadedMods())
		s_Mod->OnDraw3D();
}

void ModSDK::OnImGuiInit()
{
	m_ImGuiInitialized = true;

	for (auto& s_Mod : m_ModLoader->GetLoadedMods())
		s_Mod->SetupUI();
}

void ModSDK::OnModLoaded(const std::string& p_Name, IPluginInterface* p_Mod)
{
	if (m_ImGuiInitialized)
		p_Mod->SetupUI();

	p_Mod->PreInit();
}

void ModSDK::OnModUnloaded(const std::string& p_Name)
{
	
}

void ModSDK::OnEngineInit()
{
	Logger::Debug("Engine was initialized.");

	Rendering::D3D12Renderer::OnEngineInit();

	for (auto& s_Mod : m_ModLoader->GetLoadedMods())
		s_Mod->OnEngineInitialized();
}

void ModSDK::RequestUIFocus()
{
	Rendering::Renderers::ImGuiRenderer::SetFocus(true);
}

void ModSDK::ReleaseUIFocus()
{
	Rendering::Renderers::ImGuiRenderer::SetFocus(false);
}

ImGuiContext* ModSDK::GetImGuiContext()
{
	return ImGui::GetCurrentContext();
}

ImGuiMemAllocFunc ModSDK::GetImGuiAlloc()
{
	ImGuiMemAllocFunc s_AllocFunc;
	ImGuiMemFreeFunc s_FreeFunc;
	void* s_UserData;
	ImGui::GetAllocatorFunctions(&s_AllocFunc, &s_FreeFunc, &s_UserData);
	
	return s_AllocFunc;
}

ImGuiMemFreeFunc ModSDK::GetImGuiFree()
{
	ImGuiMemAllocFunc s_AllocFunc;
	ImGuiMemFreeFunc s_FreeFunc;
	void* s_UserData;
	ImGui::GetAllocatorFunctions(&s_AllocFunc, &s_FreeFunc, &s_UserData);

	return s_FreeFunc;
}

void* ModSDK::GetImGuiAllocatorUserData()
{
	ImGuiMemAllocFunc s_AllocFunc;
	ImGuiMemFreeFunc s_FreeFunc;
	void* s_UserData;
	ImGui::GetAllocatorFunctions(&s_AllocFunc, &s_FreeFunc, &s_UserData);

	return s_UserData;
}

ImFont* ModSDK::GetImGuiLightFont()
{
	return Rendering::Renderers::ImGuiRenderer::GetFontLight();
}

ImFont* ModSDK::GetImGuiRegularFont()
{
	return Rendering::Renderers::ImGuiRenderer::GetFontRegular();
}

ImFont* ModSDK::GetImGuiMediumFont()
{
	return Rendering::Renderers::ImGuiRenderer::GetFontMedium();
}

ImFont* ModSDK::GetImGuiBoldFont()
{
	return Rendering::Renderers::ImGuiRenderer::GetFontBold();
}

ImFont* ModSDK::GetImGuiBlackFont()
{
	return Rendering::Renderers::ImGuiRenderer::GetFontBlack();
}

DECLARE_DETOUR_WITH_CONTEXT(ModSDK, bool, Engine_Init, void* th, void* a2)
{
	auto s_Result = p_Hook->CallOriginal(th, a2);

	OnEngineInit();

	return HookResult<bool>(HookAction::Return {}, s_Result);
}
