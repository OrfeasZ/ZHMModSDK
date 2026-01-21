#include "Editor.h"

#include "Glacier/ZRoom.h"

#include "Util/StringUtils.h"

void Editor::DrawRooms(const bool p_HasFocus) {
    if (!p_HasFocus || !m_RoomsMenuActive) {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("Rooms", &m_RoomsMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing && p_HasFocus) {
        if (!m_CachedEntityTree) {
            if (ImGui::Button("Get Rooms")) {
                UpdateEntities();
            }

            ImGui::PopFont();
            ImGui::End();
            ImGui::PopFont();

            return;
        }

        const auto s_LocalHitman = SDK()->GetLocalPlayer();

        if (s_LocalHitman) {
            const ZSpatialEntity* s_SpatialEntity = SDK()->GetLocalPlayer().m_entityRef.QueryInterface<ZSpatialEntity>();
            const uint16 s_CurrentRoomEntityIndex = Functions::ZRoomManager_GetRoomFromPoint->Call(
                *Globals::RoomManager,
                s_SpatialEntity->GetWorldMatrix().Pos
            );
            const ZRoomEntity* s_CurrentRoomEntity = (*Globals::RoomManager)->m_RoomEntities[s_CurrentRoomEntityIndex];
            ZEntityRef s_CurrentRoomEntityRef;

            s_CurrentRoomEntity->GetID(s_CurrentRoomEntityRef);

            auto s_Iterator = m_CachedEntityTreeMap.find(s_CurrentRoomEntityRef);
            
            if (s_Iterator != m_CachedEntityTreeMap.end()) {
                std::shared_ptr<EntityTreeNode> s_EntityTreeNode = s_Iterator->second;

                ImGui::Text("Room player is in: %s", s_EntityTreeNode->Name.c_str());

                ImGui::SameLine();

                if (ImGui::SmallButton("Select In Entity Tree##Player")) {
                    OnSelectEntity(s_CurrentRoomEntityRef, true, std::nullopt);
                }

                ImGui::Separator();
            }
        }

        const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

        if (s_CurrentCamera) {
            const uint16 s_CurrentRoomEntityIndex = Functions::ZRoomManager_GetRoomFromPoint->Call(
                *Globals::RoomManager,
                s_CurrentCamera->GetWorldMatrix().Pos
            );
            const ZRoomEntity* s_CurrentRoomEntity = (*Globals::RoomManager)->m_RoomEntities[s_CurrentRoomEntityIndex];
            ZEntityRef s_CurrentRoomEntityRef;

            s_CurrentRoomEntity->GetID(s_CurrentRoomEntityRef);

            auto s_Iterator = m_CachedEntityTreeMap.find(s_CurrentRoomEntityRef);

            if (s_Iterator != m_CachedEntityTreeMap.end()) {
                std::shared_ptr<EntityTreeNode> s_EntityTreeNode = s_Iterator->second;

                ImGui::Text("Room camera is in: %s", s_EntityTreeNode->Name.c_str());

                ImGui::SameLine();

                if (ImGui::SmallButton("Select In Entity Tree##Camera")) {
                    OnSelectEntity(s_CurrentRoomEntityRef, true, std::nullopt);
                }

                ImGui::Separator();
            }
        }

        ImGui::Checkbox("Show only visible rooms", &m_ShowOnlyVisibleRooms);
        ImGui::Checkbox("Show only visible gates", &m_ShowOnlyVisibleGates);

        static char s_RoomName[2048]{ "" };

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Room Name");
        ImGui::SameLine();

        ImGui::InputText("##RoomName", s_RoomName, sizeof(s_RoomName));

        static char s_GateName[2048]{ "" };

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Gate Name");
        ImGui::SameLine();

        ImGui::InputText("##GateName", s_GateName, sizeof(s_GateName));

        if (m_SortedRoomEntities.empty()) {
            m_SortedRoomEntities.reserve((*Globals::RoomManager)->m_RoomEntities.size());

            for (const auto& s_RoomEntity : (*Globals::RoomManager)->m_RoomEntities) {
                ZEntityRef s_EntityRef;

                s_RoomEntity->GetID(s_EntityRef);
                m_SortedRoomEntities.push_back(s_EntityRef);
            }

            std::sort(
                m_SortedRoomEntities.begin(),
                m_SortedRoomEntities.end(),
                [&](const ZEntityRef& a, const ZEntityRef& b) {
                    auto s_IteratorA = m_CachedEntityTreeMap.find(a);
                    auto s_IteratorB = m_CachedEntityTreeMap.find(b);

                    if (s_IteratorA == m_CachedEntityTreeMap.end()) {
                        return false;
                    }

                    if (s_IteratorB == m_CachedEntityTreeMap.end()) {
                        return true;
                    }

                    return s_IteratorA->second->Name < s_IteratorB->second->Name;
                }
            );
        }

        for (size_t i = 0; i < m_SortedRoomEntities.size(); ++i) {
            ZEntityRef& s_RoomEntityRef = m_SortedRoomEntities[i];
            ZRoomEntity* s_RoomEntity = s_RoomEntityRef.QueryInterface<ZRoomEntity>();
            bool s_IsRoomVisible = false;

            if (m_ShowOnlyVisibleRooms) {
                for (const auto& s_RoomEntityIndex : (*Globals::RoomManager)->m_RoomsVisibleMain) {
                    ZRoomEntity* s_RoomEntity2 = (*Globals::RoomManager)->m_RoomEntities[s_RoomEntityIndex];

                    if (s_RoomEntity2 == s_RoomEntity) {
                        s_IsRoomVisible = true;
                        break;
                    }
                }

                if (!s_IsRoomVisible) {
                    continue;
                }
            }

            auto s_Iterator = m_CachedEntityTreeMap.find(s_RoomEntityRef);

            if (s_Iterator == m_CachedEntityTreeMap.end()) {
                continue;
            }

            std::shared_ptr<EntityTreeNode> s_RoomEntityTreeNode = s_Iterator->second;

            if (!Util::StringUtils::FindSubstringUTF8(s_RoomEntityTreeNode->Name, s_RoomName)) {
                continue;
            }

            ImGuiTreeNodeFlags s_RoomNodeFlags =
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanFullWidth;

            ImGui::SetNextItemAllowOverlap();

            ImGui::PushID(i);

            bool s_IsTreeNodeOpen = ImGui::TreeNodeEx(s_RoomEntityTreeNode->Name.c_str(), s_RoomNodeFlags);

            ImGui::SameLine();

            if (ImGui::SmallButton("Select In Entity Tree")) {
                OnSelectEntity(s_RoomEntityRef, true, std::nullopt);
            }

            if (s_IsTreeNodeOpen) {
                if (ImGui::TreeNode("Gates")) {
                    std::vector<ZEntityRef> s_SortedGates;

                    s_SortedGates.reserve(s_RoomEntity->m_Gates.size());

                    for (const auto& s_GateEntity : s_RoomEntity->m_Gates) {
                        ZEntityRef gateRef;

                        s_GateEntity->GetID(gateRef);
                        s_SortedGates.push_back(gateRef);
                    }

                    std::sort(
                        s_SortedGates.begin(),
                        s_SortedGates.end(),
                        [&](const ZEntityRef& a, const ZEntityRef& b) {
                            auto s_IteratorA = m_CachedEntityTreeMap.find(a);
                            auto s_IteratorB = m_CachedEntityTreeMap.find(b);

                            if (s_IteratorA == m_CachedEntityTreeMap.end()) {
                                return false;
                            }

                            if (s_IteratorB == m_CachedEntityTreeMap.end()) {
                                return true;
                            }

                            return s_IteratorA->second->Name < s_IteratorB->second->Name;
                        }
                    );

                    for (const auto& s_GateEntityRef : s_SortedGates) {
                        const ZGateEntity* s_GateEntity = s_GateEntityRef.QueryInterface<ZGateEntity>();
                        bool s_IsGateVisible = false;

                        if (m_ShowOnlyVisibleGates) {
                            for (const auto& s_GateIndex : (*Globals::RoomManager)->m_GatesVisibleMain) {
                                SGateInfoHeader& s_GateInfoHeader = (*Globals::RoomManager)->m_GateHeaders[s_GateIndex];

                                if (s_GateInfoHeader.pGateEntity == s_GateEntity) {
                                    s_IsGateVisible = true;
                                    break;
                                }
                            }

                            if (!s_IsGateVisible) {
                                continue;
                            }
                        }

                        auto s_Iterator = m_CachedEntityTreeMap.find(s_GateEntityRef);

                        if (s_Iterator == m_CachedEntityTreeMap.end()) {
                            continue;
                        }

                        std::shared_ptr<EntityTreeNode> s_GateEntityTreeNode = s_Iterator->second;

                        if (!Util::StringUtils::FindSubstringUTF8(s_GateEntityTreeNode->Name, s_GateName)) {
                            continue;
                        }

                        ImGuiTreeNodeFlags s_GateNodeFlags =
                            ImGuiTreeNodeFlags_Leaf |
                            ImGuiTreeNodeFlags_NoTreePushOnOpen |
                            ImGuiTreeNodeFlags_SpanFullWidth;

                        ImGui::SetNextItemAllowOverlap();

                        ImGui::TreeNodeEx(s_GateEntityTreeNode->Name.c_str(), s_GateNodeFlags);

                        ImGui::SameLine();

                        ImGui::PushID(s_GateEntityTreeNode->EntityId);

                        if (ImGui::SmallButton("Select In Entity Tree")) {
                            OnSelectEntity(s_GateEntityRef, true, std::nullopt);
                        }

                        ImGui::PopID();
                    }

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}