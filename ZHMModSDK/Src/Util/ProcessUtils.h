#pragma once

#include <Windows.h>
#include <tuple>
#include <string>
#include <cstdint>
#include <filesystem>
#include <optional>

namespace Util {
    class ProcessUtils {
    public:
        static uintptr_t GetBaseOfCode(HMODULE p_Module);
        static uint32_t GetSizeOfCode(HMODULE p_Module);
        static uint32_t GetSizeOfImage(HMODULE p_Module);
        static uintptr_t SearchPattern(
            uintptr_t p_BaseAddress, size_t p_ScanSize, const uint8_t* p_Pattern, const char* p_Mask
        );
        static std::tuple<uintptr_t, uintptr_t> GetSectionStartAndEnd(
            HMODULE p_Module, const std::string& p_SectionName
        );
        static uintptr_t GetRelativeAddr(uintptr_t p_Base, int32_t p_Offset);

        /**
         * Gets the value of an environment variable of the current process.
         * @param p_Name The name of the variable.
         * @return The value, or an empty string if the variable is not set or is empty.
         */
        static std::wstring GetEnvironmentVariableValue(const wchar_t* p_Name);

        /**
         * Gets the folder that contains the executable of the current process.
         * @return The folder, or std::nullopt if the executable path could not be read.
         */
        static std::optional<std::filesystem::path> GetExecutableDirectory();

        /**
         * Resolves a file or folder to its real path, following symlinks and junctions and using the casing
         * stored on disk. The resulting path is in the verbatim form (e.g. \\?\C:\Games\HITMAN3\Retail).
         * @param p_Path The path of an existing file or folder.
         * @return The real path, or std::nullopt if the path could not be opened or resolved.
         */
        static std::optional<std::wstring> GetRealPath(const std::filesystem::path& p_Path);

        /**
         * Checks if the current process is running under Wine (or Proton).
         */
        static bool IsRunningUnderWine();

        /**
         * Converts a Windows path to a Unix path. Symlinks are not resolved.
         * @param p_Path The Windows path.
         * @return The UTF-8 Unix path, or std::nullopt if it could not be converted.
         */
        static std::optional<std::string> GetWineUnixPath(const std::filesystem::path& p_Path);

        /**
         * Converts a Unix path to a Windows path.
         * @param p_UnixPath The UTF-8 Unix path.
         * @return The Windows path (e.g. Z:\home\user, \\?\unix\home\user), or std::nullopt if it couldn't be converted.
         */
        static std::optional<std::filesystem::path> GetWineDosPath(const std::string& p_UnixPath);
    };
}