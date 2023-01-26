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
	void CopyToClipboard(const std::string& p_String) const;
	void OnMouseDown(SVector2 p_Pos, bool p_FirstClick);

private:
	// UI Drawing
	void DrawOptions(bool p_HasFocus);
	void DrawPositionBox(bool p_HasFocus);
	void DrawEntityBox(bool p_HasFocus);
	void DrawPlayerBox(bool p_HasFocus);
	void DrawItemsBox(bool p_HasFocus);
	void DrawAssetsBox(bool p_HasFocus);
	void DrawNPCsBox(bool p_HasFocus);
	void DrawSceneBox(bool p_HasFocus);

	void EquipOutfit(const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit, unsigned int p_CurrentCharSetIndex, const char* p_CurrentCharSetCharacterType, unsigned int p_CurrentOutfitVariationIndex, ZHitman5* p_LocalHitman);
	void EquipOutfit(const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit, unsigned int p_CurrentCharSetIndex, const char* p_CurrentCharSetCharacterType, unsigned int p_CurrentOutfitVariationindex, ZActor* p_Actor);
	void SpawnRepositoryProp(const ZRepositoryID& p_RepositoryId, bool addToWorld);
	void SpawnNonRepositoryProp(const char* p_PropAssemblyPath);
	void SpawnNPC(const char* p_NpcName, const ZRepositoryID& repositoryID, const TEntityRef<ZGlobalOutfitKit>* p_GlobalOutfitKit, const char* p_CurrentCharacterSetIndex, const char* p_CurrentcharSetCharacterType, const char* p_CurrentOutfitVariationIndex);
	void LoadRepositoryProps();
	static void LoadHashMap();
	static void DownloadHashMap();
	std::string GetEntityName(unsigned long long p_TempBrickHash, unsigned long long p_EntityId, unsigned long long& p_ResourceHash);
	std::string FindNPCEntityNameInBrickBackReferences(unsigned long long p_TempBrickHash, unsigned long long p_EntityId, unsigned long long& p_ResourceHash);

	std::string ConvertDynamicObjectValueTString(ZDynamicObject* p_DynamicObject);
	void LoadResourceData(unsigned long long p_Hash, std::vector<char>& p_ResourceData);
	void LoadResourceData(unsigned long long p_Hash, std::vector<char>& p_ResourceData, const std::string& p_RpkgFilePath);
	std::string GetPatchRPKGFilePath();
	unsigned long long GetDDSTextureHash(const std::string p_Image);

	void EnableInfiniteAmmo();
	
	DEFINE_PLUGIN_DETOUR(DebugMod, void, OnClearScene, ZEntitySceneContext* th, bool fullyClear);

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

	ZEntityRef m_SelectedEntity;
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
	const char* m_CharSetCharacterTypes[3] = { "Actor", "Nude", "HeroA" };

	inline static std::unordered_map<unsigned long long, std::string> m_RuntimeResourceIDsToResourceIDs;
	inline static std::mutex m_Mutex;

	ZHM5CrippleBox* m_Hm5CrippleBox = nullptr;
};

DEFINE_ZHM_PLUGIN(DebugMod)
