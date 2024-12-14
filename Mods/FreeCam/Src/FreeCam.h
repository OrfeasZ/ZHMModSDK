#pragma once

#include <unordered_map>

#include "IPluginInterface.h"

#include <Glacier/ZEntity.h>
#include <Glacier/ZInput.h>
#include <Glacier/ZCollision.h>

class FreeCam : public IPluginInterface
{
public:
    FreeCam();
    ~FreeCam() override;

    void Init() override;
    void OnEngineInitialized() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    void ToggleFreecam();
    void EnableFreecam();
    void DisableFreecam();
	void InstantlyKillNpc();
	void TeleportMainCharacter();
	bool GetFreeCameraRayCastClosestHitQueryOutput(ZRayQueryOutput& p_RayOutput);

private:
    DECLARE_PLUGIN_DETOUR(FreeCam, bool, ZInputAction_Digital, ZInputAction* th, int a2);
    DECLARE_PLUGIN_DETOUR(FreeCam, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);
    DECLARE_PLUGIN_DETOUR(FreeCam, void, OnClearScene, ZEntitySceneContext*, bool);

private:
    volatile bool m_FreeCamActive;
    volatile bool m_ShouldToggle;
    volatile bool m_FreeCamFrozen;
	volatile bool m_GamePaused;
    ZEntityRef m_OriginalCam;
    ZInputAction m_ToggleFreeCamAction;
    ZInputAction m_FreezeFreeCamActionGc;
    ZInputAction m_FreezeFreeCamActionKb;
	ZInputAction m_InstantlyKillNpcAction;
	ZInputAction m_TeleportMainCharacterAction;
	ZInputAction m_TogglePauseGame;
    bool m_ControlsVisible;
    bool m_HasToggledFreecamBefore;
	bool m_EditorStyleFreecam;
    std::unordered_map<std::string, std::string> m_PcControls;
	std::unordered_map<std::string, std::string> m_PcControlsEditorStyle;
    std::unordered_map<std::string, std::string> m_ControllerControls;
};

DECLARE_ZHM_PLUGIN(FreeCam)
