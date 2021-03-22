#include "Logging.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/dist_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

static std::vector<spdlog::logger*>* g_Loggers;

ZHMSDK_API LoggerList GetLoggers()
{
	if (g_Loggers == nullptr)
		g_Loggers = new std::vector<spdlog::logger*>();

	return LoggerList
	{
		g_Loggers->data(),
		g_Loggers->size(),
	};
}

void ClearLoggers()
{
	if (g_Loggers == nullptr)
		return;

	for (auto& s_Logger : *g_Loggers)
	{
		delete s_Logger;
	}

	g_Loggers->clear();

	delete g_Loggers;
	g_Loggers = nullptr;
}

void SetupLogging(spdlog::level::level_enum p_LogLevel)
{
	ClearLoggers();

	if (g_Loggers == nullptr)
		g_Loggers = new std::vector<spdlog::logger*>();

	auto s_ConsoleDistSink = std::make_shared<spdlog::sinks::dist_sink_mt>();
	auto s_StdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

	s_ConsoleDistSink->add_sink(s_StdoutSink);

	auto s_ConsoleLogger = new spdlog::logger("con", s_ConsoleDistSink);

	s_ConsoleLogger->set_level(p_LogLevel);
	s_ConsoleLogger->set_pattern("%v");

	g_Loggers->push_back(s_ConsoleLogger);

	//////////////////////////////////////////////////////////////////////////

	auto s_FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("ZHMModLoader.log", true);

	auto s_FileLogger = new spdlog::logger("file", s_FileSink);

	s_FileLogger->set_level(p_LogLevel);
	s_FileLogger->set_pattern("%v");

	g_Loggers->push_back(s_FileLogger);
}

void FlushLoggers()
{
	if (g_Loggers == nullptr)
		return;

	for (auto& s_Logger : *g_Loggers)
	{
		s_Logger->flush();
	}
}
