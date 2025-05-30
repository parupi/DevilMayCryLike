#pragma once
#include "PlayerStateBase.h"
#include <GameObject/Player/Player.h>
class PlayerStateAttack1 : public PlayerStateBase
{
public:
	PlayerStateAttack1() = default;
	~PlayerStateAttack1() override = default;
	void Enter(Player& player) override;
	void Update(Player& player) override;
	void Exit(Player& player) override;
private:
	TimeData stateTime_;

	Vector3 targetRot_;
	Vector3 startRot_{};

	enum class AttackPhase {
		Raise,
		Wait,
		Swing,
		End
	}attackPhase_;

	const float raiseDuration = 0.25f;
	const float waitDuration = 0.3f;
	const float swingDuration = 0.15f;
};

