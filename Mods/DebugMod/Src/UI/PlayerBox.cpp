#include "DebugMod.h"

#include <Glacier/ZContentKitManager.h>
#include <Glacier/ZActor.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZAction.h>
#include <Glacier/ZItem.h>
#include <imgui_internal.h>

void DebugMod::DrawPlayerBox(bool p_HasFocus)
{
	if (!p_HasFocus || !m_PlayerMenuActive)
	{
		return;
	}

	ZContentKitManager* contentKitManager = Globals::ContentKitManager;
	TEntityRef<ZHitman5> s_LocalHitman;

	Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("PLAYER", &m_PlayerMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing)
	{
		if (s_LocalHitman.m_pInterfaceRef)
		{
			static bool isInvincible = s_LocalHitman.m_ref.GetProperty<bool>("m_bIsInvincible").Get();

			if (ImGui::Checkbox("Is Invincible", &isInvincible))
			{
				s_LocalHitman.m_ref.SetProperty("m_bIsInvincible", isInvincible);
			}

			if (ImGui::Button("Enable Infinite Ammo"))
			{
				EnableInfiniteAmmo();
			}
		}

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
		static const char* currentcharSetCharacterType = "HeroA";
		static const char* currentcharSetCharacterType2 = "HeroA";
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

					EquipOutfit(it->second, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), s_LocalHitman.m_pInterfaceRef);

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
							EquipOutfit(*globalOutfitKit, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), s_LocalHitman.m_pInterfaceRef);
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
							EquipOutfit(*globalOutfitKit, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), s_LocalHitman.m_pInterfaceRef);
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
							EquipOutfit(*globalOutfitKit, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), s_LocalHitman.m_pInterfaceRef);
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

		static char npcName[256] { "" };

		ImGui::Text("NPC Name");
		ImGui::SameLine();

		ImGui::InputText("##NPCName", npcName, sizeof(npcName));
		ImGui::SameLine();

		if (ImGui::Button("Get NPC Outfit"))
		{
			ZActor* actor = Globals::ActorManager->GetActorByName(npcName);

			if (actor)
			{
				EquipOutfit(actor->m_rOutfit, actor->m_nOutfitCharset, currentcharSetCharacterType2, actor->m_nOutfitVariation, s_LocalHitman.m_pInterfaceRef);
			}
		}

		if (ImGui::Button("Get Nearest NPC's Outfit"))
		{
			ZSpatialEntity* hitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

			for (int i = 0; i < *Globals::NextActorId; ++i)
			{
				ZActor* actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
				ZEntityRef s_Ref;

				actor->GetID(&s_Ref);

				ZSpatialEntity* actorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

				SVector3 temp = actorSpatialEntity->m_mTransform.Trans - hitmanSpatialEntity->m_mTransform.Trans;
				float distance = sqrt(temp.x * temp.x + temp.y * temp.y + temp.z * temp.z);

				if (distance <= 3.0f)
				{
					EquipOutfit(actor->m_rOutfit, actor->m_nOutfitCharset, currentcharSetCharacterType2, actor->m_nOutfitVariation, s_LocalHitman.m_pInterfaceRef);

					break;
				}
			}
		}

		ImGui::Text("CharSet Character Type");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharSetCharacterType2", currentcharSetCharacterType2))
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

		ImGui::Separator();

		if (ImGui::Button("Teleport All Items To Player"))
		{
			ZSpatialEntity* s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
			ZHM5ActionManager* hm5ActionManager = Globals::HM5ActionManager;

			for (unsigned int i = 0; i < hm5ActionManager->m_Actions.size(); ++i)
			{
				ZHM5Action* action = hm5ActionManager->m_Actions[i];

				if (action->m_eActionType == EActionType::AT_PICKUP)
				{
					ZHM5Item* item = action->m_Object.QueryInterface<ZHM5Item>();

					item->m_rGeomentity.m_pInterfaceRef->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());
				}
			}
		}

		if (ImGui::Button("Teleport All NPCs To Player"))
		{
			TEntityRef<ZHitman5> localHitman;

			Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &localHitman);

			ZSpatialEntity* hitmanSpatialEntity = localHitman.m_ref.QueryInterface<ZSpatialEntity>();

			for (int i = 0; i < *Globals::NextActorId; ++i)
			{
				ZActor* actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
				ZEntityRef s_Ref;

				actor->GetID(&s_Ref);

				ZSpatialEntity* actorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

				actorSpatialEntity->SetWorldMatrix(hitmanSpatialEntity->GetWorldMatrix());
			}
		}
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}
