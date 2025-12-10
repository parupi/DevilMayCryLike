#pragma once
#include "PlayerStateBase.h"
class PlayerStateJump : public PlayerStateBase
{
public:
	PlayerStateJump() = default;
	~PlayerStateJump() override = default;
	void Enter(Player& player) override;
	void Update(Player& player) override;
	void Exit(Player& player) override;
};

