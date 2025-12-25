#pragma once

#include "Glacier/ZString.h"

namespace Util {
    class ResourceUtils {
    public:
        static bool TryParseChunkIndexFromResourcePackagePath(const ZString& s_ResourcePackagePath, uint32_t& p_OutChunkIndex) {
            if (s_ResourcePackagePath.size() < 2 ||
                s_ResourcePackagePath[0] != '.' ||
                s_ResourcePackagePath[1] != '.') {
                return false;
            }

            size_t s_Position = std::string(s_ResourcePackagePath.c_str(), s_ResourcePackagePath.size()).find("chunk");

            if (s_Position == std::string::npos) {
                return false;
            }

            s_Position += 5;

            if (s_Position >= s_ResourcePackagePath.size() ||
                !std::isdigit(static_cast<unsigned char>(s_ResourcePackagePath[s_Position]))) {
                return false;
            }

            uint32_t s_Value = 0;

            while (s_Position < s_ResourcePackagePath.size() &&
                std::isdigit(static_cast<unsigned char>(s_ResourcePackagePath[s_Position]))) {
                s_Value = s_Value * 10 + (s_ResourcePackagePath[s_Position] - '0');
                ++s_Position;
            }

            p_OutChunkIndex = s_Value;

            return true;
        }

        static bool TryParseChunkIndexFromResourcePackageFileName(const std::string& p_FileName, uint32_t& p_OutChunkIndex) {
            if (!p_FileName.starts_with("chunk")) {
                return false;
            }

            uint32_t s_Value = 0;
            bool s_IsDigitFound = false;

            for (size_t i = 5; i < p_FileName.size(); ++i) {
                if (!std::isdigit(static_cast<unsigned char>(p_FileName[i]))) {
                    break;
                }

                s_IsDigitFound = true;
                s_Value = s_Value * 10 + (p_FileName[i] - '0');
            }

            if (!s_IsDigitFound) {
                return false;
            }

            p_OutChunkIndex = s_Value;

            return true;
        }
    };
}