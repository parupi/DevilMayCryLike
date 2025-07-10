#pragma once
#include "PlayerStateBase.h"
class PlayerStateAir : public PlayerStateBase
{
public:
	PlayerStateAir() = default;
	~PlayerStateAir() override = default;
	void Enter(Player& player) override;
	void Update(Player& player) override;
	void Exit(Player& player) override;
};

