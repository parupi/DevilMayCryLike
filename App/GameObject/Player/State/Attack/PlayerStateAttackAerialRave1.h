#pragma once
#include "PlayerStateAttackBase.h"
class PlayerStateAttackAerialRave1 : public PlayerStateAttackBase
{
public:
	PlayerStateAttackAerialRave1(std::string attackName);
	~PlayerStateAttackAerialRave1() override = default;
	void Enter(Player& player) override;
	void Update(Player& player) override;
	void Exit(Player& player) override;
};

