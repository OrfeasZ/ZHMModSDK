#pragma once

#include "Hooks.h"

class IPluginInterface
{
public:
	virtual ~IPluginInterface() {}
	virtual void Init() = 0;
};

typedef IPluginInterface* (__cdecl* GetPluginInterface_t)();

#define DEFINE_ZHM_PLUGIN(PluginClass) extern "C" __declspec(dllexport) IPluginInterface* GetPluginInterface();\
	\
	inline PluginClass* Plugin()\
	{\
		return reinterpret_cast<PluginClass*>(GetPluginInterface());\
	}

#define DECLARE_ZHM_PLUGIN(PluginClass) \
	static IPluginInterface* g_ ## PluginClass ## _Instance = nullptr;\
	\
	extern "C" __declspec(dllexport) IPluginInterface* GetPluginInterface()\
	{\
		if (g_ ## PluginClass ## _Instance == nullptr)\
			g_ ## PluginClass ## _Instance = new PluginClass();\
		\
		return g_ ## PluginClass ## _Instance;\
	}

#define DEFINE_PLUGIN_DETOUR(PluginClass, ReturnType, DetourName, ...) \
	template <class... Args>\
	static HookResult<ReturnType> DetourName(IPluginInterface* th, Args... p_Args)\
	{\
		return reinterpret_cast<PluginClass*>(th)->DetourName ## _Internal(p_Args...);\
	}\
	\
	HookResult<ReturnType> DetourName ## _Internal(Hook<ReturnType(__VA_ARGS__)>* p_Hook, __VA_ARGS__);

#define DECLARE_PLUGIN_DETOUR(PluginClass, ReturnType, DetourName, ...) \
	HookResult<ReturnType> PluginClass::DetourName ## _Internal(Hook<ReturnType(__VA_ARGS__)>* p_Hook, __VA_ARGS__)