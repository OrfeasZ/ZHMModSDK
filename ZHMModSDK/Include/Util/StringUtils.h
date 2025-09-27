#pragma once

#include <vector>
#include <string>

#include "Common.h"

namespace Util {
    class StringUtils {
    public:
        static std::vector<std::string> Split(const std::string& p_String, const std::string& p_Delimeter);
        static std::string ToLowerCase(const std::string& p_String);
        static std::string ToUpperCase(const std::string& p_String);
        ZHMSDK_API static bool FindSubstring(
            const std::string& str, const std::string& substring, const bool bCaseSensitive = false
        );
    };
}