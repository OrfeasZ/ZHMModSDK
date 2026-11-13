#include "ModSDK.h"

#include <filesystem>
#include <format>

#include <Shlobj.h>

#include "Logging.h"
#include "Glacier/Hash.h"
#include "Util/ProcessUtils.h"
#include "Util/StringUtils.h"

static std::optional<std::filesystem::path> GetWindowsLocalAppDataPath() {
    PWSTR s_LocalAppDataPath = nullptr;

    if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &s_LocalAppDataPath) != S_OK) {
        CoTaskMemFree(s_LocalAppDataPath);
        return std::nullopt;
    }

    std::filesystem::path s_Result(s_LocalAppDataPath);
    CoTaskMemFree(s_LocalAppDataPath);

    return s_Result;
}

// Matches Rust's dirs::data_local_dir() on Linux: $XDG_DATA_HOME if it is absolute, otherwise $HOME/.local/share.
// Wine 11+ exposes Unix XDG_* variables with a WINE_HOST_ prefix, while previous versions pass them as is :)
// It also hides the HOME variable from Windows processes, but exposes it as an NT path in WINEHOMEDIR (:
// Amazing technology!
static std::optional<std::filesystem::path> GetWineLocalDataPath() {
    std::wstring s_XdgDataHome = Util::ProcessUtils::GetEnvironmentVariableValue(L"WINE_HOST_XDG_DATA_HOME");

    if (s_XdgDataHome.empty()) {
        s_XdgDataHome = Util::ProcessUtils::GetEnvironmentVariableValue(L"XDG_DATA_HOME");
    }

    if (s_XdgDataHome.starts_with(L'/')) {
        return Util::ProcessUtils::GetWineDosPath(Util::StringUtils::UTF16ToUTF8(s_XdgDataHome));
    }

    std::wstring s_HomeDir = Util::ProcessUtils::GetEnvironmentVariableValue(L"WINEHOMEDIR");

    if (s_HomeDir.empty()) {
        return std::nullopt;
    }

    if (s_HomeDir.starts_with(L"\\??\\")) {
        s_HomeDir = s_HomeDir.substr(4);
    }

    return std::filesystem::path(s_HomeDir) / ".local" / "share";
}

static std::string MD5Hex(std::string_view p_Data) {
    const auto s_Hash = Hash::MD5(p_Data);

    std::string s_Result;

    for (const uint32_t s_Word : { s_Hash.A, s_Hash.B, s_Hash.C, s_Hash.D }) {
        for (int i = 0; i < 4; ++i) {
            s_Result += std::format("{:02x}", (s_Word >> (i * 8)) & 0xFF);
        }
    }

    return s_Result;
}

void ModSDK::ResolveSmfDeploymentPath() {
    const auto s_RetailPath = Util::ProcessUtils::GetExecutableDirectory();

    if (!s_RetailPath) {
        Logger::Warn("Could not get the game's Retail folder. SMF deployment data will not be available.");
        return;
    }

    // SMF names the folder after the MD5 of Rust's fs::canonicalize of the Retail folder.
    // On Windows that is the verbatim (\\?\C:\...) path, and under Wine it is the native Unix path.
    std::optional<std::string> s_CanonicalRetailPath;
    std::optional<std::filesystem::path> s_LocalDataPath;

    if (Util::ProcessUtils::IsRunningUnderWine()) {
        s_CanonicalRetailPath = Util::ProcessUtils::GetWineUnixPath(*s_RetailPath);
        s_LocalDataPath = GetWineLocalDataPath();
    }
    else {
        if (const auto s_FinalPath = Util::ProcessUtils::GetRealPath(*s_RetailPath)) {
            s_CanonicalRetailPath = Util::StringUtils::UTF16ToUTF8(*s_FinalPath);
        }

        s_LocalDataPath = GetWindowsLocalAppDataPath();
    }

    if (!s_CanonicalRetailPath || !s_LocalDataPath) {
        Logger::Warn("Could not resolve the paths needed to find the SMF deployment folder.");
        return;
    }

    const auto s_DeploymentPath = *s_LocalDataPath / "Simple Mod Framework" / "deployments" /
        MD5Hex(*s_CanonicalRetailPath);

    const std::string s_DeploymentPathUtf8 = Util::StringUtils::UTF16ToUTF8(s_DeploymentPath.wstring());

    std::error_code s_Error;

    if (!std::filesystem::exists(s_DeploymentPath / "summary.json", s_Error)) {
        Logger::Info(
            "No SMF deployment found for Retail folder '{}' (expected at '{}').",
            *s_CanonicalRetailPath,
            s_DeploymentPathUtf8
        );
        return;
    }

    Logger::Info("Found SMF deployment at '{}'.", s_DeploymentPathUtf8);

    m_SmfDeploymentPath = s_DeploymentPathUtf8;
}

bool ModSDK::GetSmfDeploymentPath(ZString& p_Path) {
    if (!m_SmfDeploymentPath) {
        return false;
    }

    p_Path = *m_SmfDeploymentPath;

    return true;
}
