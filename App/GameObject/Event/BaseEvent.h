#pragma once
#include "3d/Object/Object3d.h"

// イベントの種類を識別するための列挙
enum class EventType {
	EnemySpawn,
	
};

class BaseEvent : public Object3d
{
public:
	BaseEvent(std::string objectName, EventType type);
    virtual ~BaseEvent() = default;

    // イベントを発動する処理
    virtual void Execute() = 0;

    // トリガー条件を判定する
    //virtual bool CheckTrigger() = 0;

    // イベントが発動済みか
    bool IsTriggered() const { return isTriggered_; }

    // イベント名取得
    const std::string& GetName() const { return name_; }

    // イベント種別
    EventType GetType() const { return type_; }
protected:
	std::string name_;
	EventType type_;
	bool isTriggered_ = false;
};

