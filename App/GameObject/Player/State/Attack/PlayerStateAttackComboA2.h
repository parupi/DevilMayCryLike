#pragma once
#include "PlayerStateAttackBase.h"
class PlayerStateAttackComboA2 : public PlayerStateAttackBase
{
public:
	PlayerStateAttackComboA2(std::string attackName);
	~PlayerStateAttackComboA2() override = default;
	void Enter(Player& player) override;
	void Update(Player& player) override;
	void Exit(Player& player) override;
};

