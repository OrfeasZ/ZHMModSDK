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
		ModSelector();
		
	public:
		void Draw(bool p_HasFocus);
		void UpdateAvailableMods(const std::unordered_set<std::string>& p_Mods, const std::unordered_set<std::string>& p_ActiveMods);
		void Show() { m_Open = true; }

	private:
		void ApplySelectedMods();

	private:
		bool m_Open = false;
		SRWLOCK m_Lock {};
		std::vector<AvailableMod> m_AvailableMods;
		bool m_ShouldShow = false;
	};
}
