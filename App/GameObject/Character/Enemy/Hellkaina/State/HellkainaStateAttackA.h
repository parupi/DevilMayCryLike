#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"
#include "3d/Object/Object3d.h"
class HellkainaStateAttackA : public EnemyStateBase
{
public:
	HellkainaStateAttackA() = default;
	~HellkainaStateAttackA() override = default;
	void Enter(Enemy& enemy) override;
	void Update(Enemy& enemy, float deltaTime) override;
	void Exit(Enemy& enemy) override;

private:
	float timer_ = 0.0f;
	float time_ = 0.25f;

	std::vector<Vector3> translate_;
	std::vector<Vector3> rotate_;
};

