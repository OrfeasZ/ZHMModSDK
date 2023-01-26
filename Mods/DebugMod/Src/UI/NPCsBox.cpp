#include "DebugMod.h"

#include <Glacier/ZActor.h>
#include <Glacier/ZContentKitManager.h>
#include <Glacier/ZSpatialEntity.h>

#include "imgui_internal.h"

void DebugMod::DrawNPCsBox(bool p_HasFocus)
{
	if (!p_HasFocus || !m_NPCsMenuActive)
	{
		return;
	}

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("NPCs", &m_NPCsMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing && p_HasFocus)
	{
		ZContentKitManager* contentKitManager = Globals::ContentKitManager;
		static size_t selected = 0;

		ImGui::BeginChild("left pane", ImVec2(300, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

		static char npcName[256] { "" };

		ImGui::Text("NPC Name");
		ImGui::SameLine();

		ImGui::InputText("##NPCName", npcName, sizeof(npcName));

		for (int i = 0; i < *Globals::NextActorId; ++i)
		{
			ZActor* actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
			std::string npcName2 = actor->m_sActorName.c_str();

			if (!strstr(npcName2.c_str(), npcName))
			{
				continue;
			}

			if (ImGui::Selectable(npcName2.c_str(), selected == i))
			{
				selected = i;
			}
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

		ZActor* actor = Globals::ActorManager->m_aActiveActors[selected].m_pInterfaceRef;
		static char outfitName[256] { "" };

		ImGui::Text("Outfit");
		ImGui::SameLine();

		const bool isInputTextEnterPressed = ImGui::InputText("##OutfitName", outfitName, sizeof(outfitName), ImGuiInputTextFlags_EnterReturnsTrue);
		const bool isInputTextActive = ImGui::IsItemActive();
		const bool isInputTextActivated = ImGui::IsItemActivated();

		if (isInputTextActivated)
		{
			ImGui::OpenPopup("##popup");
		}

		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
		ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

		static TEntityRef<ZGlobalOutfitKit>* globalOutfitKit = nullptr;
		static char currentCharacterSetIndex[3] { "0" };
		static const char* currentcharSetCharacterType = "Actor";
		static const char* currentcharSetCharacterType2 = "Actor";
		static char currentOutfitVariationIndex[3] { "0" };

		if (ImGui::BeginPopup("##popup", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ChildWindow))
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

					EquipOutfit(it->second, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), actor);

					globalOutfitKit = globalOutfitKit2;
				}
			}

			if (isInputTextEnterPressed || (!isInputTextActive && !ImGui::IsWindowFocused()))
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

						if (globalOutfitKit)
						{
							EquipOutfit(*globalOutfitKit, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), actor);
						}
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

						if (globalOutfitKit)
						{
							EquipOutfit(*globalOutfitKit, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), actor);
						}
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

						if (globalOutfitKit)
						{
							EquipOutfit(*globalOutfitKit, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), actor);
						}
					}
				}
			}

			ImGui::EndCombo();
		}

		static bool weaponsAllowed = false;
		static bool authorityFigure = false;

		if (globalOutfitKit)
		{
			ImGui::Checkbox("Weapons Allowed", &globalOutfitKit->m_pInterfaceRef->m_bWeaponsAllowed);
			ImGui::Checkbox("Authority Figure", &globalOutfitKit->m_pInterfaceRef->m_bAuthorityFigure);
		}

		ImGui::Separator();

		static char npcName2[256] { "" };

		ImGui::Text("NPC Name");
		ImGui::SameLine();

		ImGui::InputText("##NPCName", npcName2, sizeof(npcName2));
		ImGui::SameLine();

		if (ImGui::Button("Get NPC Outfit"))
		{
			ZActor* actor2 = Globals::ActorManager->GetActorByName(npcName2);

			if (actor2)
			{
				EquipOutfit(actor2->m_rOutfit, actor2->m_nOutfitCharset, currentcharSetCharacterType2, actor2->m_nOutfitVariation, actor);
			}
		}

		if (ImGui::Button("Get Nearest NPC's Outfit"))
		{
			ZEntityRef s_Ref;

			actor->GetID(&s_Ref);

			ZSpatialEntity* actorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

			for (int i = 0; i < *Globals::NextActorId; ++i)
			{
				ZActor* actor2 = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
				ZEntityRef s_Ref;

				actor2->GetID(&s_Ref);

				ZSpatialEntity* actorSpatialEntity2 = s_Ref.QueryInterface<ZSpatialEntity>();

				SVector3 temp = actorSpatialEntity->m_mTransform.Trans - actorSpatialEntity2->m_mTransform.Trans;
				float distance = sqrt(temp.x * temp.x + temp.y * temp.y + temp.z * temp.z);

				if (distance <= 3.0f)
				{
					EquipOutfit(actor2->m_rOutfit, actor2->m_nOutfitCharset, currentcharSetCharacterType2, actor2->m_nOutfitVariation, actor);

					break;
				}
			}
		}

		ImGui::Text("CharSet Character Type");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharSetCharacterType", currentcharSetCharacterType2))
		{
			if (globalOutfitKit)
			{
				for (size_t i = 0; i < 3; ++i)
				{
					bool isSelected = currentcharSetCharacterType2 == charSetCharacterTypes[i];

					if (ImGui::Selectable(charSetCharacterTypes[i], isSelected))
					{
						currentcharSetCharacterType2 = charSetCharacterTypes[i];
					}
				}
			}

			ImGui::EndCombo();
		}

		if (ImGui::Button("Teleport NPC To Player"))
		{
			TEntityRef<ZHitman5> localHitman;

			Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &localHitman);

			ZEntityRef s_Ref;

			actor->GetID(&s_Ref);

			ZSpatialEntity* hitmanSpatialEntity = localHitman.m_ref.QueryInterface<ZSpatialEntity>();
			ZSpatialEntity* actorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

			actorSpatialEntity->SetWorldMatrix(hitmanSpatialEntity->GetWorldMatrix());
		}

		ImGui::EndChild();
		ImGui::EndGroup();
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}
