#pragma once
#include "BaseEvent.h"
#include <GameObject/Character/Enemy/Enemy.h>
#include <Math/Vector3.h>
#include <vector>

class Player;

/// <summary>
/// 強制戦闘イベント。
/// プレイヤーがエリア（コライダー）に進入すると発動し、
///  ・関連付けられた敵を出現させる
///  ・全滅するまでプレイヤーをエリア範囲内(XZ)に閉じ込める（範囲外へ出られない）
/// 全ての対象敵を撃破するとロックが解除される。
///
/// レベルエディタからは "Event_ForceBattle" として配置する。
/// エリア兼トリガーとして BOX コライダーが必須。
/// </summary>
class ForceBattleEvent : public BaseEvent {
public:
	ForceBattleEvent(std::string objectName);
	virtual ~ForceBattleEvent() override = default;

	void Initialize() override;
	void Update(float deltaTime) override;

	/// <summary>イベント発動：敵の出現＋プレイヤーの閉じ込め</summary>
	void Execute() override;

	/// <summary>プレイヤー進入で発動する（トリガー）</summary>
	void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;

	/// <summary>出現＆撃破監視の対象となる敵を追加</summary>
	void AddEnemy(Enemy* enemy);

private:
	/// <summary>全滅時にプレイヤーの移動制限を解除する</summary>
	void EndBattle();

	/// <summary>全ての対象敵が撃破されたか</summary>
	bool AreAllEnemiesDefeated() const;

	// 敵は死亡後 Object3dManager から削除されるため、ポインタではなく名前で保持し
	// 毎回 FindObject で解決する（削除済み＝撃破済みとみなす）
	std::vector<std::string> enemyNames_;
	Player* player_ = nullptr;

	bool isBattleActive_ = false;
	bool isFinished_ = false;

	int skipFrames_ = 30;
	int currentFrame_ = 0;
};
