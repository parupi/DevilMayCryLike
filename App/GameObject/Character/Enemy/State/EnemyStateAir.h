#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"
class EnemyStateAir : public EnemyStateBase
{
public:
	EnemyStateAir() = default;
	~EnemyStateAir() override = default;
	void Enter(Enemy& enemy) override;
	void Update(Enemy& enemy, float deltaTime) override;
	void Exit(Enemy& enemy) override;

	// 落下の処理。行動を止められていても着地まで走らせる
	bool IsReaction() const override { return true; }
};

