#include "namedPedType2.h"
#include "util.h"
#include "entityIterator.h"
#include "inc/enums.h"

Hash NamedPedType2::GetRelationshipGroup()
{
	static Hash relationshipGroup = 0;

	if (relationshipGroup == 0)
	{
		PED::ADD_RELATIONSHIP_GROUP("_NAMED_PED_TYPE_2", &relationshipGroup);
		PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, relationshipGroup, Util::GetHashKey("PLAYER"));
		PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, Util::GetHashKey("PLAYER"), relationshipGroup);
	}
	return relationshipGroup;
}

NamedPedType2::NamedPedType2(Ped handle, std::string viewerId, std::string nickname, ENicknameVehicleRender vehRender) : NamedPed(handle, viewerId, nickname, vehRender)
{
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(handle, GetRelationshipGroup());

	PED::SET_PED_SUFFERS_CRITICAL_HITS(handle, false);
	PED::SET_PED_HEARING_RANGE(handle, 9999.f);
	PED::SET_PED_CONFIG_FLAG(handle, 281, true);
	PED::SET_PED_CAN_RAGDOLL_FROM_PLAYER_IMPACT(handle, false);

	PED::SET_PED_AS_GROUP_MEMBER(handle, PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()));
	PED::SET_PED_NEVER_LEAVES_GROUP(handle, true);
	PED::SET_PED_CAN_TELEPORT_TO_GROUP_LEADER(handle, PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()), false);
	PED::SET_GROUP_FORMATION(PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()), 1);
	PED::SET_GROUP_FORMATION_SPACING(PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()), 10.0f, 10.0f, 200.0f);
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

		PED::SET_PED_AS_GROUP_MEMBER(handle, 0);
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(handle, spotX, spotY, spotZ, true, true, true);
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle);
		PED::SET_PED_AS_GROUP_MEMBER(handle, PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()));
		PED::SET_PED_CAN_TELEPORT_TO_GROUP_LEADER(handle, PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()), false);
		PED::SET_GROUP_FORMATION(PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()), 1);
		PED::SET_GROUP_FORMATION_SPACING(PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()), 10.0f, 10.0f, 200.0f);
	}

	if (MISC::GET_GAME_TIMER() - lastGroupResetTime < 5000)
	{
		PED::SET_PED_RELATIONSHIP_GROUP_HASH(handle, GetRelationshipGroup());
		PED::SET_PED_AS_GROUP_MEMBER(handle, 0);
		PED::SET_PED_AS_GROUP_MEMBER(handle, PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()));

		lastGroupResetTime = MISC::GET_GAME_TIMER();
	}

	for (const auto& companion : Game::GetCompanions())
	{
		if (PED::GET_RELATIONSHIP_BETWEEN_PEDS(handle, companion) != 0)
		{
			PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, GetRelationshipGroup(), PED::GET_PED_RELATIONSHIP_GROUP_HASH(companion));
			PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, PED::GET_PED_RELATIONSHIP_GROUP_HASH(companion), GetRelationshipGroup());
		}
	}

	if (PED::IS_PED_IN_ANY_VEHICLE(plPed, true))
	{
		auto veh = PED::GET_VEHICLE_PED_IS_IN(plPed, true);
		auto vehModel = ENTITY::GET_ENTITY_MODEL(veh);
		auto numSeats = VEHICLE::GET_VEHICLE_MODEL_NUMBER_OF_SEATS(vehModel);

		auto plGroup = PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID());
		int groupMembersCount = 0;
		for (const auto& companion : Game::GetCompanions())
			if (PED::IS_PED_GROUP_MEMBER(companion, plGroup) || PED::GET_RELATIONSHIP_BETWEEN_PEDS(companion, plPed) == 0)
				groupMembersCount++;

		if (PED::IS_PED_IN_ANY_VEHICLE(handle, false))
		{
			int seat = -1;
			for (int i = 0; i <= 2; i++)
				if (!VEHICLE::IS_VEHICLE_SEAT_FREE(veh, i, false) && VEHICLE::GET_PED_IN_VEHICLE_SEAT(veh, i, true) == handle)
					seat = i;

			if (numSeats - 1 <= groupMembersCount || seat < groupMembersCount)
			{
				if (ENTITY::GET_ENTITY_SPEED(veh) < 3.0f)
					TASK::TASK_LEAVE_ANY_VEHICLE(handle, 0, 0);
				else
					TASK::TASK_LEAVE_ANY_VEHICLE(handle, 0, 4160);
			}
		}
		else
		{
			auto veh2 = PED::GET_VEHICLE_PED_IS_ENTERING(handle);
			auto seat = PED::GET_SEAT_PED_IS_TRYING_TO_ENTER(handle);
			if (veh2 == veh && ENTITY::DOES_ENTITY_EXIST(veh2) && (seat < groupMembersCount || !VEHICLE::IS_VEHICLE_SEAT_FREE(veh, seat, false)))
			{
				TASK::CLEAR_PED_TASKS(handle);
				int trySeat = max(groupMembersCount, seat + 1) % 3;
				if (groupMembersCount <= trySeat && VEHICLE::IS_VEHICLE_SEAT_FREE(veh, trySeat, false))
					TASK::TASK_ENTER_VEHICLE(handle, veh, -1, trySeat, 4.0f, 1, nullptr, false);
			}
		}
	}
	else if (PED::IS_PED_IN_ANY_VEHICLE(handle, true))
	{
		auto veh = PED::GET_VEHICLE_PED_IS_IN(handle, true);

		if (ENTITY::GET_ENTITY_SPEED(veh) < 3.0f)
			TASK::TASK_LEAVE_ANY_VEHICLE(handle, 0, 0);
		else
			TASK::TASK_LEAVE_ANY_VEHICLE(handle, 0, 4160);
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

NamedPedType2::~NamedPedType2()
{
	if (!ENTITY::DOES_ENTITY_EXIST(handle))
		return;

	TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle);
	WEAPON::REMOVE_ALL_PED_WEAPONS(handle, true);
	PED::SET_PED_AS_GROUP_MEMBER(handle, 0);
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(handle, 0);

	PED::SET_PED_SUFFERS_CRITICAL_HITS(handle, true);
	PED::SET_PED_CONFIG_FLAG(handle, 281, false);
	PED::SET_PED_CAN_RAGDOLL_FROM_PLAYER_IMPACT(handle, true);
	PED::SET_PED_COMBAT_ATTRIBUTES(handle, 5, false);
	PED::SET_PED_COMBAT_ATTRIBUTES(handle, 46, false);

	if (PED::IS_PED_IN_ANY_VEHICLE(handle, false))
		TASK::TASK_LEAVE_ANY_VEHICLE(handle, 0, 4160);
}

bool NamedPedType2::FindSpot(Vector3* spot)
{
	Vector3 camPos = CAM::GET_FINAL_RENDERED_CAM_COORD();
	Vector3 camRot = CAM::GET_FINAL_RENDERED_CAM_ROT(2);

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

bool NamedPedType2::TryCreate(Game::Redemption* redemption, NamedPed** res)
{
	auto vehRender = ENicknameVehicleRender::NEVER;
	if (redemption->extra == "fivesec")
		vehRender = ENicknameVehicleRender::FIVESEC;
	else if (redemption->extra == "always")
		vehRender = ENicknameVehicleRender::ALWAYS;

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
			*res = new NamedPedType2(ped, redemption->userId, redemption->userName, vehRender);

			Game::ShowNotification(redemption->userName + " ready to fight");

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
		*res = new NamedPedType2(ped, redemption->userId, redemption->userName, vehRender);

		Game::ShowNotification(redemption->userName + " ready to fight");

		return true;
	}
}
