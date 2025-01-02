#pragma once

#include <shared_mutex>
#include <random>
#include <unordered_map>
#include <map>
#include <set>

#include "IPluginInterface.h"

#include <Glacier/ZInput.h>
#include <Glacier/ZEntity.h>
#include <Glacier/ZResource.h>

#include "ImGuizmo.h"
#include "Glacier/ZScene.h"

class ZGlobalOutfitKit;
class ZHitman5;
class ZTemplateEntityFactory;

inline bool FindSubstring(const std::string& str, const std::string& substring, const bool bCaseSensitive = false) {

	if (substring.empty())
	{
		return true;
	}

	const auto it = std::ranges::search(str, substring,
	                                    [bCaseSensitive](const char ch1, const char ch2) {
		                                    if (bCaseSensitive) {
			                                    return ch1 == ch2;
		                                    }
		                                    return std::tolower(ch1) == std::tolower(ch2);
	                                    })
	                    .begin();
	return (it != str.end());
}

class DebugMod : public IPluginInterface
{
public:
    ~DebugMod() override;

    void Init() override;
    void OnEngineInitialized() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;
    void OnDraw3D(IRenderer* p_Renderer) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    static void CopyToClipboard(const std::string& p_String);
    void OnMouseDown(SVector2 p_Pos, bool p_FirstClick);

private:
    // UI Drawing
    void DrawOptions(bool p_HasFocus);
    void DrawPositionBox(bool p_HasFocus);
    void DrawPlayerBox(bool p_HasFocus);
    void DrawItemsBox(bool p_HasFocus);
    void DrawAssetsBox(bool p_HasFocus);
    void DrawNPCsBox(bool p_HasFocus);
    void DrawSceneBox(bool p_HasFocus);

	static void EquipOutfit(const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit, uint8_t n_CurrentCharSetIndex, const std::string& s_CurrentCharSetCharacterType, uint8_t n_CurrentOutfitVariationIndex, ZHitman5* p_LocalHitman);
    static void EquipOutfit(const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit, uint8_t n_CurrentCharSetIndex, const std::string& s_CurrentCharSetCharacterType, uint8_t n_CurrentOutfitVariationindex, ZActor* p_Actor);
	static void SpawnRepositoryProp(const ZRepositoryID& p_RepositoryId, const bool addToWorld);
	static void SpawnNonRepositoryProp(const std::string& p_PropAssemblyPath);
    static void SpawnNPC(const std::string& s_NpcName, const ZRepositoryID& repositoryID, const TEntityRef<ZGlobalOutfitKit>* p_GlobalOutfitKit, uint8_t n_CurrentCharacterSetIndex, const std::string& s_CurrentcharSetCharacterType, uint8_t p_CurrentOutfitVariationIndex);
	void LoadRepositoryProps();
    static void LoadHashMap();
    static void DownloadHashMap();
    static std::string GetEntityName(unsigned long long p_TempBrickHash, unsigned long long p_EntityId, unsigned long long& p_ResourceHash);
    static std::string FindNPCEntityNameInBrickBackReferences(unsigned long long p_TempBrickHash, unsigned long long p_EntityId, unsigned long long& p_ResourceHash);

    std::string ConvertDynamicObjectValueTString(const ZDynamicObject& p_DynamicObject);

    void LoadResourceData(unsigned long long p_Hash, std::vector<char>& p_ResourceData);
    static void LoadResourceData(unsigned long long p_Hash, std::vector<char>& p_ResourceData, const std::string& p_RpkgFilePath);
    std::string GetPatchRPKGFilePath();
    unsigned long long GetDDSTextureHash(const std::string p_Image);

    static void EnableInfiniteAmmo();

    DECLARE_PLUGIN_DETOUR(DebugMod, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);
    DECLARE_PLUGIN_DETOUR(DebugMod, void, OnClearScene, ZEntitySceneContext* th, bool forReload);

private:
    bool m_DebugMenuActive = false;
    bool m_PositionsMenuActive = false;
    bool m_EntityMenuActive = false;
    bool m_PlayerMenuActive = false;
    bool m_ItemsMenuActive = false;
    bool m_AssetsMenuActive = false;
    bool m_NPCsMenuActive = false;
    bool m_SceneMenuActive = false;
    bool m_RenderNpcBoxes = false;
    bool m_RenderNpcNames = false;
    bool m_RenderNpcRepoIds = false;
    bool m_RenderRaycast = false;
    bool m_UseSnap = false;

    float m_SnapValue[3] = { 1.0f, 1.0f, 1.0f };

    float4 m_From;
    float4 m_To;
    float4 m_Hit;
    float4 m_Normal;

    bool m_Moving = false;
    float m_MoveDistance = 0.0f;
    bool m_HoldingMouse = false;

	std::string m_SelectedCharacterName;
    ZEntityRef m_SelectedEntity;
	ZActor* s_CurrentlySelectedActor = nullptr;
    std::shared_mutex m_EntityMutex;
    std::string m_SelectedEntityName;
    unsigned long long m_SelectedResourceHash = 0;
    unsigned long long m_EntityId = 0;
    unsigned long long m_BrickEntityId = 0;
    std::set<unsigned long long> m_BrickHashes;

    ImGuizmo::OPERATION m_GizmoMode = ImGuizmo::OPERATION::TRANSLATE;
    ImGuizmo::MODE m_GizmoSpace = ImGuizmo::MODE::WORLD;

    ZInputAction m_RaycastAction = "SkipLoadingAction"; // space
    ZInputAction m_DeleteAction = "TemporaryCamSpeedMultiplier0"; // shift

    D3D12_GPU_DESCRIPTOR_HANDLE m_TextureSrvGpuHandle {};
    int m_Width = 0;
    int m_Height = 0;
    TResourcePtr<ZTemplateEntityFactory> m_RepositoryResource;
    std::vector<char> m_TextureResourceData;
    std::multimap<std::string, ZRepositoryID> m_RepositoryProps;
	const std::vector<std::string> m_CharSetCharacterTypes = {"Actor", "Nude", "HeroA"};

	bool bActorSelectedByCamera = false;

    inline static std::unordered_map<unsigned long long, std::string> m_RuntimeResourceIDsToResourceIDs;
    inline static std::mutex m_Mutex;

    ZHM5CrippleBox* m_Hm5CrippleBox = nullptr;

	TEntityRef<ZGlobalOutfitKit>* m_GlobalOutfitKit = nullptr;

private:
    ZActor* m_NPCTracked = nullptr;
    bool m_TrackCamActive = false;
    ZEntityRef m_PlayerCam = nullptr;
    TEntityRef<ZCameraEntity> m_TrackCam {};
    TEntityRef<IRenderDestinationEntity> m_RenderDest {};

private:
    void EnableTrackCam();
    void UpdateTrackCam() const;
    void DisableTrackCam();
    void GetPlayerCam();
    void GetTrackCam();
    void GetRenderDest();
    static void SetPlayerControlActive(bool active);
};

DECLARE_ZHM_PLUGIN(DebugMod)
