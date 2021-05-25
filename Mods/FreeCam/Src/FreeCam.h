#pragma once

#include "IPluginInterface.h"

#include <Glacier/ZEntity.h>

#include "Glacier/ZInput.h"

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
	ZEntityRef m_OriginalCam;
	ZInputAction m_ToggleFreecamAction;
};

DEFINE_ZHM_PLUGIN(FreeCam)
