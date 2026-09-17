#pragma once
#include "World3D/Object/Object3d.h"
#include <string>
#include <vector>

/// <summary>
/// イベントの種類を識別する列挙体
/// </summary>
enum class EventType {
	EnemySpawn,	// 敵出現イベント
	Clear,		// ステージクリアイベント
	ForceBattle,	// 強制戦闘イベント（エリアに閉じ込めて敵を出現させる）
	BossSpawn	// ボス出現イベント（カメラでボスをアップにしてゆっくりディゾルブ出現）
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
	virtual ~BaseEvent();

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
	const std::string& GetName() const { return Object3d::name_; }

	/// <summary>
	/// イベントの種類を取得
	/// </summary>
	EventType GetType() const { return type_; }

	/// <summary>
	/// このイベントが対象にするオブジェクト名（出現させる敵・撃破対象など）。
	/// 実行中の状態（撃破されて消えたか等）に左右されない「ステージに書かれた値」で、
	/// SceneBuilder が設定し SceneSaver が読む。
	/// </summary>
	void SetTargetNames(std::vector<std::string> names) { targetNames_ = std::move(names); }
	const std::vector<std::string>& GetTargetNames() const { return targetNames_; }
	// エディタが直接編集する用。書き換えても実行中のイベントには効かない（次のシーン読み込みから）
	std::vector<std::string>& GetTargetNamesRef() { return targetNames_; }

protected:
	EventType type_;
	bool isTriggered_ = false;

private:
	// ステージデータに書き戻すための対象名。実行中のロジックはここを見ない
	std::vector<std::string> targetNames_;
};
