#pragma once
#include "PlayerStateAttackBase.h"
class PlayerStateAttackComboA3 : public PlayerStateAttackBase
{
public:
	PlayerStateAttackComboA3(std::string attackName);
	~PlayerStateAttackComboA3() override = default;
	void Enter(Player& player) override;
	void Update(Player& player) override;
	void Exit(Player& player) override;
};

