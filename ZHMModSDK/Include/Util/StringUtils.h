#pragma once

#include <vector>
#include <string>
#include <algorithm>

#include "Common.h"

namespace Util {
    class StringUtils {
    public:
        static std::vector<std::string> Split(const std::string& p_String, const std::string& p_Delimeter) {
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

        static std::string ToLowerCase(const std::string& p_String) {
            std::string s_String = p_String;
            std::transform(s_String.begin(), s_String.end(), s_String.begin(), ::tolower);
            return s_String;
        }

        static std::string ToUpperCase(const std::string& p_String) {
            std::string s_String = p_String;
            std::transform(s_String.begin(), s_String.end(), s_String.begin(), ::toupper);
            return s_String;
        }

        static bool FindSubstring(
            const std::string& p_String, const std::string& p_SubString, const bool p_CaseSensitive = false
        ) {
            if (p_SubString.empty()) {
                return true;
            }

            const auto it = std::ranges::search(
                        p_String, p_SubString,
                        [p_CaseSensitive](const char ch1, const char ch2) {
                            if (p_CaseSensitive) {
                                return ch1 == ch2;
                            }
                            return std::tolower(ch1) == std::tolower(ch2);
                        }
                    )
                    .begin();

            return (it != p_String.end());
        }

        static bool EndsWith(const std::string& p_String, const std::string& p_Suffix) {
            return p_String.size() >= p_Suffix.size() &&
                    p_String.compare(p_String.size() - p_Suffix.size(), p_Suffix.size(), p_Suffix) == 0;
        }

        static bool StartsWith(const std::string& p_String, const std::string& p_Prefix) {
            return p_String.size() >= p_Prefix.size() &&
                    p_String.compare(0, p_Prefix.size(), p_Prefix) == 0;
        }
    };
}