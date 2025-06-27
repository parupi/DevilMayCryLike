#pragma once
#include "PlayerStateAttackBase.h"
#include <GameObject/Player/Player.h>
#include <math/function.h>
class PlayerStateAttack1 : public PlayerStateAttackBase
{
public:
	PlayerStateAttack1(std::string attackName);
	~PlayerStateAttack1() override = default;
	void Enter(Player& player) override;
	void Update(Player& player) override;
	void Exit(Player& player) override;
private:
	TimeData stateTime_;

	Vector3 targetRot_;
	Vector3 startRot_{};

	enum class AttackPhase {
		Startup, // 予備動作
		Active, // 攻撃中
		Recovery, // 硬直
	}attackPhase_;



	const float raiseDuration = 0.25f;
	const float waitDuration = 0.3f;
	const float swingDuration = 0.15f;
	
};

