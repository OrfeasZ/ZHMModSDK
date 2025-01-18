#include "ModSDK.h"

#include "Functions.h"
#include "ModLoader.h"
#include "Globals.h"
#include "HookImpl.h"
#include "Hooks.h"
#include "Events.h"
#include "ini.h"
#include "Logging.h"
#include "IPluginInterface.h"
#include "PinRegistry.h"
#include "Util/ProcessUtils.h"
#include "Util/HashingUtils.h"
#include "Util/StringUtils.h"

#include "Rendering/Renderers/DirectXTKRenderer.h"
#include "Rendering/Renderers/ImGuiRenderer.h"

#include "Rendering/D3D12Hooks.h"
#include "Rendering/D3D12SwapChain.h"

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

#include "Glacier/ZLobby.h"
#include "Glacier/ZRakNet.h"
#include "Multiplayer/Lobby.h"

#include <winhttp.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#include <format>
#include <shellapi.h>
#include <simdjson.h>
#include <semver.hpp>
#include <limits.h>

#include "Glacier/ZPlayerRegistry.h"
#include "Glacier/ZServerProxyRoute.h"

// Needed for TaskDialogIndirect
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#if _DEBUG

#include "DebugConsole.h"

#endif

extern void SetupLogging(spdlog::level::level_enum p_LogLevel);

extern void FlushLoggers();

extern void ClearLoggers();

ZHMSDK_API IModSDK* SDK() {
    return ModSDK::GetInstance();
}

extern "C" ZHMSDK_API const char* SDKVersion() {
    return "3.0.0";
}

ModSDK* ModSDK::g_Instance = nullptr;

ModSDK* ModSDK::GetInstance() {
    if (g_Instance == nullptr)
        g_Instance = new ModSDK();

    return g_Instance;
}

void ModSDK::DestroyInstance() {
    if (g_Instance == nullptr)
        return;

    // We request to pause the game as that increases our chances of succeeding at unloading
    // all our hooks, since less of them will be called.
    if (Globals::GameUIManager->m_pGameUIManagerEntity.m_pInterfaceRef)
        Hooks::ZGameUIManagerEntity_TryOpenMenu->Call(
            Globals::GameUIManager->m_pGameUIManagerEntity.m_pInterfaceRef,
            EGameUIMenu::eUIMenu_PauseMenu,
            true
        );

    // We do this in a different thread so the game has time to pause.
    auto* s_ExitThread = CreateThread(
        nullptr, 0, [](LPVOID) -> DWORD {
            Sleep(1000);
            delete g_Instance;
            g_Instance = nullptr;
            return 0;
        }, nullptr, 0, nullptr
    );

    WaitForSingleObject(s_ExitThread, INFINITE);
}

ModSDK::ModSDK() {
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

ModSDK::~ModSDK() {
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

bool ModSDK::PatchCode(
    const char* p_Pattern, const char* p_Mask, void* p_NewCode, size_t p_CodeSize, ptrdiff_t p_Offset
) {
    return PatchCodeInternal(p_Pattern, p_Mask, p_NewCode, p_CodeSize, p_Offset, nullptr);
}

bool ModSDK::PatchCodeStoreOriginal(
    const char* p_Pattern, const char* p_Mask, void* p_NewCode, size_t p_CodeSize, ptrdiff_t p_Offset,
    void* p_OriginalCode
) {
    return PatchCodeInternal(p_Pattern, p_Mask, p_NewCode, p_CodeSize, p_Offset, p_OriginalCode);
}

bool ModSDK::PatchCodeInternal(
    const char* p_Pattern, const char* p_Mask, void* p_NewCode, size_t p_CodeSize, ptrdiff_t p_Offset,
    void* p_OriginalCode
) {
    if (!p_Pattern || !p_Mask || !p_NewCode || p_CodeSize == 0) {
        Logger::Error("Invalid parameters provided to PatchCode call.");
        return false;
    }

    const auto* s_Pattern = reinterpret_cast<const uint8_t*>(p_Pattern);
    const auto s_Target = Util::ProcessUtils::SearchPattern(
        ModSDK::GetInstance()->GetModuleBase(),
        ModSDK::GetInstance()->GetSizeOfCode(),
        s_Pattern,
        p_Mask
    );

    if (s_Target == 0) {
        Logger::Error("Could not find pattern in call to PatchCode. Game might have been updated.");
        return false;
    }

    auto* s_TargetPtr = reinterpret_cast<void*>(s_Target + p_Offset);

    if (p_OriginalCode != nullptr) memcpy(p_OriginalCode, s_TargetPtr, p_CodeSize);

    Logger::Debug(
        "Patching {} bytes of code at {} with new code from {}.", p_CodeSize, fmt::ptr(s_TargetPtr), p_NewCode
    );

    DWORD s_OldProtect;
    VirtualProtect(s_TargetPtr, p_CodeSize, PAGE_EXECUTE_READWRITE, &s_OldProtect);

    memcpy(s_TargetPtr, p_NewCode, p_CodeSize);

    VirtualProtect(s_TargetPtr, p_CodeSize, s_OldProtect, nullptr);

    return true;
}


void ModSDK::LoadConfiguration() {
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

    for (auto& s_Mod : s_Ini) {
        // We are looking for the sdk entry.
        if (Util::StringUtils::ToLowerCase(s_Mod.first) != "sdk")
            continue;

        if (s_Mod.second.has("noui") && s_Mod.second.get("noui") == "true") {
            m_UiEnabled = false;
            MessageBoxA(
                nullptr,
                "WARNING: The mod SDK UI is currently disabled!\n\nIf you want to re-enable it, remove the 'noui = true' line from Retail/mods.ini and restart your game.",
                "Mod SDK Warning",
                MB_OK | MB_ICONWARNING
            );
        }

        if (s_Mod.second.has("console_key") && !s_Mod.second.get("console_key").empty()) {
            // Try to parse its value as a uint8_t.
            try {
                m_ConsoleScanCode = std::stoul(s_Mod.second.get("console_key"), nullptr, 0);
            }
            catch (const std::exception&) {
                Logger::Error("Could not parse console key value from mod.ini. Using default value.");
            }
        }

        if (s_Mod.second.has("ui_toggle_key") && !s_Mod.second.get("ui_toggle_key").empty()) {
            // Try to parse its value as a uint8_t.
            try {
                m_UiToggleScanCode = std::stoul(s_Mod.second.get("ui_toggle_key"), nullptr, 0);
            }
            catch (const std::exception&) {
                Logger::Error("Could not parse ui_toggle_key value from mod.ini. Using default value.");
            }
        }

        if (s_Mod.second.has("ignore_version")) {
            m_IgnoredVersion = s_Mod.second.get("ignore_version");
        }

        if (s_Mod.second.has("no_updates_for_me_please")) {
            m_DisableUpdateCheck = true;
        }

        if (s_Mod.second.has("shown_ui_toggle_warning")) {
            m_HasShownUiToggleWarning = true;
        }
    }
}

void ModSDK::SetHasShownUiToggleWarning() {
    m_HasShownUiToggleWarning = true;

    UpdateSdkIni(
        [&](auto& s_SdkMap) {
            s_SdkMap.set("shown_ui_toggle_warning", "true");
        }
    );
}

std::pair<uint32_t, std::string> ModSDK::RequestLatestVersion() {
    // Initialize WinHTTP session
    HINTERNET s_Session = WinHttpOpen(
        L"ZHMModSDK/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0
    );

    if (!s_Session) {
        throw std::runtime_error("WinHttpOpen failed");
    }

    // Smart pointer for automatic cleanup
    std::unique_ptr<void, decltype(&WinHttpCloseHandle)> s_SessionHandle(s_Session, &WinHttpCloseHandle);

    HINTERNET s_Connect = WinHttpConnect(s_SessionHandle.get(), L"api.github.com", 443, 0);

    if (!s_Connect) {
        throw std::runtime_error("WinHttpConnect failed");
    }

    std::unique_ptr<void, decltype(&WinHttpCloseHandle)> s_ConnectHandle(s_Connect, &WinHttpCloseHandle);

    // Create an HTTP request handle
    HINTERNET s_Request = WinHttpOpenRequest(
        s_ConnectHandle.get(), L"GET", L"/repos/OrfeasZ/ZHMModSDK/releases",
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );

    if (!s_Request) {
        throw std::runtime_error("WinHttpOpenRequest failed");
    }

    std::unique_ptr<void, decltype(&WinHttpCloseHandle)> s_RequestHandle(s_Request, &WinHttpCloseHandle);

    // Send a request
    if (!WinHttpSendRequest(
        s_RequestHandle.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0
    )) {
        throw std::runtime_error("WinHttpSendRequest failed");
    }

    // End the request
    if (!WinHttpReceiveResponse(s_RequestHandle.get(), nullptr)) {
        throw std::runtime_error("WinHttpReceiveResponse failed");
    }

    // Get status code
    DWORD s_StatusCode = 0;
    DWORD s_StatusCodeSize = sizeof(s_StatusCode);
    WinHttpQueryHeaders(
        s_RequestHandle.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &s_StatusCode, &s_StatusCodeSize, WINHTTP_NO_HEADER_INDEX
    );

    // Read response data
    DWORD s_ResponseSize = 0;
    std::string s_Response;

    do {
        if (!WinHttpQueryDataAvailable(s_RequestHandle.get(), &s_ResponseSize)) {
            throw std::runtime_error("WinHttpQueryDataAvailable failed");
        }

        if (s_ResponseSize > 0) {
            // Read up to 128kb at a time.
            const auto s_SizeToRead = std::min<size_t>(s_ResponseSize, 128 * 1024);
            std::unique_ptr<char[]> s_Buffer(new char[s_SizeToRead]);

            DWORD s_BytesRead;
            if (WinHttpReadData(s_RequestHandle.get(), s_Buffer.get(), s_SizeToRead, &s_BytesRead)) {
                s_Response.append(s_Buffer.get(), s_BytesRead);
            }
        }
    }
    while (s_ResponseSize > 0);

    return {s_StatusCode, s_Response};
}

void ModSDK::ShowVersionNotice(const std::string& p_Version) {
    const auto s_ContentStr = std::format(
        "A new version of the Mod SDK ({}) is available.\nYou can update by downloading it and replacing "
        "the current version.\n\nKeep in mind you might also have to update any additional mods you have "
        "installed.", p_Version
    );

    // Convert to wide string.
    const auto s_ContentSize = MultiByteToWideChar(CP_UTF8, 0, s_ContentStr.c_str(), -1, nullptr, 0);
    std::wstring s_WideContent(s_ContentSize, 0);
    MultiByteToWideChar(CP_UTF8, 0, s_ContentStr.c_str(), -1, s_WideContent.data(), s_ContentSize);

    TASKDIALOGCONFIG s_Config = {0};
    s_Config.cbSize = sizeof(s_Config);
    s_Config.hInstance = GetModuleHandle(nullptr);
    s_Config.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS;
    s_Config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    s_Config.pszMainIcon = TD_INFORMATION_ICON;
    s_Config.pszWindowTitle = L"ZHM Mod SDK - Update Available";
    s_Config.pszMainInstruction = L"A new SDK update is available!";
    s_Config.pszContent = s_WideContent.c_str();

    TASKDIALOG_BUTTON s_Buttons[] = {
        {1337, L"Download update\nUpdate must be manually applied after being downloaded."},
        {1338, L"View update notes\nOpen a page with the update notes for the new version."},
        {1339, L"Ignore this version\nSkip this update and further update notifications for this version."},
    };

    s_Config.pButtons = s_Buttons;
    s_Config.cButtons = 3;

    struct TaskDialogCallbackData {
        std::string Version;
    };

    auto* s_CallbackData = new TaskDialogCallbackData();
    s_CallbackData->Version = p_Version;

    s_Config.lpCallbackData = reinterpret_cast<LONG_PTR>(s_CallbackData);
    s_Config.pfCallback = [](HWND hWnd, UINT type, WPARAM wParam, LPARAM lParam, LONG_PTR data) -> HRESULT {
        auto* s_CallbackData = reinterpret_cast<TaskDialogCallbackData*>(data);

        if (type == TDN_DESTROYED) {
            delete s_CallbackData;
            return S_OK;
        }
        else if (type == TDN_BUTTON_CLICKED) {
            if (wParam == 1337) {
                const auto s_DownloadURL = std::format(
                    "https://github.com/OrfeasZ/ZHMModSDK/releases/download/{}/ZHMModSDK-Release.zip",
                    s_CallbackData->Version
                );

                ShellExecuteA(nullptr, "open", s_DownloadURL.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            }
            else if (wParam == 1338) {
                const auto s_ReleaseURL = std::format(
                    "https://github.com/OrfeasZ/ZHMModSDK/releases/tag/{}",
                    s_CallbackData->Version
                );

                ShellExecuteA(nullptr, "open", s_ReleaseURL.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            }
            else if (wParam == 1339) {
                // Ignore this version.
                ModSDK::GetInstance()->SkipVersionUpdate(s_CallbackData->Version);
            }
        }

        return S_OK;
    };

    TaskDialogIndirect(&s_Config, nullptr, nullptr, nullptr);
}

// Write the version to skip update notifications for to the mods.ini file.
void ModSDK::SkipVersionUpdate(const std::string& p_Version) {
    UpdateSdkIni(
        [&](auto& s_SdkMap) {
            s_SdkMap.set("ignore_version", p_Version);
        }
    );
}

void ModSDK::CheckForUpdates() const {
    // Try to get latest version from GitHub.
    std::pair<uint32_t, std::string> s_VersionCheckResult;

    try {
        Logger::Info("Checking for updates...");
        s_VersionCheckResult = RequestLatestVersion();
    }
    catch (const std::exception& e) {
        Logger::Error("Could not check for updates: {}", e.what());
        return;
    }

    if (s_VersionCheckResult.first != 200) {
        Logger::Error("Could not check for updates: HTTP status code {}", s_VersionCheckResult.first);
        return;
    }

    // Parse the JSON response with simdjson.
    try {
        simdjson::ondemand::parser s_Parser;
        const auto s_Json = simdjson::padded_string(s_VersionCheckResult.second);
        simdjson::ondemand::document s_JsonMsg = s_Parser.iterate(s_Json);

        // Get tag name of the latest release (top of the list).
        std::string_view s_LatestVersion = s_JsonMsg.get_array().at(0)["tag_name"].get_string();

        // Check if we should ignore this version.
        if (m_IgnoredVersion == s_LatestVersion) {
            Logger::Info("Ignoring update notification for version {}.", s_LatestVersion);
            return;
        }

        // Strip v prefix.
        const std::string s_LatestVersionStr(s_LatestVersion.substr(1));

        // Compare the latest version with the current version.
        semver::version s_CurrentVersion(SDKVersion());
        semver::version s_LatestSemver(s_LatestVersionStr);

        if (s_LatestSemver > s_CurrentVersion) {
            Logger::Info("A new version of the Mod SDK is available: {}.", s_LatestVersion);
            ShowVersionNotice(std::string(s_LatestVersion));
        }
        else {
            Logger::Info("Mod SDK is up to date.");
        }
    }
    catch (const simdjson::simdjson_error& e) {
        Logger::Error("Could not parse JSON response: {}", e.what());
    }
    catch (const std::exception& e) {
        Logger::Error("An error occurred while checking for updates: {}", e.what());
    }
}

// Built-in console commands
void OnConsoleCommand(void* context, TArray<ZString> p_Args) {
    if (p_Args.size() == 1) {
        if (p_Args[0] == "unloadall") {
            ModSDK::GetInstance()->GetModLoader()->UnloadAllMods();
        }
        else if (p_Args[0] == "reloadall") {
            ModSDK::GetInstance()->GetModLoader()->ReloadAllMods();
        }
    }

    if (p_Args.size() == 2) {
        if (p_Args[0] == "load") {
            ModSDK::GetInstance()->GetModLoader()->LoadMod(p_Args[1].c_str(), true);
        }
        else if (p_Args[0] == "unload") {
            ModSDK::GetInstance()->GetModLoader()->UnloadMod(p_Args[1].c_str());
        }
        else if (p_Args[0] == "reload") {
            ModSDK::GetInstance()->GetModLoader()->ReloadMod(p_Args[1].c_str());
        }
        else if (p_Args[0] == "config") {
            ZConfigCommand* s_Command = ZConfigCommand::Get(p_Args[1]);

            if (!s_Command)
                return Logger::Error("[ZConfigCommand] Invalid command.");

            switch (s_Command->GetType()) {
                case ZConfigCommand_ECLASSTYPE::ECLASS_FLOAT:
                    return Logger::Info(
                        "[ZConfigCommand] {} - float - {}", p_Args[1], s_Command->As<ZConfigFloat>()->GetValue()
                    );
                case ZConfigCommand_ECLASSTYPE::ECLASS_INT:
                    return Logger::Info(
                        "[ZConfigCommand] {} - int - {}", p_Args[1], s_Command->As<ZConfigInt>()->GetValue()
                    );
                case ZConfigCommand_ECLASSTYPE::ECLASS_STRING:
                    return Logger::Info(
                        "[ZConfigCommand] {} - string - \"{}\"", p_Args[1], s_Command->As<ZConfigString>()->GetValue()
                    );
                case ZConfigCommand_ECLASSTYPE::ECLASS_UNKNOWN:
                    return Logger::Error("[ZConfigCommand] Unsupported command type (ECLASS_UNKNOWN).");
            }
        }
    }

    if (p_Args.size() == 3) {
        if (p_Args[0] == "config") {
            ZConfigCommand* s_Command = ZConfigCommand::Get(p_Args[1]);

            if (!s_Command)
                return Logger::Info("[ZConfigCommand] Invalid command.");

            // Now we validate the input, we technically don't need to do this as it'll be done by the engine function
            // we call. But we do this to provide output to the user.
            switch (s_Command->GetType()) {
                case ZConfigCommand_ECLASSTYPE::ECLASS_FLOAT: {
                    try {
                        size_t pos;
                        static_cast<void>(std::stof(p_Args[2].c_str(), &pos));
                        if (pos != p_Args[2].size())
                            return Logger::Error(
                                "[ZConfigCommand] Invalid input (float), not all characters provided were processed."
                            );
                    }
                    catch (const std::invalid_argument&) {
                        return Logger::Error(
                            "[ZConfigCommand] Invalid input (float), input does not represent a float."
                        );
                    } catch (const std::out_of_range&) {
                        return Logger::Error("[ZConfigCommand] Invalid input (float), float is out of range.");
                    }
                    break;
                }
                case ZConfigCommand_ECLASSTYPE::ECLASS_INT: {
                    try {
                        size_t pos;
                        unsigned long value = std::stoul(p_Args[2].c_str(), &pos);
                        if (pos != p_Args[2].size())
                            return Logger::Error(
                                "[ZConfigCommand] Invalid input (integer), not all characters provided were processed."
                            );
                        if (value > (std::numeric_limits<unsigned int>::max)())
                            return Logger::Error("[ZConfigCommand] Invalid input (integer), out of u32 range.");
                    }
                    catch (const std::invalid_argument&) {
                        return Logger::Error(
                            "[ZConfigCommand] Invalid input (integer), input does not represent a integer."
                        );
                    } catch (const std::out_of_range&) {
                        return Logger::Error("[ZConfigCommand] Invalid input (integer), integer is out of range.");
                    }
                    break;
                }
                case ZConfigCommand_ECLASSTYPE::ECLASS_STRING:
                    if (p_Args[2].size() >= 256)
                        return Logger::Error(
                            "[ZConfigCommand] Invalid input (string), maximum length of 255 exceeded."
                        );
                    break;
                case ZConfigCommand_ECLASSTYPE::ECLASS_UNKNOWN:
                    return Logger::Error("[ZConfigCommand] Unsupported command type (ECLASS_UNKNOWN).");
            }

            Functions::ZConfigCommand_ExecuteCommand->Call(p_Args[1].c_str(), p_Args[2].c_str());
            Logger::Info(R"([ZConfigCommand] Set "{}" to "{}")", p_Args[1], p_Args[2]);
        }
    }
}

bool ModSDK::Startup() {
    #if _DEBUG
    m_DebugConsole->StartRedirecting();
    #endif

    m_ModLoader->Startup();

    // Notify all loaded mods that the engine has intialized once it has.
    Hooks::Engine_Init->AddDetour(this, &ModSDK::Engine_Init);
    Hooks::EOS_Platform_Create->AddDetour(this, &ModSDK::EOS_Platform_Create);
    Hooks::DrawScaleform->AddDetour(this, &ModSDK::DrawScaleform);
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &ModSDK::OnLoadScene);
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &ModSDK::OnClearScene);

    m_D3D12Hooks->Startup();

    // Patch mutex creation to allow multiple instances.
    uint8_t s_NopBytes[84] = {0x90};
    memset(s_NopBytes, 0x90, 84);
    if (!PatchCode(
        "\x4C\x8D\x05\x00\x00\x00\x00\xBA\x00\x00\x00\x00\x33\xC9\xFF\x15", "xxx????x????xxxx", s_NopBytes, 84, 0
    )) {
        Logger::Warn(
            "Could not patch multi-instance detection. You will not be able to launch the game more than once."
        );
    }

    // Setup custom multiplayer code.
    //Multiplayer::Lobby::Setup();

    if (!m_DisableUpdateCheck) {
        std::thread s_VersionCheckThread(
            [&]() {
                CheckForUpdates();
            }
        );

        s_VersionCheckThread.detach();
    }

    // Register built-in console commands
    Events::OnConsoleCommand->AddListener(this, &OnConsoleCommand);

    return true;
}

void ModSDK::ThreadedStartup() const {
    m_ModLoader->LockRead();

    for (const auto& s_Mod : m_ModLoader->GetLoadedMods()) {
        s_Mod->SetupUI();
        s_Mod->Init();
    }

    m_ModLoader->UnlockRead();

    // If the engine is already initialized, inform the mods.
    if (Globals::Hitman5Module->IsEngineInitialized())
        OnEngineInit();
}

void ModSDK::OnDrawMenu() const {
    m_ModLoader->LockRead();

    for (auto& s_Mod : m_ModLoader->GetLoadedMods()) {
        s_Mod->OnDrawMenu();
    }

    m_ModLoader->UnlockRead();
}

void ModSDK::OnDrawUI(bool p_HasFocus) const {
    m_UIConsole->Draw(p_HasFocus);
    m_UIMainMenu->Draw(p_HasFocus);
    m_UIModSelector->Draw(p_HasFocus);

    m_ModLoader->LockRead();

    for (auto& s_Mod : m_ModLoader->GetLoadedMods())
        s_Mod->OnDrawUI(p_HasFocus);

    m_ModLoader->UnlockRead();
}

void ModSDK::OnDraw3D() const {
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

void ModSDK::OnModLoaded(const std::string& p_Name, IPluginInterface* p_Mod, bool p_LiveLoad) {
    p_Mod->SetupUI();
    p_Mod->Init();

    if (p_LiveLoad && Globals::Hitman5Module->IsEngineInitialized())
        p_Mod->OnEngineInitialized();

    Logger::Info("Mod {} successfully loaded.", p_Name);
}

void ModSDK::OnModUnloaded(const std::string& p_Name) {}

void ModSDK::OnEngineInit() const {
    Logger::Debug("Engine was initialized.");

    if (m_UiEnabled) {
        m_DirectXTKRenderer->OnEngineInit();
        m_ImguiRenderer->OnEngineInit();
    }

    /*if (Globals::ZProfileServerPageProxyBase_m_aRouteMap) {
        for (auto s_Pair : *Globals::ZProfileServerPageProxyBase_m_aRouteMap) {
            Logger::Debug("Route map: {} -> {}", s_Pair->first.c_str(), s_Pair->second->m_sUrl);

            if (s_Pair->first == "hub") {
                const auto s_OriginalFn = s_Pair->second->m_fnExecute;

                s_Pair->second->m_fnExecute = [=](
                    const ZDynamicObject& p_Request,
                    std::function<void(const ZDynamicObject&)> p_OkCb,
                    std::function<void(int)> p_ErrorCb,
                    ZAsyncContext* p_AsyncContext,
                    const SHttpRequestBehavior& p_Behavior
                ) {
                            const auto s_NewOkCb = [=](const ZDynamicObject& p_Response) {
                                ZDynamicObject s_NewResponse = p_Response;

                                // Insert custom tile to dashboard.
                                auto s_NewDestination = ZDynamicObject::Object();
                                s_NewDestination["Location"] = ZDynamicObject::Object();
                                s_NewDestination["Location"]["Id"] = "CUSTOM_MISSIONS"_zs;
                                s_NewDestination["Location"]["Guid"] = "a8b5e0a0-e7d7-4c0f-a5a1-b2d5c1e5d1d2"_zs;
                                s_NewDestination["Location"]["Type"] = "location"_zs;
                                s_NewDestination["Location"]["Subtype"] = "location"_zs;
                                s_NewDestination["Location"]["RMTPrice"] = -1.0f;
                                s_NewDestination["Location"]["GamePrice"] = -1.0f;
                                s_NewDestination["Location"]["IsPurchasable"] = false;
                                s_NewDestination["Location"]["IsPublished"] = true;
                                s_NewDestination["Location"]["IsDroppable"] = false;
                                s_NewDestination["Location"]["Capabilities"] = ZDynamicObject::Array();
                                s_NewDestination["Location"]["Qualities"] = ZDynamicObject::Object();
                                s_NewDestination["Location"]["Properties"] = ZDynamicObject::Object();
                                s_NewDestination["Location"]["Properties"]["Quality"] = ""_zs;

                                s_NewDestination["Location"]["Properties"]["Icon"] =
                                        "images/locations/LOCATION_siberia/tile.jpg"_zs;

                                s_NewDestination["Location"]["Properties"]["Background"] =
                                        "images/locations/LOCATION_siberia/background.jpg"_zs;

                                s_NewDestination["Location"]["Properties"]["DlcImage"] =
                                        "images/livetile/dlc/tile_hitman3.jpg"_zs;

                                s_NewDestination["Location"]["Properties"]["DlcName"] =
                                        "GAME_STORE_METADATA_S3_GAME_TITLE"_zs;

                                s_NewDestination["Location"]["Properties"]["Season"] = 1.0f;
                                s_NewDestination["Location"]["Properties"]["Order"] = 10.0f;

                                s_NewDestination["Location"]["Properties"]["ProgressionKey"] =
                                        "LOCATION_ICA_FACILITY"_zs;

                                s_NewDestination["Location"]["Properties"]["HideProgression"] = true;
                                s_NewDestination["Location"]["Properties"]["RequiredResources"] =
                                        ZDynamicObject::Array();


                                s_NewResponse["data"]["DestinationsData"].Insert(0, s_NewDestination);

                                // Print the response.
                                ZString s_SerializedResponse;
                                Functions::ZDynamicObject_ToString->Call(
                                    &s_NewResponse, &s_SerializedResponse
                                );

                                Logger::Debug(
                                    "[{}] response: {}", s_Pair->first.c_str(), s_SerializedResponse.c_str()
                                );

                                p_OkCb(s_NewResponse);
                            };

                            s_OriginalFn(
                                p_Request,
                                s_NewOkCb,
                                p_ErrorCb,
                                p_AsyncContext,
                                p_Behavior
                            );
                        };
            }

            if (s_Pair->first == "destination") {
                const auto s_OriginalFn = s_Pair->second->m_fnExecute;
                s_Pair->second->m_fnExecute = [=](
                    const ZDynamicObject& p_Request,
                    std::function<void(const ZDynamicObject&)> p_OkCb,
                    std::function<void(int)> p_ErrorCb,
                    ZAsyncContext* p_AsyncContext,
                    const SHttpRequestBehavior& p_Behavior
                ) {
                            ZString s_SerializedRequest;
                            Functions::ZDynamicObject_ToString->Call(
                                const_cast<ZDynamicObject*>(&p_Request), &s_SerializedRequest
                            );

                            Logger::Debug(
                                "[{}] request: {}", s_Pair->first.c_str(), s_SerializedRequest.c_str()
                            );

                            ZString s_LocationId;
                            if (p_Request.Get("locationId", s_LocationId)) {
                                Logger::Debug("Location ID: {}", s_LocationId.c_str());
                            }
                            else {
                                Logger::Debug("No location ID");
                            }

                            const auto s_NewOkCb = [=](const ZDynamicObject& p_Response) {
                                ZDynamicObject s_NewResponse = p_Response;

                                for (auto& s_MissionsData : s_NewResponse["data"]["MissionData"][
                                         "SubLocationMissionsData"]) {
                                    for (auto& s_Mission : s_MissionsData["Missions"]) {
                                        s_Mission["UserCentricContract"]["Contract"]["Metadata"]["Title"] =
                                                "El em ay oh"_zs;
                                    }
                                }

                                Logger::Debug("Serializing response");

                                ZString s_SerializedResponse;
                                Functions::ZDynamicObject_ToString->Call(
                                    &s_NewResponse, &s_SerializedResponse
                                );

                                Logger::Debug(
                                    "[{}] response: {}", s_Pair->first.c_str(), s_SerializedResponse.c_str()
                                );

                                p_OkCb(s_NewResponse);
                            };

                            const auto s_NewErrorCb = [=](int p_ResponseCode) {
                                Logger::Debug("[{}] error code: {}", s_Pair->first.c_str(), p_ResponseCode);

                                p_ErrorCb(p_ResponseCode);
                            };

                            s_OriginalFn(
                                p_Request,
                                s_NewOkCb,
                                s_NewErrorCb,
                                p_AsyncContext,
                                p_Behavior
                            );
                        };
            }
        }
    }*/

    m_ModLoader->LockRead();

    for (auto& s_Mod : m_ModLoader->GetLoadedMods())
        s_Mod->OnEngineInitialized();

    m_ModLoader->UnlockRead();
}

static IDXGISwapChain* g_SwapChain = nullptr;
static ID3D12CommandQueue* g_CommandQueue = nullptr;

void ModSDK::SetSwapChain(Rendering::D3D12SwapChain* p_SwapChain) {
    Logger::Debug("Setting swap chain to {}.", fmt::ptr(p_SwapChain));
    g_SwapChain = p_SwapChain;
}

void ModSDK::OnPresent(IDXGISwapChain3* p_SwapChain) {
    m_DirectXTKRenderer->OnPresent(p_SwapChain);
    m_ImguiRenderer->OnPresent(p_SwapChain);
}

void ModSDK::PostPresent(IDXGISwapChain3* p_SwapChain, HRESULT p_PresentResult) {
    m_ImguiRenderer->PostPresent(p_SwapChain, p_PresentResult);
    m_DirectXTKRenderer->PostPresent(p_SwapChain, p_PresentResult);
}

void ModSDK::SetCommandQueue(ID3D12CommandQueue* p_CommandQueue) {
    g_CommandQueue = p_CommandQueue;

    m_DirectXTKRenderer->SetCommandQueue(p_CommandQueue);
    m_ImguiRenderer->SetCommandQueue(p_CommandQueue);
}

void ModSDK::OnReset(IDXGISwapChain3* p_SwapChain) {
    m_DirectXTKRenderer->OnReset();
    m_ImguiRenderer->OnReset();
}

void ModSDK::PostReset(IDXGISwapChain3* p_SwapChain) {
    m_ImguiRenderer->PostReset();
    m_DirectXTKRenderer->PostReset();
}

void ModSDK::RequestUIFocus() {
    if (!m_UiEnabled)
        return;

    m_ImguiRenderer->SetFocus(true);
}

void ModSDK::ReleaseUIFocus() {
    if (!m_UiEnabled)
        return;

    m_ImguiRenderer->SetFocus(false);
}

ImGuiContext* ModSDK::GetImGuiContext() {
    return ImGui::GetCurrentContext();
}

ImGuiMemAllocFunc ModSDK::GetImGuiAlloc() {
    ImGuiMemAllocFunc s_AllocFunc;
    ImGuiMemFreeFunc s_FreeFunc;
    void* s_UserData;
    ImGui::GetAllocatorFunctions(&s_AllocFunc, &s_FreeFunc, &s_UserData);

    return s_AllocFunc;
}

ImGuiMemFreeFunc ModSDK::GetImGuiFree() {
    ImGuiMemAllocFunc s_AllocFunc;
    ImGuiMemFreeFunc s_FreeFunc;
    void* s_UserData;
    ImGui::GetAllocatorFunctions(&s_AllocFunc, &s_FreeFunc, &s_UserData);

    return s_FreeFunc;
}

void* ModSDK::GetImGuiAllocatorUserData() {
    ImGuiMemAllocFunc s_AllocFunc;
    ImGuiMemFreeFunc s_FreeFunc;
    void* s_UserData;
    ImGui::GetAllocatorFunctions(&s_AllocFunc, &s_FreeFunc, &s_UserData);

    return s_UserData;
}

ImFont* ModSDK::GetImGuiLightFont() {
    return m_ImguiRenderer->GetFontLight();
}

ImFont* ModSDK::GetImGuiRegularFont() {
    return m_ImguiRenderer->GetFontRegular();
}

ImFont* ModSDK::GetImGuiMediumFont() {
    return m_ImguiRenderer->GetFontMedium();
}

ImFont* ModSDK::GetImGuiBoldFont() {
    return m_ImguiRenderer->GetFontBold();
}

ImFont* ModSDK::GetImGuiBlackFont() {
    return m_ImguiRenderer->GetFontBlack();
}

bool ModSDK::GetPinName(int32_t p_PinId, ZString& p_Name) {
    return TryGetPinName(p_PinId, p_Name);
}

bool ModSDK::WorldToScreen(const SVector3& p_WorldPos, SVector2& p_Out) {
    return m_DirectXTKRenderer->WorldToScreen(p_WorldPos, p_Out);
}

bool ModSDK::ScreenToWorld(const SVector2& p_ScreenPos, SVector3& p_WorldPosOut, SVector3& p_DirectionOut) {
    return m_DirectXTKRenderer->ScreenToWorld(p_ScreenPos, p_WorldPosOut, p_DirectionOut);
}

void ModSDK::ImGuiGameRenderTarget(ZRenderDestination* p_RT, const ImVec2& p_Size) {
    if (!p_RT)
        return;

    auto s_Size = p_Size;

    if (s_Size.x == 0 && s_Size.y == 0) {
        const auto s_Desc = p_RT->m_pTexture2D->m_pResource->GetDesc();
        s_Size = {static_cast<float>(s_Desc.Width), static_cast<float>(s_Desc.Height)};
    }

    const auto s_HandleIncrementSize = Globals::RenderManager->m_pDevice->m_pDevice->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    );

    D3D12_GPU_DESCRIPTOR_HANDLE s_Handle {};
    s_Handle.ptr = Globals::RenderManager->m_pDevice->m_pFrameHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart().ptr +
            (p_RT->m_pSRV->m_nHeapDescriptorIndex * s_HandleIncrementSize);

    ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_SetGameDescriptorHeap, nullptr);
    ImGui::Image(reinterpret_cast<ImTextureID>(s_Handle.ptr), s_Size);
    ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_ResetDescriptorHeap, nullptr);
}

void ModSDK::SetPluginSetting(
    IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, const ZString& p_Value
) {
    if (!p_Plugin) {
        return;
    }

    auto s_Settings = m_ModLoader->GetModSettings(p_Plugin);

    if (!s_Settings) {
        return;
    }

    s_Settings->SetSetting(p_Section.c_str(), p_Name.c_str(), p_Value.c_str());
}

void ModSDK::SetPluginSettingInt(
    IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, int64_t p_Value
) {
    if (!p_Plugin) {
        return;
    }

    auto s_Settings = m_ModLoader->GetModSettings(p_Plugin);

    if (!s_Settings) {
        return;
    }

    s_Settings->SetSetting(p_Section.c_str(), p_Name.c_str(), std::to_string(p_Value));
}

void ModSDK::SetPluginSettingUInt(
    IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, uint64_t p_Value
) {
    if (!p_Plugin) {
        return;
    }

    auto s_Settings = m_ModLoader->GetModSettings(p_Plugin);

    if (!s_Settings) {
        return;
    }

    s_Settings->SetSetting(p_Section.c_str(), p_Name.c_str(), std::to_string(p_Value));
}

void ModSDK::SetPluginSettingDouble(
    IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, double p_Value
) {
    if (!p_Plugin)
        return;

    auto s_Settings = m_ModLoader->GetModSettings(p_Plugin);

    if (!s_Settings) {
        return;
    }

    s_Settings->SetSetting(p_Section.c_str(), p_Name.c_str(), std::to_string(p_Value));
}

void ModSDK::SetPluginSettingBool(
    IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, bool p_Value
) {
    if (!p_Plugin) {
        return;
    }

    auto s_Settings = m_ModLoader->GetModSettings(p_Plugin);

    if (!s_Settings) {
        return;
    }

    s_Settings->SetSetting(p_Section.c_str(), p_Name.c_str(), p_Value ? "true" : "false");
}

ZString ModSDK::GetPluginSetting(
    IPluginInterface* p_Plugin,
    const ZString& p_Section,
    const ZString& p_Name,
    const ZString& p_DefaultValue
) {
    if (!p_Plugin) {
        return p_DefaultValue;
    }

    auto s_Settings = m_ModLoader->GetModSettings(p_Plugin);

    if (!s_Settings) {
        return p_DefaultValue;
    }

    return s_Settings->GetSetting(p_Section.c_str(), p_Name.c_str(), p_DefaultValue.c_str());
}

int64_t ModSDK::GetPluginSettingInt(
    IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, int64_t p_DefaultValue
) {
    if (!p_Plugin) {
        return p_DefaultValue;
    }

    auto s_Settings = m_ModLoader->GetModSettings(p_Plugin);

    if (!s_Settings) {
        return p_DefaultValue;
    }

    const auto s_Value = s_Settings->GetSetting(p_Section.c_str(), p_Name.c_str(), std::to_string(p_DefaultValue));

    try {
        return std::stoll(s_Value);
    }
    catch (const std::exception&) {
        return p_DefaultValue;
    }
}

uint64_t ModSDK::GetPluginSettingUInt(
    IPluginInterface* p_Plugin,
    const ZString& p_Section,
    const ZString& p_Name,
    uint64_t p_DefaultValue
) {
    if (!p_Plugin) {
        return p_DefaultValue;
    }

    auto s_Settings = m_ModLoader->GetModSettings(p_Plugin);

    if (!s_Settings) {
        return p_DefaultValue;
    }

    const auto s_Value = s_Settings->GetSetting(p_Section.c_str(), p_Name.c_str(), std::to_string(p_DefaultValue));

    try {
        return std::stoull(s_Value);
    }
    catch (const std::exception&) {
        return p_DefaultValue;
    }
}

double ModSDK::GetPluginSettingDouble(
    IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, double p_DefaultValue
) {
    if (!p_Plugin) {
        return p_DefaultValue;
    }

    auto s_Settings = m_ModLoader->GetModSettings(p_Plugin);

    if (!s_Settings) {
        return p_DefaultValue;
    }

    const auto s_Value = s_Settings->GetSetting(p_Section.c_str(), p_Name.c_str(), std::to_string(p_DefaultValue));

    try {
        return std::stod(s_Value);
    }
    catch (const std::exception&) {
        return p_DefaultValue;
    }
}

bool ModSDK::GetPluginSettingBool(
    IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, bool p_DefaultValue
) {
    if (!p_Plugin) {
        return p_DefaultValue;
    }

    auto s_Settings = m_ModLoader->GetModSettings(p_Plugin);

    if (!s_Settings) {
        return p_DefaultValue;
    }

    const auto s_Value = s_Settings->GetSetting(p_Section.c_str(), p_Name.c_str(), p_DefaultValue ? "true" : "false");

    if (s_Value == "true" || s_Value == "1" || s_Value == "yes" || s_Value == "on" || s_Value == "y") {
        return true;
    }
    else if (s_Value == "false" || s_Value == "0" || s_Value == "no" || s_Value == "off" || s_Value == "n") {
        return false;
    }
    else {
        return p_DefaultValue;
    }
}

bool ModSDK::HasPluginSetting(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name) {
    if (!p_Plugin) {
        return false;
    }

    auto s_Settings = m_ModLoader->GetModSettings(p_Plugin);

    if (!s_Settings) {
        return false;
    }

    return s_Settings->HasSetting(p_Section.c_str(), p_Name.c_str());
}

void ModSDK::RemovePluginSetting(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name) {
    if (!p_Plugin) {
        return;
    }

    auto s_Settings = m_ModLoader->GetModSettings(p_Plugin);

    if (!s_Settings) {
        return;
    }

    s_Settings->RemoveSetting(p_Section.c_str(), p_Name.c_str());
}

void ModSDK::ReloadPluginSettings(IPluginInterface* p_Plugin) {
    if (!p_Plugin) {
        return;
    }

    auto s_Settings = m_ModLoader->GetModSettings(p_Plugin);

    if (!s_Settings) {
        return;
    }

    s_Settings->Reload();
}

TEntityRef<ZHitman5> ModSDK::GetLocalPlayer() {
    if (!Globals::PlayerRegistry) {
        return {};
    }

    SNetPlayerData* s_PlayerData = nullptr;

    for (int i = 0; i < _countof(Globals::PlayerRegistry->m_pPlayerData); ++i) {
        if (!Globals::PlayerRegistry->m_pPlayerData[i]->m_Controller.m_pNetPlayer) {
            s_PlayerData = Globals::PlayerRegistry->m_pPlayerData[i];
            break;
        }

        // TODO: Game does some additional checks here but I have no idea what's going on currently and I'm not sure if it's necessary.
        // It might become relevant when dealing with multiplayer sessions. It seems to:
        // Keep going through the m_pPlayerData array until it finds a player without a s_PlayerData->m_Controller.m_pNetPlayer
        // or one that does have one but passes some check (no idea what that check is - some vfunc call).
    }

    // If we still don't have a player, pick the first non-null one.
    if (!s_PlayerData) {
        for (int i = 0; i < _countof(Globals::PlayerRegistry->m_pPlayerData); ++i) {
            if (Globals::PlayerRegistry->m_pPlayerData[i]) {
                s_PlayerData = Globals::PlayerRegistry->m_pPlayerData[i];
                break;
            }
        }
    }

    // Still nothing? Return an empty entity.
    if (!s_PlayerData) {
        return {};
    }

    return TEntityRef<ZHitman5>(s_PlayerData->m_Controller.m_HitmanEntity);
}

DEFINE_DETOUR_WITH_CONTEXT(ModSDK, bool, Engine_Init, void* th, void* a2) {
    auto s_Result = p_Hook->CallOriginal(th, a2);

    OnEngineInit();

    return HookResult<bool>(HookAction::Return(), s_Result);
}

using EOS_Bool = int32_t;
constexpr auto EOS_TRUE = 1;
constexpr auto EOS_FALSE = 0;

struct EOS_Platform_ClientCredentials {
    const char* ClientId;
    const char* ClientSecret;
};

struct EOS_Platform_Options {
    int32_t ApiVersion;
    void* Reserved;
    const char* ProductId;
    const char* SandboxId;
    EOS_Platform_ClientCredentials ClientCredentials;
    EOS_Bool bIsServer;
    const char* EncryptionKey;
    const char* OverrideCountryCode;
    const char* OverrideLocaleCode;
    const char* DeploymentId;
    uint64_t Flags;
    const char* CacheDirectory;
    uint32_t TickBudgetInMilliseconds;
};

constexpr auto EOS_PF_LOADING_IN_EDITOR = 0x00001;
constexpr auto EOS_PF_DISABLE_OVERLAY = 0x00002;

DEFINE_DETOUR_WITH_CONTEXT(ModSDK, EOS_PlatformHandle*, EOS_Platform_Create, EOS_Platform_Options* Options) {
    // Disable overlay in debug mode since it conflicts with Nsight and the like.
    #if _DEBUG
    Logger::Debug("Disabling Epic overlay.");
    Options->Flags |= EOS_PF_LOADING_IN_EDITOR | EOS_PF_DISABLE_OVERLAY;
    #endif

    return {HookAction::Continue()};
}

void ModSDK::UpdateSdkIni(std::function<void(mINI::INIMap<std::string>&)> p_Callback) {
    char s_ExePathStr[MAX_PATH];
    auto s_PathSize = GetModuleFileNameA(nullptr, s_ExePathStr, MAX_PATH);

    if (s_PathSize == 0)
        return;

    std::filesystem::path s_ExePath(s_ExePathStr);
    auto s_ExeDir = s_ExePath.parent_path();

    const auto s_IniPath = absolute(s_ExeDir / "mods.ini");

    mINI::INIFile s_File(s_IniPath.string());

    mINI::INIStructure s_Ini;

    if (is_regular_file(s_IniPath)) {
        s_File.read(s_Ini);
    }

    mINI::INIMap<std::string> s_SdkMap;

    if (s_Ini.has("sdk")) {
        s_SdkMap = s_Ini.get("sdk");
    }

    p_Callback(s_SdkMap);

    s_Ini.set("sdk", s_SdkMap);

    s_File.generate(s_Ini, true);
}

DEFINE_DETOUR_WITH_CONTEXT(
    ModSDK, void, DrawScaleform, ZRenderContext* ctx, ZRenderTargetView** rtv, uint32_t a3,
    ZRenderDepthStencilView** dsv,
    uint32_t a5, bool bCaptureOnly
) {
    if(dsv) {
        if (*dsv && m_DirectXTKRenderer) {
            m_DirectXTKRenderer->SetDsvIndex((*Globals::D3D12ObjectPools)->DepthStencilViews.IndexOf(*dsv) + 1);
        }
    }

    return {HookAction::Continue()};
}

DEFINE_DETOUR_WITH_CONTEXT(
    ModSDK, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData
) {
    if (m_DirectXTKRenderer) {
        m_DirectXTKRenderer->ClearDsvIndex();
    }

    return {HookAction::Continue()};
}

DEFINE_DETOUR_WITH_CONTEXT(ModSDK, void, OnClearScene, ZEntitySceneContext* th, bool forReload) {
    if (m_DirectXTKRenderer) {
        m_DirectXTKRenderer->ClearDsvIndex();
    }

    return {HookAction::Continue()};
}
