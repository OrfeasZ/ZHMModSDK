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

        static bool FindSubstringUTF8(
            const std::string& p_String, const std::string& p_SubString, bool p_CaseSensitive = false
        ) {
            if (p_SubString.empty()) {
                return true;
            }

            std::u32string s_String = UTF8ToUTF32(p_String);
            std::u32string s_SubString = UTF8ToUTF32(p_SubString);

            auto normalize = [&](std::u32string& s) {
                for (auto& c : s) {
                    if (!p_CaseSensitive) {
                        c = std::tolower(c);
                    }

                    c = StripDiacritic(c);
                }
            };

            normalize(s_String);
            normalize(s_SubString);

            return std::ranges::search(s_String, s_SubString).begin() != s_String.end();
        }

        static bool EndsWith(const std::string& p_String, const std::string& p_Suffix) {
            return p_String.size() >= p_Suffix.size() &&
                    p_String.compare(p_String.size() - p_Suffix.size(), p_Suffix.size(), p_Suffix) == 0;
        }

        static bool StartsWith(const std::string& p_String, const std::string& p_Prefix) {
            return p_String.size() >= p_Prefix.size() &&
                    p_String.compare(0, p_Prefix.size(), p_Prefix) == 0;
        }

    private:
        static uint32_t DecodeUTF8(const char*& p) {
            const uint8_t c = *p;

            if (c < 0x80) {
                return *p++;
            }
            else if ((c >> 5) == 0x6) {
                const uint32_t s_CodePoint = ((c & 0x1F) << 6) |
                    (p[1] & 0x3F);

                p += 2;

                return s_CodePoint;
            }
            else if ((c >> 4) == 0xE) {
                const uint32_t s_CodePoint = ((c & 0x0F) << 12) |
                    ((p[1] & 0x3F) << 6) |
                    (p[2] & 0x3F);

                p += 3;

                return s_CodePoint;
            }
            else if ((c >> 3) == 0x1E) {
                const uint32_t s_CodePoint = ((c & 0x07) << 18) |
                    ((p[1] & 0x3F) << 12) |
                    ((p[2] & 0x3F) << 6) |
                    (p[3] & 0x3F);

                p += 4;

                return s_CodePoint;
            }

            p++;

            return '?';
        }

        static std::u32string UTF8ToUTF32(const std::string& p_UTF8) {
            std::u32string s_Out;
            const char* p = p_UTF8.data();
            const char* s_End = p + p_UTF8.size();

            while (p < s_End) {
                uint32_t s_Code = DecodeUTF8(p);

                s_Out.push_back((char32_t)s_Code);
            }

            return s_Out;
        }

        static char32_t StripDiacritic(char32_t c) {
            switch (c) {
                case U'á': case U'à': case U'ä': case U'â': case U'ã': case U'å': case U'ā': case U'ă': case U'ą':
                case U'Á': case U'À': case U'Ä': case U'Â': case U'Ã': case U'Å': case U'Ā': case U'Ă': case U'Ą':
                    return U'a';

                case U'é': case U'è': case U'ë': case U'ê': case U'ē': case U'ė': case U'ę':
                case U'É': case U'È': case U'Ë': case U'Ê': case U'Ē': case U'Ė': case U'Ę':
                    return U'e';

                case U'í': case U'ì': case U'ï': case U'î': case U'ī':
                case U'Í': case U'Ì': case U'Ï': case U'Î': case U'Ī':
                    return U'i';

                case U'ó': case U'ò': case U'ö': case U'ô': case U'õ': case U'ø': case U'ō':
                case U'Ó': case U'Ò': case U'Ö': case U'Ô': case U'Õ': case U'Ø': case U'Ō':
                    return U'o';

                case U'ú': case U'ù': case U'ü': case U'û': case U'ū':
                case U'Ú': case U'Ù': case U'Ü': case U'Û': case U'Ū':
                    return U'u';

                case U'ý': case U'ÿ': case U'Ý':
                    return U'y';

                case U'ç': case U'ć': case U'č': case U'Ç': case U'Ć': case U'Č':
                    return U'c';

                case U'ñ': case U'Ñ':
                    return U'n';

                case U'š': case U'Š':
                    return U's';

                case U'ž': case U'Ž':
                    return U'z';
            }

            return c;
        }
    };
}