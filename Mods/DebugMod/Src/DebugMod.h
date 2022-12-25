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
#include <7zUtil.h>

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

	void EquipOutfit(TEntityRef<ZGlobalOutfitKit>& globalOutfitKit, unsigned int currentCharacterSetIndex, const char* currentcharSetCharacterType, unsigned int currentOutfitVariationIndex, ZHitman5* localHitman);
	void EquipOutfit(TEntityRef<ZGlobalOutfitKit>& globalOutfitKit, unsigned int currentCharacterSetIndex, const char* currentcharSetCharacterType, unsigned int currentOutfitVariationIndex, ZActor* actor);
	void SpawnRepositoryProp(const ZRepositoryID& repositoryID, bool addToWorld);
	void SpawnNonRepositoryProp(const char* propAssemblyPath);
	void SpawnNPC(const char* npcName, const ZRepositoryID& repositoryID, TEntityRef<ZGlobalOutfitKit>* globalOutfitKit, const char* currentCharacterSetIndex, const char* currentcharSetCharacterType, const char* currentOutfitVariationIndex);
	void LoadRepositoryProps();
	static void LoadHashMap();
	static void DownloadHashMap();
	std::string GetEntityName(unsigned long long tempBrickHash, unsigned long long entityID, unsigned long long& resourceHash);
	std::string FindNPCEntityNameInBrickBackReferences(unsigned long long tempBrickHash, unsigned long long entityID, unsigned long long& resourceHash);

	std::string ConvertDynamicObjectValueTString(ZDynamicObject* dynamicObject);
	void LoadResourceData(unsigned long long hash, std::vector<char>& resourceData);
	void LoadResourceData(unsigned long long hash, std::vector<char>& resourceData, const std::string& rpkgFilePath);
	std::string GetPatchRPKGFilePath();
	unsigned long long GetDDSTextureHash(const std::string image);

	void EnableInfiniteAmmo();

	DEFINE_PLUGIN_DETOUR(DebugMod, void, ZHttpBufferReady, ZHttpResultDynamicObject* th);
	DEFINE_PLUGIN_DETOUR(DebugMod, void, WinHttpCallback, void* dwContext, void* hInternet, void* param_3, int dwInternetStatus, void* param_5, int length_param_6);
	DEFINE_PLUGIN_DETOUR(DebugMod, void, OnClearScene, ZEntitySceneContext* th, bool fullyClear);
	//DEFINE_PLUGIN_DETOUR(DebugMod, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);

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
	bool m_useSnap = false;

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
	std::string selectedEntityName;
	unsigned long long selectedResourceHash;
	unsigned long long entityID;
	unsigned long long brickEntityID;
	std::set<unsigned long long> brickHashes;

	ImGuizmo::OPERATION m_GizmoMode = ImGuizmo::OPERATION::TRANSLATE;
	ImGuizmo::MODE m_GizmoSpace = ImGuizmo::MODE::WORLD;

	ZInputAction m_RaycastAction = "SkipLoadingAction"; // space
	ZInputAction m_DeleteAction = "TemporaryCamSpeedMultiplier0"; // shift

	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvGPUHandle;
	int width = 0;
	int height = 0;
	TResourcePtr<ZTemplateEntityFactory> repositoryResource;
	std::vector<char> textureResourceData;
	std::multimap<std::string, ZRepositoryID> repositoryProps;
	const char* charSetCharacterTypes[3] = { "Actor", "Nude", "HeroA" };

	inline static std::unordered_map<unsigned long long, std::string> runtimeResourceIDsToResourceIDs;
	inline static std::mutex mutex;

	ZHM5CrippleBox* hm5CrippleBox;
};

DEFINE_ZHM_PLUGIN(DebugMod)
