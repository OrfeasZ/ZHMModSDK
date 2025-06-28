#pragma once

#include <IPluginInterface.h>

class OnlineTools : public IPluginInterface {
public:
    void OnEngineInitialized() override;
    ~OnlineTools() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    // Patching Helpers
    void PatchAuthHeaderChecks();
    void RestoreAuthHeaderChecks();

    // UI
    void SettingsMenu();
    void HelpMenu();

    // Saving Helpers
    inline void SaveProtocol();
    inline void SaveCertPin();
    inline void UpdateHeaders();
    inline void UpdateEnableDynRes();
    inline void UpdateDynRes();
    inline void SaveDomains();

    // Hooks
    DECLARE_PLUGIN_DETOUR(OnlineTools, ZString*, GetConfigHost, void* th, void* a1);
    DECLARE_PLUGIN_DETOUR(OnlineTools, bool, Check_SSL_Cert, void*, void*);

private:
    void* m_OldAuthPatch1Code = nullptr;
    void* m_OldAuthPatch2Code = nullptr;

    // UI
    bool m_ShowSettings = false;
    bool m_ShowHelp = false;

    // Online Settings
    bool m_UseHttp = false;
    bool m_AlwaysSendAuth = false;
    bool m_CertPinBypass = false;
    bool m_EnableDynRes = false;
    bool m_OptionalDynRes = false;

    // Saved Domains
    int64_t m_DefaultDomain = -1;
    std::vector<std::string> m_Domains = {};
};

DEFINE_ZHM_PLUGIN(OnlineTools)
