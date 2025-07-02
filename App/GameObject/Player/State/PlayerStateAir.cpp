#include "PlayerStateAir.h"
#include <GameObject/Player/Player.h>

void PlayerStateAir::Enter(Player& player)
{
	player.GetAcceleration().y = -12.0f;

}

void PlayerStateAir::Update(Player& player)
{
	// 空中でも移動可
	player.Move();
	// 空中攻撃
	if (Input::GetInstance()->TriggerKey(DIK_J)) {
		player.ChangeState("AttackAerialRave1");
		return;
	}

	// 地面についたら待機状態にする
	if (player.GetOnGround()) {
 		player.ChangeState("Idle");
		return;
	}
}

void PlayerStateAir::Exit(Player& player)
{
	player;
}
