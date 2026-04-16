#include "PlayerStateMove.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Player/Controller/PlayerInput.h"
#include <numbers>

void PlayerStateMove::Enter(Player& player)
{
	player.GetAcceleration().y = 0.0f;
}

void PlayerStateMove::Update(Player& player, float deltaTime)
{
	player.Move(deltaTime);
	player.GetVelocity().y = 0.0f;

	// 現在の入力状態が無ければ待機
	if (!player.GetInput()->GetContext().isMove) {
		player.ChangeState("Idle");
		return;
	}

	// 地面についていなければ空中状態へ
	if (!player.GetOnGround()) {
		player.ChangeState("Air");
		return;
	}
}

void PlayerStateMove::Exit(Player& player)
{
	player.GetVelocity().x = 0.0f;
	player.GetVelocity().z = 0.0f;
}

void PlayerStateMove::ExecuteCommand(Player& player, const PlayerCommand& command)
{
	// ジャンプを要求されたら飛ぶ
	if (command.action == PlayerAction::Jump) {
		player.ChangeState("Jump");
		return;
	}
}