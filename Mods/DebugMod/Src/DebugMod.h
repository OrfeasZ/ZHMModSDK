#pragma once

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
	bool m_RenderNpcLines = false;
	bool m_RenderNpcNames = false;
	bool m_RenderNpcRepoIds = false;
};

DEFINE_ZHM_PLUGIN(DebugMod)
