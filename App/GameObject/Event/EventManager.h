#pragma once
#include <memory>
#include "BaseEvent.h"
#include <map>
class EventManager
{
private:
	static EventManager* instance;

	EventManager() = default;
	~EventManager() = default;
	EventManager(EventManager&) = default;
	EventManager& operator=(EventManager&) = default;
public:
	static EventManager* GetInstance();
	// 終了処理
	void Finalize();

	void AddEvent(BaseEvent* event);

	BaseEvent* FindEvent(std::string eventName);

private:
	std::map<std::string, BaseEvent*> events_;
};

