#pragma once

#include <ini.h>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_set>

#include "IModSDK.h"
#include "Hooks.h"
#include "Glacier/ZEntity.h"

struct ResourceMem;

namespace Rendering {
    class D3D12Hooks;
    class D3D12SwapChain;
}

namespace Rendering::Renderers {
    class ImGuiRenderer;
    class DirectXTKRenderer;
}

namespace UI {
    class ModSelector;
    class MainMenu;
    class Console;
}

class IRenderer;
class IPluginInterface;
class ModLoader;
class DebugConsole;
struct IDXGISwapChain3;

class ModSDK : public IModSDK {
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
    static std::pair<uint32_t, std::string> RequestLatestVersion();
    static void ShowVersionNotice(const std::string& p_Version);
    void SkipVersionUpdate(const std::string& p_Version);
    bool CheckForUpdates() const;

public:
    void OnModLoaded(const std::string& p_Name, IPluginInterface* p_Mod, bool p_LiveLoad);
    void OnModUnloading(const std::string& p_Name, IPluginInterface* p_Mod);
    void OnModUnloaded(const std::string& p_Name);
    void OnEngineInit();
    void OnDrawUI(bool p_HasFocus);
    void OnDraw3D() const;
    void OnDepthDraw3D() const;
    void OnDrawMenu() const;

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

    std::shared_ptr<Rendering::Renderers::DirectXTKRenderer> GetDirectXTKRenderer() const {
        return m_DirectXTKRenderer;
    }

    std::shared_ptr<Rendering::Renderers::ImGuiRenderer> GetImguiRenderer() const { return m_ImguiRenderer; }
    std::shared_ptr<Rendering::D3D12Hooks> GetD3D12Hooks() const { return m_D3D12Hooks; }

    std::shared_ptr<UI::Console> GetUIConsole() const { return m_UIConsole; }
    std::shared_ptr<UI::MainMenu> GetUIMainMenu() const { return m_UIMainMenu; }
    std::shared_ptr<UI::ModSelector> GetUIModSelector() const { return m_UIModSelector; }

    uint8_t GetConsoleScanCode() const { return m_ConsoleScanCode; }
    uint8_t GetUiToggleScanCode() const { return m_UiToggleScanCode; }
    bool HasShownUiToggleWarning() const { return m_HasShownUiToggleWarning; }

    void SetHasShownUiToggleWarning();

public:
    #pragma region IModSDK implementation

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
    ImPlotContext* GetImPlotContext() override;
    bool GetPinName(int32_t p_PinId, ZString& p_Name) override;
    bool WorldToScreen(const SVector3& p_WorldPos, SVector2& p_Out) override;
    bool ScreenToWorld(const SVector2& p_ScreenPos, SVector3& p_WorldPosOut, SVector3& p_DirectionOut) override;

    bool PatchCode(
        const char* p_Pattern, const char* p_Mask, void* p_NewCode, size_t p_CodeSize, ptrdiff_t p_Offset
    ) override;

    bool PatchCodeStoreOriginal(
        const char* p_Pattern, const char* p_Mask, void* p_NewCode, size_t p_CodeSize, ptrdiff_t p_Offset,
        void* p_OriginalCode
    ) override;

    void ImGuiGameRenderTarget(ZRenderDestination* p_RT, const ImVec2& p_Size = {0, 0}) override;

    // Plugin settings
    void SetPluginSetting(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, const ZString& p_Value
    ) override;

    void SetPluginSettingInt(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, int64_t p_Value
    ) override;

    void SetPluginSettingUInt(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, uint64_t p_Value
    ) override;

    void SetPluginSettingDouble(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, double p_Value
    ) override;

    void SetPluginSettingBool(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, bool p_Value
    ) override;

    ZString GetPluginSetting(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, const ZString& p_DefaultValue
    ) override;

    int64_t GetPluginSettingInt(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, int64_t p_DefaultValue
    ) override;

    uint64_t GetPluginSettingUInt(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, uint64_t p_DefaultValue
    ) override;

    double GetPluginSettingDouble(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, double p_DefaultValue
    ) override;

    bool GetPluginSettingBool(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, bool p_DefaultValue
    ) override;

    bool HasPluginSetting(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name) override;
    void RemovePluginSetting(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name) override;
    void ReloadPluginSettings(IPluginInterface* p_Plugin) override;

    TEntityRef<ZHitman5> GetLocalPlayer() override;

    bool CreateDDSTextureFromMemory(
        const void* p_Data,
        size_t p_DataSize,
        ScopedD3DRef<ID3D12Resource>& p_OutTexture,
        ImGuiTexture& p_OutImGuiTexture
    ) override;

    bool CreateDDSTextureFromFile(
        const std::string& p_Path,
        ScopedD3DRef<ID3D12Resource>& p_OutTexture,
        ImGuiTexture& p_OutImGuiTexture
    ) override;

    bool CreateWICTextureFromMemory(
        const void* p_Data,
        size_t p_DataSize,
        ScopedD3DRef<ID3D12Resource>& p_OutTexture,
        ImGuiTexture& p_OutImGuiTexture
    ) override;

    bool CreateWICTextureFromFile(
        const std::string& p_Path,
        ScopedD3DRef<ID3D12Resource>& p_OutTexture,
        ImGuiTexture& p_OutImGuiTexture
    ) override;

    #pragma endregion

    void AllocateZString(ZString* p_Target, const char* p_Str, uint32_t p_Size);

private:
    #pragma region Detours

    DECLARE_DETOUR_WITH_CONTEXT(ModSDK, bool, Engine_Init, void* th, void* a2);
    DECLARE_DETOUR_WITH_CONTEXT(ModSDK, EOS_PlatformHandle*, EOS_Platform_Create, EOS_Platform_Options* Options);

    DECLARE_DETOUR_WITH_CONTEXT(ModSDK, bool, OnLoadScene, ZEntitySceneContext* th, SSceneInitParameters& p_Parameters);
    DECLARE_DETOUR_WITH_CONTEXT(ModSDK, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene);

    DECLARE_DETOUR_WITH_CONTEXT(
        ModSDK, void, DrawScaleform, ZRenderContext* ctx, ZRenderTargetView** rtv, uint32_t a3,
        ZRenderDepthStencilView** dsv, uint32_t a5, bool bCaptureOnly
    );

    DECLARE_DETOUR_WITH_CONTEXT(
        ModSDK, void, ZUserChannelContractsProxyBase_GetForPlay2, const ZString& id, const ZString& locationId,
        const ZDynamicObject& extraGameChangedIds, int difficulty,
        const std::function<void(const ZDynamicObject&)>& onOk, const std::function<void(int)>& onError,
        ZAsyncContext* ctx, const SHttpRequestBehavior& behavior
    );

    DECLARE_DETOUR_WITH_CONTEXT(
        ModSDK, void, ZLevelManager_SetGameState, ZLevelManager* th, ZLevelManager::EGameState state
    );
    DECLARE_DETOUR_WITH_CONTEXT(
        ModSDK, void, ZEntitySceneContext_SetLoadingStage, ZEntitySceneContext* th, ESceneLoadingStage stage
    );

    DECLARE_DETOUR_WITH_CONTEXT(
        ModSDK, void, Scaleform_GFx_AS3_MovieRoot_Output,
        Scaleform::GFx::AS3::MovieRoot* th,
        Scaleform::GFx::AS3::FlashUI::OutputMessageType type,
        const char* msg
    );

    #pragma endregion

    bool PatchCodeInternal(
        const char* p_Pattern, const char* p_Mask, void* p_NewCode, size_t p_CodeSize, ptrdiff_t p_Offset,
        void* p_OriginalCode
    );

    void UpdateSdkIni(std::function<void(mINI::INIMap<std::string>&)> p_Callback);

    #pragma region ResourceLoading

public:
    bool LoadQnEntity(
        const ZString& p_Json,
        TResourcePtr<ZTemplateEntityBlueprintFactory>& p_BlueprintFactoryOut,
        TResourcePtr<ZTemplateEntityFactory>& p_FactoryOut
    ) override;

    bool IsChunkMounted(uint32_t p_ChunkIndex) override;
    void MountChunk(uint32_t p_ChunkIndex) override;
    void UnmountChunk(uint32_t p_ChunkIndex, bool p_RemountChunksBelow) override;
    const TArray<uint32_t>& GetChunkIndicesForRuntimeResourceId(const ZRuntimeResourceID& id) override;

private:
    std::tuple<ZResourceIndex, ZRuntimeResourceID> LoadResourceFromBIN1(
        ResourceMem* p_ResourceMem, std::string_view p_MetaJson, std::function<void(ZResourcePending*)> p_Install
    );

    void LoadResourceChunkMap();

    #pragma endregion

private:
    static const char* GameStateToString(ZLevelManager::EGameState p_GameState);
    static const char* SceneLoadingStageToString(ESceneLoadingStage p_SceneLoadingStage);

private:
    bool m_UiEnabled = true;
    uint8_t m_ConsoleScanCode = 0x29; // Grave / Tilde key
    uint8_t m_UiToggleScanCode = 0x57; // F11
    bool m_HasShownUiToggleWarning = false;
    bool m_ForceLoad = false;
    std::optional<bool> m_EnableSentry;
    uintptr_t m_ModuleBase;
    uint32_t m_SizeOfCode;
    uint32_t m_ImageSize;
    std::string m_IgnoredVersion;
    std::string m_AutoLoadScene;
    bool m_DisableUpdateCheck = false;
    float m_LoadedModsUIScrollOffset = 0;
    bool m_IsGameStateLoggingEnabled = false;
    bool m_IsSceneLoadingLoggingEnabled = false;
    bool m_IsScaleformLoggingEnabled = false;

    std::unordered_map<ZRuntimeResourceID, TArray<uint32_t>> m_ResourceIdToChunkMap;

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