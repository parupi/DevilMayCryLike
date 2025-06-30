#pragma once
#include "GameObject/Enemy/EnemyStateBase.h"
#include "Object3d.h"
class EnemyStateMove : public EnemyStateBase
{
public:
	EnemyStateMove() = default;
	~EnemyStateMove() override = default;
	void Enter(Enemy& enemy) override;
	void Update(Enemy& enemy) override;
	void Exit(Enemy& enemy) override;

private:
	TimeData stateTime_;
};

