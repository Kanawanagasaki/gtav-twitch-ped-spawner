#pragma once

#include "namedPed.h"

class NamedPedType2 : public NamedPed
{
private:

	int lastTimeCompanionUpdate = 0;

public:
	NamedPedType2(Ped handle, std::string viewerId, std::string nickname);

	bool ShouldDelete() override;

	void Tick() override;
};
