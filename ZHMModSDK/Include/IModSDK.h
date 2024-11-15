#pragma once

#include "Glacier/ZPrimitives.h"
#include "Common.h"
#include "imgui.h"

class IPluginInterface;
class ZRenderDestination;
class SVector2;
class SVector3;

class IModSDK
{
public:
    /**
     * Make the SDK receive focus.
     * This will prevent the user from interacting with the game
     * and will allow the SDK to receive input.
     */
    virtual void RequestUIFocus() = 0;

    /**
     * Release the focus from the SDK.
     * This will allow the user to interact with the game again.
     */
    virtual void ReleaseUIFocus() = 0;


    virtual ImGuiContext* GetImGuiContext() = 0;
    virtual ImGuiMemAllocFunc GetImGuiAlloc() = 0;
    virtual ImGuiMemFreeFunc GetImGuiFree() = 0;
    virtual void* GetImGuiAllocatorUserData() = 0;
    virtual ImFont* GetImGuiLightFont() = 0;
    virtual ImFont* GetImGuiRegularFont() = 0;
    virtual ImFont* GetImGuiMediumFont() = 0;
    virtual ImFont* GetImGuiBoldFont() = 0;
    virtual ImFont* GetImGuiBlackFont() = 0;

    /**
     * Try to get the name of a pin by its ID.
     * @param p_PinId The pid id (CRC32 of the pin name).
     * @param p_Name The output name of the pin.
     * @return True if the pin name was found, false otherwise.
     */
    virtual bool GetPinName(int32_t p_PinId, ZString& p_Name) = 0;

    /**
     * Convert a position in 3D world space to a 2D position on the screen.
     * @param p_WorldPos The 3D world position.
     * @param p_Out The output 2D position on the screen.
     * @return True if the conversion was successful, false otherwise (eg. if the position is behind the camera).
     */
    virtual bool WorldToScreen(const SVector3& p_WorldPos, SVector2& p_Out) = 0;

    /**
     * Convert a 2D position on the screen to a 3D world position.
     * @param p_ScreenPos The 2D position on the screen.
     * @param p_WorldPosOut The output 3D world position.
     * @param p_DirectionOut The output direction of the ray.
     * @return True if the conversion was successful, false otherwise (eg. if the position is outside the screen bounds).
     */
    virtual bool ScreenToWorld(const SVector2& p_ScreenPos, SVector3& p_WorldPosOut, SVector3& p_DirectionOut) = 0;

    /**
     * Search for a pattern in the game's memory and patch it with the given code.
     * @param p_Pattern A sequence of bytes to search for in the game's memory.
     * @param p_Mask A mask to use when searching for the pattern. x = pattern byte, ? = any byte (eg. xxx????x).
     * @param p_NewCode A buffer containing the new code to write to the location where the pattern was found.
     * @param p_CodeSize The size of the code buffer.
     * @param p_Offset The offset to add to the address where the pattern was found.
	 * @param p_OldCode A (optional) buffer to store the old code.
     * @return True if the pattern was found and patched, false otherwise.
     */
    virtual bool PatchCode(const char* p_Pattern, const char* p_Mask, void* p_NewCode, size_t p_CodeSize, ptrdiff_t p_Offset, void* p_OldCode = nullptr) = 0;

    virtual void ImGuiGameRenderTarget(ZRenderDestination* p_RT, const ImVec2& p_Size = { 0, 0 }) = 0;

    /**
     * Set a plugin setting value for the given name.
     * @param p_Plugin The plugin to set the setting for.
     * @param p_Section The section of the setting in the INI file.
     * @param p_Name The name of the setting.
     * @param p_Value The value of the setting.
     */
    virtual void SetPluginSetting(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, const ZString& p_Value) = 0;

	/**
	 * Set a plugin setting integer value for the given name.
	 * @param p_Plugin The plugin to set the setting for.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_Value The value of the setting.
	 */
	virtual void SetPluginSettingInt(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, int64 p_Value) = 0;

	/**
	 * Set a plugin setting unsigned integer value for the given name.
	 * @param p_Plugin The plugin to set the setting for.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_Value The value of the setting.
	 */
	virtual void SetPluginSettingUInt(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, uint64 p_Value) = 0;

	/**
	 * Set a plugin setting double value for the given name.
	 * @param p_Plugin The plugin to set the setting for.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_Value The value of the setting.
	 */
	virtual void SetPluginSettingDouble(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, double p_Value) = 0;

	/**
	 * Set a plugin setting boolean value for the given name.
	 * @param p_Plugin The plugin to set the setting for.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_Value The value of the setting.
	 */
	virtual void SetPluginSettingBool(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, bool p_Value) = 0;

    /**
     * Get a plugin setting string value for the given name.
     * @param p_Plugin The plugin to get the setting for.
     * @param p_Section The section of the setting in the INI file.
     * @param p_Name The name of the setting.
     * @param p_DefaultValue The default value to return if the setting does not exist.
     * @return The value of the setting, or the default value if the setting does not exist.
     */
    virtual ZString GetPluginSetting(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, const ZString& p_DefaultValue) = 0;

	/**
	 * Get a plugin setting integer value for the given name.
	 * @param p_Plugin The plugin to get the setting for.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_DefaultValue The default value to return if the setting does not exist or is not an integer.
	 * @return The value of the setting, or the default value if the setting does not exist or is not an integer.
	 */
	virtual int64 GetPluginSettingInt(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, int64 p_DefaultValue) = 0;

	/**
	 * Get a plugin setting unsigned integer value for the given name.
	 * @param p_Plugin The plugin to get the setting for.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_DefaultValue The default value to return if the setting does not exist or is not an unsigned integer.
	 * @return The value of the setting, or the default value if the setting does not exist or is not an unsigned integer.
	 */
	virtual uint64 GetPluginSettingUInt(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, uint64 p_DefaultValue) = 0;

	/**
	 * Get a plugin setting double value for the given name.
	 * @param p_Plugin The plugin to get the setting for.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_DefaultValue The default value to return if the setting does not exist or is not a double.
	 * @return The value of the setting, or the default value if the setting does not exist or is not a double.
	 */
	virtual double GetPluginSettingDouble(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, double p_DefaultValue) = 0;

	/**
	 * Get a plugin setting boolean value for the given name.
	 * @param p_Plugin The plugin to get the setting for.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 * @param p_DefaultValue The default value to return if the setting does not exist or is not a boolean.
	 * @return The value of the setting, or the default value if the setting does not exist or is not a boolean.
	 */
	virtual bool GetPluginSettingBool(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, bool p_DefaultValue) = 0;

    /**
     * Check if a plugin setting with the given name exists.
     * @param p_Plugin The plugin to check the setting for.
     * @param p_Section The section of the setting in the INI file.
     * @param p_Name The name of the setting.
     * @return True if the setting exists, false otherwise.
     */
    virtual bool HasPluginSetting(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name) = 0;

	/**
	 * Remove a plugin setting with the given name.
	 * @param p_Plugin The plugin to remove the setting for.
	 * @param p_Section The section of the setting in the INI file.
	 * @param p_Name The name of the setting.
	 */
	 virtual void RemovePluginSetting(IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name) = 0;

	 /**
	  * Reload the settings for the given plugin.
	  * @param p_Plugin The plugin to reload the settings for.
	  */
	 virtual void ReloadPluginSettings(IPluginInterface* p_Plugin) = 0;
};

/**
 * Get the global instance of the SDK.
 */
ZHMSDK_API IModSDK* SDK();

/**
* Get the current SDK version.
*/
extern "C" ZHMSDK_API const char* SDKVersion();