#include "DebugMod.h"

#include <numbers>

#include <Glacier/ZGeomEntity.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZPhysics.h>

void DebugMod::DrawEntityBox(bool p_HasFocus)
{
	if (!p_HasFocus || !m_EntityMenuActive)
	{
		return;
	}

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("SELECTED ENTITY", &m_EntityMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing)
	{
		m_EntityMutex.lock_shared();

		if (!m_SelectedEntity)
		{
			ImGui::Text("No entity selected.");
		}
		else
		{
			if (brickHashes.empty())
			{
				ZEntitySceneContext* entitySceneContext = Globals::Hitman5Module->m_pEntitySceneContext;

				for (size_t i = 0; i < entitySceneContext->m_aLoadedBricks.size(); ++i)
				{
					ZRuntimeResourceID runtimeResourceID = entitySceneContext->m_aLoadedBricks[i].runtimeResourceID;

					brickHashes.insert(runtimeResourceID.GetID());
				}
			}

			if (selectedEntityName.empty())
			{
				ZEntityType** parentEntityType = reinterpret_cast<ZEntityType**>(reinterpret_cast<char*>(m_SelectedEntity.m_pEntity) + m_SelectedEntity.m_pEntity[0]->m_nLogicalParentEntityOffset);
				entityID = m_SelectedEntity.m_pEntity[0]->m_nEntityId;
				unsigned long long parentEntityID = parentEntityType[0]->m_nEntityId;
				ZEntityType** parentEntityType2 = parentEntityType;

				while (true)
				{
					parentEntityType2 = reinterpret_cast<ZEntityType**>(reinterpret_cast<char*>(parentEntityType2) + parentEntityType2[0]->m_nLogicalParentEntityOffset);
					unsigned long long entityID2 = (*reinterpret_cast<ZEntityType**>(reinterpret_cast<char*>(parentEntityType2) + parentEntityType2[0]->m_nLogicalParentEntityOffset))->m_nEntityId;

					if (brickHashes.contains(entityID2))
					{
						brickEntityID = entityID2;
						selectedEntityName = GetEntityName(entityID2, entityID, selectedResourceHash);

						if (selectedEntityName.empty())
						{
							entityID = parentEntityType[0]->m_nEntityId;
							selectedEntityName = GetEntityName(entityID2, entityID, selectedResourceHash);
						}

						if (selectedEntityName.empty() && m_SelectedEntity.m_pEntity[0]->m_nEntityId == 0x01e018a77e7655ca) //Case when NPC is added by another brick
						{
							selectedEntityName = FindNPCEntityNameInBrickBackReferences(entityID2, entityID, selectedResourceHash);
						}

						break;
					}
				}
			}

			ImGui::TextUnformatted(fmt::format("Entity Name: {}", selectedEntityName).c_str());
			ImGui::TextUnformatted(fmt::format("Entity ID: {:016x}", entityID).c_str());

			if (runtimeResourceIDsToResourceIDs.contains(selectedResourceHash))
			{
				ImGui::TextUnformatted(fmt::format("Template Entity: {}", runtimeResourceIDsToResourceIDs[selectedResourceHash]).c_str());
			}
			else
			{
				ImGui::TextUnformatted(fmt::format("Template Entity: {}", selectedResourceHash).c_str());
			}

			if (runtimeResourceIDsToResourceIDs.contains(brickEntityID))
			{
				ImGui::TextUnformatted(fmt::format("Brick Template Entity: {}", runtimeResourceIDsToResourceIDs[brickEntityID]).c_str());
			}
			else
			{
				ImGui::TextUnformatted(fmt::format("Brick Template Entity: {}", brickEntityID).c_str());
			}

			const ZGeomEntity* geomEntity = m_SelectedEntity.QueryInterface<ZGeomEntity>();

			if (geomEntity)
			{
				ZVariant<ZResourcePtr> property = m_SelectedEntity.GetProperty<ZResourcePtr>("m_ResourceID");
				ZResourceContainer::SResourceInfo primResourceInfo = (*Globals::ResourceContainer)->m_resources[property.Get().m_nResourceIndex];
				unsigned long long primHash = primResourceInfo.rid.GetID();

				if (runtimeResourceIDsToResourceIDs.contains(primHash))
				{
					ImGui::TextUnformatted(fmt::format("PRIM Assembly Path: {}", runtimeResourceIDsToResourceIDs[primHash]).c_str());
				}
				else
				{
					ImGui::TextUnformatted(fmt::format("PRIM Assembly Path: {}", primHash).c_str());
				}
			}

			const auto& s_Interfaces = *(*m_SelectedEntity.m_pEntity)->m_pInterfaces;

			ImGui::TextUnformatted(fmt::format("Entity Type: {}", s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName).c_str());
			ImGui::TextUnformatted(fmt::format("Entity ID: {:016x}", (*m_SelectedEntity.m_pEntity)->m_nEntityId).c_str());

			std::string s_InterfacesStr;

			for (const auto& s_Interface : s_Interfaces)
			{
				if (s_Interface.m_pTypeId->typeInfo() == nullptr)
					continue;

				if (!s_InterfacesStr.empty())
					s_InterfacesStr += ", ";

				s_InterfacesStr += s_Interface.m_pTypeId->typeInfo()->m_pTypeName;
			}

			ImGui::TextUnformatted(fmt::format("Entity Interfaces: {}", s_InterfacesStr).c_str());

			std::string s_Properties01;

			for (const auto& s_Property : *(*m_SelectedEntity.m_pEntity)->m_pProperties01)
			{
				if (!s_Property.m_pType->getPropertyInfo())
					continue;

				if (!s_Property.m_pType->getPropertyInfo()->m_pName)
					continue;

				if (!s_Properties01.empty())
					s_Properties01 += ", ";

				s_Properties01 += s_Property.m_pType->getPropertyInfo()->m_pName;
			}

			ImGui::TextUnformatted(fmt::format("Entity Properties1: {}", s_Properties01).c_str());

			std::string s_Properties02;

			for (const auto& s_Property : *(*m_SelectedEntity.m_pEntity)->m_pProperties02)
			{
				if (!s_Property.m_pType->getPropertyInfo())
					continue;

				if (!s_Property.m_pType->getPropertyInfo()->m_pName)
					continue;

				if (!s_Properties02.empty())
					s_Properties02 += ", ";

				s_Properties02 += s_Property.m_pType->getPropertyInfo()->m_pName;
			}

			ImGui::TextUnformatted(fmt::format("Entity Properties2: {}", s_Properties02).c_str());


			if (const auto s_Spatial = m_SelectedEntity.QueryInterface<ZSpatialEntity>())
			{
				const auto s_Trans = s_Spatial->GetWorldMatrix();

				ImGui::TextUnformatted("Entity Transform:");

				if (ImGui::BeginTable("DebugMod_HitmanPosition", 4))
				{
					for (int i = 0; i < 4; ++i)
					{
						ImGui::TableNextRow();

						for (int j = 0; j < 4; ++j)
						{
							ImGui::TableSetColumnIndex(j);
							ImGui::Text("%f", s_Trans.flt[(i * 4) + j]);
						}
					}

					ImGui::EndTable();
				}

				if (ImGui::Button("Copy Transform"))
				{
					CopyToClipboard(fmt::format("{}", s_Trans));
				}

				ImGui::SameLine();

				if (ImGui::Button("RT JSON##EntRT"))
				{
					CopyToClipboard(fmt::format(
						"{{\"XAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"YAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"ZAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"Trans\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
						s_Trans.XAxis.x, s_Trans.XAxis.y, s_Trans.XAxis.z,
						s_Trans.YAxis.x, s_Trans.YAxis.y, s_Trans.YAxis.z,
						s_Trans.ZAxis.x, s_Trans.ZAxis.y, s_Trans.ZAxis.z,
						s_Trans.Trans.x, s_Trans.Trans.y, s_Trans.Trans.z
					));
				}

				ImGui::SameLine();

				if (ImGui::Button("QN JSON##EntQN"))
				{
					// This is adapted from QN: https://github.com/atampy25/quickentity-rs/blob/b94dbab32c91ccd0e1612a2e5dda8fb83eb2a8e9/src/lib.rs#L243
					constexpr double c_RAD2DEG = 180.0 / std::numbers::pi;

					double s_RotationX = abs(s_Trans.XAxis.z) < 0.9999999f
						? atan2f(-s_Trans.YAxis.z, s_Trans.ZAxis.z) * c_RAD2DEG
						: atan2f(s_Trans.ZAxis.y, s_Trans.YAxis.y) * c_RAD2DEG;

					double s_RotationY = asinf(min(max(-1.f, s_Trans.XAxis.z), 1.f)) * c_RAD2DEG;

					double s_RotationZ = abs(s_Trans.XAxis.z) < 0.9999999f
						? atan2f(-s_Trans.XAxis.y, s_Trans.XAxis.x) * c_RAD2DEG
						: 0.f;

					CopyToClipboard(fmt::format(
						"{{\"rotation\":{{\"x\":{},\"y\":{},\"z\":{}}},\"position\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
						s_RotationX, s_RotationY, s_RotationZ,
						s_Trans.Trans.x, s_Trans.Trans.y, s_Trans.Trans.z
					));
				}

				if (ImGui::Button("Copy ID"))
				{
					CopyToClipboard(fmt::format("{:016x}", (*m_SelectedEntity.m_pEntity)->m_nEntityId));
				}

				if (ImGui::Button("Move to Hitman"))
				{
					TEntityRef<ZHitman5> s_LocalHitman;
					Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

					auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

					s_Spatial->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());

					if (const auto s_PhysicsAspect = m_SelectedEntity.QueryInterface<ZStaticPhysicsAspect>())
						s_PhysicsAspect->m_pPhysicsObject->SetTransform(s_Spatial->GetWorldMatrix());
				}
			}
		}

		m_EntityMutex.unlock_shared();
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}
