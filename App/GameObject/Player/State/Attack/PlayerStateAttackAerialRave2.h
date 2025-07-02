#pragma once
#include "PlayerStateAttackBase.h"
class PlayerStateAttackAerialRave2 : public PlayerStateAttackBase
{
public:
	PlayerStateAttackAerialRave2(std::string attackName);
	~PlayerStateAttackAerialRave2() override = default;
	void Enter(Player& player) override;
	void Update(Player& player) override;
	void Exit(Player& player) override;
};

