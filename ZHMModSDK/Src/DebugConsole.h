#pragma once
#include <thread>

#if _DEBUG
class DebugConsole
{
public:
    DebugConsole();
    ~DebugConsole();

public:
    void StartRedirecting();

private:
    void StopRedirecting();

private:
    std::thread m_InputThread;
    volatile bool m_Running;

    int m_OriginalStdin;
    int m_OriginalStdout;
    int m_OriginalStderr;

    bool m_Redirected;
};
#endif
