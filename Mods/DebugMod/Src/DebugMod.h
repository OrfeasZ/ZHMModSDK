#pragma once

#include <shared_mutex>
#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

#include <Glacier/ZInput.h>
#include <Glacier/ZEntity.h>

#include "ImGuizmo.h"
#include "Glacier/ZScene.h"

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

	DEFINE_PLUGIN_DETOUR(DebugMod, void, ZHttpBufferReady, ZHttpResultDynamicObject* th);
	DEFINE_PLUGIN_DETOUR(DebugMod, void, WinHttpCallback, void* dwContext, void* hInternet, void* param_3, int dwInternetStatus, void* param_5, int length_param_6);
	DEFINE_PLUGIN_DETOUR(DebugMod, void, OnClearScene, ZEntitySceneContext* th, bool fullyClear);

private:
	bool m_MenuActive = false;
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

	ImGuizmo::OPERATION m_GizmoMode = ImGuizmo::OPERATION::TRANSLATE;
	ImGuizmo::MODE m_GizmoSpace = ImGuizmo::MODE::WORLD;

	ZInputAction m_RaycastAction = "SkipLoadingAction"; // space
	ZInputAction m_DeleteAction = "TemporaryCamSpeedMultiplier0"; // shift
};

DEFINE_ZHM_PLUGIN(DebugMod)
