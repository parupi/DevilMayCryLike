#include "PlayerStateAir.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Player/Controller/PlayerInput.h"

void PlayerStateAir::Enter(Player& player)
{
	player.GetAcceleration().y = -12.0f;

}

void PlayerStateAir::Update(Player& player, float deltaTime)
{
	// 空中でも移動可
	player.Move();

	// 地面についたら待機状態にする
	if (player.GetOnGround()) {
 		player.ChangeState("Idle");
		return;
	}
}

void PlayerStateAir::Exit(Player& player)
{
	player.GetVelocity() = { 0.0f, 0.0f, 0.0f };
}

void PlayerStateAir::ExecuteCommand(Player& player, const PlayerCommand& command)
{
	if (command.action == PlayerAction::Attack) {
		player.RequestAttack(AttackType::Air);
		return;
	}
}
