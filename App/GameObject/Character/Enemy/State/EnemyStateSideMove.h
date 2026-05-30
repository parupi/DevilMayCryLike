#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"
class EnemyStateSideMove : public EnemyStateBase
{
public:
	EnemyStateSideMove() = default;
	~EnemyStateSideMove() override = default;
	void Enter(Enemy& enemy) override;
	void Update(Enemy& enemy, float deltaTime) override;
	void Exit(Enemy& enemy) override;

private:
	float sideDir_;
	float timer_;
	float time_ = 1.0f;
	float moveSpeed_ = 0.03f;
};

