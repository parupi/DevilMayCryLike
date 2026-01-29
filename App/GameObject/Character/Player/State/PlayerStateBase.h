#pragma once
#include <string>
#include <memory>

class Player;
class PlayerStateBase {
public:
	virtual ~PlayerStateBase() = default;
	virtual void Enter(Player& player) = 0;
	virtual void Update(Player& player, float deltaTime) = 0;
	virtual void Exit(Player& player) = 0;
};

