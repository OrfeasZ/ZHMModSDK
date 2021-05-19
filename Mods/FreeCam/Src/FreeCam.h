#pragma once

#include "IPluginInterface.h"

#include <random>
#include <unordered_map>

#include <Glacier/ZEntity.h>

class FreeCam : public IPluginInterface
{
public:
	FreeCam();
	~FreeCam() override;

	void PreInit() override;
	void OnEngineInitialized() override;
	void OnDrawMenu() override;

private:
	void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

private:
	volatile bool m_FreeCamActive;
	volatile bool m_ShouldToggle;
	SMatrix43 m_CameraTransform;
	ZEntityRef m_OriginalCam;
};

DEFINE_ZHM_PLUGIN(FreeCam)
