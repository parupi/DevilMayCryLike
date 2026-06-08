#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyStateIdle : public EnemyStateBase
{
public:
	EnemyStateIdle() = default;
	~EnemyStateIdle() override = default;
	void Enter(Enemy& enemy) override;
	void Update(Enemy& enemy, float deltaTime) override;
	void Exit(Enemy& enemy) override;

private:
	bool isAttackCooldown_ = false;
	float cooldownTimer_ = 0.0f;
};
