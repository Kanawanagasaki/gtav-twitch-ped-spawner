#include "game.h"
#include "rewards.h"
#include "entityIterator.h"
#include "util.h"
#include "namedPedType0.h"
#include "namedPedType1.h"
#include "namedPedType2.h"

#include <queue>

#include "inc/natives.h"

namespace Game
{
	std::unordered_map<int, Redemption*> redemptions;

	std::unordered_map<Ped, NamedPed*> peds = {};
	std::unordered_set<std::string> spawnedViewerIds = {};
	std::unordered_map<int, std::queue<Ped>*> pedsQueue = {};

	std::unordered_set<Ped> companions = {};

	std::vector<Ped> pedsToDespawn = {};
	bool shouldDespawnAllPeds = false;

	int playerDeathTime = 0;

	void Process(Redemption* toProcess)
	{
		if (redemptions.contains(toProcess->rewardType))
			delete redemptions[toProcess->rewardType];
		redemptions[toProcess->rewardType] = toProcess;
	}

	int ShowNotification(std::string text)
	{
		HUD::BEGIN_TEXT_COMMAND_THEFEED_POST("STRING");
		HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text.c_str());
		return HUD::END_TEXT_COMMAND_THEFEED_POST_TICKER(0, 1);
	}

	float DistanceSq(Entity entityA, Entity entityB)
	{
		auto entityAPos = ENTITY::GET_ENTITY_COORDS(entityA, true);
		auto entityBPos = ENTITY::GET_ENTITY_COORDS(entityB, true);

		auto vx = entityAPos.x - entityBPos.x;
		auto vy = entityAPos.y - entityBPos.y;
		auto vz = entityAPos.z - entityBPos.z;

		return vx * vx + vy * vy + vz * vz;
	}

	std::unordered_map<Ped, NamedPed*>* GetSpawnedPeds()
	{
		return &peds;
	}
	int GetSpawnedPedsCount()
	{
		return peds.size();
	}

	int CompanionsCount()
	{
		return companions.size();
	}
	bool IsPedCompanion(Ped ped)
	{
		return companions.contains(ped);
	}
	std::unordered_set<Ped> GetCompanions()
	{
		return companions;
	}

	typedef bool (*TryCreate)(Redemption*, NamedPed**);
	TryCreate createFuncs[] = { NamedPedType0::TryCreate, NamedPedType1::TryCreate, NamedPedType2::TryCreate };
	void Tick()
	{
		companions.clear();
		auto plPed = PLAYER::PLAYER_PED_ID();
		auto plGroup = PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID());
		for (const auto& ped : EntityIterator::GetAllPeds())
		{
			if (ped == plPed)
				continue;
			if (peds.contains(ped))
				continue;
			if (PED::IS_PED_GROUP_MEMBER(ped, plGroup) || PED::GET_RELATIONSHIP_BETWEEN_PEDS(ped, plPed) <= 2)
				companions.insert(ped);
		}

		int now = MISC::GET_GAME_TIMER();
		if (ENTITY::IS_ENTITY_DEAD(plPed, false))
			playerDeathTime = now;
		if (!HUD::IS_RADAR_HIDDEN() && 15000 < now - playerDeathTime)
		{
			for (int i = 0; i <= 2; i++)
			{
				if (!redemptions.contains(i))
					continue;

				auto redemption = redemptions[i];
				NamedPed* namedPed;
				if (spawnedViewerIds.contains(redemption->userId))
				{
					Rewards::Cancel(redemption);

					delete redemption;
					redemptions.erase(i);
				}
				else if (createFuncs[i](redemption, &namedPed))
				{
					peds[namedPed->GetHandle()] = namedPed;
					if (!pedsQueue.contains(i))
						pedsQueue[i] = new std::queue<Ped>;
					pedsQueue[i]->push(namedPed->GetHandle());
					spawnedViewerIds.insert(redemption->userId);
					Rewards::Fulfill(redemption);

					delete redemption;
					redemptions.erase(i);
				}
			}
		}

		for (auto it = peds.begin(); it != peds.end(); )
		{
			if (it->second->ShouldDelete())
			{
				spawnedViewerIds.erase(it->second->GetViewerId());

				delete it->second;
				it = peds.erase(it);
			}
			else
			{
				it->second->Tick();
				++it;
			}
		}

		for (int i = 0; i <= 2; i++)
		{
			if (!pedsQueue.contains(i))
				continue;
			auto queue = pedsQueue[i];
			if (queue->size() == 0)
				continue;
			auto frontPed = queue->front();
			if (ENTITY::DOES_ENTITY_EXIST(frontPed))
				continue;

			if (peds.contains(frontPed))
			{
				auto namedPed = peds[frontPed];
				spawnedViewerIds.erase(namedPed->GetViewerId());

				delete namedPed;
				peds.erase(frontPed);
			}
			queue->pop();

		}

		for (int i = 0; i <= 2; i++)
		{
			if (!pedsQueue.contains(i))
				continue;

			auto queue = pedsQueue[i];
			while ((i == 2 ? 2 : 4) < queue->size())
			{
				auto ped = queue->front();
				if (peds.contains(ped))
				{
					auto namedPed = peds[ped];
					spawnedViewerIds.erase(namedPed->GetViewerId());

					delete namedPed;
					peds.erase(ped);
				}
				queue->pop();
			}
		}

		if (0 < pedsToDespawn.size())
		{
			for (const auto& ped : pedsToDespawn)
			{
				auto pedRef = ped;
				if (ENTITY::DOES_ENTITY_EXIST(pedRef))
				{
					ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped, true, true);
					ENTITY::DELETE_ENTITY(&pedRef);
				}
			}
			pedsToDespawn.clear();
		}

		if (shouldDespawnAllPeds)
		{
			for (const auto& pair : peds)
			{
				auto ped = pair.first;
				if (ENTITY::DOES_ENTITY_EXIST(ped))
				{
					ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped, true, true);
					ENTITY::DELETE_ENTITY(&ped);
				}
			}

			shouldDespawnAllPeds = false;
		}
	}

	void DespawnPed(Ped ped)
	{
		pedsToDespawn.push_back(ped);
	}
	void DespawnAllPeds()
	{
		shouldDespawnAllPeds = true;
	}
}
