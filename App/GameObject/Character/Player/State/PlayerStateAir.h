#pragma once
#include "PlayerStateBase.h"
class PlayerStateAir : public PlayerStateBase
{
public:
	PlayerStateAir() = default;
	~PlayerStateAir() override = default;
	void Enter(Player& player) override;
	void Update(Player& player, float deltaTime) override;
	void Exit(Player& player) override;
	void ExecuteCommand(Player& player, const PlayerCommand& command) override;
	const char* GetDebugName() const override { return "Air"; };
};

