#pragma once

#include <filesystem>

namespace Util
{
    class FileUtils
    {
    public:
        static std::filesystem::path GetExecutablePath();
    };
}