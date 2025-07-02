#pragma once
#include "PlayerStateAttackBase.h"
class PlayerStateAttackHighTime : public PlayerStateAttackBase
{
public:
	PlayerStateAttackHighTime(std::string attackName);
	~PlayerStateAttackHighTime() override = default;
	void Enter(Player& player) override;
	void Update(Player& player) override;
	void Exit(Player& player) override;
};

