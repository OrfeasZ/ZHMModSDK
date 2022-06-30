#include "ModSDK.h"

#include "Functions.h"
#include "ModLoader.h"
#include "Globals.h"
#include "HookImpl.h"
#include "Hooks.h"
#include "Logging.h"
#include "IPluginInterface.h"
#include "PinRegistry.h"
#include "Util/ProcessUtils.h"
#include "Rendering/D3D12Renderer.h"
#include "Rendering/Renderers/ImGuiRenderer.h"
#include "Glacier/ZModule.h"
#include "Glacier/ZScene.h"
#include "Glacier/ZGameUIManager.h"
#include "Glacier/ZSpatialEntity.h"

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
			Sleep(1000);
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

	// Install hooks so we can keep track of entities.
	Hooks::ZEntityManager_ActivateEntity->AddDetour(this, &ModSDK::ZEntityManager_ActivateEntity);
	Hooks::ZEntityManager_DeleteEntities->AddDetour(this, &ModSDK::ZEntityManager_DeleteEntities);

	return true;
}

void ModSDK::ThreadedStartup()
{
	Rendering::D3D12Renderer::Init();

	for (auto& s_Mod : m_ModLoader->GetLoadedMods())
		s_Mod->Init();

	// If the engine is already initialized, inform the mods.
	if (Globals::Hitman5Module->IsEngineInitialized())
		OnEngineInit();
}

void ModSDK::OnDrawMenu()
{
	for (auto& s_Mod : m_ModLoader->GetLoadedMods())
		s_Mod->OnDrawMenu();
}

void ModSDK::OnDrawUI(bool p_HasFocus)
{
	for (auto& s_Mod : m_ModLoader->GetLoadedMods())
		s_Mod->OnDrawUI(p_HasFocus);
}

void ModSDK::OnDraw3D(IRenderer* p_Renderer)
{
	for (auto& s_Mod : m_ModLoader->GetLoadedMods())
		s_Mod->OnDraw3D(p_Renderer);

	if (m_DoDrawBboxes)
	{
		m_EntityMutex.lock_shared();

		for (auto s_Ref : m_Entities)
		{
			auto* s_SpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

			if (!s_SpatialEntity)
				continue;

			SMatrix s_Transform;
			Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

			float4 s_Min, s_Max;

			s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

			p_Renderer->DrawOBB3D(SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform, SVector4(1.f, 0.f, 0.f, 1.f));

			SVector2 s_ScreenPos;
			if (p_Renderer->WorldToScreen(SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.05f), s_ScreenPos))
				p_Renderer->DrawText2D(std::to_string((*s_Ref.m_pEntity)->m_nEntityId).c_str(), s_ScreenPos, SVector4(1.f, 0.f, 0.f, 1.f), 0.f, 0.5f);
		}

		m_EntityMutex.unlock_shared();
	}
}

void ModSDK::OnImGuiInit()
{
	m_ImGuiInitialized = true;

	for (auto& s_Mod : m_ModLoader->GetLoadedMods())
		s_Mod->SetupUI();
}

void ModSDK::OnModLoaded(const std::string& p_Name, IPluginInterface* p_Mod, bool p_LiveLoad)
{
	if (m_ImGuiInitialized)
		p_Mod->SetupUI();

	p_Mod->PreInit();

	if (p_LiveLoad && Globals::Hitman5Module->IsEngineInitialized())
	{
		p_Mod->Init();
		p_Mod->OnEngineInitialized();
	}

	if (m_ImGuiInitialized)
		p_Mod->SetupUI();
}

void ModSDK::OnModUnloaded(const std::string& p_Name)
{
	
}

void ModSDK::ToggleBboxDrawing()
{
	m_DoDrawBboxes = !m_DoDrawBboxes;
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

bool ModSDK::GetPinName(int32_t p_PinId, ZString& p_Name)
{
	return TryGetPinName(p_PinId, p_Name);
}

DECLARE_DETOUR_WITH_CONTEXT(ModSDK, bool, Engine_Init, void* th, void* a2)
{
	auto s_Result = p_Hook->CallOriginal(th, a2);

	OnEngineInit();

	return HookResult<bool>(HookAction::Return(), s_Result);
}

DECLARE_DETOUR_WITH_CONTEXT(ModSDK, void, ZEntityManager_ActivateEntity, ZEntityManager* th, ZEntityRef* entity, void* a3)
{
	const auto& s_Interfaces = *(*entity->m_pEntity)->m_pInterfaces;
	Logger::Trace("Activating entity of type '{}' with id '{:x}' at {}.", s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName, (*entity->m_pEntity)->m_nEntityId, fmt::ptr(entity->m_pEntity));

	m_EntityMutex.lock();
	m_Entities.insert(*entity);
	m_EntityMutex.unlock();

	return HookResult<void>(HookAction::Continue());
}

DECLARE_DETOUR_WITH_CONTEXT(ModSDK, void, ZEntityManager_DeleteEntities, ZEntityManager* th, const TFixedArray<ZEntityRef>& entities, void* a3)
{
	m_EntityMutex.lock();

	for (size_t i = 0; i < entities.size(); ++i)
	{
		const auto& s_Interfaces = *(*entities[i].m_pEntity)->m_pInterfaces;
		Logger::Trace("Deleting entity of type '{}' with id '{:x}' at {}.", s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName, (*entities[i].m_pEntity)->m_nEntityId, fmt::ptr(entities[i].m_pEntity));

		m_Entities.erase(entities[i]);
	}

	m_EntityMutex.unlock();

	return HookResult<void>(HookAction::Continue());
}
