#pragma once
#include "PlayerStateAttackBase.h"
#include <GameObject/Player/Player.h>
#include <math/function.h>
class PlayerStateAttackComboA1 : public PlayerStateAttackBase
{
public:
	PlayerStateAttackComboA1(std::string attackName);
	~PlayerStateAttackComboA1() override = default;
	void Enter(Player& player) override;
	void Update(Player& player) override;
	void Exit(Player& player) override;
private:
	float cancelTime_ = 0.0f;
};

