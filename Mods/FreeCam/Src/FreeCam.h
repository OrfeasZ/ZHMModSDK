#pragma once

#include <unordered_map>

#include "IPluginInterface.h"

#include <Glacier/ZEntity.h>
#include <Glacier/ZInput.h>
#include <Glacier/ZCollision.h>

class FreeCam : public IPluginInterface {
public:
    FreeCam();
    ~FreeCam() override;

    void Init() override;
    void OnEngineInitialized() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

    void ToggleFreeCam();
    void EnableFreeCam();
    void DisableFreeCam();

    void HandleDrag(ZFreeCameraControlEditorStyleEntity* p_FreeCamera, bool bRotationIsActive, bool bObjectHookIsActive, bool bIsOrbitActive);

    void InstantlyKillNpc();

    void TeleportMainCharacter();

    bool GetFreeCameraRayCastClosestHitQueryOutput(ZRayQueryOutput& p_RayOutput);

    DECLARE_PLUGIN_DETOUR(FreeCam, bool, ZInputAction_Digital, ZInputAction* th, int32_t controllerId);

    DECLARE_PLUGIN_DETOUR(FreeCam, bool, OnLoadScene, ZEntitySceneContext*, SSceneInitParameters&);
    DECLARE_PLUGIN_DETOUR(FreeCam, void, OnClearScene, ZEntitySceneContext*, bool);

    DECLARE_PLUGIN_DETOUR(
        FreeCam, void, ZFreeCameraControlEditorStyleEntity_UpdateMovementFromInput, ZFreeCameraControlEditorStyleEntity* th
    );
    DECLARE_PLUGIN_DETOUR(
        FreeCam, ZString*, ZFreeCameraControlEditorStyleEntity_GenerateActionBindingString,
        ZFreeCameraControlEditorStyleEntity* th, ZString& result
    );

    DECLARE_PLUGIN_DETOUR(FreeCam, ZInputTokenStream::ZTokenData*, ZInputTokenStream_ParseToken, ZInputTokenStream* th, ZInputTokenStream::ZTokenData* result);

    DECLARE_PLUGIN_DETOUR(FreeCam, bool, ZInputActionManager_ParseAsignment, ZInputActionManager* th, ZInputTokenStream* pkStream);

private:
    volatile bool m_FreeCamActive;
    volatile bool m_ShouldToggle;
    volatile bool m_FreeCamFrozen;
    bool m_GamePaused;

    ZEntityRef m_OriginalCam;

    ZInputAction m_ToggleFreeCamAction;
    ZInputAction m_FreezeFreeCamActionGc;
    ZInputAction m_FreezeFreeCamActionKb;
    ZInputAction m_InstantlyKillNpcAction;
    ZInputAction m_TeleportMainCharacterAction;
    ZInputAction m_TogglePauseGame;

    ZInputAction m_MousePosX;
    ZInputAction m_MousePosY;
    ZInputAction m_ZoomToSelection;
    ZInputAction m_ActivateRotate;
    ZInputAction m_ActivateObjectHook;
    ZInputAction m_OrbitCamera;
    ZInputAction m_Zoom;

    bool m_ShowFreeCamWindow;
    bool m_ShowControlsWindow;
    bool m_HasToggledFreeCamBefore;
    bool m_EditorStyleFreeCam;
    bool m_MoveInFreeCam;

    std::unordered_map<std::string, std::string> m_PcControls;
    std::unordered_map<std::string, std::string> m_PcControlsEditorStyle;
    std::unordered_map<std::string, std::string> m_ControllerControls;
};

DECLARE_ZHM_PLUGIN(FreeCam)
