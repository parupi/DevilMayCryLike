#include "PlayerStateIdle.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Player/Controller/PlayerInput.h"

void PlayerStateIdle::Enter(Player& player)
{
	player.GetAcceleration().y = 0.0f;
	player.GetVelocity().y = 0.0f;
}

void PlayerStateIdle::Update(Player& player, float deltaTime)
{
	player.GetAcceleration().y = 0.0f;
	player.GetVelocity().y = 0.0f;

	// 地面についていなければ空中状態へ
	if (!player.GetOnGround()) {
		player.ChangeState("Air");
		return;
	}
}

void PlayerStateIdle::Exit(Player& player)
{
	player;
}

void PlayerStateIdle::ExecuteCommand(Player& player, const PlayerCommand& command)
{
	if (command.action == PlayerAction::Move) {
		player.ChangeState("Move");
		return;
	}
	if (command.action == PlayerAction::Jump) {
 		player.ChangeState("Jump");
		return;
	}
}
