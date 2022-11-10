#include "ModSelector.h"

#include "imgui.h"
#include "IModSDK.h"
#include "ModSDK.h"
#include "ModLoader.h"
#include "Util/StringUtils.h"

using namespace UI;

ModSelector::ModSelector()
{
	InitializeSRWLock(&m_Lock);
}

void ModSelector::UpdateAvailableMods(const std::unordered_set<std::string>& p_Mods, const std::unordered_set<std::string>& p_ActiveMods)
{
	ScopedExclusiveGuard s_Guard(&m_Lock);

	m_AvailableMods.clear();

	for (auto& s_Mod : p_Mods)
		m_AvailableMods.push_back({ s_Mod, p_ActiveMods.contains(Util::StringUtils::ToLowerCase(s_Mod)) });

	// If there are no active mods then set this flag which will make it
	// so the mod selector is shown automatically when a user launches the game.
	if (p_ActiveMods.empty())
		m_ShouldShow = true;
	else
		m_ShouldShow = false;
}

void ModSelector::Draw(bool p_HasFocus)
{
	if (m_ShouldShow)
	{
		m_Open = true;
		m_ShouldShow = false;
		SDK()->RequestUIFocus();
	}

	if (!m_Open || !p_HasFocus)
		return;

	const auto s_WasOpen = m_Open;
	
	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("MODS", &m_Open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing)
	{
		ImGui::TextUnformatted("Select the mods you'd like to use.\nKeep in mind that some mods might require a game restart to function properly.");
		ImGui::Separator();

		ImGui::BeginChild("ScrollingRegion", ImVec2(0, 200), false, ImGuiWindowFlags_HorizontalScrollbar);

		AcquireSRWLockShared(&m_Lock);

		for (auto& s_Mod : m_AvailableMods)
		{
			ImGui::Checkbox(s_Mod.Name.c_str(), &s_Mod.Enabled);
		}

		ReleaseSRWLockShared(&m_Lock);

		ImGui::EndChild();

		ImGui::Separator();

		if (ImGui::Button("OK"))
		{
			ApplySelectedMods();
			m_Open = false;
		}

		if (ImGui::Button("Rescan Mods"))
		{
			ModSDK::GetInstance()->GetModLoader()->ScanAvailableMods();
		}
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();

	// If a user closed this, then release focus.
	if (s_WasOpen && !m_Open)
	{
		SDK()->ReleaseUIFocus();
	}
}

void ModSelector::ApplySelectedMods()
{
	std::unordered_set<std::string> s_Mods;

	for (auto& s_Mod : m_AvailableMods)
		if (s_Mod.Enabled)
			s_Mods.insert(s_Mod.Name);

	ModSDK::GetInstance()->GetModLoader()->SetActiveMods(s_Mods);
}
