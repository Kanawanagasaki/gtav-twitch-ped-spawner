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
	std::queue<Ped> pedsQueue = {};

	std::unordered_set<Ped> companions = {};
	Hash relationshipGroup = 0;

	std::vector<Ped> pedsToDespawn = {};
	bool shouldDespawnAllPeds = false;

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

	std::unordered_map<Ped, NamedPed*> GetSpawnedPeds()
	{
		return peds;
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
	Hash GetNamedPedType2Group()
	{
		if (relationshipGroup == 0)
		{
			PED::ADD_RELATIONSHIP_GROUP("_NAMED_PED_TYPE_2", &relationshipGroup);
			PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, relationshipGroup, Util::GetHashKey("PLAYER"));
			PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, Util::GetHashKey("PLAYER"), relationshipGroup);
		}
		return relationshipGroup;
	}

	bool TryProcessType0()
	{
		auto plPed = PLAYER::PLAYER_PED_ID();

		if (PED::IS_PED_IN_ANY_VEHICLE(plPed, true))
			return false;

		for (const auto& ped : EntityIterator::GetAllPeds())
		{
			if (plPed == ped)
				continue;
			if (ENTITY::IS_ENTITY_DEAD(ped, false))
				continue;
			if (PED::IS_PED_IN_ANY_VEHICLE(ped, true))
				continue;
			if (peds.contains(ped))
				continue;
			auto pedType = PED::GET_PED_TYPE(ped);
			if (pedType != 4 && pedType != 5)
				continue;
			if (ENTITY::IS_ENTITY_A_MISSION_ENTITY(ped))
				continue;

			auto distSq = DistanceSq(plPed, ped);

			if (35.0f < distSq)
				continue;

			auto model = ENTITY::GET_ENTITY_MODEL(ped);
			Vector3 min;
			Vector3 max;
			MISC::GET_MODEL_DIMENSIONS(model, &min, &max);

			auto pedPos = ENTITY::GET_ENTITY_COORDS(ped, true);
			float worldZ = pedPos.z + max.z / 2.0f;
			float screenX;
			float screenY;
			if (!GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(pedPos.x, pedPos.y, worldZ, &screenX, &screenY))
				continue;

			auto redemption = redemptions[0];
			peds[ped] = new NamedPedType0(ped, redemption->userId, redemption->userName);

			ShowNotification(redemption->userName + " waves hello");

			return true;
		}

		return false;
	}

	bool FindSpot(Vector3* spot)
	{
		Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
		Vector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);

		float pitch = camRot.x / 180.0f * 3.14159265f;
		float yaw = (camRot.z + 90.0f) / 180.0f * 3.14159265f;

		auto vx = cos(yaw) * cos(pitch);
		auto vy = sin(yaw) * cos(pitch);

		auto len = sqrt(vx * vx + vy * vy);

		auto nx = vx / len;
		auto ny = vy / len;

		auto plPed = PLAYER::PLAYER_PED_ID();
		auto plPos = ENTITY::GET_ENTITY_COORDS(plPed, true);
		auto spotX = plPos.x - nx * 10.0f;
		auto spotY = plPos.y - ny * 10.0f;
		auto spotZ = plPos.z + 10.0f;

		float groundZ;
		if (!MISC::GET_GROUND_Z_FOR_3D_COORD(spotX, spotY, spotZ, &groundZ, true, true))
			return false;
		if (groundZ == 0.0f)
			return false;
		if (7.0f < abs(plPos.z - groundZ))
			return false;

		groundZ += 1.0f;

		auto rayX = plPos.x - nx * 15.0f;
		auto rayY = plPos.y - ny * 15.0f;
		auto hRaycast = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(camPos.x, camPos.y, camPos.z, rayX, rayY, groundZ, 0x1FF, plPed, 7);

		BOOL hits;
		Vector3 hitRes;
		Vector3 surfaceNormal;
		Entity hitEntity;
		if (SHAPETEST::GET_SHAPE_TEST_RESULT(hRaycast, &hits, &hitRes, &surfaceNormal, &hitEntity) == 2 && hits)
			return false;

		spot->x = spotX;
		spot->y = spotY;
		spot->z = groundZ;

		return true;
	}

	bool TryProcessType1()
	{
		auto plPed = PLAYER::PLAYER_PED_ID();
		if (PED::IS_PED_IN_ANY_VEHICLE(plPed, true))
			return false;

		Vector3 spot = {};
		if (!FindSpot(&spot))
			return false;

		auto ped = PED::CREATE_RANDOM_PED(spot.x, spot.y, spot.z);

		auto redemption = redemptions[1];
		peds[ped] = new NamedPedType1(ped, redemption->userId, redemption->userName);
		pedsQueue.push(ped);

		auto pronounce = PED::GET_PED_TYPE(ped) == 5 ? "her" : "his";
		ShowNotification(redemption->userName + " prepared " + pronounce + " camera");

		return true;
	}

	bool TryProcessType2()
	{
		auto redemption = redemptions[2];
		auto plPed = PLAYER::PLAYER_PED_ID();
		if (PED::IS_PED_IN_ANY_VEHICLE(plPed, false))
		{
			auto veh = PED::GET_VEHICLE_PED_IS_IN(plPed, false);

			for (int i = 0; i <= 2; i++)
			{
				if (!VEHICLE::IS_VEHICLE_SEAT_FREE(veh, i, true))
					continue;

				auto ped = PED::CREATE_RANDOM_PED(0, 0, 0);
				PED::SET_PED_INTO_VEHICLE(ped, veh, i);
				peds[ped] = new NamedPedType2(ped, redemption->userId, redemption->userName);
				pedsQueue.push(ped);

				ShowNotification(redemption->userName + " ready to fight");

				return true;
			}

			return false;
		}
		else
		{
			Vector3 spot = {};
			if (!FindSpot(&spot))
				return false;

			auto ped = PED::CREATE_RANDOM_PED(spot.x, spot.y, spot.z);
			peds[ped] = new NamedPedType2(ped, redemption->userId, redemption->userName);
			pedsQueue.push(ped);

			ShowNotification(redemption->userName + " ready to fight");

			return true;
		}
	}

	typedef bool (*TryProcess)();
	TryProcess processFuncs[] = { TryProcessType0, TryProcessType1, TryProcessType2 };
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
			if (PED::IS_PED_GROUP_MEMBER(ped, plGroup))
				companions.insert(ped);
		}

		for (int i = 0; i <= 2; i++)
		{
			if (!redemptions.contains(i))
				continue;

			auto redemption = redemptions[i];
			if (spawnedViewerIds.contains(redemption->userId))
			{
				Rewards::Cancel(redemption);

				delete redemption;
				redemptions.erase(i);
			}
			else if (processFuncs[i]())
			{
				spawnedViewerIds.insert(redemption->userId);
				Rewards::Fulfill(redemption);

				delete redemption;
				redemptions.erase(i);
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

		if (0 < pedsQueue.size() && !ENTITY::DOES_ENTITY_EXIST(pedsQueue.front()))
		{
			auto ped = pedsQueue.front();
			if (peds.contains(ped))
			{
				auto namedPed = peds[ped];
				spawnedViewerIds.erase(namedPed->GetViewerId());

				delete namedPed;
				peds.erase(ped);
			}
			pedsQueue.pop();
		}

		while (15 < pedsQueue.size())
		{
			auto ped = pedsQueue.front();
			if (peds.contains(ped))
			{
				auto namedPed = peds[ped];
				spawnedViewerIds.erase(namedPed->GetViewerId());

				delete namedPed;
				peds.erase(ped);
			}
			pedsQueue.pop();
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
