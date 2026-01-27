#pragma once

#include "ModSDKVersion.h"
#include "Glacier/ZPrimitives.h"
#include "Glacier/ZEntity.h"
#include "Glacier/ZResource.h"
#include "Glacier/EntityFactory.h"
#include "Common.h"
#include "imgui.h"
#include "D3DUtils.h"
#include "directx/d3d12.h"

#include "implot.h"

class IPluginInterface;
class ZRenderDestination;
class SVector2;
class SVector3;

template <typename T>
class TEntityRef;

class ZHitman5;

struct ImGuiTexture;

class IModSDK {
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

    virtual ImPlotContext* GetImPlotContext() = 0;

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
     * @return True if the pattern was found and patched, false otherwise.
     */
    virtual bool PatchCode(
        const char* p_Pattern, const char* p_Mask, void* p_NewCode, size_t p_CodeSize, ptrdiff_t p_Offset
    ) = 0;

    virtual void ImGuiGameRenderTarget(ZRenderDestination* p_RT, const ImVec2& p_Size = {0, 0}) = 0;

    /**
     * Set a plugin setting value for the given name.
     * @param p_Plugin The plugin to set the setting for.
     * @param p_Section The section of the setting in the INI file.
     * @param p_Name The name of the setting.
     * @param p_Value The value of the setting.
     */
    virtual void SetPluginSetting(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, const ZString& p_Value
    ) = 0;

    /**
     * Set a plugin setting integer value for the given name.
     * @param p_Plugin The plugin to set the setting for.
     * @param p_Section The section of the setting in the INI file.
     * @param p_Name The name of the setting.
     * @param p_Value The value of the setting.
     */
    virtual void SetPluginSettingInt(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, int64 p_Value
    ) = 0;

    /**
     * Set a plugin setting unsigned integer value for the given name.
     * @param p_Plugin The plugin to set the setting for.
     * @param p_Section The section of the setting in the INI file.
     * @param p_Name The name of the setting.
     * @param p_Value The value of the setting.
     */
    virtual void SetPluginSettingUInt(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, uint64 p_Value
    ) = 0;

    /**
     * Set a plugin setting double value for the given name.
     * @param p_Plugin The plugin to set the setting for.
     * @param p_Section The section of the setting in the INI file.
     * @param p_Name The name of the setting.
     * @param p_Value The value of the setting.
     */
    virtual void SetPluginSettingDouble(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, double p_Value
    ) = 0;

    /**
     * Set a plugin setting boolean value for the given name.
     * @param p_Plugin The plugin to set the setting for.
     * @param p_Section The section of the setting in the INI file.
     * @param p_Name The name of the setting.
     * @param p_Value The value of the setting.
     */
    virtual void SetPluginSettingBool(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, bool p_Value
    ) = 0;

    /**
     * Get a plugin setting string value for the given name.
     * @param p_Plugin The plugin to get the setting for.
     * @param p_Section The section of the setting in the INI file.
     * @param p_Name The name of the setting.
     * @param p_DefaultValue The default value to return if the setting does not exist.
     * @return The value of the setting, or the default value if the setting does not exist.
     */
    virtual ZString GetPluginSetting(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, const ZString& p_DefaultValue
    ) = 0;

    /**
     * Get a plugin setting integer value for the given name.
     * @param p_Plugin The plugin to get the setting for.
     * @param p_Section The section of the setting in the INI file.
     * @param p_Name The name of the setting.
     * @param p_DefaultValue The default value to return if the setting does not exist or is not an integer.
     * @return The value of the setting, or the default value if the setting does not exist or is not an integer.
     */
    virtual int64 GetPluginSettingInt(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, int64 p_DefaultValue
    ) = 0;

    /**
     * Get a plugin setting unsigned integer value for the given name.
     * @param p_Plugin The plugin to get the setting for.
     * @param p_Section The section of the setting in the INI file.
     * @param p_Name The name of the setting.
     * @param p_DefaultValue The default value to return if the setting does not exist or is not an unsigned integer.
     * @return The value of the setting, or the default value if the setting does not exist or is not an unsigned integer.
     */
    virtual uint64 GetPluginSettingUInt(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, uint64 p_DefaultValue
    ) = 0;

    /**
     * Get a plugin setting double value for the given name.
     * @param p_Plugin The plugin to get the setting for.
     * @param p_Section The section of the setting in the INI file.
     * @param p_Name The name of the setting.
     * @param p_DefaultValue The default value to return if the setting does not exist or is not a double.
     * @return The value of the setting, or the default value if the setting does not exist or is not a double.
     */
    virtual double GetPluginSettingDouble(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, double p_DefaultValue
    ) = 0;

    /**
     * Get a plugin setting boolean value for the given name.
     * @param p_Plugin The plugin to get the setting for.
     * @param p_Section The section of the setting in the INI file.
     * @param p_Name The name of the setting.
     * @param p_DefaultValue The default value to return if the setting does not exist or is not a boolean.
     * @return The value of the setting, or the default value if the setting does not exist or is not a boolean.
     */
    virtual bool GetPluginSettingBool(
        IPluginInterface* p_Plugin, const ZString& p_Section, const ZString& p_Name, bool p_DefaultValue
    ) = 0;

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

    /**
     * Try to get the local Hitman player.
     */
    virtual TEntityRef<ZHitman5> GetLocalPlayer() = 0;

    /**
     * Search for a pattern in the game's memory and patch it with the given code, storing the original code in a provided buffer.
     * @param p_Pattern A sequence of bytes to search for in the game's memory.
     * @param p_Mask A mask to use when searching for the pattern. x = pattern byte, ? = any byte (eg. xxx????x).
     * @param p_NewCode A buffer containing the new code to write to the location where the pattern was found.
     * @param p_CodeSize The size of the code buffer.
     * @param p_Offset The offset to add to the address where the pattern was found.
     * @param p_OriginalCode A buffer to store the original code.
     * @return True if the pattern was found and patched, false otherwise.
     */
    virtual bool PatchCodeStoreOriginal(
        const char* p_Pattern, const char* p_Mask, void* p_NewCode, size_t p_CodeSize, ptrdiff_t p_Offset,
        void* p_OriginalCode
    ) = 0;

    /**
     * Load entity resources from QuickEntity JSON. This makes them available
     * to the engine for spawning.
     * @param p_Json The QuickEntity JSON string.
     * @param p_BlueprintFactoryOut The resulting blueprint factory resource.
     * @param p_FactoryOut The resulting factory resource.
     * @return True if the resources were loaded successfully, false otherwise.
     */
    virtual bool LoadQnEntity(
        const ZString& p_Json,
        TResourcePtr<ZTemplateEntityBlueprintFactory>& p_BlueprintFactoryOut,
        TResourcePtr<ZTemplateEntityFactory>& p_FactoryOut
    ) = 0;

    /**
     * Check if a chunk is mounted.
     * @param p_ChunkIndex The index of the chunk to check.
     * @return True if the chunk is mounted, false otherwise.
     */
    virtual bool IsChunkMounted(uint32_t p_ChunkIndex) = 0;

    /**
     * Mount a chunk.
     * @param p_ChunkIndex The index of the chunk to mount.
     */
    virtual void MountChunk(uint32_t p_ChunkIndex) = 0;

    /**
     * Unmounts a chunk and all chunks mounted after it.
     *
     * Resources in the resource container are stored in a single contiguous array
     * and grouped by package using prefix indices. Because resource indices are
     * assigned sequentially and packages occupy a tail range of the resource array,
     * it is not possible to unmount a package without also unmounting all packages
     * mounted after it.
     *
     * If the target chunk belongs to a parent chunk, the parent chunk will also be
     * unmounted if it is not required by any other currently mounted chunk.
     *
     * If p_RemountChunksBelow is true, all chunks below the target chunk that were
     * unmounted as a result of this operation will be
     * mounted again in their original order.
     *
     * The unmount operation will fail if any resource belonging to the affected
     * packages still has a non-zero reference count.
     *
     * @param p_ChunkIndex Index of the chunk to unmount.
     * @param p_RemountChunksBelow Whether to remount chunks that were unmounted
     *                             after the target chunk.
     */
    virtual void UnmountChunk(uint32_t p_ChunkIndex, bool p_RemountChunksBelow) = 0;

    /**
     * Get the list of indices of chunks that contain the resource with the specified runtime resource ID.
     * @param id The runtime resource ID to look up.
     * @return A constant reference to an array of chunk indices. If no chunks are found, an empty array is returned.
     */
    virtual const TArray<uint32_t>& GetChunkIndicesForRuntimeResourceId(const ZRuntimeResourceID& id) = 0;

    /**
     * Create a DirectX 12 texture from DDS data in memory and initialize an ImGui texture descriptor.
     * @param p_Data Pointer to DDS data in memory.
     * @param p_DataSize Size of the DDS data buffer.
     * @param p_OutTexture Output DirectX 12 texture resource.
     * @param p_OutImGuiTexture Output ImGui texture descriptor.
     * @return True if the texture was successfully created, false otherwise.
     */
    virtual bool CreateDDSTextureFromMemory(
        const void* p_Data,
        size_t p_DataSize,
        ScopedD3DRef<ID3D12Resource>& p_OutTexture,
        ImGuiTexture& p_OutImGuiTexture
    ) = 0;

    /**
     * Create a DirectX 12 texture from a DDS file and initialize an ImGui texture descriptor.
     * @param p_FilePath Path to the DDS file.
     * @param p_OutTexture Output DirectX 12 texture resource.
     * @param p_OutImGuiTexture Output ImGui texture descriptor.
     * @return True if the texture was successfully created, false otherwise.
     */
    virtual bool CreateDDSTextureFromFile(
        const std::string& p_FilePath,
        ScopedD3DRef<ID3D12Resource>& p_OutTexture,
        ImGuiTexture& p_OutImGuiTexture
    ) = 0;

    /**
     * Create a DirectX 12 texture from WIC-compatible image data in memory (PNG, JPEG, etc.)
     * and initialize an ImGui texture descriptor.
     * @param p_Data Pointer to image data in memory.
     * @param p_DataSize Size of the image data buffer.
     * @param p_OutTexture Output DirectX 12 texture resource.
     * @param p_OutImGuiTexture Output ImGui texture descriptor.
     * @return True if the texture was successfully created, false otherwise.
     */
    virtual bool CreateWICTextureFromMemory(
        const void* p_Data,
        size_t p_DataSize,
        ScopedD3DRef<ID3D12Resource>& p_OutTexture,
        ImGuiTexture& p_OutImGuiTexture
    ) = 0;

    /**
     * Create a DirectX 12 texture from a WIC-compatible image file (PNG, JPEG, etc.)
     * and initialize an ImGui texture descriptor.
     * @param p_FilePath Path to the image file.
     * @param p_OutTexture Output DirectX 12 texture resource.
     * @param p_OutImGuiTexture Output ImGui texture descriptor.
     * @return True if the texture was successfully created, false otherwise.
     */
    virtual bool CreateWICTextureFromFile(
        const std::string& p_FilePath,
        ScopedD3DRef<ID3D12Resource>& p_OutTexture,
        ImGuiTexture& p_OutImGuiTexture
    ) = 0;
};

/**
 * Get the global instance of the SDK.
 */
ZHMSDK_API IModSDK* SDK();

/**
* Get the current SDK version.
*/
extern "C" ZHMSDK_API const char* SDKVersion();