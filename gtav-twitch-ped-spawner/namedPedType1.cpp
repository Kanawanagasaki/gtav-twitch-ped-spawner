#include "namedPedType1.h"
#include "util.h"

#include <vector>
#include <cmath>

NamedPedType1::NamedPedType1(Ped handle, std::string viewerId, std::string nickname) : NamedPed(handle, viewerId, nickname)
{
	if (getGameVersion() < 80) // idk what the update number is for CASINO HEIST. The latest GTA V update number (at the time of writing) is 86.
	{
		waveAnimDict = "friends@frj@ig_1";
		waveAnimName = "wave_a";
	}
	else if (PED::GET_PED_TYPE(handle) == 5)
	{
		waveAnimDict = "anim@amb@waving@female";
		waveAnimName = "ground_wave";
	}
	else
	{
		waveAnimDict = "anim@amb@waving@male";
		waveAnimName = "ground_wave";
	}

	ENTITY::SET_ENTITY_AS_MISSION_ENTITY(handle, false, false);
	TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle);
	TASK::TASK_GO_TO_ENTITY(handle, PLAYER::PLAYER_PED_ID(), -1, 3.0f, 4.0f, 5.0f, 0);

	PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(handle, TRUE);
	PED::SET_PED_FLEE_ATTRIBUTES(handle, 0, 0);
	PED::SET_PED_COMBAT_ATTRIBUTES(handle, 46, TRUE);
	PED::SET_PED_CONFIG_FLAG(handle, 2, FALSE);

	Hash relationshipGroup;
	PED::ADD_RELATIONSHIP_GROUP("_NAMED_PED", &relationshipGroup);
	PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, relationshipGroup, Util::GetHashKey("PLAYER"));
	PED::SET_RELATIONSHIP_BETWEEN_GROUPS(3, Util::GetHashKey("PLAYER"), relationshipGroup);
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(handle, relationshipGroup);

	SetState(eState::Initial);
}

bool NamedPedType1::ShouldDelete()
{
	return NamedPed::ShouldDelete();
}

void NamedPedType1::SetState(eState s)
{
	state = s;
	stateChangeTime = MISC::GET_GAME_TIMER();
}

void NamedPedType1::ProcessInitial()
{
	auto plPed = PLAYER::PLAYER_PED_ID();
	auto distSq = Game::DistanceSq(plPed, handle);

	if (distSq < 16.0f)
	{
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle);
		SetState(eState::Waving);
	}
}
void NamedPedType1::ProcessWaving()
{
	if (!LoadAnimDict(waveAnimDict.c_str()))
		return;

	auto plPed = PLAYER::PLAYER_PED_ID();
	auto plPos = ENTITY::GET_ENTITY_COORDS(plPed, true);
	auto pedPos = ENTITY::GET_ENTITY_COORDS(handle, true);

	auto vx = plPos.x - pedPos.x;
	auto vy = plPos.y - pedPos.y;
	auto vz = plPos.y - pedPos.y;

	auto distSq = vx * vx + vy * vy + vz * vz;

	if (distSq < 150.0f)
	{
		auto angle = std::atan2(vy, vx) / 3.14159f * 180.0f - 90.0f;
		auto heading = ENTITY::GET_ENTITY_HEADING(handle);

		auto headingDiff = angle - heading;
		if (180.0f < headingDiff)
			headingDiff -= 360.0f;
		else if (headingDiff < -180.0f)
			headingDiff += 360.0f;
		heading += headingDiff / 20.0f;

		ENTITY::SET_ENTITY_HEADING(handle, heading);

		TASK::TASK_LOOK_AT_ENTITY(handle, plPed, -1, 1024, 0);

		if (!ENTITY::IS_ENTITY_PLAYING_ANIM(handle, waveAnimDict.c_str(), waveAnimName.c_str(), 3))
			TASK::TASK_PLAY_ANIM(handle, waveAnimDict.c_str(), waveAnimName.c_str(), 8.0f, -8.0f, -1, 1, 0.0f, 0, 0, 0);
	}
	else
	{
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle);
		SetState(eState::CameraEnter);
	}
}
void NamedPedType1::ProcessCameraEnter()
{
	if (!LoadAnimDict("amb@world_human_paparazzi@male@enter"))
		return;

	if (400 <= MISC::GET_GAME_TIMER() - stateChangeTime && !ENTITY::DOES_ENTITY_EXIST(camera))
	{
		if (!STREAMING::HAS_MODEL_LOADED(0x288DC72A))
		{
			if (!isCameraModelRequested)
			{
				STREAMING::REQUEST_MODEL(0x288DC72A);
				isCameraModelRequested = true;
			}
			return;
		}

		camera = OBJECT::CREATE_OBJECT(0x288DC72A, 0.0f, 0.0f, 0.0f, false, false, false);
		ENTITY::SET_ENTITY_COLLISION(camera, false, false);
		ENTITY::SET_ENTITY_COMPLETELY_DISABLE_COLLISION(camera, false, false);
		if (PED::GET_PED_TYPE(handle) == 5)
			ENTITY::ATTACH_ENTITY_TO_ENTITY(camera, handle, PED::GET_PED_BONE_INDEX(handle, 28422), -0.055f, 0, 0, 0, 0, 0, false, false, false, false, 2, true, true);
		else
			ENTITY::ATTACH_ENTITY_TO_ENTITY(camera, handle, PED::GET_PED_BONE_INDEX(handle, 28422), 0, 0, 0, 0, 0, 0, false, false, false, false, 2, true, true);

		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(0x288DC72A);
		isCameraModelRequested = false;
	}

	if (!ENTITY::IS_ENTITY_PLAYING_ANIM(handle, "amb@world_human_paparazzi@male@enter", "enter", 3))
		TASK::TASK_PLAY_ANIM(handle, "amb@world_human_paparazzi@male@enter", "enter", 8.0f, -8.0f, -1, 2, 0.0f, 0, 0, 0);

	if (1500 <= MISC::GET_GAME_TIMER() - stateChangeTime)
		SetState(eState::CameraIdle);
}
void NamedPedType1::ProcessCameraIdle()
{
	if (!LoadAnimDict("amb@world_human_paparazzi@male@base"))
		return;

	auto plPed = PLAYER::PLAYER_PED_ID();
	auto plPos = ENTITY::GET_ENTITY_COORDS(plPed, true);
	auto pedPos = ENTITY::GET_ENTITY_COORDS(handle, true);

	auto vx = plPos.x - pedPos.x;
	auto vy = plPos.y - pedPos.y;
	auto vz = plPos.y - pedPos.y;

	auto distSq = vx * vx + vy * vy + vz * vz;

	if ((60.0f < distSq || 3.0f <= ENTITY::GET_ENTITY_SPEED(plPed)) && distSq < 2500.0f)
	{
		auto angle = std::atan2(vy, vx) / 3.14159f * 180.0f - 90.0f;
		auto heading = ENTITY::GET_ENTITY_HEADING(handle);

		auto headingDiff = angle - heading;
		if (180.0f < headingDiff)
			headingDiff -= 360.0f;
		else if (headingDiff < -180.0f)
			headingDiff += 360.0f;
		heading += headingDiff / 20.0f;

		ENTITY::SET_ENTITY_HEADING(handle, heading);

		if (!ENTITY::IS_ENTITY_PLAYING_ANIM(handle, "amb@world_human_paparazzi@male@base", "base", 3))
			TASK::TASK_PLAY_ANIM(handle, "amb@world_human_paparazzi@male@base", "base", 8.0f, -8.0f, -1, 1, 0.0f, 0, 0, 0);
	}
	else
		SetState(eState::CameraExit);
}
void NamedPedType1::ProcessCameraExit()
{
	if (!LoadAnimDict("amb@world_human_paparazzi@male@exit"))
		return;

	if (!ENTITY::IS_ENTITY_PLAYING_ANIM(handle, "amb@world_human_paparazzi@male@exit", "exit", 3))
		TASK::TASK_PLAY_ANIM(handle, "amb@world_human_paparazzi@male@exit", "exit", 8.0f, -8.0f, -1, 2, 0.0f, 0, 0, 0);

	if (1175 <= MISC::GET_GAME_TIMER() - stateChangeTime && ENTITY::DOES_ENTITY_EXIST(camera))
	{
		ENTITY::DELETE_ENTITY(&camera);
		camera = 0;
	}

	if (2500 <= MISC::GET_GAME_TIMER() - stateChangeTime)
	{
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle);
		auto distSq = Game::DistanceSq(handle, PLAYER::PLAYER_PED_ID());
		if (distSq < 150.0f)
			SetState(eState::Waving);
		else
			SetState(eState::Follow);
	}
}
void NamedPedType1::ProcessFollow()
{
	auto plPed = PLAYER::PLAYER_PED_ID();
	auto plPos = ENTITY::GET_ENTITY_COORDS(plPed, true);
	auto pedPos = ENTITY::GET_ENTITY_COORDS(handle, true);

	auto vx = plPos.x - pedPos.x;
	auto vy = plPos.y - pedPos.y;
	auto vz = plPos.y - pedPos.y;

	auto distSq = vx * vx + vy * vy + vz * vz;

	if (isFirstFollowTick || 5000 < MISC::GET_GAME_TIMER() - stateChangeTime)
	{
		isFirstFollowTick = false;

		auto angle = std::atan2(vy, vx) / 3.14159f * 180.0f - 90.0f;
		TASK::TASK_GO_TO_ENTITY(handle, plPed, -1, 0.0f, 4.0f, 0.0f, 0x01);
		stateChangeTime = MISC::GET_GAME_TIMER();
	}

	if (distSq < 1600.0f)
	{
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle);
		SetState(eState::CameraEnter);
		isFirstFollowTick = true;
	}
	else if (250000.0f < distSq)
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

		auto spotX = plPos.x - nx * 50.0f;
		auto spotY = plPos.y - ny * 50.0f;
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
	}
}

void NamedPedType1::Tick()
{
	NamedPed::Tick();

	if (!ENTITY::DOES_ENTITY_EXIST(handle))
		return;

	switch (state)
	{
	case eState::Initial: ProcessInitial(); break;
	case eState::Waving: ProcessWaving(); break;
	case eState::CameraEnter: ProcessCameraEnter(); break;
	case eState::CameraIdle: ProcessCameraIdle(); break;
	case eState::CameraExit: ProcessCameraExit(); break;
	case eState::Follow: ProcessFollow(); break;
	}
}

NamedPedType1::~NamedPedType1()
{
	if (ENTITY::DOES_ENTITY_EXIST(camera))
		ENTITY::DELETE_ENTITY(&camera);
}

bool NamedPedType1::FindSpot(Vector3* spot)
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

bool NamedPedType1::TryCreate(Game::Redemption* redemption, NamedPed** res)
{
	auto plPed = PLAYER::PLAYER_PED_ID();
	if (PED::IS_PED_IN_ANY_VEHICLE(plPed, true))
		return false;

	Vector3 spot = {};
	if (!FindSpot(&spot))
		return false;

	auto ped = PED::CREATE_RANDOM_PED(spot.x, spot.y, spot.z);

	*res = new NamedPedType1(ped, redemption->userId, redemption->userName);

	Game::ShowNotification(redemption->userName + " prepared their camera");

	return true;
}
