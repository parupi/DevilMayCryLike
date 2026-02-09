#pragma once
#include "PlayerStateBase.h"
class PlayerStateJump : public PlayerStateBase
{
public:
	PlayerStateJump() = default;
	~PlayerStateJump() override = default;
	void Enter(Player& player) override;
	void Update(Player& player, float deltaTime) override;
	void Exit(Player& player) override;
	void ExecuteCommand(Player& player, const PlayerCommand& command) override;
	const char* GetDebugName() const override { return "Jump"; };
};

