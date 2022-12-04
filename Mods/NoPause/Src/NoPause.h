#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

class NoPause : public IPluginInterface
{
public:
	void Init() override;

private:
	DEFINE_PLUGIN_DETOUR(NoPause, bool, GetOption, const ZString&, bool);
	DEFINE_PLUGIN_DETOUR(NoPause, LRESULT, WndProc, ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM);
};

DEFINE_ZHM_PLUGIN(NoPause)
