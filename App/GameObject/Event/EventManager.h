#pragma once
#include <memory>
#include "BaseEvent.h"
#include <map>

/// <summary>
/// イベント全体を管理するクラス  
/// イベントの登録・検索・終了処理などを行う
/// </summary>
class EventManager
{
private:
	static EventManager* instance;

	EventManager() = default;
	~EventManager() = default;
	EventManager(EventManager&) = default;
	EventManager& operator=(EventManager&) = default;

public:
	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	static EventManager* GetInstance();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// イベントの登録
	/// </summary>
	void AddEvent(BaseEvent* event);

	/// <summary>
	/// イベント名からイベントを検索
	/// </summary>
	BaseEvent* FindEvent(std::string eventName);

private:
	std::map<std::string, BaseEvent*> events_;
};
