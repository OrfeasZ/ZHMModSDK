#include "ProcessUtils.h"

#include <TlHelp32.h>
#include <unordered_set>

#include "Logging.h"

using namespace Util;

uintptr_t ProcessUtils::SearchPattern(
    uintptr_t p_BaseAddress, size_t p_ScanSize, const uint8_t* p_Pattern, const char* p_Mask
) {
    const size_t s_PatternSize = strlen(p_Mask);

    if (s_PatternSize <= 1) {
        return 0;
    }

    const uintptr_t s_SearchEnd = p_BaseAddress + p_ScanSize - s_PatternSize;

    for (uintptr_t s_SearchAddr = p_BaseAddress; s_SearchAddr <= s_SearchEnd; ++s_SearchAddr) {
        const uint8_t* s_MemoryPtr = reinterpret_cast<uint8_t*>(s_SearchAddr);

        bool s_Found = true;

        for (int i = 0; i < s_PatternSize; ++i) {
            if (p_Mask[i] == '?') {
                continue;
            }

            if (s_MemoryPtr[i] != p_Pattern[i]) {
                s_Found = false;
                break;
            }
        }

        if (s_Found) {
            return s_SearchAddr;
        }
    }

    return 0;
}

uint32_t ProcessUtils::GetSizeOfCode(HMODULE p_Module) {
    PIMAGE_DOS_HEADER s_DOSHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(p_Module);
    PIMAGE_NT_HEADERS s_NTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(p_Module) +
        s_DOSHeader->e_lfanew);

    if (!s_NTHeader)
        return 0;

    return static_cast<uint32_t>(s_NTHeader->OptionalHeader.SizeOfCode);
}

uintptr_t ProcessUtils::GetBaseOfCode(HMODULE p_Module) {
    PIMAGE_DOS_HEADER s_DOSHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(p_Module);
    PIMAGE_NT_HEADERS s_NTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(p_Module) +
        s_DOSHeader->e_lfanew);

    if (!s_NTHeader)
        return 0;

    return static_cast<uintptr_t>(s_NTHeader->OptionalHeader.BaseOfCode);
}

uint32_t ProcessUtils::GetSizeOfImage(HMODULE p_Module) {
    PIMAGE_DOS_HEADER s_DOSHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(p_Module);
    PIMAGE_NT_HEADERS s_NTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(p_Module) +
        s_DOSHeader->e_lfanew);

    if (!s_NTHeader)
        return 0;

    return static_cast<uint32_t>(s_NTHeader->OptionalHeader.SizeOfImage);
}

std::tuple<uintptr_t, uintptr_t> ProcessUtils::GetSectionStartAndEnd(
    HMODULE p_Module, const std::string& p_SectionName
) {
    PIMAGE_DOS_HEADER s_DOSHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(p_Module);
    PIMAGE_NT_HEADERS s_NTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(p_Module) +
        s_DOSHeader->e_lfanew);

    if (!s_NTHeader)
        return std::make_tuple<uintptr_t, uintptr_t>(0, 0);

    PIMAGE_SECTION_HEADER s_Section = IMAGE_FIRST_SECTION(s_NTHeader);

    for (int i = 0; i < s_NTHeader->FileHeader.NumberOfSections; ++i) {
        if (strcmp(reinterpret_cast<const char*>(s_Section->Name), p_SectionName.c_str()) == 0) {
            uintptr_t s_RDataSectionStart = s_Section->VirtualAddress;
            s_RDataSectionStart += reinterpret_cast<uintptr_t>(p_Module);

            uintptr_t s_RDataSectionEnd = s_RDataSectionStart + s_Section->SizeOfRawData;

            return std::make_tuple(s_RDataSectionStart, s_RDataSectionEnd);
        }

        ++s_Section;
    }

    return std::make_tuple<uintptr_t, uintptr_t>(0, 0);
}

uintptr_t ProcessUtils::GetRelativeAddr(uintptr_t p_Base, int32_t p_Offset) {
    uintptr_t s_RelAddrPtr = p_Base + p_Offset;
    int32_t s_RelAddr = *reinterpret_cast<int32_t*>(s_RelAddrPtr);

    return s_RelAddrPtr + s_RelAddr + sizeof(int32_t);
}

std::wstring ProcessUtils::GetEnvironmentVariableValue(const wchar_t* p_Name) {
    const DWORD s_Size = GetEnvironmentVariableW(p_Name, nullptr, 0);

    if (s_Size == 0) {
        return {};
    }

    std::wstring s_Value(s_Size, L'\0');
    const DWORD s_Length = GetEnvironmentVariableW(p_Name, s_Value.data(), s_Size);
    s_Value.resize(s_Length);

    return s_Value;
}

std::optional<std::filesystem::path> ProcessUtils::GetExecutableDirectory() {
    std::wstring s_ExePath(32768, L'\0');
    const auto s_Length = GetModuleFileNameW(nullptr, s_ExePath.data(), static_cast<DWORD>(s_ExePath.size()));

    if (s_Length == 0 || s_Length == s_ExePath.size()) {
        return std::nullopt;
    }

    s_ExePath.resize(s_Length);

    return std::filesystem::path(s_ExePath).parent_path();
}

std::optional<std::wstring> ProcessUtils::GetRealPath(const std::filesystem::path& p_Path) {
    const auto s_Handle = CreateFileW(
        p_Path.c_str(),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );

    if (s_Handle == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }

    const auto s_Size = GetFinalPathNameByHandleW(s_Handle, nullptr, 0, VOLUME_NAME_DOS);

    if (s_Size == 0) {
        CloseHandle(s_Handle);
        return std::nullopt;
    }

    std::wstring s_FinalPath(s_Size, L'\0');
    const auto s_Length = GetFinalPathNameByHandleW(s_Handle, s_FinalPath.data(), s_Size, VOLUME_NAME_DOS);

    CloseHandle(s_Handle);

    if (s_Length == 0 || s_Length >= s_Size) {
        return std::nullopt;
    }

    s_FinalPath.resize(s_Length);

    return s_FinalPath;
}

bool ProcessUtils::IsRunningUnderWine() {
    const HMODULE s_Ntdll = GetModuleHandleW(L"ntdll.dll");
    return s_Ntdll && GetProcAddress(s_Ntdll, "wine_get_version");
}

std::optional<std::string> ProcessUtils::GetWineUnixPath(const std::filesystem::path& p_Path) {
    typedef char* (CDECL* wine_get_unix_file_name_t)(LPCWSTR p_DosPath);

    const auto wine_get_unix_file_name = reinterpret_cast<wine_get_unix_file_name_t>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "wine_get_unix_file_name")
    );

    if (!wine_get_unix_file_name) {
        return std::nullopt;
    }

    const auto s_UnixPath = wine_get_unix_file_name(p_Path.c_str());

    if (!s_UnixPath) {
        return std::nullopt;
    }

    std::string s_Result = s_UnixPath;
    HeapFree(GetProcessHeap(), 0, s_UnixPath);

    return s_Result;
}

std::optional<std::filesystem::path> ProcessUtils::GetWineDosPath(const std::string& p_UnixPath) {
    typedef WCHAR* (CDECL* wine_get_dos_file_name_t)(LPCSTR p_UnixPath);

    const auto wine_get_dos_file_name = reinterpret_cast<wine_get_dos_file_name_t>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "wine_get_dos_file_name")
    );

    if (!wine_get_dos_file_name) {
        return std::nullopt;
    }

    const auto s_DosPath = wine_get_dos_file_name(p_UnixPath.c_str());

    if (!s_DosPath) {
        return std::nullopt;
    }

    std::filesystem::path s_Result(s_DosPath);
    HeapFree(GetProcessHeap(), 0, s_DosPath);

    return s_Result;
}
