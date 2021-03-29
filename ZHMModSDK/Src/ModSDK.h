#pragma once

#include <memory>
#include <string>

#include "IModSDK.h"
#include "Hooks.h"

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
	void OnDrawUI(bool p_HasFocus);
	void OnDraw3D();
	void OnImGuiInit();
	void OnModLoaded(const std::string& p_Name, IPluginInterface* p_Mod);
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

private:
	DEFINE_DETOUR_WITH_CONTEXT(ModSDK, bool, Engine_Init, void* th, void* a2);

private:
	ModLoader* m_ModLoader= nullptr;

#if _DEBUG
	DebugConsole* m_DebugConsole = nullptr;
#endif

	uintptr_t m_ModuleBase;
	uint32_t m_SizeOfCode;
	uint32_t m_ImageSize;
	bool m_ImGuiInitialized = false;
};
