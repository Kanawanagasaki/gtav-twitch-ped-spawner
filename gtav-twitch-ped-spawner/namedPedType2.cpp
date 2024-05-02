#include "namedPedType2.h"
#include "util.h"
#include "game.h"
#include "entityIterator.h"
#include "inc/enums.h"

NamedPedType2::NamedPedType2(Ped handle, std::string viewerId, std::string nickname) : NamedPed(handle, viewerId, nickname)
{
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(handle, Game::GetNamedPedType2Group());

	PED::SET_PED_SUFFERS_CRITICAL_HITS(handle, false);
	PED::SET_PED_HEARING_RANGE(handle, 9999.f);
	PED::SET_PED_CONFIG_FLAG(handle, 281, true);
	PED::SET_PED_CAN_RAGDOLL_FROM_PLAYER_IMPACT(handle, false);
	PED::SET_RAGDOLL_BLOCKING_FLAGS(handle, 5);

	PED::SET_PED_AS_GROUP_MEMBER(handle, PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()));
	PED::SET_PED_NEVER_LEAVES_GROUP(handle, true);
	PED::SET_PED_COMBAT_ATTRIBUTES(handle, 5, true);
	PED::SET_PED_COMBAT_ATTRIBUTES(handle, 46, true);
	PED::SET_PED_ACCURACY(handle, 100);
	PED::SET_PED_FIRING_PATTERN(handle, eFiringPattern::FiringPatternFullAuto);
}

bool NamedPedType2::ShouldDelete()
{
	return NamedPed::ShouldDelete();
}

void NamedPedType2::Tick()
{
	NamedPed::Tick();

	if (!ENTITY::DOES_ENTITY_EXIST(handle))
		return;

	for (const auto& companion : Game::GetCompanions())
	{
		if (PED::GET_RELATIONSHIP_BETWEEN_PEDS(handle, companion) != 0)
		{
			Hash relationshipGroup;
			PED::ADD_RELATIONSHIP_GROUP("_COMPANION", &relationshipGroup);
			PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, relationshipGroup, Game::GetNamedPedType2Group());
			PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, Game::GetNamedPedType2Group(), relationshipGroup);
			PED::SET_PED_RELATIONSHIP_GROUP_HASH(companion, relationshipGroup);
		}
	}

	auto plPed = PLAYER::PLAYER_PED_ID();
	auto distSq = Game::DistanceSq(plPed, handle);

	if (250000.0f < distSq)
	{
		auto plPos = ENTITY::GET_ENTITY_COORDS(plPed, true);

		Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
		Vector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);

		float pitch = camRot.x / 180.0f * 3.14159265f;
		float yaw = (camRot.z + 90.0f) / 180.0f * 3.14159265f;

		auto vx = cos(yaw) * cos(pitch);
		auto vy = sin(yaw) * cos(pitch);

		auto len = sqrt(vx * vx + vy * vy);

		auto nx = vx / len;
		auto ny = vy / len;

		auto spotX = plPos.x - nx * 100.0f;
		auto spotY = plPos.y - ny * 100.0f;
		auto spotZ = PATHFIND::GET_APPROX_FLOOR_FOR_POINT(spotX, spotY) + 1.0f;
		float groundZ;
		if (MISC::GET_GROUND_Z_FOR_3D_COORD(spotX, spotY, spotZ, &groundZ, true, true) && 0.0f < groundZ)
			spotZ = groundZ;

		Vector3 node;
		if (PATHFIND::GET_NTH_CLOSEST_VEHICLE_NODE(spotX, spotY, spotZ, 1, &node, 1, 3.0f, 0.0f))
		{
			spotX = node.x;
			spotY = node.y;
			spotZ = node.z + 1.0f;

			if (PATHFIND::GET_SAFE_COORD_FOR_PED(spotX, spotY, spotZ, true, &node, 0))
			{
				spotX = node.x;
				spotY = node.y;
				spotZ = node.z + 1.0f;
			}
		}

		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(handle, spotX, spotY, spotZ, true, true, true);
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle);
		TASK::TASK_GO_TO_ENTITY(handle, plPed, -1, 5.0f, 4.0f, 10.0f, 0);
	}
	
	if (PED::IS_PED_IN_ANY_VEHICLE(plPed, true))
	{
		auto plGroup = PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID());

		auto veh = PED::GET_VEHICLE_PED_IS_IN(plPed, true);
		auto vehModel = ENTITY::GET_ENTITY_MODEL(veh);
		auto numSeats = VEHICLE::GET_VEHICLE_MODEL_NUMBER_OF_SEATS(vehModel);

		if (0 < Game::CompanionsCount() && numSeats - 1 < Game::CompanionsCount() + Game::GetSpawnedPedsCount())
		{
			if (PED::IS_PED_IN_ANY_VEHICLE(handle, false))
			{
				if (ENTITY::GET_ENTITY_SPEED(veh) < 2.0f)
					TASK::TASK_LEAVE_ANY_VEHICLE(handle, 0, 0);
				else
					TASK::TASK_LEAVE_ANY_VEHICLE(handle, 0, 4160);
			}
			else
			{
				auto veh2 = PED::GET_VEHICLE_PED_IS_ENTERING(handle);
				if (ENTITY::DOES_ENTITY_EXIST(veh2))
					TASK::CLEAR_PED_TASKS(handle);
			}
		}
	}

	auto pos = ENTITY::GET_ENTITY_COORDS(handle, true);

	Hash weapon;
	WEAPON::GET_CURRENT_PED_WEAPON(handle, &weapon, true);

	if (weapon == eWeapon::WeaponUnarmed || weapon == 0)
	{
		auto meleeTarget = PED::GET_MELEE_TARGET_FOR_PED(handle);
		if (ENTITY::DOES_ENTITY_EXIST(meleeTarget))
		{
			Hash meleeTargetWeapon;
			WEAPON::GET_CURRENT_PED_WEAPON(meleeTarget, &meleeTargetWeapon, true);

			if (meleeTargetWeapon != eWeapon::WeaponUnarmed && ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(meleeTarget, handle, false))
			{
				WEAPON::REMOVE_ALL_PED_WEAPONS(meleeTarget, true);

				WEAPON::GIVE_WEAPON_TO_PED(handle, meleeTargetWeapon, 9999, true, true);
				WEAPON::SET_PED_INFINITE_AMMO(handle, true, meleeTargetWeapon);
				WEAPON::SET_PED_INFINITE_AMMO_CLIP(handle, true);
				WEAPON::SET_CURRENT_PED_WEAPON(handle, meleeTargetWeapon, true);
			}
		}
	}
}
