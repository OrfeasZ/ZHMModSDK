#pragma once

#include "Glacier/ZString.h"

namespace Util {
    class ResourceUtils {
    public:
        /**
         * Attempts to extract a chunk index from a mounted resource package path
         * (e.g. "../runtime//chunk3patch3.rpkg").
         *
         * Used for entries stored in ResourceContainer::m_MountedPackages.
         *
         * @param s_ResourcePackagePath Resource package path from mounted packages.
         * @return Chunk index if found, std::nullopt otherwise.
         */
        static std::optional<uint32_t> TryParseChunkIndexFromResourcePackagePath(
            const ZString& s_ResourcePackagePath
        ) {
            if (s_ResourcePackagePath.size() < 2 ||
                s_ResourcePackagePath[0] != '.' ||
                s_ResourcePackagePath[1] != '.') {
                return std::nullopt;
            }

            size_t s_Position = std::string(s_ResourcePackagePath.c_str(), s_ResourcePackagePath.size()).find("chunk");

            if (s_Position == std::string::npos) {
                return std::nullopt;
            }

            s_Position += 5;

            if (s_Position >= s_ResourcePackagePath.size() ||
                !std::isdigit(static_cast<unsigned char>(s_ResourcePackagePath[s_Position]))) {
                return std::nullopt;
            }

            uint32_t s_Value = 0;

            while (s_Position < s_ResourcePackagePath.size() &&
                std::isdigit(static_cast<unsigned char>(s_ResourcePackagePath[s_Position]))) {
                s_Value = s_Value * 10 + (s_ResourcePackagePath[s_Position] - '0');
                ++s_Position;
            }

            return s_Value;
        }

        /**
         * Attempts to extract a chunk index from a resource package file name
         * (e.g. "chunk3patch3.rpkg").
         *
         * @param p_FileName Resource package file name.
         * @return Chunk index if found, std::nullopt otherwise.
         */
        static std::optional<uint32_t> TryParseChunkIndexFromResourcePackageFileName(
            const std::string& p_FileName
        ) {
            if (!p_FileName.starts_with("chunk")) {
                return std::nullopt;
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
                return std::nullopt;
            }

            return s_Value;
        }
    };
}