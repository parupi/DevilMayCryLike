#pragma once
#include "BaseEvent.h"
#include <GameObject/Enemy/Enemy.h>
class ClearEvent : public BaseEvent
{
public:
	ClearEvent(std::string objectName);
	virtual ~ClearEvent() override = default;

	void Update() override;
	// イベントを発動する処理
	void Execute() override;

	void AddTargetEnemy(Enemy* enemy);

private:

	std::vector<Enemy*> targetEnemies_;
	bool isClear_ = false;

	int skipFrames_ = 30;  // 最初の何フレーム処理をスキップするか
	int currentFrame_ = 0;
};

