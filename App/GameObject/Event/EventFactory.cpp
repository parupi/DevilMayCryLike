#include "EventFactory.h"
#include "EnemySpawnEvent.h"

std::unique_ptr<BaseEvent> EventFactory::Create(const std::string& className, const std::string& objectName)
{
	if (className == "Event_EnemySpawn") {
		return std::make_unique<EnemySpawnEvent>(objectName);
	} else {
		return nullptr;
	}
}
