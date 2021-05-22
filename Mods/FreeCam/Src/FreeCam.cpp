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

FreeCam::FreeCam() :
	m_FreeCamActive(false), m_ShouldToggle(false)
{
}

FreeCam::~FreeCam()
{
	const ZMemberDelegate<FreeCam, void(const SGameUpdateEvent&)> s_Delegate(this, &FreeCam::OnFrameUpdate);
	Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 99999, EUpdateMode::eUpdatePlayMode);
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
	
	auto s_Camera = (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01;
	
	if (m_ShouldToggle)
	{
		m_ShouldToggle = false;

		TEntityRef<IRenderDestinationEntity> s_RenderDest;
		Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);
		
		if (m_FreeCamActive)
		{
			m_OriginalCam = *s_RenderDest.m_pInterfaceRef->GetSource();

			auto s_CurrentCamera = Functions::GetCurrentCamera->Call();
			s_Camera.m_pInterfaceRef->m_mTransform = s_CurrentCamera->m_mTransform;
			
			s_RenderDest.m_pInterfaceRef->SetSource(&s_Camera.m_ref);
		}
		else
		{
			s_RenderDest.m_pInterfaceRef->SetSource(&m_OriginalCam);
		}
	}


	/*if (!m_FreeCamActive)
	{
		m_CameraTransform = s_Camera->m_mTransform;
	}
	else
	{
		s_Camera->m_mTransform = m_CameraTransform;
	}

	Logger::Debug("Camera: {} {}", fmt::ptr(s_Camera), offsetof(ZCameraEntity, m_mTransform));

	Logger::Debug("{} {} {}, {} {} {}, {} {} {}, {} {} {}",
		s_Camera->m_mTransform.XAxis.x, s_Camera->m_mTransform.XAxis.y, s_Camera->m_mTransform.XAxis.z,
		s_Camera->m_mTransform.YAxis.x, s_Camera->m_mTransform.YAxis.y, s_Camera->m_mTransform.YAxis.z,
		s_Camera->m_mTransform.ZAxis.x, s_Camera->m_mTransform.ZAxis.y, s_Camera->m_mTransform.ZAxis.z,
		s_Camera->m_mTransform.Trans.x, s_Camera->m_mTransform.Trans.y, s_Camera->m_mTransform.Trans.z
	);*/
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
