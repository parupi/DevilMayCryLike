#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"
class HellkainaStateSideMove : public EnemyStateBase
{
public:
	HellkainaStateSideMove() = default;
	~HellkainaStateSideMove() override = default;
	void Enter(Enemy& enemy) override;
	void Update(Enemy& enemy) override;
	void Exit(Enemy& enemy) override;

private:
	float sideDir_;
	float timer_;
	float time_ = 1.0f;
	float moveSpeed_ = 0.03f;
};

