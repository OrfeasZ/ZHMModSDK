#include "DebugMod.h"
#include "imgui_internal.h"

#include <Glacier/ZContentKitManager.h>

void DebugMod::DrawAssetsBox(bool p_HasFocus)
{
	if (!p_HasFocus || !m_AssetsMenuActive)
	{
		return;
	}

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("ASSETS", &m_AssetsMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing && p_HasFocus)
	{
		if (repositoryProps.size() == 0)
		{
			LoadRepositoryProps();
		}

		ZContentKitManager* contentKitManager = Globals::ContentKitManager;
		static char propTitle[36] { "" };
		static char propAssemblyPath[512] { "" };
		static char numberOfPropsToSpawn[5] { "1" };
		static char numberOfPropsToSpawn2[5] { "1" };
		static char numberOfPropsToSpawn3[5] { "1" };
		static int button = 1;
		static char npcName[100] {};

		ImGui::Text("Repository Props");
		ImGui::Text("");
		ImGui::Text("Prop Title");
		ImGui::SameLine();

		const bool isInputTextEnterPressed = ImGui::InputText("##PropRepositoryID", propTitle, sizeof(propTitle), ImGuiInputTextFlags_EnterReturnsTrue);
		const bool isInputTextActive = ImGui::IsItemActive();
		const bool isInputTextActivated = ImGui::IsItemActivated();

		if (isInputTextActivated)
		{
			ImGui::OpenPopup("##popup");
		}

		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
		ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

		if (ImGui::BeginPopup("##popup", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ChildWindow))
		{
			for (auto it = repositoryProps.begin(); it != repositoryProps.end(); ++it)
			{
				const char* propTitle2 = it->first.c_str();

				if (!strstr(propTitle2, propTitle))
				{
					continue;
				}

				if (ImGui::Selectable(propTitle2))
				{
					ImGui::ClearActiveID();
					strcpy_s(propTitle, propTitle2);

					int numberOfPropsToSpawn2 = std::atoi(numberOfPropsToSpawn);

					for (int i = 0; i < numberOfPropsToSpawn2; ++i)
					{
						SpawnRepositoryProp(it->second, button == 1);
					}
				}
			}

			if (isInputTextEnterPressed || (!isInputTextActive && !ImGui::IsWindowFocused()))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (ImGui::RadioButton("Add To World", button == 1))
		{
			button = 1;
		}

		ImGui::SameLine();

		if (ImGui::RadioButton("Add To Inventory", button == 2))
		{
			button = 2;
		}

		ImGui::Text("Number Of Props To Spawn");
		ImGui::SameLine();

		ImGui::InputText("##NumberOfPropsToSpawn", numberOfPropsToSpawn, sizeof(numberOfPropsToSpawn));

		ImGui::Separator();
		ImGui::Text("Non Repository Props");
		ImGui::Text("");
		ImGui::Text("Prop Assembly Path");
		ImGui::SameLine();

		ImGui::InputText("##Prop Assembly Path", propAssemblyPath, sizeof(propAssemblyPath));
		ImGui::SameLine();

		if (ImGui::Button("Spawn Prop"))
		{
			int numberOfPropsToSpawn3 = std::atoi(numberOfPropsToSpawn2);

			for (int i = 0; i < numberOfPropsToSpawn3; ++i)
			{
				SpawnNonRepositoryProp(propAssemblyPath);
			}
		}

		ImGui::Text("Number Of Props To Spawn");
		ImGui::SameLine();

		ImGui::InputText("##NumberOfPropsToSpawn2", numberOfPropsToSpawn2, sizeof(numberOfPropsToSpawn2));
		ImGui::Separator();

		ImGui::Text("NPCs");
		ImGui::Text("");
		ImGui::Text("NPC Name");
		ImGui::SameLine();

		ImGui::InputText("##NPCName", npcName, sizeof(npcName));

		static char outfitName[256] { "" };

		ImGui::Text("Outfit");
		ImGui::SameLine();

		const bool isInputTextEnterPressed2 = ImGui::InputText("##OutfitName", outfitName, sizeof(outfitName), ImGuiInputTextFlags_EnterReturnsTrue);
		const bool isInputTextActive2 = ImGui::IsItemActive();
		const bool isInputTextActivated2 = ImGui::IsItemActivated();

		if (isInputTextActivated2)
		{
			ImGui::OpenPopup("##popup2");
		}

		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
		ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

		static ZRepositoryID repositoryID = ZRepositoryID("");
		static TEntityRef<ZGlobalOutfitKit>* globalOutfitKit = nullptr;
		static char currentCharacterSetIndex[3] { "0" };
		static const char* currentcharSetCharacterType = "HeroA";
		static char currentOutfitVariationIndex[3] { "0" };

		if (ImGui::BeginPopup("##popup2", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ChildWindow))
		{
			for (auto it = contentKitManager->m_repositoryGlobalOutfitKits.begin(); it != contentKitManager->m_repositoryGlobalOutfitKits.end(); ++it)
			{
				TEntityRef<ZGlobalOutfitKit>* globalOutfitKit2 = &it->second;
				const char* outfitName2 = globalOutfitKit2->m_pInterfaceRef->m_sCommonName.c_str();

				if (!strstr(outfitName2, outfitName))
				{
					continue;
				}

				if (ImGui::Selectable(outfitName2))
				{
					ImGui::ClearActiveID();
					strcpy_s(outfitName, outfitName2);

					repositoryID = it->first;
					globalOutfitKit = globalOutfitKit2;
				}
			}

			if (isInputTextEnterPressed2 || (!isInputTextActive2 && !ImGui::IsWindowFocused()))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::Text("Character Set Index");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharacterSetIndex", currentCharacterSetIndex))
		{
			if (globalOutfitKit)
			{
				for (size_t i = 0; i < globalOutfitKit->m_pInterfaceRef->m_aCharSets.size(); ++i)
				{
					std::string characterSetIndex = std::to_string(i);
					bool isSelected = currentCharacterSetIndex == characterSetIndex.c_str();

					if (ImGui::Selectable(characterSetIndex.c_str(), isSelected))
					{
						strcpy_s(currentCharacterSetIndex, characterSetIndex.c_str());
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("CharSet Character Type");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharSetCharacterType", currentcharSetCharacterType))
		{
			if (globalOutfitKit)
			{
				for (size_t i = 0; i < 3; ++i)
				{
					bool isSelected = currentcharSetCharacterType == charSetCharacterTypes[i];

					if (ImGui::Selectable(charSetCharacterTypes[i], isSelected))
					{
						currentcharSetCharacterType = charSetCharacterTypes[i];
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("Outfit Variation");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##OutfitVariation", currentOutfitVariationIndex))
		{
			if (globalOutfitKit)
			{
				unsigned int currentCharacterSetIndex2 = std::stoi(currentCharacterSetIndex);
				size_t variationCount = globalOutfitKit->m_pInterfaceRef->m_aCharSets[currentCharacterSetIndex2].m_pInterfaceRef->m_aCharacters[0].m_pInterfaceRef->m_aVariations.size();

				for (size_t i = 0; i < variationCount; ++i)
				{
					std::string outfitVariationIndex = std::to_string(i);
					bool isSelected = currentOutfitVariationIndex == outfitVariationIndex.c_str();

					if (ImGui::Selectable(outfitVariationIndex.c_str(), isSelected))
					{
						strcpy_s(currentOutfitVariationIndex, outfitVariationIndex.c_str());
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("Number Of Props To Spawn");
		ImGui::SameLine();

		ImGui::InputText("##NumberOfPropsToSpawn3", numberOfPropsToSpawn3, sizeof(numberOfPropsToSpawn3));

		if (ImGui::Button("Spawn NPC"))
		{
			int numberOfPropsToSpawn4 = std::atoi(numberOfPropsToSpawn3);

			for (int i = 0; i < numberOfPropsToSpawn4; ++i)
			{
				SpawnNPC(npcName, repositoryID, globalOutfitKit, currentCharacterSetIndex, currentcharSetCharacterType, currentOutfitVariationIndex);
			}
		}
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}
