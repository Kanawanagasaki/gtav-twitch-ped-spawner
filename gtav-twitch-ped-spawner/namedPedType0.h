#pragma once

#include "namedPed.h"

class NamedPedType0 : public NamedPed
{
private:
	bool isNextToPlayer = false;
	bool isDictRequested = false;
	std::string animDict;
	std::string animName;

public:
	NamedPedType0(Ped handle, std::string viewerId, std::string nickname);

	bool ShouldDelete() override;

	void Tick() override;
};
