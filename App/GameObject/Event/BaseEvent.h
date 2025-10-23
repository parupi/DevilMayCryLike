#pragma once
#include "3d/Object/Object3d.h"

/// <summary>
/// イベントの種類を識別する列挙体
/// </summary>
enum class EventType {
	EnemySpawn,	// 敵出現イベント
	Clear		// ステージクリアイベント
};

/// <summary>
/// すべてのイベントの基底クラス  
/// 共通の処理やインターフェースを定義する
/// </summary>
class BaseEvent : public Object3d
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="objectName">オブジェクト名</param>
	/// <param name="type">イベントの種類</param>
	BaseEvent(std::string objectName, EventType type);

	/// <summary>
	/// デストラクタ
	/// </summary>
	virtual ~BaseEvent() = default;

	/// <summary>
	/// イベントを発動する純粋仮想関数  
	/// 各派生クラスで具体的な動作を実装する
	/// </summary>
	virtual void Execute() = 0;

	/// <summary>
	/// イベントが発動済みか確認
	/// </summary>
	bool IsTriggered() const { return isTriggered_; }

	/// <summary>
	/// イベント名を取得
	/// </summary>
	const std::string& GetName() const { return name_; }

	/// <summary>
	/// イベントの種類を取得
	/// </summary>
	EventType GetType() const { return type_; }

protected:
	std::string name_;
	EventType type_;
	bool isTriggered_ = false;
};
