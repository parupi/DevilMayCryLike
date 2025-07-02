#pragma once
#include "PlayerStateAttackBase.h"
class PlayerStateAttackComboB2 : public PlayerStateAttackBase
{
public:
	PlayerStateAttackComboB2(std::string attackName);
	~PlayerStateAttackComboB2() override = default;
	void Enter(Player& player) override;
	void Update(Player& player) override;
	void Exit(Player& player) override;
};

