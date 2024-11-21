#include "OnlineTools.h"

#include <Logging.h>
#include <IconsMaterialDesign.h>
#include <Functions.h>

#include <regex>
#include <numeric>

#include <Glacier/ZConfigCommand.h>

std::string DOMAIN_DELIMETER = "|||";

#pragma region String Utils

// From "Utils/StringUtils.cpp"
std::vector<std::string> Split(const std::string& p_String, const std::string& p_Delimeter)
{
    std::vector<std::string> s_Parts;

    size_t s_PartStart = p_String.find_first_not_of(p_Delimeter);
    size_t s_PartEnd;

    while ((s_PartEnd = p_String.find_first_of(p_Delimeter, s_PartStart)) != std::string::npos)
    {
        s_Parts.push_back(p_String.substr(s_PartStart, s_PartEnd - s_PartStart));
        s_PartStart = p_String.find_first_not_of(p_Delimeter, s_PartEnd);
    }

    if (s_PartStart != std::string::npos)
        s_Parts.push_back(p_String.substr(s_PartStart));

    return s_Parts;
}

std::string Join(std::vector<std::string>& p_Strings, std::string& p_Delimeter)
{
    return std::accumulate(
        p_Strings.begin(), p_Strings.end(), std::string(),
        [&p_Delimeter](std::string x, std::string y)
        {
            return x.empty() ? y : x + p_Delimeter + y;
        }
    );
}

#pragma endregion

#pragma region Patching Helpers

// Patches the game so the Authorization header is sent to non-HTTPS and untrusted servers.
void OnlineTools::PatchAuthHeaderChecks()
{
    uint8_t nop[6] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
    
    if (m_OldAuthPatch1Code == nullptr) m_OldAuthPatch1Code = malloc(6);
    if (m_OldAuthPatch2Code == nullptr) m_OldAuthPatch2Code = malloc(6);

    // Patch protocol check in ZOnlineManager::HttpRequest
    if (!SDK()->PatchCodeStoreOriginal("\x0F\x85\x00\x00\x00\x00\x48\x83\xF9\x00\x75\x00\xE9\x00\x00\x00\x00\x32\xDB", "xx????xxx?x?x????xx", nop, sizeof(nop), 0, m_OldAuthPatch1Code))
    {
        Logger::Error("[OnlineTools] Failed to patch protocol check in ZOnlineManager::HttpRequest!");
        free(m_OldAuthPatch1Code);
        m_OldAuthPatch1Code = nullptr;
    }

    // Patch trusted domain check in ZOnlineManager::HttpRequest
    if (!SDK()->PatchCodeStoreOriginal("\x0F\x84\x00\x00\x00\x00\x84\xDB\x0F\x85\x00\x00\x00\x00\x48\x8B\x05", "xx????xxxx????xxx", nop, sizeof(nop), 0, m_OldAuthPatch2Code))
    {
        Logger::Warn("[OnlineTools] Failed to patch trusted domain check in ZOnlineManager::HttpRequest! Trying Game Pass pattern...");

        // It's different on gamepass because of course it is.
        if (!SDK()->PatchCodeStoreOriginal("\x0F\x84\x00\x00\x00\x00\x84\xDB\x0F\x85\x00\x00\x00\x00\x48\x8D\x55", "xx????xxxx????xxx", nop, sizeof(nop), 0, m_OldAuthPatch2Code))
        {
            Logger::Error("[OnlineTools] Failed to patch trusted domain check in ZOnlineManager::HttpRequest!");
            free(m_OldAuthPatch2Code);
            m_OldAuthPatch2Code = nullptr;
        }
    }
}

// Restores the patches we made to allow Authorization headers to be sent to non-HTTPS and untrusted servers.
void OnlineTools::RestoreAuthHeaderChecks()
{
    // Restore protocol check in ZOnlineManager::HttpRequest
    if (m_OldAuthPatch1Code)
    {
        if (!SDK()->PatchCode("\x90\x90\x90\x90\x90\x90\x48\x83\xF9\x00\x75\x00\xE9\x00\x00\x00\x00\x32\xDB", "xxxxxxxxx?x?x????xx", m_OldAuthPatch1Code, 6, 0))
            Logger::Error("[OnlineTools] Failed to restore protocol check in ZOnlineManager::HttpRequest!");

        free(m_OldAuthPatch1Code);
        m_OldAuthPatch1Code = nullptr;
    }
    else
        Logger::Warn("[OnlineTools] Unable to restore protocol check in ZOnlineManager::HttpRequest! Old code not found.");

    // Patch trusted domain check in ZOnlineManager::HttpRequest
    if (m_OldAuthPatch2Code)
    {
        if (!SDK()->PatchCode("\x90\x90\x90\x90\x90\x90\x84\xDB\x0F\x85\x00\x00\x00\x00\x48\x8B\x05", "xxxxxxxxxx????xxx", m_OldAuthPatch2Code, 6, 0))
        {
            Logger::Warn("[OnlineTools] Failed to restore trusted domain check in ZOnlineManager::HttpRequest! Trying Game Pass pattern...");

            // It's different on gamepass because of course it is.
            if (!SDK()->PatchCode("\x90\x90\x90\x90\x90\x90\x84\xDB\x0F\x85\x00\x00\x00\x00\x48\x8D\x55", "xxxxxxxxxx????xxx", m_OldAuthPatch2Code, 6, 0))
                Logger::Error("[OnlineTools] Failed to restore trusted domain check in ZOnlineManager::HttpRequest!");
        }

        free(m_OldAuthPatch2Code);
        m_OldAuthPatch2Code = nullptr;
    }
    else
        Logger::Warn("[OnlineTools] Unable to restore trusted domain check in ZOnlineManager::HttpRequest! Old code not found.");
}

#pragma endregion

void OnlineTools::OnEngineInitialized()
{
    // Load online settings
    m_UseHttp = GetSettingBool("online", "use_http", false);
    m_AlwaysSendAuth = GetSettingBool("online", "always_send_auth_header", false);
    m_CertPinBypass = GetSettingBool("online", "bypass_cert_pinning", false);
    m_OptionalDynRes = GetSettingBool("online", "optional_dynamic_resources", false);

    // Load saved domains
    m_DefaultDomain = GetSettingInt("domains", "default", -1);

    if (HasSetting("domains", "saved"))
        m_Domains = Split(GetSetting("domains", "saved", "").c_str(), DOMAIN_DELIMETER);

    // Apply settings
    if (m_AlwaysSendAuth) PatchAuthHeaderChecks();

    if (m_OptionalDynRes)
        Functions::ZConfigCommand_ExecuteCommand->Call("OnlineResources_ForceOfflineOnFailure", "0");

    if (m_DefaultDomain >= 0 && m_Domains.size() > m_DefaultDomain)
        Functions::ZConfigCommand_ExecuteCommand->Call("online_VersionConfigDomain", m_Domains.at(m_DefaultDomain).c_str());

    // Add hooks
    Hooks::ZOnlineVersionConfig_GetConfigHost->AddDetour(this, &OnlineTools::GetConfigHost);
    Hooks::Check_SSL_Cert->AddDetour(this, &OnlineTools::Check_SSL_Cert);
}

OnlineTools::~OnlineTools()
{
    RestoreAuthHeaderChecks();
}

#pragma region Hooks

DEFINE_PLUGIN_DETOUR(OnlineTools, ZString*, GetConfigHost, void* th, void* a1)
{
    if (!m_UseHttp) return HookResult<ZString*>(HookAction::Continue());

    ZString* s_OriginalUrl = p_Hook->CallOriginal(th, a1);
    ZString* s_NewUrl = new ZString(std::regex_replace(s_OriginalUrl->c_str(), std::regex("https://"), "http://"));

    return HookResult<ZString*>(HookAction::Return(), s_NewUrl); 
}

DEFINE_PLUGIN_DETOUR(OnlineTools, bool, Check_SSL_Cert, void* unk1, void* unk2)
{
    if (!m_CertPinBypass) return HookResult<bool>(HookAction::Continue());

    return HookResult<bool>(HookAction::Return(), true);
}

#pragma endregion

#pragma region Saving Helpers

inline void OnlineTools::SaveProtocol()
{
    SetSettingBool("online", "use_http", m_UseHttp);
}

inline void OnlineTools::SaveCertPin()
{
    SetSettingBool("online", "bypass_cert_pinning", m_CertPinBypass);
}

inline void OnlineTools::UpdateHeaders()
{
    SetSettingBool("online", "always_send_auth_header", m_AlwaysSendAuth);
    m_AlwaysSendAuth ? PatchAuthHeaderChecks() : RestoreAuthHeaderChecks();
}

inline void OnlineTools::UpdateDynRes()
{
    SetSettingBool("online", "optional_dynamic_resources", m_OptionalDynRes);
    Functions::ZConfigCommand_ExecuteCommand->Call("OnlineResources_ForceOfflineOnFailure", m_OptionalDynRes ? "0" : "1");
}

inline void OnlineTools::SaveDomains()
{
    SetSetting("domains", "saved", Join(m_Domains, DOMAIN_DELIMETER));
}

#pragma endregion

#pragma region UI

void CenteredText(const char* p_Text)
{
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(p_Text).x) * 0.5f);
    ImGui::Text(p_Text);
}

void OnlineTools::OnDrawMenu()
{
    if (ImGui::Button(ICON_MD_PUBLIC " OnlineTools"))
        m_ShowSettings = !m_ShowSettings;
}

void OnlineTools::SettingsMenu()
{
    if (ImGui::Begin(ICON_MD_PUBLIC " OnlineTools", &m_ShowSettings, ImGuiWindowFlags_NoResize))
    {
        ImGui::SetWindowSize(ImVec2(588.f, 605.f));

        // Online Settings
        ImGui::SeparatorText("Settings");

        if (ImGui::Checkbox("Use HTTP", &m_UseHttp))
            SaveProtocol();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Use http:// instead of https://. Useful if the server you're connecting to is local or doesn't support secure connections.");

        if (ImGui::Checkbox("Always Send Authorization Header", &m_AlwaysSendAuth))
            UpdateHeaders();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Always send a token that can identify you to the server. This is only required if the server is not using HTTPS and is not a trusted domain (i.e. one ran by IO Interactive).");

        if (ImGui::Checkbox("Bypass SSL Certificate Pinning", &m_CertPinBypass))
            SaveCertPin();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Allows you to decrypt SSL traffic between the game and a server if you use a proxy.");

        if (ImGui::Checkbox("Make Dynamic Resources Optional", &m_OptionalDynRes))
            UpdateDynRes();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Makes the dynamic resources package optional, meaning if a server doesn't have it, you won't be forced offline.");

        // Domains
        ImGui::SeparatorText("Domains");

        if (m_Domains.size() == 0)
            CenteredText("If you're unsure on what to do, go to \"Help\".");
        else
            CenteredText("Press enter after editing a domain to save it.");

        std::unique_ptr<bool[]> s_IsDefault(new bool[m_Domains.size()]);

        for (size_t i = 0; i < m_Domains.size(); ++i)
        {
            s_IsDefault[i] = m_DefaultDomain == i;

            // Checkbox to mark as default domain
            if (ImGui::Checkbox(("##Default" + std::to_string(i)).c_str(), &s_IsDefault[i]))
            {
                m_DefaultDomain = s_IsDefault[i] ? i : -1;
                SetSettingInt("domains", "default", m_DefaultDomain);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(s_IsDefault[i] ? "This domain is currently set as the default. Uncheck or check another to remove." : "Make this domain the default one when the game starts.");

            // Delete button
            ImGui::SameLine();
            if (ImGui::Button((ICON_MD_DELETE "##" + std::to_string(i)).c_str()))
            {
                m_Domains.erase(m_Domains.begin() + i);

                if (s_IsDefault[i])
                {
                    m_DefaultDomain = -1;
                    SetSettingInt("domains", "default", -1);
                }

                if (m_Domains.size() == 0)
                    RemoveSetting("domains", "saved");
                else
                    SaveDomains();

                continue;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Delete this domain%s.", s_IsDefault[i] ? " and remove it as the default" : "");

            // Maximum total size of a domain is 255 characters
            char s_Domain[256] = {};
            strncpy_s(s_Domain, m_Domains[i].c_str(), sizeof(s_Domain) - 1);

            // Apply button
            ImGui::SameLine();
            if (ImGui::Button((ICON_MD_KEYBOARD_RETURN "##" + std::to_string(i)).c_str()))
                Functions::ZConfigCommand_ExecuteCommand->Call("online_VersionConfigDomain", s_Domain);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Apply this domain now, it will persist until you apply a different one or close the game.");

            // Domain box
            ImGui::SameLine();
            ImGui::InputText(("##Domain" + std::to_string(i)).c_str(), s_Domain, IM_ARRAYSIZE(s_Domain));
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enter a domain here, press enter to save.");

            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                m_Domains[i] = s_Domain;
                SaveDomains();
            }
        }

        if (ImGui::Button(ICON_MD_ADD " Add Domain"))
        {
            m_Domains.push_back("localhost");
            SaveDomains();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add a new domain.");

        ImGui::SameLine();
        if (ImGui::Button(ICON_MD_RESTORE " Restore to Official"))
            Functions::ZConfigCommand_ExecuteCommand->Call("online_VersionConfigDomain", "config.hitman.io");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Restore the domain to official's. It will be overridden if you apply a different domain or have a default set on game startup.");

        ImGui::SameLine();
        if (ImGui::Button(ICON_MD_HELP " Help"))
            m_ShowHelp = !m_ShowHelp;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Open the help page.");
    }

    ImGui::End();
}

void OnlineTools::HelpMenu()
{
    if (ImGui::Begin(ICON_MD_PUBLIC " OnlineTools - Help", &m_ShowHelp, ImGuiWindowFlags_NoResize))
    {
        ImGui::SetWindowSize(ImVec2(400.f, 446.f));

        ImGui::SeparatorText("Quick Start");

        ImGui::TextWrapped("%s", "If you're used to the Peacock/LocalGhost patcher you can press the button below to load those settings. Make sure you either press the apply button or set a domain as the default.");
        if (ImGui::Button(ICON_MD_RESTORE_FROM_TRASH " Load Old Patcher Settings"))
        {
            m_UseHttp = true;
            m_AlwaysSendAuth = true;
            m_CertPinBypass = true;
            m_OptionalDynRes = true;

            m_Domains = {"localhost", "gm.hitmaps.com", "ghostmode.rdil.rocks"};

            SaveProtocol(); SaveCertPin(); UpdateHeaders(); UpdateDynRes(); SaveDomains();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("This will reset all your currently saved domains!");

        ImGui::SeparatorText("Menu Overview");
        ImGui::TextWrapped("%s", "All the buttons, checkboxes, and text inputs on the main OnlineTools menu have tooltips. Hover over them to see what they/to do.");
    }

    ImGui::End();
}

void OnlineTools::OnDrawUI(bool p_HasFocus)
{
    if (p_HasFocus)
    {
        if (m_ShowSettings) SettingsMenu();
        if (m_ShowHelp) HelpMenu();
    }
}

#pragma endregion

DECLARE_ZHM_PLUGIN(OnlineTools);
