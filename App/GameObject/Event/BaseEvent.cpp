#include "BaseEvent.h"

BaseEvent::BaseEvent(std::string objectName, EventType type) : Object3d(objectName) {
	type_ = type;
}
