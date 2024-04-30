#include "script.h"

#include "localServer.h"
#include "rewards.h"
#include "util.h"
#include "memory.h"
#include "game.h"
#include "entityIterator.h"
#include "inc/main.h"
#include "inc/natives.h"
#include "inc/nativeCaller.h"

void ScriptMain()
{
	Memory::Init();

	WAIT(10000);

	LocalServer::Start();

	while (true)
	{
		try
		{
			LocalServer::Tick();
			Rewards::Tick();
			Game::Tick();
		}
		catch (...)
		{
			Game::ShowNotification("Twtich NPC Spawner: Internal error occurred");
		}

		WAIT(0);
	}
}
