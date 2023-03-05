#include "ModSDK.h"

#include "Functions.h"
#include "ModLoader.h"
#include "Globals.h"
#include "HookImpl.h"
#include "Hooks.h"
#include "ini.h"
#include "Logging.h"
#include "IPluginInterface.h"
#include "PinRegistry.h"
#include "Util/ProcessUtils.h"

#include "Rendering/Renderers/DirectXTKRenderer.h"
#include "Rendering/Renderers/ImGuiRenderer.h"

#include "Rendering/D3D12Hooks.h"

#include "UI/Console.h"
#include "UI/MainMenu.h"
#include "UI/ModSelector.h"

#include "Glacier/ZModule.h"
#include "Glacier/ZScene.h"
#include "Glacier/ZGameUIManager.h"
#include "Glacier/ZSpatialEntity.h"
#include "Glacier/ZActor.h"
#include "D3DUtils.h"
#include "Glacier/ZRender.h"
#include "Rendering/Renderers/ImGuiImpl.h"
#include "Util/StringUtils.h"

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

    LoadConfiguration();

#if _DEBUG
    m_DebugConsole = std::make_shared<DebugConsole>();
    SetupLogging(spdlog::level::trace);
#else
    SetupLogging(spdlog::level::info);
#endif

    m_ModLoader = std::make_shared<ModLoader>();

    m_UIConsole = std::make_shared<UI::Console>();
    m_UIMainMenu = std::make_shared<UI::MainMenu>();
    m_UIModSelector = std::make_shared<UI::ModSelector>();

    m_DirectXTKRenderer = std::make_shared<Rendering::Renderers::DirectXTKRenderer>();
    m_ImguiRenderer = std::make_shared<Rendering::Renderers::ImGuiRenderer>();
    m_D3D12Hooks = std::make_shared<Rendering::D3D12Hooks>();

    HMODULE s_Module = GetModuleHandleA(nullptr);

    m_ModuleBase = reinterpret_cast<uintptr_t>(s_Module) + Util::ProcessUtils::GetBaseOfCode(s_Module);
    m_SizeOfCode = Util::ProcessUtils::GetSizeOfCode(s_Module);
    m_ImageSize = Util::ProcessUtils::GetSizeOfImage(s_Module);
}

ModSDK::~ModSDK()
{
    m_ModLoader.reset();

    HookRegistry::ClearDetoursWithContext(this);

    m_D3D12Hooks.reset();
    m_ImguiRenderer.reset();
    m_DirectXTKRenderer.reset();

    HookRegistry::DestroyHooks();
    Trampolines::ClearTrampolines();

#if _DEBUG
    FlushLoggers();
    ClearLoggers();

    m_DebugConsole.reset();
#endif
}

bool ModSDK::PatchCode(const char* p_Pattern, const char* p_Mask, void* p_NewCode, size_t p_CodeSize)
{
    if (!p_Pattern || !p_Mask || !p_NewCode || p_CodeSize == 0)
    {
        Logger::Error("Invalid parameters provided to PatchCode call.");
        return false;
    }

    const auto* s_Pattern = reinterpret_cast<const uint8_t*>(p_Pattern);
    const auto s_Target = Util::ProcessUtils::SearchPattern(ModSDK::GetInstance()->GetModuleBase(), ModSDK::GetInstance()->GetSizeOfCode(), s_Pattern, p_Mask);

    if (s_Target == 0)
    {
        Logger::Error("Could not find pattern in call to PatchCode. Game might have been updated.");
        return false;
    }

    auto* s_TargetPtr = reinterpret_cast<void*>(s_Target);

    Logger::Debug("Patching {} bytes of code at {} with new code from {}.", p_CodeSize, fmt::ptr(s_TargetPtr), p_NewCode);

    DWORD s_OldProtect;
    VirtualProtect(s_TargetPtr, p_CodeSize, PAGE_EXECUTE_READWRITE, &s_OldProtect);

    memcpy(s_TargetPtr, p_NewCode, p_CodeSize);

    VirtualProtect(s_TargetPtr, p_CodeSize, s_OldProtect, nullptr);

    return true;
}


void ModSDK::LoadConfiguration()
{
    char s_ExePathStr[MAX_PATH];
    auto s_PathSize = GetModuleFileNameA(nullptr, s_ExePathStr, MAX_PATH);

    if (s_PathSize == 0)
        return;

    std::filesystem::path s_ExePath(s_ExePathStr);
    auto s_ExeDir = s_ExePath.parent_path();

    const auto s_IniPath = absolute(s_ExeDir / "mods.ini");

    mINI::INIFile s_File(s_IniPath.string());
    mINI::INIStructure s_Ini;

    // now we can read the file
    s_File.read(s_Ini);

    for (auto& s_Mod : s_Ini)
    {
        // We are looking for the sdk entry.
        if (Util::StringUtils::ToLowerCase(s_Mod.first) != "sdk")
            continue;

        if (s_Mod.second.has("noui") && s_Mod.second.get("noui") == "true")
        {
            m_UiEnabled = false;
            MessageBoxA(nullptr, "WARNING: The mod SDK UI is currently disabled!\n\nIf you want to re-enable it, remove the 'noui = true' line from Retail/mods.ini and restart your game.", "Mod SDK Warning", MB_OK | MB_ICONWARNING);
        }
    }
}

bool ModSDK::Startup()
{
#if _DEBUG
    m_DebugConsole->StartRedirecting();
#endif

    m_ModLoader->Startup();

    // Notify all loaded mods that the engine has intialized once it has.
    Hooks::Engine_Init->AddDetour(this, &ModSDK::Engine_Init);

    m_D3D12Hooks->Startup();

    // Patch mutex creation to allow multiple instances.
    uint8_t s_NopBytes[84] = { 0x90 };
    memset(s_NopBytes, 0x90, 84);
    if (!PatchCode("\x4C\x8D\x05\x00\x00\x00\x00\xBA\x00\x00\x00\x00\x33\xC9\xFF\x15", "xxx????x????xxxx", s_NopBytes, 84))
    {
        Logger::Warn("Could not patch multi-instance detection. You will not be able to launch the game more than once.");
    }

    return true;
}

void ModSDK::ThreadedStartup()
{
    m_ModLoader->LockRead();

    for (const auto& s_Mod : m_ModLoader->GetLoadedMods())
    {
        s_Mod->SetupUI();
        s_Mod->Init();
    }

    m_ModLoader->UnlockRead();

    // If the engine is already initialized, inform the mods.
    if (Globals::Hitman5Module->IsEngineInitialized())
        OnEngineInit();
}

void ModSDK::OnDrawMenu()
{
    m_ModLoader->LockRead();

    for (auto& s_Mod : m_ModLoader->GetLoadedMods())
        s_Mod->OnDrawMenu();

    m_ModLoader->UnlockRead();
}

void ModSDK::OnDrawUI(bool p_HasFocus)
{
    m_UIConsole->Draw(p_HasFocus);
    m_UIMainMenu->Draw(p_HasFocus);
    m_UIModSelector->Draw(p_HasFocus);

    m_ModLoader->LockRead();

    for (auto& s_Mod : m_ModLoader->GetLoadedMods())
        s_Mod->OnDrawUI(p_HasFocus);

    m_ModLoader->UnlockRead();
}

void ModSDK::OnDraw3D()
{
    m_ModLoader->LockRead();

    for (auto& s_Mod : m_ModLoader->GetLoadedMods())
        s_Mod->OnDraw3D(m_DirectXTKRenderer.get());

    m_ModLoader->UnlockRead();

    /*m_EntityMutex.lock_shared();

    for (auto s_EntityRef : m_Entities)
    {
        auto* s_SpatialEntity = s_EntityRef.QueryInterface<ZSpatialEntity>();

        if (!s_SpatialEntity)
            continue;

        SMatrix s_Transform;
        Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

        float4 s_Min, s_Max;

        s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

        p_Renderer->DrawOBB3D(SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform, SVector4(1.f, 0.f, 0.f, 1.f));

        SVector2 s_ScreenPos;
        if (p_Renderer->WorldToScreen(SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.05f), s_ScreenPos))
            p_Renderer->DrawText2D(std::to_string((*s_EntityRef.m_pEntity)->m_nEntityId).c_str(), s_ScreenPos, SVector4(1.f, 0.f, 0.f, 1.f), 0.f, 0.5f);
    }

    m_EntityMutex.unlock_shared();*/
}

void ModSDK::OnModLoaded(const std::string& p_Name, IPluginInterface* p_Mod, bool p_LiveLoad)
{
    p_Mod->SetupUI();
    p_Mod->Init();

    if (p_LiveLoad && Globals::Hitman5Module->IsEngineInitialized())
        p_Mod->OnEngineInitialized();

    Logger::Info("Mod {} successfully loaded.", p_Name);
}

void ModSDK::OnModUnloaded(const std::string& p_Name)
{

}

void ModSDK::OnEngineInit()
{
    Logger::Debug("Engine was initialized.");

    if (m_UiEnabled)
    {
        m_DirectXTKRenderer->OnEngineInit();
        m_ImguiRenderer->OnEngineInit();
    }

    m_ModLoader->LockRead();

    for (auto& s_Mod : m_ModLoader->GetLoadedMods())
        s_Mod->OnEngineInitialized();

    m_ModLoader->UnlockRead();
}

static IDXGISwapChain* g_SwapChain = nullptr;
static ID3D12CommandQueue* g_CommandQueue = nullptr;

void ModSDK::OnPresent(IDXGISwapChain3* p_SwapChain)
{
    if (g_SwapChain == nullptr)
    {
        Logger::Debug("Setting swap chain to {}.", fmt::ptr(p_SwapChain));
        g_SwapChain = p_SwapChain;
    }

    if (g_SwapChain != p_SwapChain)
        return;

    m_DirectXTKRenderer->OnPresent(p_SwapChain);
    m_ImguiRenderer->OnPresent(p_SwapChain);
}

void ModSDK::PostPresent(IDXGISwapChain3* p_SwapChain, HRESULT p_PresentResult)
{
    if (g_SwapChain != p_SwapChain)
        return;

    m_ImguiRenderer->PostPresent(p_SwapChain, p_PresentResult);
    m_DirectXTKRenderer->PostPresent(p_SwapChain, p_PresentResult);
}

void ModSDK::SetCommandQueue(ID3D12CommandQueue* p_CommandQueue)
{
    if (!g_SwapChain)
        return;

    if (g_CommandQueue != nullptr)
        return;

    if (p_CommandQueue->GetDesc().Type != D3D12_COMMAND_LIST_TYPE_DIRECT)
        return;

    ScopedD3DRef<ID3D12Device> s_Device;

    if (g_SwapChain->GetDevice(REF_IID_PPV_ARGS(s_Device)) != S_OK)
        return;

    ScopedD3DRef<ID3D12Device> s_CommandQueueDevice;

    if (p_CommandQueue->GetDevice(REF_IID_PPV_ARGS(s_CommandQueueDevice)) != S_OK)
        return;

    if (s_Device.Ref != s_CommandQueueDevice.Ref)
        return;

    g_CommandQueue = p_CommandQueue;

    m_DirectXTKRenderer->SetCommandQueue(p_CommandQueue);
    m_ImguiRenderer->SetCommandQueue(p_CommandQueue);
}

void ModSDK::OnReset(IDXGISwapChain3* p_SwapChain)
{
    if (g_SwapChain != p_SwapChain)
        return;

    m_DirectXTKRenderer->OnReset();
    m_ImguiRenderer->OnReset();
}

void ModSDK::PostReset(IDXGISwapChain3* p_SwapChain)
{
    if (g_SwapChain != p_SwapChain)
        return;

    m_ImguiRenderer->PostReset();
    m_DirectXTKRenderer->PostReset();
}

void ModSDK::RequestUIFocus()
{
    if (!m_UiEnabled)
        return;

    m_ImguiRenderer->SetFocus(true);
}

void ModSDK::ReleaseUIFocus()
{
    if (!m_UiEnabled)
        return;

    m_ImguiRenderer->SetFocus(false);
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
    return m_ImguiRenderer->GetFontLight();
}

ImFont* ModSDK::GetImGuiRegularFont()
{
    return m_ImguiRenderer->GetFontRegular();
}

ImFont* ModSDK::GetImGuiMediumFont()
{
    return m_ImguiRenderer->GetFontMedium();
}

ImFont* ModSDK::GetImGuiBoldFont()
{
    return m_ImguiRenderer->GetFontBold();
}

ImFont* ModSDK::GetImGuiBlackFont()
{
    return m_ImguiRenderer->GetFontBlack();
}

bool ModSDK::GetPinName(int32_t p_PinId, ZString& p_Name)
{
    return TryGetPinName(p_PinId, p_Name);
}

bool ModSDK::WorldToScreen(const SVector3& p_WorldPos, SVector2& p_Out)
{
    return m_DirectXTKRenderer->WorldToScreen(p_WorldPos, p_Out);
}

bool ModSDK::ScreenToWorld(const SVector2& p_ScreenPos, SVector3& p_WorldPosOut, SVector3& p_DirectionOut)
{
    return m_DirectXTKRenderer->ScreenToWorld(p_ScreenPos, p_WorldPosOut, p_DirectionOut);
}

void ModSDK::ImGuiGameRenderTarget(ZRenderDestination* p_RT, const ImVec2& p_Size)
{
    if (!p_RT)
        return;

    auto s_Size = p_Size;

    if (s_Size.x == 0 && s_Size.y == 0)
    {
        const auto s_Desc = p_RT->m_pTexture2D->m_pResource->GetDesc();
        s_Size = { static_cast<float>(s_Desc.Width), static_cast<float>(s_Desc.Height) };
    }

    const auto s_HandleIncrementSize = Globals::RenderManager->m_pDevice->m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_GPU_DESCRIPTOR_HANDLE s_Handle {};
    s_Handle.ptr = Globals::RenderManager->m_pDevice->m_pFrameHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart().ptr + (p_RT->m_pSRV->m_nHeapDescriptorIndex * s_HandleIncrementSize);

    ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_SetGameDescriptorHeap, nullptr);
    ImGui::Image(reinterpret_cast<ImTextureID>(s_Handle.ptr), s_Size);
    ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_ResetDescriptorHeap, nullptr);
}


DECLARE_DETOUR_WITH_CONTEXT(ModSDK, bool, Engine_Init, void* th, void* a2)
{
    auto s_Result = p_Hook->CallOriginal(th, a2);

    OnEngineInit();

    return HookResult<bool>(HookAction::Return(), s_Result);
}
