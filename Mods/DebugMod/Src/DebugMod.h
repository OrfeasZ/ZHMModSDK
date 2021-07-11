#pragma once

#include <shared_mutex>
#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

class DebugMod : public IPluginInterface
{
public:
	void OnDrawMenu() override;
	void OnDrawUI(bool p_HasFocus) override;
	void OnDraw3D(IRenderer* p_Renderer) override;

private:
	bool m_MenuActive = false;
	bool m_RenderNpcBoxes = false;
	bool m_RenderNpcNames = false;
	bool m_RenderNpcRepoIds = false;
	std::shared_mutex m_EntityMutex;
	std::vector<ZEntityRef> m_EntitiesToTrack;
};

DEFINE_ZHM_PLUGIN(DebugMod)
