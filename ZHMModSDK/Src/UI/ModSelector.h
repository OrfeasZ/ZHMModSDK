#pragma once

#include "Common.h"
#include <unordered_set>
#include <string>
#include <unordered_map>

namespace UI
{
	class ModSelector
	{
	private:
		struct AvailableMod
		{
			std::string Name;
			bool Enabled;
		};
		
	public:
		static void Init();
		static void Draw(bool p_HasFocus);
		static void UpdateAvailableMods(const std::unordered_set<std::string>& p_Mods, const std::unordered_set<std::string>& p_ActiveMods);
		static void Show() { m_Open = true; }

	private:
		static void ApplySelectedMods();

	private:
		static bool m_Open;
		static SRWLOCK m_Lock;
		static std::vector<AvailableMod> m_AvailableMods;
		static bool m_ShouldShow;
	};
}
