#pragma once
#include "BaseEvent.h"
#include <GameObject/Character/Enemy/Enemy.h>

/// <summary>
/// 全ての敵を撃破した際に発動するクリアイベント
/// </summary>
class ClearEvent : public BaseEvent
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	ClearEvent(std::string objectName);

	/// <summary>
	/// デストラクタ
	/// </summary>
	virtual ~ClearEvent() override = default;

	/// <summary>
	/// 更新処理  
	/// 対象の敵が全滅したかどうかを監視する
	/// </summary>
	void Update(float deltaTime) override;

	/// <summary>
	/// イベントを発動する処理
	/// </summary>
	void Execute() override;

	/// <summary>
	/// 対象となる敵を追加
	/// </summary>
	void AddTargetEnemy(Enemy* enemy);

	/// <summary>
	/// クリア済みか確認
	/// </summary>
	bool IsClear() const { return isClear_; }

private:
	std::vector<Enemy*> targetEnemies_;

	bool isClear_ = false;

	int skipFrames_ = 30;
	int currentFrame_ = 0;
};
