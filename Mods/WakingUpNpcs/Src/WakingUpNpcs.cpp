#include "WakingUpNpcs.h"

#include <random>

#include "Events.h"
#include "Functions.h"
#include "Logging.h"

#include <Glacier/ZActor.h>
#include <Glacier/ZKnowledge.h>
#include <Glacier/SGameUpdateEvent.h>
#include <Glacier/ZObject.h>
#include <Glacier/ZScene.h>

WakingUpNpcs::WakingUpNpcs() :
	m_Generator(m_RandomDevice())
{
}

WakingUpNpcs::~WakingUpNpcs()
{
	const ZMemberDelegate<WakingUpNpcs, void(const SGameUpdateEvent&)> s_Delegate(this, &WakingUpNpcs::OnFrameUpdate);
	Hooks::ZGameLoopManager_UnregisterFrameUpdate->Call(Globals::GameLoopManager, s_Delegate, 0, EUpdateMode::eUpdatePlayMode);
}

void WakingUpNpcs::Init()
{
	Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &WakingUpNpcs::OnLoadScene);
	Events::OnConsoleCommand->AddListener(this, &WakingUpNpcs::OnConsoleCommand);
}

void WakingUpNpcs::OnEngineInitialized()
{
	const ZMemberDelegate<WakingUpNpcs, void(const SGameUpdateEvent&)> s_Delegate(this, &WakingUpNpcs::OnFrameUpdate);
	Hooks::ZGameLoopManager_RegisterFrameUpdate->Call(Globals::GameLoopManager, s_Delegate, 0, EUpdateMode::eUpdatePlayMode);
}

void WakingUpNpcs::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
	for (int i = 0; i < *Globals::NextActorId; ++i)
	{
		auto* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

		// Process NPCs that are pacified (knocked out) and are not hidden (or being hidden) in a container.
		if (s_Actor->IsPacified() && !s_Actor->m_bBodyHidden && !s_Actor->m_bIsBeingDumped)
		{
			auto it = m_PacifiedTimes.find(s_Actor);

			if (it == m_PacifiedTimes.end())
			{
				// If this is the first time we see this NPC it means they were just now pacified.
				// Wake them up at some point in the future, between 4 and 8 minutes.
				std::uniform_real_distribution<double> s_Distribution(4.0 * 60.0, 8.0 * 60.0);
				double s_WakeUpTime = s_Distribution(m_Generator);

				Logger::Debug("Actor '{}' was pacified. Waking up in {} seconds.", s_Actor->m_sActorName.c_str(), s_WakeUpTime);

				m_PacifiedTimes[s_Actor] = s_WakeUpTime;
			}
			else
			{
				// If we already have them in the list, just tick down their wake up timer.
				auto s_RemainingTime = it->second - p_UpdateEvent.m_GameTimeDelta.ToSeconds();

				// As soon as the timer reaches 0, then it means it's time to wake up the NPC.
				// If however the NPC is being dragged we should wait until that action stops.
				if (s_RemainingTime <= 0 && !s_Actor->m_bIsBeingDragged)
				{
					// TODO: Set alerted state.
					Logger::Debug("Waking up actor '{}'.", s_Actor->m_sActorName.c_str());
					Functions::ZActor_ReviveActor->Call(s_Actor);

					m_PacifiedTimes.erase(it);
				}
				else
				{
					m_PacifiedTimes[s_Actor] = s_RemainingTime;
				}
			}
		}
	}
}

DECLARE_PLUGIN_DETOUR(WakingUpNpcs, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& sceneData)
{
	// TODO: This doesn't get called when loading a save on the same level or restarting. Find something that does.
	Logger::Debug("Loading scene: {}", sceneData.m_sceneName.c_str());

	m_PacifiedTimes.clear();

	return HookResult<void>(HookAction::Continue());
}

DECLARE_PLUGIN_LISTENER(WakingUpNpcs, OnConsoleCommand)
{
	Logger::Debug("On console command!");

	for (int i = 0; i < *Globals::NextActorId; ++i)
	{
		auto* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

		s_Actor->m_OutfitRepositoryID = "3a7a202d-48db-423c-8668-a431f744613c";
		Functions::ZActor_OnOutfitChanged->Call(s_Actor);

		ZEntityRef s_ActorRef;
		s_Actor->GetID(&s_ActorRef);

		for (auto s_Interface : *(*s_ActorRef.m_pEntity)->m_pInterfaces)
		{
			Logger::Debug("Interface: {}", s_Interface.m_pTypeId->typeInfo()->m_pTypeName);
		}

		break;

		auto s_RepoItem = s_ActorRef.QueryInterface<ZRepositoryItemEntity>();

		if (s_RepoItem)
			Logger::Debug("Actor '{}' with repo id: {}", s_Actor->m_sActorName.c_str(), s_RepoItem->m_sId.ToString());
		else
			Logger::Debug("Actor '{}' without repo id.", s_Actor->m_sActorName.c_str());

		// TODO: Check if dragged or in a container.
		/*if (s_Actor->IsPacified())
		{
			auto s_Knowledge = s_Actor->Knowledge();

			Logger::Debug("Got knowledge for actor {}. Setting tension to alerted.", fmt::ptr(s_Knowledge));

			ZEntityRef s_EntityRef;
			s_Actor->GetID(&s_EntityRef);

			if ((*s_EntityRef.m_pEntity)->m_pProperties01)
			{
				for (auto& s_Property : *(*s_EntityRef.m_pEntity)->m_pProperties01)
				{
					Logger::Debug("Property: {} => {} {} {}", s_Property.m_nPropertyId, s_Property.m_pType->getPropertyInfo()->m_pName, fmt::ptr(s_Property.m_pType), fmt::ptr(s_Property.m_pType->getPropertyInfo()));
				}
			}

			/*s_EntityRef.SetProperty("m_OutfitRepositoryID", ZRepositoryID("8F928C7A-99DD-43DB-A027-1310CF49F3AD"));

			ZVariant<ZRepositoryID> s_Property = s_EntityRef.GetProperty<ZRepositoryID>("m_OutfitRepositoryID");

			ZObjectRef s_Thing;
			Logger::Debug("Real property: {}. Read property: {} {}.", s_Actor->m_OutfitRepositoryID.ToString(), s_Property.As<ZRepositoryID>()->ToString(), s_Property.IsEmpty());#1#
		}*/

		//Functions::ZHM5BaseCharacter_ActivateRagdoll->Call(s_Actor, true);
	}

	//Hooks::ZActorManager_SetHitmanSharedEvent->Call(Globals::ActorManager, EAISharedEventType::AISET_Hunted, true);
}

DECLARE_ZHM_PLUGIN(WakingUpNpcs);
