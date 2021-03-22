#pragma once

#include "Hooks.h"
#include "IModSDK.h"

class IPluginInterface
{
public:
	virtual ~IPluginInterface() {}
	virtual void Init() = 0;
	virtual void OnEngineInitialized() {}
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

#define DEFINE_PLUGIN_DETOUR(PluginClass, ReturnType, DetourName, ...) DEFINE_DETOUR_WITH_CONTEXT(PluginClass, ReturnType, DetourName, __VA_ARGS__)

#define DECLARE_PLUGIN_DETOUR(PluginClass, ReturnType, DetourName, ...) DECLARE_DETOUR_WITH_CONTEXT(PluginClass, ReturnType, DetourName, __VA_ARGS__)

#define DEFINE_PLUGIN_LISTENER(PluginClass, EventName, ...) \
	template <class... Args>\
	static void EventName(void* th, Args... p_Args)\
	{\
		return reinterpret_cast<PluginClass*>(th)->EventName ## _Internal(p_Args...);\
	}\
	\
	void EventName ## _Internal(__VA_ARGS__);

#define DECLARE_PLUGIN_LISTENER(PluginClass, EventName, ...) \
	void PluginClass::EventName ## _Internal(__VA_ARGS__)
