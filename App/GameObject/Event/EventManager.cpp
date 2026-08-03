#include "EventManager.h"
#include <iostream>

EventManager& EventManager::GetInstance()
{
	static EventManager instance;
	return instance;
}

void EventManager::Finalize()
{
    // 既存のイベントを削除
    events_.clear();

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

void EventManager::RemoveEvent(BaseEvent* event)
{
    if (!event) return;

    // 同名で上書きされている可能性があるので、ポインタが一致するときだけ消す
    auto it = events_.find(event->GetName());
    if (it != events_.end() && it->second == event) {
        events_.erase(it);
    }
}

BaseEvent* EventManager::FindEvent(std::string eventName)
{
	return events_[eventName];
}
