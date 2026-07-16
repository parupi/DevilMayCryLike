#include "EventFactory.h"
#include "EnemySpawnEvent.h"
#include "ClearEvent.h"
#include "ForceBattleEvent.h"
#include "BossSpawnEvent.h"

std::unique_ptr<BaseEvent> EventFactory::Create(const std::string& className, const std::string& objectName)
{
	if (className == "Event_EnemySpawn") {
		return std::make_unique<EnemySpawnEvent>(objectName);
	} else if (className == "Event_Clear") {
		return std::make_unique<ClearEvent>(objectName);
	} else if (className == "Event_ForceBattle") {
		return std::make_unique<ForceBattleEvent>(objectName);
	} else if (className == "Event_BossSpawn") {
		return std::make_unique<BossSpawnEvent>(objectName);
	} else {
		return nullptr;
	}
}
