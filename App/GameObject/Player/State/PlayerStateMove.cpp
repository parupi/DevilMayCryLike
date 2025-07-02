#include "PlayerStateMove.h"
#include <GameObject/Player/Player.h>
#include <numbers>

void PlayerStateMove::Enter(Player& player)
{
	player.GetAcceleration().y = 0.0f;
}

void PlayerStateMove::Update(Player& player)
{
	player.Move();
	player.GetVelocity().y = 0.0f;

	// 地面についていなければ空中状態へ
	if (!player.GetOnGround()) {
		player.ChangeState("Air");
		return;
	}

	// キー入力が無かったら待機状態へ
	if (!Input::GetInstance()->PushKey(DIK_W) && !Input::GetInstance()->PushKey(DIK_A) && !Input::GetInstance()->PushKey(DIK_S) && !Input::GetInstance()->PushKey(DIK_D)) {
		player.ChangeState("Idle");
		return;
	}
	// スペース入力でジャンプ
	if (Input::GetInstance()->PushKey(DIK_SPACE)) {
		player.ChangeState("Jump");
		return;
	}
	// 攻撃のトリガー
	if (Input::GetInstance()->TriggerKey(DIK_J)) {
		player.ChangeState("AttackComboA1");
		return;
	}
	// 攻撃のトリガー
	if (Input::GetInstance()->TriggerKey(DIK_H)) {
		player.ChangeState("AttackHighTime");
		return;
	}

}

void PlayerStateMove::Exit(Player& player)
{
	player.GetVelocity().x = 0.0f;
	player.GetVelocity().z = 0.0f;
}
