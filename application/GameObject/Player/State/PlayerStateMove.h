#pragma once
#include "PlayerStateBase.h"
class PlayerStateMove : public PlayerStateBase
{
public:
	PlayerStateMove() = default;
	~PlayerStateMove() override = default;
	void Enter(Player& player) override;
	void Update(Player& player) override;
	void Exit(Player& player) override;
};

