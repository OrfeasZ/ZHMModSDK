#include "DebugMod.h"
#include "imgui_internal.h"

#include <Glacier/ZContentKitManager.h>

void DebugMod::DrawAssetsBox(bool p_HasFocus) {
    if (!p_HasFocus || !m_AssetsMenuActive) {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("ASSETS", &m_AssetsMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing && p_HasFocus) {
        if (m_RepositoryProps.size() == 0) {
            LoadRepositoryProps();
        }

        ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;

        static char s_PropTitle_SubString[2048] {""};
        static char s_PropAssemblyPath[2048] {""};

        static int s_NumberOfPropsToSpawn_Repo = 1;
        static int s_NumberOfPropsToSpawn_NonRepo = 1;
        static int s_NumberOfPropsToSpawn_NPCs = 1;

        static int s_WorldInventoryButton = 1;
        static char s_NpcName[2048] {};

        ImGui::Text("Repository Props");
        ImGui::Text("");
        ImGui::Text("Prop Title");
        ImGui::SameLine();

        const bool s_IsInputTextEnterPressed = ImGui::InputText(
            "##PropRepositoryID", s_PropTitle_SubString, sizeof(s_PropTitle_SubString),
            ImGuiInputTextFlags_EnterReturnsTrue
        );
        const bool s_IsInputTextActive = ImGui::IsItemActive();

        if (ImGui::IsItemActivated()) {
            ImGui::OpenPopup("##popup");
        }

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

        if (ImGui::BeginPopup(
            "##popup",
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_ChildWindow
        )) {
            for (auto it = m_RepositoryProps.begin(); it != m_RepositoryProps.end(); ++it) {
                const std::string s_PropTitle = it->first.c_str();

                if (s_PropTitle.empty()) {
                    continue;
                }

                if (!FindSubstring(s_PropTitle, s_PropTitle_SubString)) {
                    continue;
                }

                std::string s_ButtonId = std::format("{}###{}", it->first.c_str(), it->second.ToString().c_str());

                if (ImGui::Selectable(s_ButtonId.c_str())) {
                    ImGui::ClearActiveID();
                    strcpy_s(s_PropTitle_SubString, s_PropTitle.c_str());

                    for (size_t i = 0; i < s_NumberOfPropsToSpawn_Repo; ++i) {
                        SpawnRepositoryProp(it->second, s_WorldInventoryButton == 1);
                    }
                }
            }

            if (s_IsInputTextEnterPressed || (!s_IsInputTextActive && !ImGui::IsWindowFocused())) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (ImGui::RadioButton("Add To World", s_WorldInventoryButton == 1)) {
            s_WorldInventoryButton = 1;
        }

        ImGui::SameLine();

        if (ImGui::RadioButton("Add To Inventory", s_WorldInventoryButton == 2)) {
            s_WorldInventoryButton = 2;
        }

        ImGui::Text("Number Of Props To Spawn");
        ImGui::SameLine();

        ImGui::InputInt("##NumberOfPropsToSpawn_Repo)", &s_NumberOfPropsToSpawn_Repo);

        ImGui::Separator();
        ImGui::Text("Non Repository Props");
        ImGui::Text("");
        ImGui::Text("Prop Assembly Path");
        ImGui::SameLine();

        ImGui::InputText("##Prop Assembly Path", s_PropAssemblyPath, sizeof(s_PropAssemblyPath));
        ImGui::SameLine();

        if (ImGui::Button("Spawn Prop")) {
            for (size_t i = 0; i < s_NumberOfPropsToSpawn_Repo; ++i) {
                SpawnNonRepositoryProp(s_PropAssemblyPath);
            }
        }

        ImGui::Text("Number Of Props To Spawn");
        ImGui::SameLine();

        ImGui::InputInt("##NumberOfPropsToSpawn_NonRepo", &s_NumberOfPropsToSpawn_NonRepo);
        ImGui::Separator();

        ImGui::Text("NPCs");
        ImGui::Text("");
        ImGui::Text("NPC Name");
        ImGui::SameLine();

        ImGui::InputText("##NPCName", s_NpcName, sizeof(s_NpcName));

        static char outfitName_SubString[2048] {""};

        ImGui::Text("Outfit");
        ImGui::SameLine();

        const bool s_IsInputTextEnterPressed2 = ImGui::InputText(
            "##OutfitName", outfitName_SubString, sizeof(outfitName_SubString), ImGuiInputTextFlags_EnterReturnsTrue
        );
        const bool s_IsInputTextActive2 = ImGui::IsItemActive();

        if (ImGui::IsItemActivated()) {
            ImGui::OpenPopup("##popup2");
        }

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

        static ZRepositoryID s_RepositoryId = ZRepositoryID("");
        static TEntityRef<ZGlobalOutfitKit>* s_GlobalOutfitKit = nullptr;
        static uint8_t n_CurrentCharacterSetIndex = 0;
        static std::string s_CurrentcharSetCharacterType = "HeroA";
        static uint8_t n_CurrentOutfitVariationIndex = 0;

        if (ImGui::BeginPopup(
            "##popup2",
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_ChildWindow
        )) {
            for (auto it = s_ContentKitManager->m_repositoryGlobalOutfitKits.begin(); it != s_ContentKitManager->
                 m_repositoryGlobalOutfitKits.end(); ++it) {
                TEntityRef<ZGlobalOutfitKit>* s_GlobalOutfitKit2 = &it->second;
                const std::string outfitName = s_GlobalOutfitKit2->m_pInterfaceRef->m_sCommonName.c_str();

                if (outfitName.empty()) {
                    continue;
                }

                if (!FindSubstring(outfitName, outfitName_SubString)) {
                    continue;
                }

                if (ImGui::Selectable(outfitName.c_str())) {
                    ImGui::ClearActiveID();
                    strcpy_s(outfitName_SubString, outfitName.c_str());

                    s_RepositoryId = it->first;
                    s_GlobalOutfitKit = s_GlobalOutfitKit2;
                }
            }

            if (s_IsInputTextEnterPressed2 || (!s_IsInputTextActive2 && !ImGui::IsWindowFocused())) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::Text("Character Set Index");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharacterSetIndex", std::to_string(n_CurrentCharacterSetIndex).data())) {
            if (s_GlobalOutfitKit) {
                for (size_t i = 0; i < s_GlobalOutfitKit->m_pInterfaceRef->m_aCharSets.size(); ++i) {
                    const bool s_IsSelected = n_CurrentCharacterSetIndex == i;

                    if (ImGui::Selectable(std::to_string(n_CurrentCharacterSetIndex).data(), s_IsSelected)) {
                        n_CurrentCharacterSetIndex = i;
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("CharSet Character Type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType", s_CurrentcharSetCharacterType.data())) {
            if (s_GlobalOutfitKit) {
                for (const auto& m_CharSetCharacterType : m_CharSetCharacterTypes) {
                    const bool s_IsSelected = s_CurrentcharSetCharacterType == m_CharSetCharacterType;

                    if (ImGui::Selectable(m_CharSetCharacterType.data(), s_IsSelected)) {
                        s_CurrentcharSetCharacterType = m_CharSetCharacterType;
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("Outfit Variation");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##OutfitVariation", std::to_string(n_CurrentOutfitVariationIndex).data())) {
            if (s_GlobalOutfitKit) {
                const uint8_t s_CurrentCharacterSetIndex2 = n_CurrentCharacterSetIndex;
                const size_t s_VariationCount = s_GlobalOutfitKit->m_pInterfaceRef->m_aCharSets[
                            s_CurrentCharacterSetIndex2].m_pInterfaceRef->m_aCharacters[0].m_pInterfaceRef->
                        m_aVariations.
                        size();

                for (size_t i = 0; i < s_VariationCount; ++i) {
                    const bool s_IsSelected = n_CurrentOutfitVariationIndex == i;

                    if (ImGui::Selectable(std::to_string(i).data(), s_IsSelected)) {
                        n_CurrentOutfitVariationIndex = i;
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("Number Of Props To Spawn");
        ImGui::SameLine();

        ImGui::InputInt("##NumberOfPropsToSpawn_NPCs", &s_NumberOfPropsToSpawn_NPCs);

        if (ImGui::Button("Spawn NPC")) {
            for (size_t i = 0; i < s_NumberOfPropsToSpawn_NPCs; ++i) {
                SpawnNPC(
                    s_NpcName,
                    s_RepositoryId,
                    s_GlobalOutfitKit,
                    n_CurrentCharacterSetIndex,
                    s_CurrentcharSetCharacterType,
                    n_CurrentOutfitVariationIndex
                );
            }
        }
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}
