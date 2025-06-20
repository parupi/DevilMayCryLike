#include "PlayerStateAir.h"
#include <GameObject/Player/Player.h>

void PlayerStateAir::Enter(Player& player)
{
	player.GetAcceleration().y = -12.0f;

}

void PlayerStateAir::Update(Player& player)
{
	player.Move();
	// 地面についたら待機状態にする
	if (player.GetOnGround()) {
 		player.ChangeState("Idle");
		return;
	}
}

void PlayerStateAir::Exit(Player& player)
{
}
