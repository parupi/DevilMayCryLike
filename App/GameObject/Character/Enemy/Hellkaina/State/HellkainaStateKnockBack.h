#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateKnockBack.h"
#include "3d/Object/Object3d.h"
class HellkainaStateKnockBack : public EnemyStateKnockBack
{
public:
	HellkainaStateKnockBack() = default;
	~HellkainaStateKnockBack() override = default;
	void Enter(const DamageInfo& info, Enemy& enemy) override;
	void Update(Enemy& enemy, float deltaTime) override;
	void Exit(Enemy& enemy) override;

	void OnLand(Enemy& enemy);
private:
	TimeData stateTime_;

	ReactionType currentType_;

	Vector3 velocity_;

	float stunTimer_;
	float tiltAmount_;
	float angularVel_;

	bool gravity_ = false;

	float currentTilt_ = 0.0f;
	float targetTilt_ = 0.0f;
};

