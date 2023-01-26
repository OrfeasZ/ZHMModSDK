#include "DebugMod.h"

#include <Glacier/ZAction.h>
#include <Glacier/ZItem.h>

void DebugMod::DrawItemsBox(bool p_HasFocus)
{
	if (!p_HasFocus || !m_ItemsMenuActive)
	{
		return;
	}

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("ITEMS", &m_ItemsMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing && p_HasFocus)
	{
		THashMap<ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>* repositoryData = nullptr;

		if (repositoryResource.m_nResourceIndex == -1)
		{
			const auto s_ID = ResId<"[assembly:/repository/pro.repo].pc_repo">;

			Globals::ResourceManager->GetResourcePtr(repositoryResource, s_ID, 0);
		}

		if (repositoryResource.GetResourceInfo().status == RESOURCE_STATUS_VALID)
		{
			repositoryData = static_cast<THashMap<ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>*>(repositoryResource.GetResourceData());
		}
		else
		{
			ImGui::PopFont();
			ImGui::End();
			ImGui::PopFont();

			return;
		}

		ZContentKitManager* contentKitManager = Globals::ContentKitManager;
		TEntityRef<ZHitman5> s_LocalHitman;

		Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

		ZSpatialEntity* s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
		ZHM5ActionManager* hm5ActionManager = Globals::HM5ActionManager;
		std::vector<ZHM5Action*> actions;

		if (hm5ActionManager->m_Actions.size() == 0)
		{
			ImGui::PopFont();
			ImGui::End();
			ImGui::PopFont();

			return;
		}

		for (unsigned int i = 0; i < hm5ActionManager->m_Actions.size(); ++i)
		{
			ZHM5Action* action = hm5ActionManager->m_Actions[i];

			if (action && action->m_eActionType == EActionType::AT_PICKUP)
			{
				actions.push_back(action);
			}
		}

		static size_t selected = 0;
		size_t count = actions.size();

		ImGui::BeginChild("left pane", ImVec2(300, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

		for (size_t i = 0; i < count; i++)
		{
			ZHM5Action* action = actions[i];
			ZHM5Item* item = action->m_Object.QueryInterface<ZHM5Item>();
			std::string title = std::format("{} {}", item->m_pItemConfigDescriptor->m_sTitle.c_str(), i + 1);

			if (ImGui::Selectable(title.c_str(), selected == i))
			{
				selected = i;
				textureResourceData.clear();
			}
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

		ZHM5Action* action = actions[selected];
		ZHM5Item* item = action->m_Object.QueryInterface<ZHM5Item>();
		ZDynamicObject* dynamicObject = &repositoryData->find(item->m_pItemConfigDescriptor->m_RepositoryId)->second;
		TArray<SDynamicObjectKeyValuePair>* entries = dynamicObject->As<TArray<SDynamicObjectKeyValuePair>>();
		std::string image;

		for (size_t i = 0; i < entries->size(); ++i)
		{
			std::string key = entries->operator[](i).sKey.c_str();

			if (key == "Image")
			{
				image = ConvertDynamicObjectValueTString(&entries->operator[](i).value);

				break;
			}
		}

		if (textureResourceData.size() == 0)
		{
			unsigned long long ddsTextureHash = GetDDSTextureHash(image);

			LoadResourceData(ddsTextureHash, textureResourceData);

			SDK()->LoadTextureFromMemory(textureResourceData, &textureSrvGPUHandle, width, height);
		}

		ImGui::Image(reinterpret_cast<ImTextureID>(textureSrvGPUHandle.ptr), ImVec2(static_cast<float>(width / 2), static_cast<float>(height / 2)));

		for (unsigned int i = 0; i < entries->size(); ++i)
		{
			std::string key = std::format("{}:", entries->operator[](i).sKey.c_str());
			IType* type = entries->operator[](i).value.m_pTypeID->typeInfo();

			if (strcmp(type->m_pTypeName, "TArray<ZDynamicObject>") == 0)
			{
				key += " [";

				ImGui::Text(key.c_str());

				TArray<ZDynamicObject>* array = entries->operator[](i).value.As<TArray<ZDynamicObject>>();

				for (unsigned int j = 0; j < array->size(); ++j)
				{
					std::string value = ConvertDynamicObjectValueTString(&array->operator[](j));

					if (!value.empty())
					{
						ImGui::Text(std::format("\t{}", value).c_str());
					}
				}

				ImGui::Text("]");
			}
			else
			{
				ImGui::Text(key.c_str());

				std::string value = ConvertDynamicObjectValueTString(&entries->operator[](i).value);

				ImGui::SameLine();
				ImGui::Text(value.c_str());
			}
		}

		if (ImGui::Button("Teleport Item To Player"))
		{
			ZSpatialEntity* s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
			//ZGeomEntity* geomEntity = s_LocalHitman.m_ref.QueryInterface<ZGeomEntity>();

			//ZEntityRef entityRef;

			//geomEntity->GetID(&entityRef);

			//item->m_pGeomEntity.m_ref = entityRef;
			//item->m_pGeomEntity.m_pInterfaceRef = geomEntity;
			item->m_rGeomentity.m_pInterfaceRef->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());
		}

		ImGui::EndChild();
		ImGui::EndGroup();
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}
