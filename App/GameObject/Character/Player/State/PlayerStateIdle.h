#pragma once 
#include "PlayerStateBase.h"
class PlayerStateIdle : public PlayerStateBase
{
public:
	PlayerStateIdle() = default;
	~PlayerStateIdle() override = default;
	void Enter(Player& player) override;
	void Update(Player& player, float deltaTime) override;
	void Exit(Player& player) override;
};

