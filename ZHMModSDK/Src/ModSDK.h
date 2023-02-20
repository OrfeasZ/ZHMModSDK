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

public:
    void OnEngineInit();
    void OnModLoaded(const std::string& p_Name, IPluginInterface* p_Mod, bool p_LiveLoad);
    void OnModUnloaded(const std::string& p_Name);
    void OnDrawUI(bool p_HasFocus);
    void OnDraw3D();
    void OnDrawMenu();

public:
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
    bool PatchCode(const char* p_Pattern, const char* p_Mask, void* p_NewCode, size_t p_CodeSize) override;
    void ImGuiGameRenderTarget(ZRenderDestination* p_RT, const ImVec2& p_Size = { 0, 0 }) override;

private:
    DEFINE_DETOUR_WITH_CONTEXT(ModSDK, bool, Engine_Init, void* th, void* a2);

private:
    bool m_UiEnabled = true;
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
