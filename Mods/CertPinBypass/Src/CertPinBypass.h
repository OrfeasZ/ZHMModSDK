#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

class CertPinBypass : public IPluginInterface
{
public:
	void Init() override;

private:
	DEFINE_PLUGIN_DETOUR(CertPinBypass, bool, On_Check_SSL_Cert, void*, void*);
};

DEFINE_ZHM_PLUGIN(CertPinBypass)
