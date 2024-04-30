#pragma once

#include "namedPed.h"

class NamedPedType2 : public NamedPed
{
private:

public:
	NamedPedType2(Ped handle, std::string viewerId, std::string nickname);

	bool ShouldDelete() override;

	void Tick() override;
};
