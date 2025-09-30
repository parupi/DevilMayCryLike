#include "EventFactory.h"
#include "EnemySpawnEvent.h"
#include "ClearEvent.h"

std::unique_ptr<BaseEvent> EventFactory::Create(const std::string& className, const std::string& objectName)
{
	if (className == "Event_EnemySpawn") {
		return std::make_unique<EnemySpawnEvent>(objectName);
	} else if (className == "Event_Clear") {
		return std::make_unique<ClearEvent>(objectName);
	}else {
		return nullptr;
	}
}
