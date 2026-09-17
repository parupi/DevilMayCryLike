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
	EventManager() = default;
	~EventManager() = default;
	EventManager(EventManager&) = delete;
	EventManager& operator=(EventManager&) = delete;

public:
	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	static EventManager& GetInstance();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// イベントの登録。BaseEvent のコンストラクタが自分で呼ぶ
	/// </summary>
	void AddEvent(BaseEvent* event);

	/// <summary>
	/// イベントの登録解除。BaseEvent のデストラクタが自分で呼ぶ
	/// </summary>
	void RemoveEvent(BaseEvent* event);

	/// <summary>
	/// イベント名からイベントを検索
	/// </summary>
	BaseEvent* FindEvent(std::string eventName);

	/// <summary>
	/// 種類からイベントを検索（最初に見つかったもの）。
	/// ステージ上のイベント名はエディタで自由に付けられるので、決まった役割のイベントはこちらで引く
	/// </summary>
	BaseEvent* FindEventByType(EventType type);

private:
	std::map<std::string, BaseEvent*> events_;
};
