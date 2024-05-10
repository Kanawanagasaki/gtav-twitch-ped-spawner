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
	auto spawnType0Hash = Util::GetHashKey("spawn-named-ped-type-0");
	auto spawnType1Hash = Util::GetHashKey("spawn-named-ped-type-1");
	auto spawnType2Hash = Util::GetHashKey("spawn-named-ped-type-2");

	Memory::Init();

	WAIT(10000);

	LocalServer::Start();

	while (true)
	{
		try
		{
			if (MISC::HAS_PC_CHEAT_WITH_HASH_BEEN_ACTIVATED(spawnType0Hash))
			{
				auto redemption = new Game::Redemption
				{
					.id = std::to_string(rand()),
					.userId = std::to_string(rand()),
					.userLogin = std::to_string(rand()),
					.userName = std::to_string(rand()),
					.userInput = std::to_string(rand()),
					.rewardType = 0
				};

				Game::Process(redemption);
			}
			else if (MISC::HAS_PC_CHEAT_WITH_HASH_BEEN_ACTIVATED(spawnType1Hash))
			{
				auto redemption = new Game::Redemption
				{
					.id = std::to_string(rand()),
					.userId = std::to_string(rand()),
					.userLogin = std::to_string(rand()),
					.userName = std::to_string(rand()),
					.userInput = std::to_string(rand()),
					.rewardType = 1
				};

				Game::Process(redemption);
			}
			else if (MISC::HAS_PC_CHEAT_WITH_HASH_BEEN_ACTIVATED(spawnType2Hash))
			{
				auto redemption = new Game::Redemption
				{
					.id = std::to_string(rand()),
					.userId = std::to_string(rand()),
					.userLogin = std::to_string(rand()),
					.userName = std::to_string(rand()),
					.userInput = std::to_string(rand()),
					.rewardType = 2
				};

				Game::Process(redemption);
			}

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
