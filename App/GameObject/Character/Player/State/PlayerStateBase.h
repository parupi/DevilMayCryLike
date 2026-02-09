#pragma once
#include <string>
#include <memory>

class Player;
struct PlayerCommand;
class PlayerStateBase {
public:
	virtual ~PlayerStateBase() = default;
	virtual void Enter(Player& player) = 0;
	virtual void Update(Player& player, float deltaTime) = 0;
	virtual void Exit(Player& player) = 0;
	virtual void ExecuteCommand(Player& player, const PlayerCommand& command) = 0;
	virtual const char* GetDebugName() const = 0;
};

