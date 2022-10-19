#pragma once

#include <shared_mutex>
#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

#include <Glacier/ZInput.h>
#include <Glacier/ZEntity.h>

class DebugMod : public IPluginInterface
{
public:
	~DebugMod() override;

	void OnEngineInitialized() override;
	void OnDrawMenu() override;
	void OnDrawUI(bool p_HasFocus) override;
	void OnDraw3D(IRenderer* p_Renderer) override;

private:
	void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
	void DoRaycast(float4 p_From, float4 p_To);
	void MoveObject();
	void CopyToClipboard(const std::string& p_String) const;

private:
	// UI Drawing
	void DrawOptions(bool p_HasFocus);
	void DrawPositionBox(bool p_HasFocus);
	void DrawEntityBox(bool p_HasFocus);

private:
	bool m_MenuActive = false;
	bool m_RenderNpcBoxes = false;
	bool m_RenderNpcNames = false;
	bool m_RenderNpcRepoIds = false;

	float4 m_From;
	float4 m_To;
	float4 m_Hit;
	float4 m_Normal;

	bool m_Moving = false;
	float m_MoveDistance = 0.0f;

	ZEntityRef m_SelectedEntity;
	std::shared_mutex m_EntityMutex;

	ZInputAction m_RaycastAction = "SkipLoadingAction"; // space
	ZInputAction m_DeleteAction = "TemporaryCamSpeedMultiplier0"; // shift
};

DEFINE_ZHM_PLUGIN(DebugMod)
