#pragma once
#include "GameObject/Enemy/EnemyStateBase.h"
class EnemyStateAir : public EnemyStateBase
{
public:
	EnemyStateAir() = default;
	~EnemyStateAir() override = default;
	void Enter(Enemy& enemy) override;
	void Update(Enemy& enemy) override;
	void Exit(Enemy& enemy) override;
};

