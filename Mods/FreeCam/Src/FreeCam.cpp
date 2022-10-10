#include "FreeCam.h"

#include <random>

#include "Events.h"
#include "Functions.h"
#include "Logging.h"

#include <Glacier/ZActor.h>
#include <Glacier/SGameUpdateEvent.h>
#include <Glacier/ZObject.h>
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZApplicationEngineWin32.h>
#include <Glacier/ZEngineAppCommon.h>
#include <Glacier/ZFreeCamera.h>
#include <Glacier/ZRender.h>
#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZHitman5.h>
#include <Glacier/ZHM5InputManager.h>

FreeCam::FreeCam() :
	m_FreeCamActive(false),
	m_ShouldToggle(false),
	m_FreeCamFrozen(false),
	m_ToggleFreeCamAction("KBMButtonX"),
	m_FreezeFreeCamActionGc("ActivateGameControl0"),
	m_FreezeFreeCamActionKb("KBMInspectNode"),
	m_ControlsVisible(false),
	m_HasToggledFreecamBefore(false)
{
	m_PcControls = {
		{ "P", "Toggle freecam" },
		{ "F3", "Lock camera and enable 47 input" },
		{ "Ctrl + W/S", "Change FOV" },
		{ "Ctrl + A/D", "Roll camera" },
		{ "Alt + W/S", "Change camera speed" },
		{ "Space + Q/E", "Change camera height" },
		{ "Space + W/S", "Move camera on axis" },
		{ "Shift", "Increase camera speed" },
	};

	m_ControllerControls = {
		{ "Y + L", "Change FOV" },
		{ "A + L", "Roll camera" },
		{ "A + L press", "Reset rotation" },
		{ "B + R", "Change camera speed" },
		{ "RT", "Increase camera speed" },
		{ "LB", "Lock camera and enable 47 input" },
		{ "LT + R", "Change camera height" },
	};
}

FreeCam::~FreeCam()
{
	const ZMemberDelegate<FreeCam, void(const SGameUpdateEvent&)> s_Delegate(this, &FreeCam::OnFrameUpdate);
	Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

	// Reset the camera to default when unloading with freecam active.
	if (m_FreeCamActive)
	{
		TEntityRef<IRenderDestinationEntity> s_RenderDest;
		Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);

		s_RenderDest.m_pInterfaceRef->SetSource(&m_OriginalCam);
		
		// Enable Hitman input.
		TEntityRef<ZHitman5> s_LocalHitman;
		Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

		if (s_LocalHitman)
		{
			auto* s_InputControl = Functions::ZHM5InputManager_GetInputControlForLocalPlayer->Call(Globals::InputManager);

			if (s_InputControl)
			{
				Logger::Debug("Got local hitman entity and input control! Enabling input. {} {}", fmt::ptr(s_InputControl), fmt::ptr(s_LocalHitman.m_pInterfaceRef));
				s_InputControl->m_bActive = true;
			}
		}
	}
}

void FreeCam::PreInit()
{
	Hooks::ZInputAction_Digital->AddDetour(this, &FreeCam::ZInputAction_Digital);
	Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &FreeCam::OnLoadScene);
	Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &FreeCam::OnClearScene);
}

void FreeCam::OnEngineInitialized()
{
	const ZMemberDelegate<FreeCam, void(const SGameUpdateEvent&)> s_Delegate(this, &FreeCam::OnFrameUpdate);
	Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void FreeCam::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
	if (!*Globals::ApplicationEngineWin32)
		return;

	if (!(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01.m_pInterfaceRef)
	{
		Logger::Debug("Creating free camera.");
		Functions::ZEngineAppCommon_CreateFreeCamera->Call(&(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon);

		// If freecam was active we need to toggle.
		// This can happen after level restarts / changes.
		if (m_FreeCamActive)
			m_ShouldToggle = true;
	}

	(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCameraControl01.m_pInterfaceRef->SetActive(m_FreeCamActive);
	
	if (Functions::ZInputAction_Digital->Call(&m_ToggleFreeCamAction, -1))
	{
		ToggleFreecam();
	}
	
	if (m_ShouldToggle)
	{
		m_ShouldToggle = false;
		
		if (m_FreeCamActive)
			EnableFreecam();
		else
			DisableFreecam();
	}

	// While freecam is active, only enable hitman input when the "freeze camera" button is pressed.
	if (m_FreeCamActive)
	{
		if (Functions::ZInputAction_Digital->Call(&m_FreezeFreeCamActionKb, -1))
			m_FreeCamFrozen = !m_FreeCamFrozen;

		const bool s_FreezeFreeCam = Functions::ZInputAction_Digital->Call(&m_FreezeFreeCamActionGc, -1) || m_FreeCamFrozen;
		
		(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCameraControl01.m_pInterfaceRef->m_bFreezeCamera = s_FreezeFreeCam;

		TEntityRef<ZHitman5> s_LocalHitman;
		Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

		if (s_LocalHitman)
		{
			auto* s_InputControl = Functions::ZHM5InputManager_GetInputControlForLocalPlayer->Call(Globals::InputManager);

			if (s_InputControl)
				s_InputControl->m_bActive = s_FreezeFreeCam;
		}
	}
}

void FreeCam::OnDrawMenu()
{
	if (ImGui::Button("TOGGLE FREE CAM"))
		ToggleFreecam();

	if (ImGui::Button("FREE CAM CONTROLS"))
		m_ControlsVisible = !m_ControlsVisible;
}

void FreeCam::ToggleFreecam()
{
	m_FreeCamActive = !m_FreeCamActive;
	m_ShouldToggle = true;

	if (m_FreeCamActive && !m_HasToggledFreecamBefore)
		m_ControlsVisible = true;

	m_HasToggledFreecamBefore = true;
}

void FreeCam::EnableFreecam()
{
	auto s_Camera = (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01;

	TEntityRef<IRenderDestinationEntity> s_RenderDest;
	Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);

	m_OriginalCam = *s_RenderDest.m_pInterfaceRef->GetSource();

	const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();
	s_Camera.m_pInterfaceRef->SetWorldMatrix(s_CurrentCamera->GetWorldMatrix());

	Logger::Debug("Camera trans: {}", fmt::ptr(&s_Camera.m_pInterfaceRef->m_mTransform.Trans));

	s_RenderDest.m_pInterfaceRef->SetSource(&s_Camera.m_ref);
}

void FreeCam::DisableFreecam()
{
	TEntityRef<IRenderDestinationEntity> s_RenderDest;
	Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);

	s_RenderDest.m_pInterfaceRef->SetSource(&m_OriginalCam);

	// Enable Hitman input.
	TEntityRef<ZHitman5> s_LocalHitman;
	Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

	if (s_LocalHitman)
	{
		auto* s_InputControl = Functions::ZHM5InputManager_GetInputControlForLocalPlayer->Call(Globals::InputManager);

		if (s_InputControl)
		{
			Logger::Debug("Got local hitman entity and input control! Enabling input. {} {}", fmt::ptr(s_InputControl), fmt::ptr(s_LocalHitman.m_pInterfaceRef));
			s_InputControl->m_bActive = true;
		}
	}
}

void FreeCam::OnDrawUI(bool p_HasFocus)
{
	if (m_ControlsVisible)
	{
		ImGui::PushFont(SDK()->GetImGuiBlackFont());
		const auto s_ControlsExpanded = ImGui::Begin("Free Cam Controls", &m_ControlsVisible);
		ImGui::PushFont(SDK()->GetImGuiRegularFont());

		if (s_ControlsExpanded)
		{
			ImGui::TextUnformatted("PC Controls");

			ImGui::BeginTable("FreeCamControlsPc", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit);

			for (auto& [s_Key, s_Description] : m_PcControls)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(s_Key.c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(s_Description.c_str());
			}

			ImGui::EndTable();

			ImGui::TextUnformatted("Controller Controls");

			ImGui::BeginTable("FreeCamControlsController", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit);

			for (auto& [s_Key, s_Description] : m_ControllerControls)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(s_Key.c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(s_Description.c_str());
			}

			ImGui::EndTable();
		}

		ImGui::PopFont();
		ImGui::End();
		ImGui::PopFont();
	}
}

DECLARE_PLUGIN_DETOUR(FreeCam, bool, ZInputAction_Digital, ZInputAction* th, int a2)
{
	if (!m_FreeCamActive)
		return HookResult<bool>(HookAction::Continue());

	if (strcmp(th->m_szName, "ActivateGameControl0") == 0 && m_FreeCamFrozen)
		return HookResult(HookAction::Return(), true);

	return HookResult<bool>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(FreeCam, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData&)
{
	if (m_FreeCamActive)
		DisableFreecam();

	m_FreeCamActive = false;
	m_ShouldToggle = false;

	return HookResult<void>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(FreeCam, void, OnClearScene, ZEntitySceneContext* th, bool)
{
	if (m_FreeCamActive)
		DisableFreecam();

	m_FreeCamActive = false;
	m_ShouldToggle = false;

	return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(FreeCam);
