#include "DebugMod.h"

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZScene.h>
#include <Glacier/ZActor.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/EntityFactory.h>
#include <Glacier/ZCollision.h>
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZHitman5.h>
#include <Glacier/ZHttp.h>
#include <Glacier/ZPhysics.h>
#include <Glacier/ZSetpieceEntity.h>
#include <Glacier/ZContentKitManager.h>
#include <Glacier/ZAction.h>
#include <Glacier/ZItem.h>
#include <Glacier/ZInventory.h>
#include <Glacier/ZHM5CrippleBox.h>
#include <IO/ZBinaryReader.h>
#include <IO/ZBinaryDeserializer.h>
#include <Crypto.h>

#include <Functions.h>
#include <Globals.h>

#include <ImGuizmo.h>
#include <lz4.h>

#include <winhttp.h>
#include <numbers>
#include <filesystem>
#include <imgui_internal.h>

#pragma comment(lib, "urlmon.lib")

DebugMod::~DebugMod()
{
	const ZMemberDelegate<DebugMod, void(const SGameUpdateEvent&)> s_Delegate(this, &DebugMod::OnFrameUpdate);
	Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void DebugMod::Init()
{
	Hooks::ZHttpResultDynamicObject_OnBufferReady->AddDetour(this, &DebugMod::ZHttpBufferReady);
	Hooks::Http_WinHttpCallback->AddDetour(this, &DebugMod::WinHttpCallback);
	Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &DebugMod::OnClearScene);
}

void DebugMod::OnEngineInitialized()
{
	const ZMemberDelegate<DebugMod, void(const SGameUpdateEvent&)> s_Delegate(this, &DebugMod::OnFrameUpdate);
	Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

	std::thread thread(LoadHashMap);

	thread.detach();
}

void DebugMod::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
	/*if (Functions::ZInputAction_Analog->Call(&m_DeleteAction, -1) > 0.f)
	{
		if (const auto s_CurrentCamera = Functions::GetCurrentCamera->Call())
		{
			const auto s_CameraTrans = s_CurrentCamera->GetWorldMatrix();
			const auto s_From = s_CameraTrans.Trans;
			const auto s_To = s_From - (s_CameraTrans.ZAxis * 200.f);
			DoRaycast(s_From, s_To);
		}
	}*/
}

void DebugMod::OnDrawMenu()
{
	if (ImGui::Button("DEBUG MENU"))
	{
		m_DebugMenuActive = !m_DebugMenuActive;
	}

	if (ImGui::Button("POSITIONS MENU"))
	{
		m_PositionsMenuActive = !m_PositionsMenuActive;
	}

	if (ImGui::Button("ENTITY MENU"))
	{
		m_EntityMenuActive = !m_EntityMenuActive;
	}

	if (ImGui::Button("PLAYER MENU"))
	{
		m_PlayerMenuActive = !m_PlayerMenuActive;
	}

	if (ImGui::Button("ITEMS MENU"))
	{
		m_ItemsMenuActive = !m_ItemsMenuActive;
	}

	if (ImGui::Button("ASSETS MENU"))
	{
		m_AssetsMenuActive = !m_AssetsMenuActive;
	}

	if (ImGui::Button("NPCs MENU"))
	{
		m_NPCsMenuActive = !m_NPCsMenuActive;
	}

	if (ImGui::Button("SCENE MENU"))
	{
		m_SceneMenuActive = !m_SceneMenuActive;
	}

	if (ImGui::Button("SPAWN CANON"))
	{
		auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

		if (!s_Scene)
		{
			Logger::Debug("Scene not loaded.");
		}
		else
		{
			//const auto s_ID = ResId<"[assembly:/_pro/environment/templates/props/containers/military_containers_a.template?/military_box_metal_e_00.entitytemplate].pc_entitytype">;
			//const auto s_ID = ResId<"[assembly:/deeznuts.entitytemplate].pc_entitytype">;
			//const auto s_ID = ResId<"[assembly:/templates/gameplay/ai2/actors.template?/npcactor.entitytemplate].pc_entitytype">;
			//const auto s_ID = ResId<"[assembly:/_pro/characters/templates/hero/agent47/agent47.template?/agent47_default.entitytemplate].pc_entitytype">;
			const auto s_ID = ResId<"[assembly:/_pro/design/setpieces/unique/setpiece_italy_unique.template?/setpiece_italy_unique_cannon_fortress_a.entitytemplate].pc_entitytype">;

			Logger::Debug("Getting resource wew: {}", s_ID);

			TResourcePtr<ZTemplateEntityFactory> s_Resource;
			Globals::ResourceManager->GetResourcePtr(s_Resource, s_ID, 0);

			Logger::Debug("Resource: {} {}", s_Resource.m_nResourceIndex, fmt::ptr(s_Resource.GetResource()));

			/*const auto s_BrickID = ResId<"[assembly:/_pro/scenes/missions/paris/_scene_lumumba.brick].pc_entitytype">;
			Logger::Debug("Getting brick resource wew: {}", s_BrickID);

			TResourcePtr<ZTemplateEntityFactory> s_BrickResource;
			Globals::ResourceManager->GetResourcePtr(s_BrickResource, s_BrickID, 0);

			Logger::Debug("Brick resource: {} {}", s_BrickResource.m_nResourceIndex, fmt::ptr(s_BrickResource.GetResource()));*/

			if (!s_Resource)
			{
				Logger::Debug("Resource is not loaded.");
			}
			else
			{
				// Spawn some shit now.
				ZEntityRef s_NewEntity;
				Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, s_NewEntity, "", s_Resource, s_Scene.m_ref, nullptr, -1);

				if (!s_NewEntity)
				{
					Logger::Debug("Failed to spawn entity.");
				}
				else
				{
					s_NewEntity.SetProperty("m_eRoomBehaviour", ZSpatialEntity::ERoomBehaviour::ROOM_DYNAMIC);

					m_EntityMutex.lock();
					m_SelectedEntity = s_NewEntity;
					m_EntityMutex.unlock();

					/*
					TEntityRef<ZHitman5> s_LocalHitman;
					Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);
					auto s_Actor = s_NewEntity.QueryInterface<ZActor>();

					Logger::Debug("Spawned entity {}!", fmt::ptr(s_Actor));

					// Set outfit and other properties.
					s_Actor->m_sActorName = "Ur Mum";
					s_Actor->m_nOutfitVariation = 0;
					s_Actor->m_bStartEnabled = true;
					s_Actor->m_OutfitRepositoryID = ZRepositoryID("8f928c7a-99dd-43db-a027-1310cf49f3ad");
					s_Actor->m_eRequiredVoiceVariation = EActorVoiceVariation::eAVV_Undefined;

					auto s_ActorSpatial = s_NewEntity.QueryInterface<ZSpatialEntity>();
					const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
					
					s_ActorSpatial->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());

					s_Actor->Activate(0);#1#

					/*auto s_NewHitman = s_NewEntity.QueryInterface<ZHitman5>();

					Logger::Debug("Spawned entity {}!", fmt::ptr(s_NewHitman));

					// Set outfit and other properties.
					s_NewHitman->m_InitialOutfitId = "bb1115ba-d250-4f8f-b486-f5aba8499ebb";
					s_NewHitman->m_CharacterId = "21174318-919a-4683-bd3b-09068f7b6fac";
					s_NewHitman->m_bIsInvincible = false;

					auto s_ActorSpatial = s_NewEntity.QueryInterface<ZSpatialEntity>();
					const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
					
					//s_ActorSpatial->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());

					s_NewHitman->Activate(0);#1#

					auto s_NewSpatialEntity = s_NewEntity.QueryInterface<ZSpatialEntity>();

					Logger::Debug("Spawned entity {}!", fmt::ptr(s_NewSpatialEntity));

					const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

					s_NewSpatialEntity->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());

					m_EntityMutex.lock();

					m_SelectedEntity = s_NewEntity;

					m_EntityMutex.unlock();*/
				}
			}
		}
	}
}

void DebugMod::OnDrawUI(bool p_HasFocus)
{
	ImGuizmo::BeginFrame();

	DrawOptions(p_HasFocus);
	DrawPositionBox(p_HasFocus);
	DrawEntityBox(p_HasFocus);

	auto s_ImgGuiIO = ImGui::GetIO();

	if (p_HasFocus)
	{
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !s_ImgGuiIO.WantCaptureMouse)
		{
			const auto s_MousePos = ImGui::GetMousePos();
			
			OnMouseDown(SVector2(s_MousePos.x, s_MousePos.y), !m_HoldingMouse);

			m_HoldingMouse = true;
		}
		else
		{
			/*// If we stopped clicking, update collisions.
			if (m_HoldingMouse && *Globals::CollisionManager)
			{
				m_EntityMutex.lock_shared();

				if (m_SelectedEntity)
				{
					if (const auto s_SpatialEntity = m_SelectedEntity.QueryInterface<ZSpatialEntity>())
					{
						if (const auto s_PhysicsAspect = m_SelectedEntity.QueryInterface<ZStaticPhysicsAspect>())
						{
							Logger::Debug("Found physics aspect. Updating its transform.");
							s_PhysicsAspect->m_pPhysicsObject->SetTransform(s_SpatialEntity->GetWorldMatrix());
						}
					}
				}

				m_EntityMutex.unlock_shared();
			}*/

			m_HoldingMouse = false;
		}

		if (ImGui::IsKeyPressed(s_ImgGuiIO.KeyMap[ImGuiKey_Tab]))
		{
			if (m_GizmoMode == ImGuizmo::TRANSLATE)
				m_GizmoMode = ImGuizmo::ROTATE;
			else if (m_GizmoMode == ImGuizmo::ROTATE)
				m_GizmoMode = ImGuizmo::SCALE;
			else if (m_GizmoMode == ImGuizmo::SCALE)
				m_GizmoMode = ImGuizmo::TRANSLATE;
		}

		if (ImGui::IsKeyPressed(s_ImgGuiIO.KeyMap[ImGuiKey_Space]))
		{
			m_GizmoSpace = m_GizmoSpace == ImGuizmo::WORLD ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
		}
	}

	ImGuizmo::Enable(p_HasFocus);

	m_EntityMutex.lock_shared();

	if (m_SelectedEntity)
	{
		if (const auto s_CurrentCamera = Functions::GetCurrentCamera->Call())
		{
			if (const auto s_SpatialEntity = m_SelectedEntity.QueryInterface<ZSpatialEntity>())
			{
				auto s_ModelMatrix = s_SpatialEntity->GetWorldMatrix();
				auto s_ViewMatrix = s_CurrentCamera->GetViewMatrix();
				const SMatrix s_ProjectionMatrix = *s_CurrentCamera->GetProjectionMatrix();

				ImGuizmo::SetRect(0, 0, s_ImgGuiIO.DisplaySize.x, s_ImgGuiIO.DisplaySize.y);

				if (ImGuizmo::Manipulate(&s_ViewMatrix.XAxis.x, &s_ProjectionMatrix.XAxis.x, m_GizmoMode, m_GizmoSpace, &s_ModelMatrix.XAxis.x, NULL, m_useSnap ? &m_SnapValue[0] : NULL))
				{
					s_SpatialEntity->SetWorldMatrix(s_ModelMatrix);

					if (const auto s_PhysicsAspect = m_SelectedEntity.QueryInterface<ZStaticPhysicsAspect>())
						s_PhysicsAspect->m_pPhysicsObject->SetTransform(s_SpatialEntity->GetWorldMatrix());
				}
			}
		}
	}

	m_EntityMutex.unlock_shared();
}

void DebugMod::OnMouseDown(SVector2 p_Pos, bool p_FirstClick)
{
	SVector3 s_World;
	SVector3 s_Direction;
	SDK()->ScreenToWorld(p_Pos, s_World, s_Direction);

	float4 s_DirectionVec(s_Direction.x, s_Direction.y, s_Direction.z, 1.f);

	float4 s_From = float4(s_World.x, s_World.y, s_World.z, 1.f);
	float4 s_To = s_From + (s_DirectionVec * 200.f);

	if (!*Globals::CollisionManager)
	{
		Logger::Error("Collision manager not found.");
		return;
	}

	ZRayQueryInput s_RayInput {
		.m_vFrom = s_From,
		.m_vTo = s_To,
	};

	ZRayQueryOutput s_RayOutput {};

	Logger::Debug("RayCasting from {} to {}.", s_From, s_To);

	if (!(*Globals::CollisionManager)->RayCastClosestHit(s_RayInput, &s_RayOutput))
	{
		Logger::Error("Raycast failed.");
		return;
	}

	Logger::Debug("Raycast result: {} {}", fmt::ptr(&s_RayOutput), s_RayOutput.m_vPosition);

	m_From = s_From;
	m_To = s_To;
	m_Hit = s_RayOutput.m_vPosition;
	m_Normal = s_RayOutput.m_vNormal;

	if (p_FirstClick)
	{
		m_EntityMutex.lock();

		if (s_RayOutput.m_BlockingEntity)
		{
			const auto& s_Interfaces = *(*s_RayOutput.m_BlockingEntity.m_pEntity)->m_pInterfaces;
			Logger::Trace("Hit entity of type '{}' with id '{:x}'.", s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName, (*s_RayOutput.m_BlockingEntity.m_pEntity)->m_nEntityId);
		}

		m_SelectedEntity = s_RayOutput.m_BlockingEntity;
		selectedEntityName.clear();

		m_EntityMutex.unlock();
	}
	else
	{
		/*m_EntityMutex.lock_shared();

		if (m_SelectedEntity)
		{
			if (const auto s_SpatialEntity = m_SelectedEntity.QueryInterface<ZSpatialEntity>())
			{
				auto s_EntityWorldMatrix = s_SpatialEntity->GetWorldMatrix();				
				s_EntityWorldMatrix.Trans = m_Hit;
				s_SpatialEntity->SetWorldMatrix(s_EntityWorldMatrix);
			}
		}

		m_EntityMutex.unlock_shared();*/
	}
}

void DebugMod::DrawOptions(bool p_HasFocus)
{
	if (!p_HasFocus || !m_DebugMenuActive)
	{
		return;
	}

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	auto s_Showing = ImGui::Begin("DEBUG MENU", &m_DebugMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing)
	{
		ImGui::Checkbox("Render NPC position boxes", &m_RenderNpcBoxes);
		ImGui::Checkbox("Render NPC names", &m_RenderNpcNames);
		ImGui::Checkbox("Render NPC repository IDs", &m_RenderNpcRepoIds);
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}

void DebugMod::EquipOutfit(TEntityRef<ZGlobalOutfitKit>& globalOutfitKit, unsigned int currentCharacterSetIndex, const char* currentcharSetCharacterType, unsigned int currentOutfitVariationIndex, ZHitman5* localHitman)
{
	std::vector<ZRuntimeResourceID> heroOutfitVariations;

	if (strcmp(currentcharSetCharacterType, "HeroA") != 0)
	{
		ZOutfitVariationCollection* outfitVariationCollection = globalOutfitKit.m_pInterfaceRef->m_aCharSets[currentCharacterSetIndex].m_pInterfaceRef;
		TEntityRef<ZCharsetCharacterType>* charsetCharacterType = nullptr;
		TEntityRef<ZCharsetCharacterType>* charsetCharacterType2 = &outfitVariationCollection->m_aCharacters[2];

		if (strcmp(currentcharSetCharacterType, "Actor") == 0)
		{
			charsetCharacterType = &outfitVariationCollection->m_aCharacters[0];
		}
		else if (strcmp(currentcharSetCharacterType, "Nude") == 0)
		{
			charsetCharacterType = &outfitVariationCollection->m_aCharacters[1];
		}

		for (size_t i = 0; i < charsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
		{
			heroOutfitVariations.push_back(charsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit);
		}

		if (charsetCharacterType)
		{
			for (size_t i = 0; i < charsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
			{
				charsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit = charsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit;
			}
		}
	}

	Functions::ZHitman5_SetOutfit->Call(localHitman, globalOutfitKit, currentCharacterSetIndex, currentOutfitVariationIndex, false, false);

	if (strcmp(currentcharSetCharacterType, "HeroA") != 0)
	{
		ZOutfitVariationCollection* outfitVariationCollection = globalOutfitKit.m_pInterfaceRef->m_aCharSets[currentCharacterSetIndex].m_pInterfaceRef;
		TEntityRef<ZCharsetCharacterType>* charsetCharacterType = &outfitVariationCollection->m_aCharacters[2];

		for (size_t i = 0; i < heroOutfitVariations.size(); ++i)
		{
			charsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit = heroOutfitVariations[i];
		}
	}
}

void DebugMod::EquipOutfit(TEntityRef<ZGlobalOutfitKit>& globalOutfitKit, unsigned int currentCharacterSetIndex, const char* currentcharSetCharacterType, unsigned int currentOutfitVariationIndex, ZActor* actor)
{
	std::vector<ZRuntimeResourceID> actorOutfitVariations;

	if (strcmp(currentcharSetCharacterType, "Actor") != 0)
	{
		ZOutfitVariationCollection* outfitVariationCollection = globalOutfitKit.m_pInterfaceRef->m_aCharSets[currentCharacterSetIndex].m_pInterfaceRef;

		TEntityRef<ZCharsetCharacterType>* charsetCharacterType = nullptr;
		TEntityRef<ZCharsetCharacterType>* charsetCharacterType2 = &outfitVariationCollection->m_aCharacters[0];

		if (strcmp(currentcharSetCharacterType, "Nude") == 0)
		{
			charsetCharacterType = &outfitVariationCollection->m_aCharacters[1];
		}
		else if (strcmp(currentcharSetCharacterType, "HeroA") == 0)
		{
			charsetCharacterType = &outfitVariationCollection->m_aCharacters[2];
		}

		for (size_t i = 0; i < charsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
		{
			actorOutfitVariations.push_back(charsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit);
		}

		if (charsetCharacterType)
		{
			for (size_t i = 0; i < charsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
			{
				charsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit = charsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit;
			}
		}
	}

	Functions::ZActor_SetOutfit->Call(actor, globalOutfitKit, currentCharacterSetIndex, currentOutfitVariationIndex, false);

	if (strcmp(currentcharSetCharacterType, "Actor") != 0)
	{
		ZOutfitVariationCollection* outfitVariationCollection = globalOutfitKit.m_pInterfaceRef->m_aCharSets[currentCharacterSetIndex].m_pInterfaceRef;
		TEntityRef<ZCharsetCharacterType>* charsetCharacterType = &outfitVariationCollection->m_aCharacters[0];

		for (size_t i = 0; i < actorOutfitVariations.size(); ++i)
		{
			charsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit = actorOutfitVariations[i];
		}
	}
}

void DebugMod::SpawnRepositoryProp(const ZRepositoryID& repositoryID, bool addToWorld)
{
	TEntityRef<ZHitman5> localHitman;

	Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &localHitman);

	if (addToWorld)
	{
		auto scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

		if (!scene)
		{
			Logger::Debug("Scene not loaded.");
		}
		else
		{
			const auto s_ID = ResId<"[modules:/zitemspawner.class].pc_entitytype">;
			const auto s_ID2 = ResId<"[modules:/zitemrepositorykeyentity.class].pc_entitytype">;

			TResourcePtr<ZTemplateEntityFactory> s_Resource, s_Resource2;

			Globals::ResourceManager->GetResourcePtr(s_Resource, s_ID, 0);
			Globals::ResourceManager->GetResourcePtr(s_Resource2, s_ID2, 0);

			Logger::Debug("Resource: {} {}", s_Resource.m_nResourceIndex, fmt::ptr(s_Resource.GetResource()));

			if (!s_Resource)
			{
				Logger::Debug("Resource is not loaded.");
			}
			else
			{
				ZEntityRef newEntity, newEntity2;

				Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, newEntity, "", s_Resource, scene.m_ref, nullptr, -1);
				Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, newEntity2, "", s_Resource2, scene.m_ref, nullptr, -1);

				if (!newEntity)
				{
					Logger::Debug("Failed to spawn entity.");
				}

				if (!newEntity2)
				{
					Logger::Debug("Failed to spawn entity2.");
				}

				const auto hitmanSpatialEntity = localHitman.m_ref.QueryInterface<ZSpatialEntity>();
				ZItemSpawner* itemSpawner = newEntity.QueryInterface<ZItemSpawner>();

				itemSpawner->m_ePhysicsMode = ZItemSpawner::EPhysicsMode::EPM_KINEMATIC;
				itemSpawner->m_rMainItemKey.m_ref = newEntity2;
				itemSpawner->m_rMainItemKey.m_pInterfaceRef = newEntity2.QueryInterface<ZItemRepositoryKeyEntity>();
				itemSpawner->m_rMainItemKey.m_pInterfaceRef->m_RepositoryId = repositoryID;
				itemSpawner->m_bUsePlacementAttach = false;
				itemSpawner->SetWorldMatrix(hitmanSpatialEntity->GetWorldMatrix());

				Functions::ZItemSpawner_RequestContentLoad->Call(itemSpawner);
			}
		}
	}
	else
	{
		TArray<TEntityRef<ZCharacterSubcontroller>>* controllers = &localHitman.m_pInterfaceRef->m_pCharacter.m_pInterfaceRef->m_rSubcontrollerContainer.m_pInterfaceRef->m_aReferencedControllers;
		ZCharacterSubcontrollerInventory* inventory = static_cast<ZCharacterSubcontrollerInventory*>(controllers->operator[](6).m_pInterfaceRef);
		TArray<ZRepositoryID> aModifierIds;

		unsigned long long result = Functions::ZCharacterSubcontrollerInventory_AddDynamicItemToInventory->Call(inventory, repositoryID, "", &aModifierIds, 2);
	}
}

void DebugMod::SpawnNonRepositoryProp(const char* propAssemblyPath)
{
	auto scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

	if (!scene)
	{
		Logger::Debug("Scene not loaded.");
	}
	else
	{
		Hash::MD5Hash hash = Hash::MD5(std::string_view(propAssemblyPath, strlen(propAssemblyPath)));

		uint32_t idHigh = ((hash.A >> 24) & 0x000000FF)
			| ((hash.A >> 8) & 0x0000FF00)
			| ((hash.A << 8) & 0x00FF0000);

		uint32_t idLow = ((hash.B >> 24) & 0x000000FF)
			| ((hash.B >> 8) & 0x0000FF00)
			| ((hash.B << 8) & 0x00FF0000)
			| ((hash.B << 24) & 0xFF000000);

		ZRuntimeResourceID runtimeResourceID = ZRuntimeResourceID(idHigh, idLow);
		TResourcePtr<ZTemplateEntityFactory> resource;

		Globals::ResourceManager->GetResourcePtr(resource, runtimeResourceID, 0);

		if (!resource)
		{
			Logger::Debug("Resource is not loaded.");
		}
		else
		{
			ZEntityRef newEntity;

			Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, newEntity, "", resource, scene.m_ref, nullptr, -1);

			if (!newEntity)
			{
				Logger::Debug("Failed to spawn entity.");
			}
			else
			{
				newEntity.SetProperty("m_eRoomBehaviour", ZSpatialEntity::ERoomBehaviour::ROOM_DYNAMIC);
			}

			TEntityRef<ZHitman5> localHitman;

			Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &localHitman);

			const auto hitmanSpatialEntity = localHitman.m_ref.QueryInterface<ZSpatialEntity>();
			const auto propSpatialEntity = newEntity.QueryInterface<ZSpatialEntity>();

			propSpatialEntity->SetWorldMatrix(hitmanSpatialEntity->GetWorldMatrix());

			/*ZSetpieceEntity* setPieceEntity = newEntity.QueryInterface<ZSetpieceEntity>();

			if (setPieceEntity)
			{
				setPieceEntity->Activate(0);
			}
			else
			{
				newEntity.GetBaseEntity()->Activate(0);
			}*/
		}
	}
}

void DebugMod::SpawnNPC(const char* npcName, const ZRepositoryID& repositoryID, TEntityRef<ZGlobalOutfitKit>* globalOutfitKit, const char* currentCharacterSetIndex, const char* currentcharSetCharacterType, const char* currentOutfitVariationIndex)
{
	auto scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

	if (!scene)
	{
		Logger::Debug("Scene not loaded.");
	}
	else
	{
		const auto runtimeResourceID = ResId<"[assembly:/templates/gameplay/ai2/actors.template?/npcactor.entitytemplate].pc_entitytype">;
		TResourcePtr<ZTemplateEntityFactory> resource;

		Globals::ResourceManager->GetResourcePtr(resource, runtimeResourceID, 0);

		if (!resource)
		{
			Logger::Debug("Resource is not loaded.");
		}
		else
		{
			ZEntityRef newEntity;

			Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, newEntity, "", resource, scene.m_ref, nullptr, -1);

			if (newEntity)
			{
				TEntityRef<ZHitman5> localHitman;

				Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &localHitman);

				ZActor* actor = newEntity.QueryInterface<ZActor>();

				actor->m_sActorName = npcName;
				actor->m_bStartEnabled = true;
				actor->m_nOutfitCharset = std::stoi(currentCharacterSetIndex);
				actor->m_nOutfitVariation = std::stoi(currentOutfitVariationIndex);
				actor->m_OutfitRepositoryID = repositoryID;
				actor->m_eRequiredVoiceVariation = EActorVoiceVariation::eAVV_Undefined;

				actor->Activate(0);

				ZSpatialEntity* actorSpatialEntity = newEntity.QueryInterface<ZSpatialEntity>();
				ZSpatialEntity* hitmanSpatialEntity = localHitman.m_ref.QueryInterface<ZSpatialEntity>();

				actorSpatialEntity->SetWorldMatrix(hitmanSpatialEntity->GetWorldMatrix());

				if (globalOutfitKit)
				{
					EquipOutfit(*globalOutfitKit, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), actor);
				}
			}
		}
	}
}

void DebugMod::LoadRepositoryProps()
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

		for (auto it = repositoryData->begin(); it != repositoryData->end(); ++it)
		{
			ZDynamicObject* dynamicObject = &it->second;
			TArray<SDynamicObjectKeyValuePair>* entries = dynamicObject->As<TArray<SDynamicObjectKeyValuePair>>();
			std::string id;

			for (size_t i = 0; i < entries->size(); ++i)
			{
				std::string key = entries->operator[](i).sKey.c_str();

				if (key == "ID_")
				{
					id = ConvertDynamicObjectValueTString(&entries->operator[](i).value);
				}

				if (key == "Title")
				{
					std::string title = ConvertDynamicObjectValueTString(&entries->operator[](i).value);

					repositoryProps.insert(std::make_pair(title, ZRepositoryID(id.c_str())));

					break;
				}
			}
		}
	}
}

void DebugMod::LoadHashMap()
{
	if (!std::filesystem::exists("hash_list.txt"))
	{
		DownloadHashMap();

		LZMA::Extract();
	}

	std::ifstream inputFile = std::ifstream("hash_list.txt", std::ios::binary | std::ios::ate);
	uint64_t fileSize = (uint64_t)inputFile.tellg();

	inputFile.seekg(0, inputFile.beg);

	std::vector<char> data(fileSize, 0);

	inputFile.read(data.data(), fileSize);

	uint64_t position = 0;
	uint64_t lastPosition = 0;
	uint64_t lineCount = 0;

	mutex.lock();

	while (position < data.size())
	{
		if (data[position] == 0x0A)
		{
			lineCount++;

			data[position] = 0x0;

			if (lineCount > 3)
			{
				std::string_view stringView = std::string_view(reinterpret_cast<char*>(&data[lastPosition]));
				size_t index = stringView.find_first_of(',');

				std::string hash = std::string(stringView.substr(0, (index - 5)));
				std::string resourceID = std::string(stringView.substr(index + 1, stringView.length() - (index + 1)));

				runtimeResourceIDsToResourceIDs[std::stoull(hash, nullptr, 16)] = resourceID;

			}

			lastPosition = position + 1;
		}

		position++;
	}

	mutex.unlock();
}

void DebugMod::DownloadHashMap()
{
	std::string folderPath = std::format("{}\\latest-hashes.7z", std::filesystem::current_path().string());

	HRESULT result = URLDownloadToFileA(nullptr, "https://hitmandb.glaciermodding.org/latest-hashes.7z", folderPath.c_str(), 0, nullptr);
}

std::string DebugMod::GetEntityName(unsigned long long tempBrickHash, unsigned long long entityID, unsigned long long& resourceHash)
{
	std::string entityName = "";
	TResourcePtr<ZTemplateEntityFactory> tempBrickResource;

	Globals::ResourceManager->GetResourcePtr(tempBrickResource, ZRuntimeResourceID(tempBrickHash), 0);

	ZResourceContainer::SResourceInfo tempBrickResourceInfo = (*Globals::ResourceContainer)->m_resources[tempBrickResource.m_nResourceIndex];
	ZTemplateEntityFactory* resource = tempBrickResource.GetResource();

	if (!resource)
	{
		return entityName;
	}

	ZResourceContainer::SResourceInfo tbluBrickResourceInfo = (*Globals::ResourceContainer)->m_resources[resource->m_blueprintResource.m_nResourceIndex];

	unsigned int entityIndex = -1;
	static unsigned long long dataSectionOffset = 0x10;
	std::vector<char> tempBrickResourceData;
	std::vector<char> tbluBrickResourceData;

	TArray<ZString>* mountedPackages = &(*Globals::ResourceContainer)->m_MountedPackages;
	std::string rpkgFilePath = (*mountedPackages)[mountedPackages->size() - 1].c_str();

	LoadResourceData(tempBrickHash, tempBrickResourceData, rpkgFilePath);
	LoadResourceData(tbluBrickResourceInfo.rid.GetID(), tbluBrickResourceData, rpkgFilePath);

	ZBinaryReader binaryReader = ZBinaryReader(&tbluBrickResourceData);

	binaryReader.Seek(dataSectionOffset + 0x8);

	unsigned long long subEntitiesStartOffset = binaryReader.Read<unsigned long long>();
	unsigned long long subEntitiesEndOffset = binaryReader.Read<unsigned long long>();
	unsigned int subEntityCount = static_cast<unsigned int>((subEntitiesEndOffset - subEntitiesStartOffset) / 0xA8); //0xA8 is size of STemplateBlueprintSubEntity

	for (unsigned int i = 0; i < subEntityCount; ++i)
	{
		binaryReader.Seek(dataSectionOffset + subEntitiesStartOffset + i * 0xA8 + 0x28);

		unsigned long long entityID2 = binaryReader.Read<unsigned long long>();

		if (entityID == entityID2)
		{
			binaryReader.Skip(0x10);

			unsigned long long entityNameOffset = binaryReader.Read<unsigned long long>();

			binaryReader.Seek(entityNameOffset + dataSectionOffset - 4);

			int stringLength = binaryReader.Read<unsigned int>();
			entityName = binaryReader.ReadString(stringLength - 1);
			entityIndex = i;

			break;
		}
	}

	if (entityIndex != -1)
	{
		ZBinaryReader binaryReader2 = ZBinaryReader(&tempBrickResourceData);

		binaryReader2.Seek(dataSectionOffset + 0x10);

		unsigned long long subEntitiesStartOffset2 = binaryReader2.Read<unsigned long long>();
		unsigned long long subEntityOffset = dataSectionOffset + subEntitiesStartOffset2 + entityIndex * 0x70; //0x70 is size of STemplateFactorySubEntity

		binaryReader2.Seek(subEntityOffset + 0x20);

		int entityTypeResourceIndex = binaryReader2.Read<unsigned int>();

		TArray<ZResourceIndex> referenceIndices;
		TArray<unsigned char> referenceFlags;

		Functions::ZResourceContainer_GetResourceReferences->Call(*Globals::ResourceContainer, ZResourceIndex(tempBrickResource.m_nResourceIndex), referenceIndices, referenceFlags);

		ZResourceContainer::SResourceInfo referenceInfo = (*Globals::ResourceContainer)->m_resources[referenceIndices[entityTypeResourceIndex].val];

		resourceHash = referenceInfo.rid.GetID();
	}

	return entityName;
}


std::string DebugMod::FindNPCEntityNameInBrickBackReferences(unsigned long long tempBrickHash, unsigned long long entityID, unsigned long long& resourceHash)
{
	std::string entityName;
	ZResourceContainer* resourceContainer = *Globals::ResourceContainer;

	for (unsigned int i = 0; i < resourceContainer->m_resourcesSize; ++i)
	{
		ZResourceContainer::SResourceInfo* resourceInfo = &resourceContainer->m_resources[i];
		unsigned long long resourceHash2 = resourceInfo->rid.GetID();

		if (resourceInfo->resourceType == 'TEMP' &&
			resourceInfo->numReferences > 0 &&
			runtimeResourceIDsToResourceIDs.contains(resourceHash2) &&
			runtimeResourceIDsToResourceIDs[resourceHash2].ends_with(".brick].pc_entitytype") &&
			runtimeResourceIDsToResourceIDs[resourceHash2].contains("/npc_"))
		{
			ZResourceIndex resourceIndex = resourceContainer->m_indices.find(resourceInfo->rid)->second;
			TArray<ZResourceIndex> referenceIndices;
			TArray<unsigned char> referenceFlags;

			Functions::ZResourceContainer_GetResourceReferences->Call(resourceContainer, ZResourceIndex(resourceIndex), referenceIndices, referenceFlags);

			for (size_t j = 0; j < referenceIndices.size(); ++j)
			{
				size_t referenceIndex = referenceIndices[j].val;

				if (referenceIndex == -1)
				{
					continue;
				}

				ZResourceContainer::SResourceInfo referenceInfo = resourceContainer->m_resources[referenceIndex];
				unsigned long long referenceHash = referenceInfo.rid.GetID();

				if (referenceHash == tempBrickHash)
				{
					entityName = GetEntityName(resourceHash2, entityID, resourceHash);

					if (!entityName.empty())
					{
						break;
					}
				}
			}
		}

		if (!entityName.empty())
		{
			break;
		}
	}

	return entityName;
}

std::string DebugMod::ConvertDynamicObjectValueTString(ZDynamicObject* dynamicObject)
{
	std::string result;
	IType* type = dynamicObject->m_pTypeID->typeInfo();

	if (strcmp(type->m_pTypeName, "ZString") == 0)
	{
		ZString* value = dynamicObject->As<ZString>();

		result = value->c_str();
	}
	else if (strcmp(type->m_pTypeName, "bool") == 0)
	{
		bool value = *dynamicObject->As<bool>();

		if (value)
		{
			result = "true";
		}
		else
		{
			result = "false";
		}
	}
	else if (strcmp(type->m_pTypeName, "float64") == 0)
	{
		double value = *dynamicObject->As<double>();

		result = std::to_string(value).c_str();
	}
	else
	{
		Logger::Debug("{}", type->m_pTypeName);
	}

	return result;
}

void DebugMod::LoadResourceData(unsigned long long hash, std::vector<char>& resourceData)
{
	static std::string rpkgFilePath = GetPatchRPKGFilePath();

	LoadResourceData(hash, resourceData, rpkgFilePath);
}

void DebugMod::LoadResourceData(unsigned long long hash, std::vector<char>& resourceData, const std::string& rpkgFilePath)
{
	ZBinaryReader binaryReader = ZBinaryReader(rpkgFilePath);

	binaryReader.Seek(0xD);

	unsigned int resourceCount = binaryReader.Read<unsigned int>();
	unsigned int resourceHeadersChunkSize = binaryReader.Read<unsigned int>();
	unsigned int resourcesChunkSize = binaryReader.Read<unsigned int>();
	unsigned int patchDeletionEntryCount = binaryReader.Read<unsigned int>();
	bool isPatchFile = false;

	if (patchDeletionEntryCount * 8 + 0x2D >= binaryReader.GetSize())
	{
		isPatchFile = false;
	}
	else
	{
		binaryReader.Seek(patchDeletionEntryCount * 8 + 0x24);

		unsigned char testZeroValue = binaryReader.Read<unsigned char>();
		unsigned long long testHeaderOffset = binaryReader.Read<unsigned long long>();

		if (testHeaderOffset == (resourceHeadersChunkSize + resourcesChunkSize + patchDeletionEntryCount * 8 + 0x1D) && testZeroValue == 0)
		{
			isPatchFile = true;
		}
	}

	if (isPatchFile)
	{
		binaryReader.Seek(0x1D);
		binaryReader.Skip(8 * patchDeletionEntryCount);
	}
	else
	{
		binaryReader.Seek(0x19);
	}

	unsigned int i = 0;

	while (i < resourceCount)
	{
		unsigned long long hash2 = binaryReader.Read<unsigned long long>();

		if (hash2 == hash)
		{
			unsigned long long resourceDataOffset = binaryReader.Read<unsigned long long>();
			unsigned int dataSize = binaryReader.Read<unsigned int>();

			bool isResourceEncrypted = (dataSize & 0x80000000) == 0x80000000;
			bool isResourceCompressed = (dataSize & 0x3FFFFFFF) != 0;

			binaryReader.Seek(resourceDataOffset);

			TResourcePtr<ZTemplateEntityFactory> resource;

			Globals::ResourceManager->GetResourcePtr(resource, ZRuntimeResourceID(hash), 0);

			ZResourceContainer::SResourceInfo resourceInfo = (*Globals::ResourceContainer)->m_resources[resource.m_nResourceIndex];

			if (isResourceEncrypted)
			{
				dataSize &= 0x3FFFFFFF;
			}
			else
			{
				dataSize = resourceInfo.finalDataSize;
			}

			std::vector<char> inputResourceData;

			inputResourceData.reserve(dataSize);
			binaryReader.ReadBytes(inputResourceData.data(), dataSize);

			if (isResourceEncrypted)
			{
				Crypto::XORData(inputResourceData.data(), dataSize);
			}

			std::vector<char> outputResourceData = std::vector<char>(resourceInfo.finalDataSize, 0);

			if (isResourceCompressed)
			{
				LZ4_decompress_safe(inputResourceData.data(), outputResourceData.data(), dataSize, resourceInfo.finalDataSize);

				resourceData = outputResourceData;
			}
			else
			{
				resourceData = inputResourceData;
			}

			break;
		}

		size_t currentPositon = binaryReader.GetPosition();

		binaryReader.Seek(currentPositon + 12);

		i++;
	}
}

std::string DebugMod::GetPatchRPKGFilePath()
{
	std::string rpkgFilePath;

	for (const auto& entry : std::filesystem::directory_iterator("../Runtime"))
	{
		if (entry.path().string().starts_with("../Runtime\\chunk0"))
		{
			rpkgFilePath = entry.path().string();
		}
		else
		{
			break;
		}
	}

	return rpkgFilePath;
}

unsigned long long DebugMod::GetDDSTextureHash(const std::string image)
{
	static std::unordered_map<std::string, unsigned long long> oresEntries;

	if (oresEntries.empty())
	{
		const auto s_ID = ResId<"[assembly:/_pro/online/default/offlineconfig/config.blobs].pc_blobs">;
		TResourcePtr<ZTemplateEntityFactory> resource;

		Globals::ResourceManager->GetResourcePtr(resource, s_ID, 0);

		ZResourceContainer::SResourceInfo resourceInfo = (*Globals::ResourceContainer)->m_resources[resource.m_nResourceIndex];

		unsigned long long oresHash = resourceInfo.rid.GetID();
		std::vector<char> oresResourceData;

		LoadResourceData(oresHash, oresResourceData);

		ZBinaryReader binaryReader = ZBinaryReader(&oresResourceData);

		binaryReader.Seek(0x10);
		binaryReader.Seek(binaryReader.Read<unsigned int>() + 0xC);

		unsigned resourceCount = binaryReader.Read<unsigned int>();

		for (unsigned int i = 0; i < resourceCount; ++i)
		{
			unsigned int stringLength = binaryReader.Read<unsigned int>();

			binaryReader.Seek(0x4, ZBinaryReader::ESeekOrigin::current);

			unsigned long long stringOffset = binaryReader.Read<unsigned long long>();
			ZRuntimeResourceID runtimeResourceID = binaryReader.Read<ZRuntimeResourceID>();

			size_t currentPosition = binaryReader.GetPosition();

			binaryReader.Seek(stringOffset + 0x10 - 0x4);

			unsigned int stringLength2 = binaryReader.Read<unsigned int>();
			std::string image2 = binaryReader.ReadString(stringLength2 - 1);

			binaryReader.Seek(currentPosition);

			oresEntries[image2.c_str()] = runtimeResourceID.GetID();
		}
	}

	return oresEntries[image];
}

void DebugMod::EnableInfiniteAmmo()
{
	auto scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

	if (!scene)
	{
		Logger::Debug("Scene not loaded.");
	}
	else
	{
		const auto runtimeResourceID = ResId<"[modules:/zhm5cripplebox.class].pc_entitytype">;

		TResourcePtr<ZTemplateEntityFactory> resource;

		Globals::ResourceManager->GetResourcePtr(resource, runtimeResourceID, 0);

		if (!resource)
		{
			Logger::Debug("Resource is not loaded.");
		}
		else
		{
			ZEntityRef s_NewEntity;

			Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, s_NewEntity, "", resource, scene.m_ref, nullptr, -1);

			if (!s_NewEntity)
			{
				Logger::Debug("Failed to spawn entity.");
			}

			TEntityRef<ZHitman5> s_LocalHitman;

			Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

			ZHM5CrippleBox* hm5CrippleBox = s_NewEntity.QueryInterface<ZHM5CrippleBox>();

			hm5CrippleBox->m_bActivateOnStart = true;
			hm5CrippleBox->m_rHitmanCharacter = s_LocalHitman;
			hm5CrippleBox->m_bLimitedAmmo = false;

			hm5CrippleBox->Activate(0);
		}
	}
}

void DebugMod::CopyToClipboard(const std::string& p_String) const
{
	if (!OpenClipboard(nullptr))
		return;

	EmptyClipboard();

	auto s_GlobalData = GlobalAlloc(GMEM_MOVEABLE, p_String.size() + 1);

	if (!s_GlobalData)
	{
		CloseClipboard();
		return;
	}

	auto s_GlobalDataPtr = GlobalLock(s_GlobalData);

	if (!s_GlobalDataPtr)
	{
		CloseClipboard();
		GlobalFree(s_GlobalData);
		return;
	}

	memset(s_GlobalDataPtr, 0, p_String.size() + 1);
	memcpy(s_GlobalDataPtr, p_String.c_str(), p_String.size());

	GlobalUnlock(s_GlobalData);

	SetClipboardData(CF_TEXT, s_GlobalData);
	CloseClipboard();
}

void DebugMod::OnDraw3D(IRenderer* p_Renderer)
{
	if (m_RenderNpcBoxes || m_RenderNpcNames || m_RenderNpcRepoIds)
	{
		for (int i = 0; i < *Globals::NextActorId; ++i)
		{
			auto* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

			ZEntityRef s_Ref;
			s_Actor->GetID(&s_Ref);

			auto* s_SpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

			SMatrix s_Transform;
			Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

			if (m_RenderNpcBoxes)
			{
				float4 s_Min, s_Max;

				s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

				p_Renderer->DrawOBB3D(SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform, SVector4(1.f, 0.f, 0.f, 1.f));
			}

			if (m_RenderNpcNames)
			{
				SVector2 s_ScreenPos;
				if (p_Renderer->WorldToScreen(SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.05f), s_ScreenPos))
					p_Renderer->DrawText2D(s_Actor->m_sActorName, s_ScreenPos, SVector4(1.f, 0.f, 0.f, 1.f), 0.f, 0.5f);
			}

			if (m_RenderNpcRepoIds)
			{
				auto* s_RepoEntity = s_Ref.QueryInterface<ZRepositoryItemEntity>();
				SVector2 s_ScreenPos;
				bool s_Success;

				if (m_RenderNpcNames)
					s_Success = p_Renderer->WorldToScreen(SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.1f), s_ScreenPos);
				else
					s_Success = p_Renderer->WorldToScreen(SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.05f), s_ScreenPos);

				if (s_Success)
					p_Renderer->DrawText2D(s_RepoEntity->m_sId.ToString(), s_ScreenPos, SVector4(1.f, 0.f, 0.f, 1.f), 0.f, 0.5f);
			}
		}
	}

	m_EntityMutex.lock_shared();

	if (m_SelectedEntity)
	{
		auto* s_SpatialEntity = m_SelectedEntity.QueryInterface<ZSpatialEntity>();

		SMatrix s_Transform;
		Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

		float4 s_Min, s_Max;

		s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

		p_Renderer->DrawOBB3D(SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform, SVector4(0.f, 0.f, 1.f, 1.f));
	}
	
	m_EntityMutex.unlock_shared();

	/*SVector2 s_StartPos;
	if (p_Renderer->WorldToScreen(SVector3(m_From.x, m_From.y, m_From.z), s_StartPos))
		p_Renderer->DrawText2D("0", s_StartPos, SVector4(1, 1, 1, 1), 0, 0.5f);

	SVector2 s_EndPos;
	if (p_Renderer->WorldToScreen(SVector3(m_To.x, m_To.y, m_To.z), s_EndPos))
		p_Renderer->DrawText2D("o", s_EndPos, SVector4(1, 1, 1, 1), 0, 0.25f);

	SVector2 s_HitPos;
	if (p_Renderer->WorldToScreen(SVector3(m_Hit.x, m_Hit.y, m_Hit.z), s_HitPos))
		p_Renderer->DrawText2D("x", s_HitPos, SVector4(1, 1, 1, 1), 0, 0.25f);

	p_Renderer->DrawBox3D(
		SVector3(m_From.x - 0.05f, m_From.y - 0.05f, m_From.z - 0.05f),
		SVector3(m_From.x + 0.05f, m_From.y + 0.05f, m_From.z + 0.05f),
		SVector4(0.f, 0.f, 1.f, 1.0f)
	);

	p_Renderer->DrawBox3D(
		SVector3(m_To.x - 0.05f, m_To.y - 0.05f, m_To.z - 0.05f),
		SVector3(m_To.x + 0.05f, m_To.y + 0.05f, m_To.z + 0.05f),
		SVector4(0.f, 1.f, 0.f, 1.0f)
	);

	p_Renderer->DrawBox3D(
		SVector3(m_Hit.x - 0.01f, m_Hit.y - 0.01f, m_Hit.z - 0.01f),
		SVector3(m_Hit.x + 0.01f, m_Hit.y + 0.01f, m_Hit.z + 0.01f),
		SVector4(0.f, 1.f, 1.f, 1.0f)
	);

	p_Renderer->DrawLine3D(
		SVector3(m_From.x, m_From.y, m_From.z),
		SVector3(m_To.x, m_To.y, m_To.z),
		SVector4(1.f, 1.f, 1.f, 1.f),
		SVector4(1.f, 1.f, 1.f, 1.f)
	);

	p_Renderer->DrawLine3D(
		SVector3(m_Hit.x + (m_Normal.x * 0.15f), m_From.y + (m_Normal.y * 0.15f), m_From.z + (m_Normal.z * 0.15f)),
		SVector3(m_Hit.x, m_Hit.y, m_Hit.z),
		SVector4(0.63f, 0.13f, 0.94f, 1.f),
		SVector4(0.63f, 0.13f, 0.94f, 1.f)
	);*/
}

DECLARE_PLUGIN_DETOUR(DebugMod, void, ZHttpBufferReady, ZHttpResultDynamicObject* th)
{
	/*Logger::Debug("ZHttp thing wow wow {} {} {} {}", fmt::ptr(th), offsetof(ZHttpResultDynamicObject, m_buffer), fmt::ptr(&th->m_buffer), fmt::ptr(th->m_buffer.data()));

	std::string s_Data(static_cast<char*>(th->m_buffer.data()), th->m_buffer.size());

	Logger::Debug("Some shit: {}", s_Data);

	th->m_buffer = ZBuffer::FromData(s_Data);

	std::string s_Data2(static_cast<char*>(th->m_buffer.data()), th->m_buffer.size());

	Logger::Debug("Explosions shit: {}", s_Data2);*/

	return HookResult<void>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(DebugMod, void, WinHttpCallback, void* dwContext, void* hInternet, void* param_3, int dwInternetStatus, void* param_5, int length_param_6)
{
	/*if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_SENDING_REQUEST)
	{
		WinHttpAddRequestHeaders(hInternet, L"Accept-Encoding: identity", (ULONG)-1L, WINHTTP_ADDREQ_FLAG_REPLACE);
		Logger::Info("header set");
	}*/

	return HookResult<void>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(DebugMod, void, OnClearScene, ZEntitySceneContext* th, bool fullyClear)
{
	m_EntityMutex.lock();
	m_SelectedEntity = ZEntityRef();
	m_EntityMutex.unlock();

	return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(DebugMod);
