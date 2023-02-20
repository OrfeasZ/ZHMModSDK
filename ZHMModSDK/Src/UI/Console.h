#pragma once

#include <Windows.h>
#include <vector>
#include <spdlog/spdlog.h>
#include <Glacier/ZString.h>

namespace UI
{
    class Console
    {
    private:
        struct LogLine
        {
            spdlog::level::level_enum Level;
            std::string Text;
        };

    public:
        Console();

    public:
        void Draw(bool p_HasFocus);
        void AddLogLine(spdlog::level::level_enum p_Level, const ZString& p_Text);

    private:
        std::vector<LogLine> m_LogLines {};
        SRWLOCK m_Lock {};
    };
}
