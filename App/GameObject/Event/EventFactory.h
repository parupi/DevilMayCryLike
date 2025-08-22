#pragma once
#include <string>
#include "BaseEvent.h"

class EventFactory
{
public:
	static std::unique_ptr<BaseEvent> Create(const std::string& className, const std::string& objectName);
};

