#include <Windows.h>

#include <filesystem>
#include <string>
#include <iostream>

int main(int argc, char* argv[])
{
    // NOTE: This app assumes that it's launched from its build directory.
    // If it isn't then the copying steps will fail.

    std::string s_HitmanPathStr = "O:\\Games\\HITMAN3";
    std::string s_EpicUsername = "NoFaTe";
    std::string s_EpicUserId = "29438d6d19854c498ff2e7b77e2e5c07";

    // Get cmd-line args
    bool s_BadArgs = false;

    for (int i = 1; i < argc; ++i)
    {
        if (strlen(argv[i]) >= 1 && argv[i][0] == '-')
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Argument " << argv[i] << " has no value" << std::endl;
                s_BadArgs = true;
                break;
            }

            if (!strcmp(argv[i], "-hitman_path"))
            {
                s_HitmanPathStr = argv[i + 1];
            }
            else if (!strcmp(argv[i], "-epic_username"))
            {
                s_EpicUsername = argv[i + 1];
            }
            else if (!strcmp(argv[i], "-epic_userid"))
            {
                s_EpicUserId = argv[i + 1];
            }
            else
            {
                std::cerr << "Unknown argument " << argv[i] << std::endl;
                s_BadArgs = true;
            }

            // Increment i since we already read from the next argv
            ++i;
        }
    }

    if (s_BadArgs)
    {
        std::cerr << "Unable to start due to invalid arguments. Correct syntax: DevLoader.exe [-hitman_path <filepath>] [-epic_username <name>] [-epic_userid <id>]" << std::endl;
        return EXIT_FAILURE;
    }

    // Print what the arguments are set to, useful for debugging or showing that defaults were used but can be removed if unnecessary
    std::cout
        << "\tHitman 3 Path: " << s_HitmanPathStr << std::endl
        << "\tEpic Games Username: " << s_EpicUsername << std::endl
        << "\tEpic Games Userid: " << s_EpicUserId << std::endl;

    std::filesystem::path s_HitmanPath(s_HitmanPathStr);

    if (!is_directory(s_HitmanPath))
    {
        std::cerr << "Invalid Hitman 3 path: " << s_HitmanPathStr << std::endl
            << "  Set path to Hitman 3 with -hitman_path <path>" << std::endl;
        return EXIT_FAILURE;
    }

    char s_LoaderPathStr[MAX_PATH];
    auto s_PathSize = GetModuleFileNameA(nullptr, s_LoaderPathStr, MAX_PATH);

    if (s_PathSize == 0)
        return 1;

    std::filesystem::path s_LoaderPath(s_LoaderPathStr);
    auto s_LoaderDir = s_LoaderPath.parent_path();

    auto s_DinputPath = s_LoaderDir / "../../DirectInputProxy/dinput8.dll";
    auto s_ModSDKPath = s_LoaderDir / "../../ZHMModSDK/ZHMModSDK.dll";
    auto s_ModsPath = s_LoaderDir / "../../Mods";

    if (!is_regular_file(s_DinputPath) || !is_regular_file(s_ModSDKPath) || !is_directory(s_ModsPath))
        return 1;

    bool s_ShouldReload = false;

    // If the SDK is already loaded we must unload it.
    auto* s_UnloadEvent = OpenEventA(EVENT_MODIFY_STATE, false, "Global_ZHMSDK_Unload_Signal");

    if (s_UnloadEvent != nullptr)
    {
        s_ShouldReload = true;

        auto* s_UnloadedEvent = CreateEventA(nullptr, true, false, "GLOBAL_ZHMSDK_Unloaded_Signal");

        // Signal the SDK loader to unload the SDK module.
        SetEvent(s_UnloadEvent);

        // Wait until the SDK has unloaded.
        if (s_UnloadedEvent != nullptr)
        {
            WaitForSingleObject(s_UnloadedEvent, INFINITE);
            CloseHandle(s_UnloadedEvent);
        }

        CloseHandle(s_UnloadEvent);
    }

    // Copy over the SDK binary.
    copy(s_ModSDKPath, s_HitmanPath / "Retail/ZHMModSDK.dll", std::filesystem::copy_options::overwrite_existing);

    // If we just want to reload the SDK (because it was already loaded)
    // then we must not copy over dinput8.dll as it will be in-use.
    if (!s_ShouldReload)
        copy(s_DinputPath, s_HitmanPath / "Retail/dinput8.dll", std::filesystem::copy_options::overwrite_existing);

    // Copy over mods.
    create_directory(s_HitmanPath / "Retail/mods");

    for (const auto& s_Entry : std::filesystem::recursive_directory_iterator(s_ModsPath))
    {
        if (!s_Entry.is_regular_file())
            continue;

        if (s_Entry.path().extension() != ".dll")
            continue;

        copy(s_Entry.path(), s_HitmanPath / "Retail/mods/" / s_Entry.path().filename(), std::filesystem::copy_options::overwrite_existing);
    }

    if (s_ShouldReload)
    {
        // If we need to reload just signal the reload event and let the
        // SDK loader handle the rest.
        auto* s_ReloadEvent = OpenEventA(EVENT_MODIFY_STATE, false, "Global_ZHMSDK_Reload_Signal");

        if (s_ReloadEvent == nullptr)
            return 1;

        SetEvent(s_ReloadEvent);

        CloseHandle(s_ReloadEvent);
    }
    else
    {
        // Otherwise we need to start HITMAN3.exe.
        std::string s_ApplicationPath = absolute(s_HitmanPath / "Retail/HITMAN3.exe").string();
        std::string s_ApplicationDir = absolute(s_HitmanPath / "Retail").string();

        std::string s_CommandLine = "-epicapp=Eider -epicenv=Prod -EpicPortal -epicusername=" + s_EpicUsername + " -epicuserid=" + s_EpicUserId + " -epiclocale=en";

        STARTUPINFO s_StartupInfo {};
        s_StartupInfo.cb = sizeof(s_StartupInfo);

        PROCESS_INFORMATION s_ProcessInfo {};

        if (!CreateProcessA(
            s_ApplicationPath.c_str(),
            const_cast<char*>(s_CommandLine.c_str()),
            nullptr,
            nullptr,
            false,
            0,
            nullptr,
            s_ApplicationDir.c_str(),
            &s_StartupInfo,
            &s_ProcessInfo
            ))
        {
            return 1;
        }

        CloseHandle(s_ProcessInfo.hProcess);
        CloseHandle(s_ProcessInfo.hThread);
    }

    return 0;
}
