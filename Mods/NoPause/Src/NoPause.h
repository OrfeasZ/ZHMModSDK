#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

class NoPause : public IPluginInterface
{
public:
    void Init() override;

private:
    DECLARE_PLUGIN_DETOUR(NoPause, bool, GetOption, const ZString&, bool);
    DECLARE_PLUGIN_DETOUR(NoPause, LRESULT, WndProc, ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM);
};

DECLARE_ZHM_PLUGIN(NoPause)
