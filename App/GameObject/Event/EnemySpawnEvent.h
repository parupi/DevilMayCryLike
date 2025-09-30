#pragma once
#include "BaseEvent.h"
#include <GameObject/Enemy/Enemy.h>
class EnemySpawnEvent : public BaseEvent
{
public:
	EnemySpawnEvent(std::string objectName);
	virtual ~EnemySpawnEvent() override = default;

	void AddEnemy(Enemy* enemy);

	void Execute() override;

	virtual void Initialize() override;

	virtual void Update() override;

	void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;

private:
	std::vector<Enemy*> enemies_;

	int skipFrames_ = 30;  // 最初の何フレーム処理をスキップするか
	int currentFrame_ = 0;
};

