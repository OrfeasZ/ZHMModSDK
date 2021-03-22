#pragma once

#include <vector>
#include <spdlog/spdlog.h>
#include <Glacier/ZString.h>

namespace UI
{
	class Console
	{
	private:
		struct LogLine
		{
			spdlog::level::level_enum Level;
			ZString Text;
		};
		
	public:
		static void Draw();
		static void AddLogLine(spdlog::level::level_enum p_Level, const ZString& p_Text);

	private:
		static std::vector<LogLine>* m_LogLines;
	};
}
