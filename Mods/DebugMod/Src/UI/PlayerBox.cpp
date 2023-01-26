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

	ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;

	TEntityRef<ZHitman5> s_LocalHitman;
	Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("PLAYER", &m_PlayerMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing)
	{
		if (s_LocalHitman.m_pInterfaceRef)
		{
			static bool s_IsInvincible = s_LocalHitman.m_ref.GetProperty<bool>("m_bIsInvincible").Get();

			if (ImGui::Checkbox("Is Invincible", &s_IsInvincible))
			{
				s_LocalHitman.m_ref.SetProperty("m_bIsInvincible", s_IsInvincible);
			}

			if (ImGui::Button("Enable Infinite Ammo"))
			{
				EnableInfiniteAmmo();
			}
		}

		static char s_OutfitName[256] { "" };

		ImGui::Text("Outfit");
		ImGui::SameLine();

		const bool s_IsInputTextEnterPressed = ImGui::InputText("##OutfitName", s_OutfitName, sizeof(s_OutfitName), ImGuiInputTextFlags_EnterReturnsTrue);
		const bool s_IsInputTextActive = ImGui::IsItemActive();

		if (ImGui::IsItemActivated())
		{
			ImGui::OpenPopup("##popup");
		}

		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
		ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

		static TEntityRef<ZGlobalOutfitKit>* s_GlobalOutfitKit = nullptr;
		static char s_CurrentCharacterSetIndex[3] { "0" };
		static const char* s_CurrentcharSetCharacterType = "HeroA";
		static const char* s_CurrentcharSetCharacterType2 = "HeroA";
		static char s_CurrentOutfitVariationIndex[3] { "0" };

		if (ImGui::BeginPopup("##popup", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ChildWindow))
		{
			for (auto it = s_ContentKitManager->m_repositoryGlobalOutfitKits.begin(); it != s_ContentKitManager->m_repositoryGlobalOutfitKits.end(); ++it)
			{
				TEntityRef<ZGlobalOutfitKit>* s_GlobalOutfitKit2 = &it->second;
				const char* s_OutfitName2 = s_GlobalOutfitKit2->m_pInterfaceRef->m_sCommonName.c_str();

				if (!strstr(s_OutfitName2, s_OutfitName))
				{
					continue;
				}

				if (ImGui::Selectable(s_OutfitName2))
				{
					ImGui::ClearActiveID();
					strcpy_s(s_OutfitName, s_OutfitName2);

					EquipOutfit(it->second, std::stoi(s_CurrentCharacterSetIndex), s_CurrentcharSetCharacterType, std::stoi(s_CurrentOutfitVariationIndex), s_LocalHitman.m_pInterfaceRef);

					s_GlobalOutfitKit = s_GlobalOutfitKit2;
				}
			}

			if (s_IsInputTextEnterPressed || (!s_IsInputTextActive && !ImGui::IsWindowFocused()))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::Text("Character Set Index");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharacterSetIndex", s_CurrentCharacterSetIndex))
		{
			if (s_GlobalOutfitKit)
			{
				for (size_t i = 0; i < s_GlobalOutfitKit->m_pInterfaceRef->m_aCharSets.size(); ++i)
				{
					std::string s_CharacterSetIndex = std::to_string(i);
					const bool s_IsSelected = s_CurrentCharacterSetIndex == s_CharacterSetIndex.c_str();

					if (ImGui::Selectable(s_CharacterSetIndex.c_str(), s_IsSelected))
					{
						strcpy_s(s_CurrentCharacterSetIndex, s_CharacterSetIndex.c_str());

						if (s_GlobalOutfitKit)
						{
							EquipOutfit(*s_GlobalOutfitKit, std::stoi(s_CurrentCharacterSetIndex), s_CurrentcharSetCharacterType, std::stoi(s_CurrentOutfitVariationIndex), s_LocalHitman.m_pInterfaceRef);
						}
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("CharSet Character Type");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharSetCharacterType", s_CurrentcharSetCharacterType))
		{
			if (s_GlobalOutfitKit)
			{
				for (size_t i = 0; i < 3; ++i)
				{
					const bool s_IsSelected = s_CurrentcharSetCharacterType == m_CharSetCharacterTypes[i];

					if (ImGui::Selectable(m_CharSetCharacterTypes[i], s_IsSelected))
					{
						s_CurrentcharSetCharacterType = m_CharSetCharacterTypes[i];

						if (s_GlobalOutfitKit)
						{
							EquipOutfit(*s_GlobalOutfitKit, std::stoi(s_CurrentCharacterSetIndex), s_CurrentcharSetCharacterType, std::stoi(s_CurrentOutfitVariationIndex), s_LocalHitman.m_pInterfaceRef);
						}
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("Outfit Variation");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##OutfitVariation", s_CurrentOutfitVariationIndex))
		{
			if (s_GlobalOutfitKit)
			{
				const unsigned int s_CurrentCharacterSetIndex2 = std::stoi(s_CurrentCharacterSetIndex);
				const size_t s_VariationCount = s_GlobalOutfitKit->m_pInterfaceRef->m_aCharSets[s_CurrentCharacterSetIndex2].m_pInterfaceRef->m_aCharacters[0].m_pInterfaceRef->m_aVariations.size();

				for (size_t i = 0; i < s_VariationCount; ++i)
				{
					std::string s_OutfitVariationIndex = std::to_string(i);
					const bool s_IsSelected = s_CurrentOutfitVariationIndex == s_OutfitVariationIndex.c_str();

					if (ImGui::Selectable(s_OutfitVariationIndex.c_str(), s_IsSelected))
					{
						strcpy_s(s_CurrentOutfitVariationIndex, s_OutfitVariationIndex.c_str());

						if (s_GlobalOutfitKit)
						{
							EquipOutfit(*s_GlobalOutfitKit, std::stoi(s_CurrentCharacterSetIndex), s_CurrentcharSetCharacterType, std::stoi(s_CurrentOutfitVariationIndex), s_LocalHitman.m_pInterfaceRef);
						}
					}
				}
			}

			ImGui::EndCombo();
		}
		
		if (s_GlobalOutfitKit)
		{
			ImGui::Checkbox("Weapons Allowed", &s_GlobalOutfitKit->m_pInterfaceRef->m_bWeaponsAllowed);
			ImGui::Checkbox("Authority Figure", &s_GlobalOutfitKit->m_pInterfaceRef->m_bAuthorityFigure);
		}

		ImGui::Separator();

		static char npcName[256] { "" };

		ImGui::Text("NPC Name");
		ImGui::SameLine();

		ImGui::InputText("##NPCName", npcName, sizeof(npcName));
		ImGui::SameLine();

		if (ImGui::Button("Get NPC Outfit"))
		{
			const ZActor* s_Actor = Globals::ActorManager->GetActorByName(npcName);

			if (s_Actor)
			{
				EquipOutfit(s_Actor->m_rOutfit, s_Actor->m_nOutfitCharset, s_CurrentcharSetCharacterType2, s_Actor->m_nOutfitVariation, s_LocalHitman.m_pInterfaceRef);
			}
		}

		if (ImGui::Button("Get Nearest NPC's Outfit"))
		{
			const ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

			for (int i = 0; i < *Globals::NextActorId; ++i)
			{
				ZActor* actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
				ZEntityRef s_Ref;

				actor->GetID(&s_Ref);

				ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

				const SVector3 s_Temp = s_ActorSpatialEntity->m_mTransform.Trans - s_HitmanSpatialEntity->m_mTransform.Trans;
				const float s_Distance = sqrt(s_Temp.x * s_Temp.x + s_Temp.y * s_Temp.y + s_Temp.z * s_Temp.z);

				if (s_Distance <= 3.0f)
				{
					EquipOutfit(actor->m_rOutfit, actor->m_nOutfitCharset, s_CurrentcharSetCharacterType2, actor->m_nOutfitVariation, s_LocalHitman.m_pInterfaceRef);

					break;
				}
			}
		}

		ImGui::Text("CharSet Character Type");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharSetCharacterType2", s_CurrentcharSetCharacterType2))
		{
			if (s_GlobalOutfitKit)
			{
				for (size_t i = 0; i < 3; ++i)
				{
					const bool s_IsSelected = s_CurrentcharSetCharacterType2 == m_CharSetCharacterTypes[i];

					if (ImGui::Selectable(m_CharSetCharacterTypes[i], s_IsSelected))
					{
						s_CurrentcharSetCharacterType2 = m_CharSetCharacterTypes[i];
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Separator();

		if (ImGui::Button("Teleport All Items To Player"))
		{
			auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
			const ZHM5ActionManager* s_Hm5ActionManager = Globals::HM5ActionManager;

			for (unsigned int i = 0; i < s_Hm5ActionManager->m_Actions.size(); ++i)
			{
				ZHM5Action* s_Action = s_Hm5ActionManager->m_Actions[i];

				if (s_Action->m_eActionType == EActionType::AT_PICKUP)
				{
					const ZHM5Item* s_Item = s_Action->m_Object.QueryInterface<ZHM5Item>();

					s_Item->m_rGeomentity.m_pInterfaceRef->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());
				}
			}
		}

		if (ImGui::Button("Teleport All NPCs To Player"))
		{
			auto s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

			for (int i = 0; i < *Globals::NextActorId; ++i)
			{
				ZActor* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
				ZEntityRef s_Ref;

				s_Actor->GetID(&s_Ref);

				ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

				s_ActorSpatialEntity->SetWorldMatrix(s_HitmanSpatialEntity->GetWorldMatrix());
			}
		}
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}
