#include "Logging.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/dist_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/msvc_sink.h"

#include <ModSDK.h>
#include <UI/Console.h>

static std::vector<spdlog::logger*>* g_Loggers;

template <class Mutex>
class ConsoleSink : public spdlog::sinks::base_sink<Mutex>
{
    using MyType = ConsoleSink<Mutex>;

public:
    ConsoleSink()
    {
    }

    static std::shared_ptr<MyType> instance()
    {
        static std::shared_ptr<MyType> instance = std::make_shared<MyType>();
        return instance;
    }

    void sink_it_(const spdlog::details::log_msg& p_Message) override
    {
        spdlog::memory_buf_t s_Formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(p_Message, s_Formatted);

        ModSDK::GetInstance()->GetUIConsole()->AddLogLine(p_Message.level, ZString(s_Formatted.data(), s_Formatted.size()));
    }

    void flush_() override
    {
    }
};

typedef ConsoleSink<spdlog::details::null_mutex> ConsoleSink_st;
typedef ConsoleSink<std::mutex> ConsoleSink_mt;

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
    auto s_UiConsoleSink = std::make_shared<ConsoleSink_mt>();

#if _DEBUG
    auto s_DebugSink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    s_ConsoleDistSink->add_sink(s_DebugSink);
#endif

    s_ConsoleDistSink->add_sink(s_StdoutSink);
    s_ConsoleDistSink->add_sink(s_UiConsoleSink);

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
