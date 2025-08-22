#pragma once
#include <memory>
#include "BaseEvent.h"
#include <map>
class EventManager
{
private:
	static EventManager* instance;
	static std::once_flag initInstanceFlag;

	EventManager() = default;
	~EventManager() = default;
	EventManager(EventManager&) = default;
	EventManager& operator=(EventManager&) = default;
public:
	static EventManager* GetInstance();

	void AddEvent(BaseEvent* event);

	BaseEvent* FindEvent(std::string eventName);

private:
	std::map<std::string, BaseEvent*> events_;
};

