#pragma once

#include <memory>

#include "Common.h"

#include "spdlog/logger.h"
#include "spdlog/fmt/ostr.h"

#include "Glacier/ZString.h"

struct LoggerList
{
	spdlog::logger** Loggers;
	size_t Count;
};

ZHMSDK_API LoggerList GetLoggers();

class Logger
{
public:
	template <typename... Args>
	static void Error(const char* p_Format, const Args&... p_Args)
	{
		const auto s_Loggers = GetLoggers();

		for (size_t i = 0; i < s_Loggers.Count; ++i)
			s_Loggers.Loggers[i]->log(spdlog::level::err, p_Format, std::move(p_Args)...);
	}

	template <typename... Args>
	static void Warn(const char* p_Format, const Args&... p_Args)
	{
		const auto s_Loggers = GetLoggers();

		for (size_t i = 0; i < s_Loggers.Count; ++i)
			s_Loggers.Loggers[i]->log(spdlog::level::warn, p_Format, std::move(p_Args)...);
	}

	template <typename... Args>
	static void Info(const char* p_Format, const Args&... p_Args)
	{
		const auto s_Loggers = GetLoggers();

		for (size_t i = 0; i < s_Loggers.Count; ++i)
			s_Loggers.Loggers[i]->log(spdlog::level::info, p_Format, std::move(p_Args)...);
	}

	template <typename... Args>
	static void Debug(const char* p_Format, const Args&... p_Args)
	{
		const auto s_Loggers = GetLoggers();

		for (size_t i = 0; i < s_Loggers.Count; ++i)
			s_Loggers.Loggers[i]->log(spdlog::level::debug, p_Format, std::move(p_Args)...);
	}

	template <typename... Args>
	static void Trace(const char* p_Format, const Args&... p_Args)
	{
		const auto s_Loggers = GetLoggers();

		for (size_t i = 0; i < s_Loggers.Count; ++i)
			s_Loggers.Loggers[i]->log(spdlog::level::trace, p_Format, std::move(p_Args)...);
	}
};
