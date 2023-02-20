#pragma once

#include <memory>

#include "Common.h"

#include "spdlog/spdlog.h"

#include "Glacier/ZString.h"

struct LoggerList
{
    spdlog::logger** Loggers;
    size_t Count;
};

ZHMSDK_API LoggerList GetLoggers();

namespace Logger
{
    template <typename... Args>
    inline void Error(spdlog::format_string_t<Args...> p_Format, const Args&... p_Args)
    {
        const auto s_Loggers = GetLoggers();

        for (size_t i = 0; i < s_Loggers.Count; ++i)
            s_Loggers.Loggers[i]->error(fmt::runtime(p_Format), p_Args...);
    }

    template <typename... Args>
    inline void Warn(spdlog::format_string_t<Args...> p_Format, const Args&... p_Args)
    {
        const auto s_Loggers = GetLoggers();

        for (size_t i = 0; i < s_Loggers.Count; ++i)
            s_Loggers.Loggers[i]->warn(fmt::runtime(p_Format), p_Args...);
    }

    template <typename... Args>
    inline void Info(spdlog::format_string_t<Args...> p_Format, const Args&... p_Args)
    {
        const auto s_Loggers = GetLoggers();

        for (size_t i = 0; i < s_Loggers.Count; ++i)
            s_Loggers.Loggers[i]->info(fmt::runtime(p_Format), p_Args...);
    }

    template <typename... Args>
    inline void Debug(spdlog::format_string_t<Args...> p_Format, const Args&... p_Args)
    {
        const auto s_Loggers = GetLoggers();

        for (size_t i = 0; i < s_Loggers.Count; ++i)
            s_Loggers.Loggers[i]->debug(fmt::runtime(p_Format), p_Args...);
    }

    template <typename... Args>
    inline void Trace(spdlog::format_string_t<Args...> p_Format, const Args&... p_Args)
    {
        const auto s_Loggers = GetLoggers();

        for (size_t i = 0; i < s_Loggers.Count; ++i)
            s_Loggers.Loggers[i]->trace(fmt::runtime(p_Format), p_Args...);
    }
};
