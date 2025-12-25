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

        auto s_LocalHitman = SDK()->GetLocalPlayer();

        if (s_LocalHitman) {
            const ZSpatialEntity* s_SpatialEntity = SDK()->GetLocalPlayer().m_ref.QueryInterface<ZSpatialEntity>();
            const uint16 s_CurrentRoomEntityIndex = Functions::ZRoomManager_GetRoomFromPoint->Call(
                *Globals::RoomManager,
                s_SpatialEntity->GetWorldMatrix().Pos
            );
            const ZRoomEntity* s_CurrentRoomEntity = (*Globals::RoomManager)->m_RoomEntities[s_CurrentRoomEntityIndex];
            ZEntityRef s_CurrentRoomEntityRef;

            s_CurrentRoomEntity->GetID(s_CurrentRoomEntityRef);

            std::shared_ptr<EntityTreeNode> s_EntityTreeNode = m_CachedEntityTreeMap[s_CurrentRoomEntityRef];

            ImGui::Text("Current Room: %s", s_EntityTreeNode->Name.c_str());

            ImGui::SameLine();

            if (ImGui::SmallButton("Select In Entity Tree")) {
                OnSelectEntity(s_CurrentRoomEntityRef, true, std::nullopt);
            }

            ImGui::Separator();
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

        if (m_SortedRooms.empty()) {
            m_SortedRooms.reserve((*Globals::RoomManager)->m_RoomEntities.size());

            for (const auto& s_RoomEntity : (*Globals::RoomManager)->m_RoomEntities) {
                ZEntityRef ref;

                s_RoomEntity->GetID(ref);
                m_SortedRooms.push_back(ref);
            }

            std::sort(
                m_SortedRooms.begin(),
                m_SortedRooms.end(),
                [&](const ZEntityRef& a, const ZEntityRef& b) {
                    return m_CachedEntityTreeMap[a]->Name <
                        m_CachedEntityTreeMap[b]->Name;
                }
            );
        }

        for (const auto& s_RoomEntityRef : m_SortedRooms) {
            ZRoomEntity* s_RoomEntity = s_RoomEntityRef.QueryInterface<ZRoomEntity>();
            bool s_IsRoomVisible = false;

            for (const auto& s_RoomEntityIndex : (*Globals::RoomManager)->m_RoomsVisibleMain) {
                ZRoomEntity* s_RoomEntity2 = (*Globals::RoomManager)->m_RoomEntities[s_RoomEntityIndex];

                if (s_RoomEntity2 == s_RoomEntity) {
                    s_IsRoomVisible = true;
                    break;
                }
            }

            if (m_ShowOnlyVisibleRooms && !s_IsRoomVisible) {
                continue;
            }

            std::shared_ptr<EntityTreeNode> s_RoomEntityTreeNode = m_CachedEntityTreeMap[s_RoomEntityRef];

            if (!Util::StringUtils::FindSubstringUTF8(s_RoomEntityTreeNode->Name, s_RoomName)) {
                continue;
            }

            ImGuiTreeNodeFlags s_RoomNodeFlags =
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanFullWidth;

            ImGui::SetNextItemAllowOverlap();

            if (ImGui::TreeNodeEx(s_RoomEntityTreeNode->Name.c_str(), s_RoomNodeFlags)) {
                ImGui::SameLine();

                ImGui::PushID(s_RoomEntityTreeNode->EntityId);

                if (ImGui::SmallButton("Select In Entity Tree")) {
                    OnSelectEntity(s_RoomEntityRef, true, std::nullopt);
                }

                ImGui::PopID();

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
                            return m_CachedEntityTreeMap[a]->Name <
                                m_CachedEntityTreeMap[b]->Name;
                        }
                    );

                    for (const auto& s_GateEntityRef : s_SortedGates) {
                        const ZGateEntity* s_GateEntity = s_GateEntityRef.QueryInterface<ZGateEntity>();
                        bool s_IsGateVisible = false;

                        for (const auto& s_GateIndex : (*Globals::RoomManager)->m_GatesVisibleMain) {
                            SGateInfoHeader& s_GateInfoHeader = (*Globals::RoomManager)->m_GateHeaders[s_GateIndex];

                            if (s_GateInfoHeader.pGateEntity == s_GateEntity) {
                                s_IsGateVisible = true;
                                break;
                            }
                        }

                        if (m_ShowOnlyVisibleGates && !s_IsGateVisible) {
                            continue;
                        }

                        auto s_GateEntityTreeNode = m_CachedEntityTreeMap[s_GateEntityRef];

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
        }
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}