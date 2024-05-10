#pragma once

#include "game.h"
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

	static bool TryCreate(Game::Redemption* redemption, NamedPed** res);
};
