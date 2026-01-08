#pragma once

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include <Windows.h>
#include <vector>
#include <spdlog/spdlog.h>
#include <Glacier/ZString.h>

namespace UI {
    class Console {
    private:
        struct LogLine {
            spdlog::level::level_enum Level;
            std::string Text;
        };

    public:
        Console();

    public:
        void Draw(bool p_HasFocus);
        void AddLogLine(spdlog::level::level_enum p_Level, const std::string& p_Text);

    private:
        std::vector<LogLine> m_LogLines {};
        SRWLOCK m_Lock {};
    };
}