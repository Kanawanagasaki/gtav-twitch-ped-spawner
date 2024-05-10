#pragma once

#include "namedPed.h"
#include "game.h"

class NamedPedType1 : public NamedPed
{
private:
	enum eState
	{
		Initial,
		Waving,
		CameraEnter,
		CameraIdle,
		CameraExit,
		Follow
	};

	eState state = eState::Initial;
	bool isFirstFollowTick = true;
	int stateChangeTime = 0;
	std::string waveAnimDict;
	std::string waveAnimName;
	bool isCameraModelRequested = false;
	Object camera;

	void SetState(eState s);

	void ProcessInitial();
	void ProcessWaving();
	void ProcessCameraEnter();
	void ProcessCameraIdle();
	void ProcessCameraExit();
	void ProcessFollow();

	static bool FindSpot(Vector3* spot);

public:
	NamedPedType1(Ped handle, std::string viewerId, std::string nickname);

	bool ShouldDelete() override;

	void Tick() override;

	~NamedPedType1() override;

	static bool TryCreate(Game::Redemption* redemption, NamedPed** res);
};
