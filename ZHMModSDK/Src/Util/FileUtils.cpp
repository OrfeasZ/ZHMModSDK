#include "FileUtils.h"
#include <Windows.h>

using namespace Util;

std::filesystem::path FileUtils::GetExecutablePath()
{
    char s_ExePathStr[MAX_PATH];
    auto s_PathSize = GetModuleFileNameA(nullptr, s_ExePathStr, MAX_PATH);

    if (s_PathSize == 0)
        return {};

    return std::filesystem::path(s_ExePathStr);
}