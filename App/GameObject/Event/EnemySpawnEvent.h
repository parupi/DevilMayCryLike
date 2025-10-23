#pragma once
#include "BaseEvent.h"
#include <GameObject/Enemy/Enemy.h>

/// <summary>
/// 敵を出現させるイベントクラス  
/// 特定条件下で敵の生成を行う
/// </summary>
class EnemySpawnEvent : public BaseEvent
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	EnemySpawnEvent(std::string objectName);

	/// <summary>
	/// デストラクタ
	/// </summary>
	virtual ~EnemySpawnEvent() override = default;

	/// <summary>
	/// 出現させる敵を追加
	/// </summary>
	void AddEnemy(Enemy* enemy);

	/// <summary>
	/// イベントを発動する処理（敵の生成）
	/// </summary>
	void Execute() override;

	/// <summary>
	/// 初期化処理
	/// </summary>
	virtual void Initialize() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	virtual void Update() override;

	/// <summary>
	/// 当たり判定に入った際の処理（発動条件）
	/// </summary>
	void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;

private:
	std::vector<Enemy*> enemies_;

	int skipFrames_ = 30;
	int currentFrame_ = 0;
};
