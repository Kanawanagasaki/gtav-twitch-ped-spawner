#include "namedPedType0.h"
#include "util.h"
#include "inc/main.h"

#include <cmath>

NamedPedType0::NamedPedType0(Ped handle, std::string viewerId, std::string nickname) : NamedPed(handle, viewerId, nickname)
{
	if (getGameVersion() < 80) // idk what the update number is for CASINO HEIST. The latest GTA V update number (at the time of writing) is 86.
	{
		animDict = "friends@frj@ig_1";
		animName = "wave_a";
	}
	else if (PED::GET_PED_TYPE(handle) == 5)
	{
		animDict = "anim@amb@waving@female";
		animName = "ground_wave";
	}
	else
	{
		animDict = "anim@amb@waving@male";
		animName = "ground_wave";
	}

	Hash relationshipGroup;
	PED::ADD_RELATIONSHIP_GROUP("_NAMED_PED", &relationshipGroup);
	PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, relationshipGroup, Util::GetHashKey("PLAYER"));
	PED::SET_RELATIONSHIP_BETWEEN_GROUPS(3, Util::GetHashKey("PLAYER"), relationshipGroup);
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(handle, relationshipGroup);
}

bool NamedPedType0::ShouldDelete()
{
	return NamedPed::ShouldDelete();
}

void NamedPedType0::Tick()
{
	NamedPed::Tick();

	if (!ENTITY::DOES_ENTITY_EXIST(handle))
		return;

	auto plPed = PLAYER::PLAYER_PED_ID();
	auto plPos = ENTITY::GET_ENTITY_COORDS(plPed, true);
	auto pedPos = ENTITY::GET_ENTITY_COORDS(handle, true);

	auto vx = plPos.x - pedPos.x;
	auto vy = plPos.y - pedPos.y;
	auto vz = plPos.z - pedPos.z;

	auto distSq = vx * vx + vy * vy + vz * vz;

	auto now = Util::Now();

	if (!LoadAnimDict(animDict))
		return;

	if (distSq < 60.0f)
	{
		if (!isNextToPlayer)
		{
			TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle);
			isNextToPlayer = true;
		}

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

		if (!ENTITY::IS_ENTITY_PLAYING_ANIM(handle, animDict.c_str(), animName.c_str(), 3))
			TASK::TASK_PLAY_ANIM(handle, animDict.c_str(), animName.c_str(), 8.0f, -8.0f, -1, 1, 0.0f, 0, 0, 0);
	}
	else if (isNextToPlayer && 150.0f < distSq)
	{
		TASK::CLEAR_PED_TASKS(handle);

		isNextToPlayer = false;
	}
}
