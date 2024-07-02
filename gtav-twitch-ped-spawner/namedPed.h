#pragma once

#include <string>
#include <set>

#include "inc/natives.h"

enum ENicknameVehicleRender
{
	NEVER, FIVESEC, ALWAYS
};

class NamedPed
{
private:
	int lastTimeOnFoot = 0;
	ENicknameVehicleRender vehRender;

	float laggedTextX = 0.0f;
	float laggedTextY = 0.0f;
	float laggedTextZ = 0.0f;

protected:
	Ped handle;
	std::string viewerId;
	std::string nickname;
	float modelHeight;

	std::set<std::string> requestedDicts;
	bool LoadAnimDict(std::string dict);

public:
	NamedPed(Ped handle, std::string viewerId, std::string nickname, ENicknameVehicleRender vehRender);

	std::string GetViewerId() { return viewerId; }

	Ped GetHandle() { return handle; }
	std::string GetNickname() { return nickname; }

	virtual bool ShouldDelete();

	virtual void Tick();

	virtual ~NamedPed();
};
