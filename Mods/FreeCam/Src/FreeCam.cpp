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

#include "Glacier/ZGameLoopManager.h"
#include "Glacier/ZHitman5.h"
#include "Glacier/ZHM5InputManager.h"

FreeCam::FreeCam() :
	m_FreeCamActive(false), m_ShouldToggle(false), m_ToggleFreecamAction("KBMButtonX")
{
}

FreeCam::~FreeCam()
{
	const ZMemberDelegate<FreeCam, void(const SGameUpdateEvent&)> s_Delegate(this, &FreeCam::OnFrameUpdate);
	Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 99999, EUpdateMode::eUpdatePlayMode);

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
}

void FreeCam::OnEngineInitialized()
{
	const ZMemberDelegate<FreeCam, void(const SGameUpdateEvent&)> s_Delegate(this, &FreeCam::OnFrameUpdate);
	Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 99999, EUpdateMode::eUpdatePlayMode);
}

void FreeCam::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
	if (!*Globals::ApplicationEngineWin32)
		return;

	if (!(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01.m_pInterfaceRef)
	{
		Logger::Debug("Creating free camera.");
		Functions::ZEngineAppCommon_CreateFreeCamera->Call(&(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon);
	}

	(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCameraControl01.m_pInterfaceRef->SetActive(m_FreeCamActive);
	
	if (Functions::ZInputAction_Digital->Call(&m_ToggleFreecamAction, -1))
	{
		m_FreeCamActive = !m_FreeCamActive;
		m_ShouldToggle = true;
	}
	
	if (m_ShouldToggle)
	{
		m_ShouldToggle = false;

		auto s_Camera = (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01;

		TEntityRef<IRenderDestinationEntity> s_RenderDest;
		Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);
		
		if (m_FreeCamActive)
		{
			m_OriginalCam = *s_RenderDest.m_pInterfaceRef->GetSource();

			auto s_CurrentCamera = Functions::GetCurrentCamera->Call();
			s_Camera.m_pInterfaceRef->m_mTransform = s_CurrentCamera->m_mTransform;
			
			s_RenderDest.m_pInterfaceRef->SetSource(&s_Camera.m_ref);

			// Disable Hitman input.
			TEntityRef<ZHitman5> s_LocalHitman;
			Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

			if (s_LocalHitman)
			{
				auto* s_InputControl = Functions::ZHM5InputManager_GetInputControlForLocalPlayer->Call(Globals::InputManager);

				if (s_InputControl)
				{
					Logger::Debug("Got local hitman entity and input control! Disabling input. {} {}", fmt::ptr(s_InputControl), fmt::ptr(s_LocalHitman.m_pInterfaceRef));
					s_InputControl->m_bActive = false;
				}
			}			
		}
		else
		{
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
}

void FreeCam::OnDrawMenu()
{
	if (ImGui::Button("TOGGLE FREE CAM"))
	{
		m_FreeCamActive = !m_FreeCamActive;
		m_ShouldToggle = true;
	}
}


DECLARE_ZHM_PLUGIN(FreeCam);
