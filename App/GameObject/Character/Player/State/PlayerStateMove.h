#pragma once
#include "PlayerStateBase.h"
class PlayerStateMove : public PlayerStateBase
{
public:
	PlayerStateMove() = default;
	~PlayerStateMove() override = default;
	void Enter(Player& player) override;
	void Update(Player& player, float deltaTime) override;
	void Exit(Player& player) override;

private:
	void Move(Player& player);
};

