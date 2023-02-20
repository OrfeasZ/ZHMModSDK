#include "DebugConsole.h"

#include <iostream>
#include <Windows.h>
#include <io.h>

#include "Events.h"
#include "Logging.h"
#include "ModSDK.h"
#include "ModLoader.h"
#include "spdlog/common.h"
#include "Util/StringUtils.h"

#if _DEBUG
DebugConsole::DebugConsole() :
    m_Running(true),
    m_Redirected(false)
{
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    SetConsoleTitleA("ZHM Mod SDK - Debug Console");

    StartRedirecting();

    m_InputThread = std::thread([&]
        {
            std::string s_ReadLine;

            while (m_Running)
            {
                std::getline(std::cin, s_ReadLine);

                if (s_ReadLine.size() == 0)
                    continue;

                auto s_Parts = Util::StringUtils::Split(s_ReadLine, " ");

                if (s_Parts.size() == 1)
                {
                    if (s_Parts[0] == "unloadall")
                    {
                        ModSDK::GetInstance()->GetModLoader()->UnloadAllMods();
                    }
                    else if (s_Parts[0] == "reloadall")
                    {
                        ModSDK::GetInstance()->GetModLoader()->ReloadAllMods();
                    }
                }
                if (s_Parts.size() == 2)
                {
                    if (s_Parts[0] == "load")
                    {
                        ModSDK::GetInstance()->GetModLoader()->LoadMod(s_Parts[1], true);
                    }
                    else if (s_Parts[0] == "unload")
                    {
                        ModSDK::GetInstance()->GetModLoader()->UnloadMod(s_Parts[1]);
                    }
                    else if (s_Parts[1] == "reload")
                    {
                        ModSDK::GetInstance()->GetModLoader()->ReloadMod(s_Parts[1]);
                    }
                }

                Events::OnConsoleCommand->Call();
            }
        });
}

DebugConsole::~DebugConsole()
{
    m_Running = false;

    // Send a key event to the console so getline unblocks.
    INPUT_RECORD s_Inputs[2];

    s_Inputs[0].EventType = KEY_EVENT;
    s_Inputs[0].Event.KeyEvent.bKeyDown = TRUE;
    s_Inputs[0].Event.KeyEvent.dwControlKeyState = 0;
    s_Inputs[0].Event.KeyEvent.uChar.AsciiChar = VK_RETURN;
    s_Inputs[0].Event.KeyEvent.wRepeatCount = 1;
    s_Inputs[0].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
    s_Inputs[0].Event.KeyEvent.wVirtualScanCode = MapVirtualKeyA(VK_RETURN, MAPVK_VK_TO_VSC);

    s_Inputs[1] = s_Inputs[0];
    s_Inputs[1].Event.KeyEvent.bKeyDown = FALSE;

    DWORD s_Written;
    WriteConsoleInputA(GetStdHandle(STD_INPUT_HANDLE), s_Inputs, 2, &s_Written);

    m_InputThread.detach();

    StopRedirecting();

    FreeConsole();
}

void DebugConsole::StartRedirecting()
{
    if (m_Redirected)
        StopRedirecting();

    m_Redirected = true;

    m_OriginalStdin = _dup(0);
    m_OriginalStdout = _dup(1);
    m_OriginalStderr = _dup(2);

    FILE* s_Con;
    freopen_s(&s_Con, "CONIN$", "r", stdin);
    freopen_s(&s_Con, "CONOUT$", "w", stderr);
    freopen_s(&s_Con, "CONOUT$", "w", stdout);

    SetConsoleOutputCP(CP_UTF8);
}

void DebugConsole::StopRedirecting()
{
    if (!m_Redirected)
        return;

    m_Redirected = false;

    _dup2(m_OriginalStdin, 0);
    _dup2(m_OriginalStdout, 1);
    _dup2(m_OriginalStderr, 2);
}
#endif
