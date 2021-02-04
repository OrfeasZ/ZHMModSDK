#pragma once

#include <string>

class IPluginInterface
{
public:
	virtual ~IPluginInterface() {}
	virtual void Init() = 0;
};

typedef IPluginInterface* (__cdecl* GetPluginInterface_t)();

#define DECLARE_ZHM_PLUGIN(PluginClass) extern "C" __declspec(dllexport) IPluginInterface* GetPluginInterface()\
	{\
		return new PluginClass();\
	}