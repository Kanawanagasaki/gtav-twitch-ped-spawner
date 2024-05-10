#include "namedPed.h"
#include "game.h"
#include "util.h"

#include <algorithm>

NamedPed::NamedPed(Ped handle, std::string viewerId, std::string nickname)
{
	this->handle = handle;
	this->viewerId = viewerId;
	this->nickname = nickname;

	auto model = ENTITY::GET_ENTITY_MODEL(handle);
	Vector3 min;
	Vector3 max;
	MISC::GET_MODEL_DIMENSIONS(model, &min, &max);

	modelHeight = max.z;
}

bool NamedPed::LoadAnimDict(std::string dict)
{
	if (STREAMING::HAS_ANIM_DICT_LOADED(dict.c_str()))
	{
		if (requestedDicts.contains(dict))
			requestedDicts.erase(dict);
		return true;
	}

	if (!requestedDicts.contains(dict))
	{
		STREAMING::REQUEST_ANIM_DICT(dict.c_str());
		requestedDicts.insert(dict);
	}

	return false;
}

bool NamedPed::ShouldDelete()
{
	return !ENTITY::DOES_ENTITY_EXIST(handle) || ENTITY::IS_ENTITY_DEAD(handle, false);
}

void NamedPed::Tick()
{
	if (!ENTITY::DOES_ENTITY_EXIST(handle))
		return;
	if (PED::IS_PED_IN_ANY_VEHICLE(handle, true))
		return;

	auto cameraPos = CAM::GET_FINAL_RENDERED_CAM_COORD();
	auto pedPos = ENTITY::GET_ENTITY_COORDS(handle, true);

	auto vx = cameraPos.x - pedPos.x;
	auto vy = cameraPos.y - pedPos.y;
	auto vz = cameraPos.z - pedPos.z;

	auto distSq = vx * vx + vy * vy + vz * vz;

	if (distSq < 400.0f)
	{
		auto distPercent = std::clamp((400.0f - distSq) / 400.0f, 0.0f, 1.0f);

		float worldZ = pedPos.z + modelHeight + 0.45f - 0.15f * distPercent;
		float screenX;
		float screenY;
		if (GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(pedPos.x, pedPos.y, worldZ, &screenX, &screenY))
		{
			auto camPos = CAM::GET_GAMEPLAY_CAM_COORD();

			vx = camPos.x - pedPos.x;
			vy = camPos.y - pedPos.y;
			vz = camPos.z - pedPos.z;

			distSq = vx * vx + vy * vy + vz * vz;

			auto opacity = std::clamp((100.0f - (distSq - 300.0f)) / 100.0f, 0.0f, 1.0f);
			auto scale = 0.175f + distPercent * 0.075f;

			HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
			HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(nickname.c_str());

			HUD::SET_TEXT_SCALE(scale, scale);
			HUD::SET_TEXT_COLOUR(255, 255, 255, std::clamp(static_cast<int>(opacity * 255.0f), 0, 255));

			HUD::SET_TEXT_OUTLINE();

			HUD::SET_TEXT_JUSTIFICATION(0);

			HUD::END_TEXT_COMMAND_DISPLAY_TEXT(screenX, screenY, 0);
		}
	}
}

NamedPed::~NamedPed()
{
	if (ENTITY::DOES_ENTITY_EXIST(handle))
		ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&handle);
}
