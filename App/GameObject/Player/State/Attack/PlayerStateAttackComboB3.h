#pragma once
#include "PlayerStateAttackBase.h"
class PlayerStateAttackComboB3 : public PlayerStateAttackBase
{
public:
	PlayerStateAttackComboB3(std::string attackName);
	~PlayerStateAttackComboB3() override = default;
	void Enter(Player& player) override;
	void Update(Player& player) override;
	void Exit(Player& player) override;
};

