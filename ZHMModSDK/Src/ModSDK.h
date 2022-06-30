#pragma once

#include <unordered_set>
#include <memory>
#include <shared_mutex>
#include <string>

#include "IModSDK.h"
#include "Hooks.h"
#include "Glacier/ZEntity.h"

class IRenderer;
class IPluginInterface;
class ModLoader;
class DebugConsole;

class ModSDK : public IModSDK
{
private:
	static ModSDK* g_Instance;

public:
	static ModSDK* GetInstance();
	static void DestroyInstance();

public:
	ModSDK();
	~ModSDK();

	bool Startup();
	void ThreadedStartup();

	ModLoader* GetModLoader() const { return m_ModLoader; }

#if _DEBUG
	DebugConsole* GetDebugConsole() const { return m_DebugConsole; }
#endif

	uintptr_t GetModuleBase() const { return m_ModuleBase; }
	uint32_t GetSizeOfCode() const { return m_SizeOfCode; }
	uint32_t GetImageSize() const { return m_ImageSize; }

public:
	void OnDrawMenu();
	void OnDrawUI(bool p_HasFocus);
	void OnDraw3D(IRenderer* p_Renderer);
	void OnImGuiInit();
	void OnModLoaded(const std::string& p_Name, IPluginInterface* p_Mod, bool p_LiveLoad);
	void OnModUnloaded(const std::string& p_Name);

private:
	void OnEngineInit();
	
public:
	void RequestUIFocus() override;
	void ReleaseUIFocus() override;
	ImGuiContext* GetImGuiContext() override;
	ImGuiMemAllocFunc GetImGuiAlloc() override;
	ImGuiMemFreeFunc GetImGuiFree() override;
	void* GetImGuiAllocatorUserData() override;
	ImFont* GetImGuiLightFont() override;
	ImFont* GetImGuiRegularFont() override;
	ImFont* GetImGuiMediumFont() override;
	ImFont* GetImGuiBoldFont() override;
	ImFont* GetImGuiBlackFont() override;
	bool GetPinName(int32_t p_PinId, ZString& p_Name) override;

private:
	DEFINE_DETOUR_WITH_CONTEXT(ModSDK, bool, Engine_Init, void* th, void* a2);
	DEFINE_DETOUR_WITH_CONTEXT(ModSDK, void, ZEntityManager_ActivateEntity, ZEntityManager* th, ZEntityRef* entity, void* a3);
	DEFINE_DETOUR_WITH_CONTEXT(ModSDK, void, ZEntityManager_DeleteEntities, ZEntityManager* th, const TFixedArray<ZEntityRef>& entities, void* a3);

private:
	ModLoader* m_ModLoader = nullptr;
	std::unordered_set<ZEntityRef, ZEntityRefHasher> m_Entities;
	std::shared_mutex m_EntityMutex;

#if _DEBUG
	DebugConsole* m_DebugConsole = nullptr;
#endif

	uintptr_t m_ModuleBase;
	uint32_t m_SizeOfCode;
	uint32_t m_ImageSize;
	bool m_ImGuiInitialized = false;
};
