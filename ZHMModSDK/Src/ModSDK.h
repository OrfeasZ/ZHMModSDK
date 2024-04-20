#pragma once

#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_set>

#include "IModSDK.h"
#include "Hooks.h"
#include "Glacier/ZEntity.h"

namespace Rendering
{
    class D3D12Hooks;
	class D3D12SwapChain;
}

namespace Rendering::Renderers
{
    class ImGuiRenderer;
    class DirectXTKRenderer;
}

namespace UI
{
    class ModSelector;
    class MainMenu;
    class Console;
}

class IRenderer;
class IPluginInterface;
class ModLoader;
class DebugConsole;
struct IDXGISwapChain3;

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

    uintptr_t GetModuleBase() const { return m_ModuleBase; }
    uint32_t GetSizeOfCode() const { return m_SizeOfCode; }
    uint32_t GetImageSize() const { return m_ImageSize; }

private:
    void LoadConfiguration();
	std::pair<uint32_t, std::string> RequestLatestVersion();
	void ShowVersionNotice(const std::wstring& p_Version);

public:
    void OnEngineInit();
    void OnModLoaded(const std::string& p_Name, IPluginInterface* p_Mod, bool p_LiveLoad);
    void OnModUnloaded(const std::string& p_Name);
    void OnDrawUI(bool p_HasFocus);
    void OnDraw3D();
    void OnDrawMenu();

public:
	void SetSwapChain(Rendering::D3D12SwapChain* p_SwapChain);
    void OnPresent(IDXGISwapChain3* p_SwapChain);
    void PostPresent(IDXGISwapChain3* p_SwapChain, HRESULT p_PresentResult);
    void SetCommandQueue(ID3D12CommandQueue* p_CommandQueue);
    void OnReset(IDXGISwapChain3* p_SwapChain);
    void PostReset(IDXGISwapChain3* p_SwapChain);

public:
    std::shared_ptr<ModLoader> GetModLoader() const { return m_ModLoader; }

#if _DEBUG
    std::shared_ptr<DebugConsole> GetDebugConsole() const { return m_DebugConsole; }
#endif

    std::shared_ptr<Rendering::Renderers::DirectXTKRenderer> GetDirectXTKRenderer() const { return m_DirectXTKRenderer; }
    std::shared_ptr<Rendering::Renderers::ImGuiRenderer> GetImguiRenderer() const { return m_ImguiRenderer; }
    std::shared_ptr<Rendering::D3D12Hooks> GetD3D12Hooks() const { return m_D3D12Hooks; }

    std::shared_ptr<UI::Console> GetUIConsole() const { return m_UIConsole; }
    std::shared_ptr<UI::MainMenu> GetUIMainMenu() const { return m_UIMainMenu; }
    std::shared_ptr<UI::ModSelector> GetUIModSelector() const { return m_UIModSelector; }

	uint8_t GetConsoleScanCode() const { return m_ConsoleScanCode; }

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
    bool WorldToScreen(const SVector3& p_WorldPos, SVector2& p_Out) override;
    bool ScreenToWorld(const SVector2& p_ScreenPos, SVector3& p_WorldPosOut, SVector3& p_DirectionOut) override;
    bool PatchCode(const char* p_Pattern, const char* p_Mask, void* p_NewCode, size_t p_CodeSize, ptrdiff_t p_Offset) override;
    void ImGuiGameRenderTarget(ZRenderDestination* p_RT, const ImVec2& p_Size = { 0, 0 }) override;

	// Plugin settings
	void SetPluginSetting(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, const ZString& p_Value) override;
	void SetPluginSettingInt(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, int64_t p_Value) override;
	void SetPluginSettingUInt(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, uint64_t p_Value) override;
	void SetPluginSettingDouble(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, double p_Value) override;
	void SetPluginSettingBool(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, bool p_Value) override;
	ZString GetPluginSetting(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, const ZString& p_DefaultValue) override;
	int64_t GetPluginSettingInt(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, int64_t p_DefaultValue) override;
	uint64_t GetPluginSettingUInt(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, uint64_t p_DefaultValue) override;
	double GetPluginSettingDouble(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, double p_DefaultValue) override;
	bool GetPluginSettingBool(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, bool p_DefaultValue) override;
	bool HasPluginSetting(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name) override;
	void RemovePluginSetting(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name) override;
	void ReloadPluginSettings(IPluginInterface* p_Plugin) override;

private:
    DECLARE_DETOUR_WITH_CONTEXT(ModSDK, bool, Engine_Init, void* th, void* a2);
    DECLARE_DETOUR_WITH_CONTEXT(ModSDK, EOS_PlatformHandle*, EOS_Platform_Create, EOS_Platform_Options* Options);

private:
    bool m_UiEnabled = true;
	uint8_t m_ConsoleScanCode = 0x29; // Grave / Tilde key
    uintptr_t m_ModuleBase;
    uint32_t m_SizeOfCode;
    uint32_t m_ImageSize;

    std::shared_ptr<ModLoader> m_ModLoader {};

#if _DEBUG
    std::shared_ptr<DebugConsole> m_DebugConsole {};
#endif

    std::shared_ptr<Rendering::Renderers::DirectXTKRenderer> m_DirectXTKRenderer {};
    std::shared_ptr<Rendering::Renderers::ImGuiRenderer> m_ImguiRenderer {};
    std::shared_ptr<Rendering::D3D12Hooks> m_D3D12Hooks {};

    std::shared_ptr<UI::Console> m_UIConsole {};
    std::shared_ptr<UI::MainMenu> m_UIMainMenu {};
    std::shared_ptr<UI::ModSelector> m_UIModSelector {};
};
