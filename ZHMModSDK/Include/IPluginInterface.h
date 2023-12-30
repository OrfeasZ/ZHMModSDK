#pragma once

#include "Hooks.h"
#include "IModSDK.h"
#include "IRenderer.h"

class IPluginInterface
{
public:
    virtual ~IPluginInterface() = default;

private:
    virtual void SetupUI()
    {
        auto* s_Context = SDK()->GetImGuiContext();

        if (!s_Context)
            return;

        ImGui::SetCurrentContext(s_Context);
        ImGui::SetAllocatorFunctions(SDK()->GetImGuiAlloc(), SDK()->GetImGuiFree(), SDK()->GetImGuiAllocatorUserData());
    }

public:
    virtual void Init() {}
    virtual void OnEngineInitialized() {}
    virtual void OnDrawUI(bool p_HasFocus) {}
    virtual void OnDraw3D(IRenderer* p_Renderer) {}
    virtual void OnDrawMenu() {}
	
public:
	/**
     * Set a setting string value for the given name.
     * @param p_Section The section of the setting in the INI file.
     * @param p_Name The name of the setting.
     * @param p_Value The value of the setting.
     */
	void SetSetting(const ZString& p_Section, const ZString& p_Name, const ZString& p_Value) {
		SDK()->SetPluginSetting(this, p_Section, p_Name, p_Value);
	}

	/**
	 * Set a setting integer value for the given name.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_Value The value of the setting.
	 */
	void SetSettingInt(const ZString& p_Section, const ZString& p_Name, int64 p_Value) {
		SDK()->SetPluginSettingInt(this, p_Section, p_Name, p_Value);
	}

	/**
	 * Set a setting unsigned integer value for the given name.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_Value The value of the setting.
	 */
	void SetSettingUInt(const ZString& p_Section, const ZString& p_Name, uint64 p_Value) {
		SDK()->SetPluginSettingUInt(this, p_Section, p_Name, p_Value);
	}

	/**
	 * Set a setting double value for the given name.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_Value The value of the setting.
	 */
	void SetSettingDouble(const ZString& p_Section, const ZString& p_Name, double p_Value) {
		SDK()->SetPluginSettingDouble(this, p_Section, p_Name, p_Value);
	}

	/**
	 * Set a setting boolean value for the given name.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_Value The value of the setting.
	 */
	void SetSettingBool(const ZString& p_Section, const ZString& p_Name, bool p_Value) {
		SDK()->SetPluginSettingBool(this, p_Section, p_Name, p_Value);
	}

	/**
	 * Get a setting string value for the given name.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_DefaultValue The default value to return if the setting does not exist.
	 * @return The value of the setting, or the default value if the setting does not exist.
	 */
	ZString GetSetting(const ZString& p_Section, const ZString& p_Name, const ZString& p_DefaultValue) {
		return SDK()->GetPluginSetting(this, p_Section, p_Name, p_DefaultValue);
	}

	/**
	 * Get a setting integer value for the given name.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_DefaultValue The default value to return if the setting does not exist or is not an integer.
	 * @return The value of the setting, or the default value if the setting does not exist or is not an integer.
	 */
	int64 GetSettingInt(const ZString& p_Section, const ZString& p_Name, int64 p_DefaultValue) {
		return SDK()->GetPluginSettingInt(this, p_Section, p_Name, p_DefaultValue);
	}

	/**
	 * Get a setting unsigned integer value for the given name.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_DefaultValue The default value to return if the setting does not exist or is not an unsigned integer.
	 * @return The value of the setting, or the default value if the setting does not exist or is not an unsigned integer.
	 */
	uint64 GetSettingUInt(const ZString& p_Section, const ZString& p_Name, uint64 p_DefaultValue) {
		return SDK()->GetPluginSettingUInt(this, p_Section, p_Name, p_DefaultValue);
	}

	/**
	 * Get a setting double value for the given name.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_DefaultValue The default value to return if the setting does not exist or is not a double.
	 * @return The value of the setting, or the default value if the setting does not exist or is not a double.
	 */
	double GetSettingDouble(const ZString& p_Section, const ZString& p_Name, double p_DefaultValue) {
		return SDK()->GetPluginSettingDouble(this, p_Section, p_Name, p_DefaultValue);
	}

	/**
	 * Get a setting boolean value for the given name.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_DefaultValue The default value to return if the setting does not exist or is not a boolean.
	 * @return The value of the setting, or the default value if the setting does not exist or is not a boolean.
	 */
	bool GetSettingBool(const ZString& p_Section, const ZString& p_Name, bool p_DefaultValue) {
		return SDK()->GetPluginSettingBool(this, p_Section, p_Name, p_DefaultValue);
	}

	/**
	 * Check if a setting with the given name exists.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @return True if the setting exists, false otherwise.
	 */
	bool HasSetting(const ZString& p_Section, const ZString& p_Name) {
		return SDK()->HasPluginSetting(this, p_Section, p_Name);
	}

	/**
	 * Remove a setting with the given name.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 */
	void RemoveSetting(const ZString& p_Section, const ZString& p_Name) {
		SDK()->RemovePluginSetting(this, p_Section, p_Name);
	}

	/**
	 * Reload the settings for the current plugin.
	 */
	void ReloadSettings() {
		SDK()->ReloadPluginSettings(this);
	}

    friend class ModSDK;
};

typedef IPluginInterface* (__cdecl* GetPluginInterface_t)();

#define DECLARE_ZHM_PLUGIN(PluginClass) extern "C" __declspec(dllexport) IPluginInterface* GetPluginInterface();\
    \
    inline PluginClass* Plugin()\
    {\
        return reinterpret_cast<PluginClass*>(GetPluginInterface());\
    }

#define DEFINE_ZHM_PLUGIN(PluginClass) \
    static IPluginInterface* g_ ## PluginClass ## _Instance = nullptr;\
    \
    extern "C" __declspec(dllexport) IPluginInterface* GetPluginInterface()\
    {\
        if (g_ ## PluginClass ## _Instance == nullptr)\
            g_ ## PluginClass ## _Instance = new PluginClass();\
        \
        return g_ ## PluginClass ## _Instance;\
    }

#define DECLARE_PLUGIN_DETOUR(PluginClass, ReturnType, DetourName, ...) DECLARE_DETOUR_WITH_CONTEXT(PluginClass, ReturnType, DetourName, __VA_ARGS__)

#define DEFINE_PLUGIN_DETOUR(PluginClass, ReturnType, DetourName, ...) DEFINE_DETOUR_WITH_CONTEXT(PluginClass, ReturnType, DetourName, __VA_ARGS__)

#define DECLARE_PLUGIN_LISTENER(PluginClass, EventName, ...) \
    template <class... Args>\
    static void EventName(void* th, Args... p_Args)\
    {\
        return reinterpret_cast<PluginClass*>(th)->EventName ## _Internal(p_Args...);\
    }\
    \
    void EventName ## _Internal(__VA_ARGS__);

#define DEFINE_PLUGIN_LISTENER(PluginClass, EventName, ...) \
    void PluginClass::EventName ## _Internal(__VA_ARGS__)
