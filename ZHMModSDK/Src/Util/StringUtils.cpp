#include "Util/StringUtils.h"

#include <sstream>
#include <algorithm>

using namespace Util;

std::vector<std::string> StringUtils::Split(const std::string& p_String, const std::string& p_Delimeter) {
    std::vector<std::string> s_Parts;

    size_t s_PartStart = p_String.find_first_not_of(p_Delimeter);
    size_t s_PartEnd;

    while ((s_PartEnd = p_String.find_first_of(p_Delimeter, s_PartStart)) != std::string::npos) {
        s_Parts.push_back(p_String.substr(s_PartStart, s_PartEnd - s_PartStart));
        s_PartStart = p_String.find_first_not_of(p_Delimeter, s_PartEnd);
    }

    if (s_PartStart != std::string::npos)
        s_Parts.push_back(p_String.substr(s_PartStart));

    return s_Parts;
}

std::string StringUtils::ToLowerCase(const std::string& p_String) {
    std::string s_String = p_String;
    std::transform(s_String.begin(), s_String.end(), s_String.begin(), ::tolower);
    return s_String;
}

std::string StringUtils::ToUpperCase(const std::string& p_String) {
    std::string s_String = p_String;
    std::transform(s_String.begin(), s_String.end(), s_String.begin(), ::toupper);
    return s_String;
}

bool StringUtils::FindSubstring(const std::string& str, const std::string& substring, const bool bCaseSensitive) {
    if (substring.empty()) {
        return true;
    }

    const auto it = std::ranges::search(
                str, substring,
                [bCaseSensitive](const char ch1, const char ch2) {
                    if (bCaseSensitive) {
                        return ch1 == ch2;
                    }
                    return std::tolower(ch1) == std::tolower(ch2);
                }
            )
            .begin();
    return (it != str.end());
}