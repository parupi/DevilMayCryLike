#include "EventManager.h"
#include <iostream>

EventManager* EventManager::instance = nullptr;
std::once_flag EventManager::initInstanceFlag;

EventManager* EventManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new EventManager();
		});
	return instance;
}

void EventManager::AddEvent(BaseEvent* event)
{
    if (!event) return; // nullptr チェック

    const std::string& name = event->GetName(); // BaseEvent で GetName() が必要

    // すでに同じ名前のイベントがある場合は上書きするか、無視するか
    auto result = events_.emplace(name, std::move(event));
    if (!result.second) {
        std::cout << "Warning: Event with name '" << name << "' already exists. Overwriting.\n";
        events_[name] = event;
    }
}

BaseEvent* EventManager::FindEvent(std::string eventName)
{
	return events_[eventName];
}
