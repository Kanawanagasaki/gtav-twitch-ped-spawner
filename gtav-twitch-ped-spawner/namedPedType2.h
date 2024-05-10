#pragma once

#include "namedPed.h"
#include "game.h"

class NamedPedType2 : public NamedPed
{
private:
	static Hash GetRelationshipGroup();
	static bool FindSpot(Vector3* spot);

	int lastGroupResetTime = 0;

public:
	NamedPedType2(Ped handle, std::string viewerId, std::string nickname);

	bool ShouldDelete() override;

	void Tick() override;

	~NamedPedType2() override;

	static bool TryCreate(Game::Redemption* redemption, NamedPed** res);
};
