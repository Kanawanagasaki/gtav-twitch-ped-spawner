#pragma once

#include <string>
#include <set>

#include "inc/natives.h"

class NamedPed
{
protected:
	Ped handle;
	std::string viewerId;
	std::string nickname;
	float modelHeight;

	std::set<std::string> requestedDicts;
	bool LoadAnimDict(std::string dict);

public:
	NamedPed(Ped handle, std::string viewerId, std::string nickname);

	std::string GetViewerId() { return viewerId; }

	Ped GetHandle() { return handle; }
	std::string GetNickname() { return nickname; }

	virtual bool ShouldDelete();

	virtual void Tick();

	~NamedPed();
};
