#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateKnockBack.h"
#include "3d/Object/Object3d.h"
#include "GameObject/Character/CharacterStructs.h"

class EnemyStateKnockBack : public EnemyStateBase
{
public:
	EnemyStateKnockBack() = default;
	~EnemyStateKnockBack() override = default;
	void Enter(Enemy& enemy) override;
	void Update(Enemy& enemy, float deltaTime) override;
	void Exit(Enemy& enemy) override;

private:
	void OnLand(Enemy& enemy);

	TimeData stateTime_;
	ReactionType currentType_;
	Vector3 velocity_;

	float stunTimer_ = 0.0f;
	float tiltAmount_ = 0.0f;
	float angularVel_ = 0.0f;

	float currentTilt_ = 0.0f;
	float targetTilt_ = 0.0f;
};
