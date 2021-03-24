#include "ModSelector.h"

#include "imgui.h"

using namespace UI;

void ModSelector::Draw()
{
	ImGui::Begin("Mods");

	ImGui::TextUnformatted("Select the mods you'd like to enable:");
	ImGui::Separator();

	static bool s_Mod1 = false;
	ImGui::Checkbox("CertPinBypass", &s_Mod1);

	static bool s_Mod2 = false;
	ImGui::Checkbox("CodeGen", &s_Mod2);

	static bool s_Mod3 = false;
	ImGui::Checkbox("NoPause", &s_Mod3);

	static bool s_Mod4 = false;
	ImGui::Checkbox("SkipIntro", &s_Mod4);

	static bool s_Mod5 = false;
	ImGui::Checkbox("WakingUpNpcs", &s_Mod5);

	ImGui::End();
}
