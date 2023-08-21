#pragma once

#include <vector>
#include <string>

namespace Util
{
    class StringUtils
    {
    public:
        static std::vector<std::string> Split(std::string_view p_String, const std::string& p_Delimeter);
        static std::string ToLowerCase(const std::string& p_String);
        static std::string ToUpperCase(const std::string& p_String);
        static bool CompareInsensitive(std::string_view p_StringA, std::string_view p_StringB);
    };
}
