#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"
#include "World3D/Object/Object3d.h"
class EnemyStateMove : public EnemyStateBase
{
public:
	EnemyStateMove() = default;
	~EnemyStateMove() override = default;
	void Enter(Enemy& enemy) override;
	void Update(Enemy& enemy, float deltaTime) override;
	void Exit(Enemy& enemy) override;

private:
	TimeData stateTime_;
};

