#pragma once

#include <cstring>

// These used to be `static`, which gives every translation unit that includes this header its
// own copy. Functions, hooks and globals are resolved in Functions.cpp, Hooks.cpp and
// Globals.cpp, but the threshold is checked in ModSDK.cpp, which always saw 0: the SDK never
// unloaded itself after a game update, and mods ended up calling null addresses.
// `inline` gives a single variable for the whole module. Everything here is a plain type,
// because Fail() runs during static initialization.

/// Number of failures encountered while setting up functions, hooks, and globals.
inline int g_Failures = 0;

/// Failures caused by a module that is not loaded at all (e.g. the EOS SDK on the Steam
/// version). Those are expected and do not mean the game was updated.
inline int g_MissingModuleFailures = 0;
inline bool g_NextFailureIsMissingModule = false;

/// Names of what could not be found, for the log.
inline char g_FailedNames[1024] = {};

inline void Fail(const char* p_Name = nullptr) {
    ++g_Failures;

    if (g_NextFailureIsMissingModule) {
        g_NextFailureIsMissingModule = false;
        ++g_MissingModuleFailures;
        return;
    }

    if (!p_Name) {
        return;
    }

    const size_t s_Used = strlen(g_FailedNames);
    const size_t s_Needed = strlen(p_Name) + (s_Used ? 2 : 0) + 1;

    if (s_Used + s_Needed > sizeof(g_FailedNames)) {
        return;
    }

    if (s_Used) {
        strcat_s(g_FailedNames, ", ");
    }

    strcat_s(g_FailedNames, p_Name);
}
