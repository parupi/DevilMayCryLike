#include "PlayerStateIdle.h"
#include "GameObject/Player/Player.h"

void PlayerStateIdle::Enter(Player& player)
{
	player.GetAcceleration().y = 0.0f;
	player.GetVelocity().y = 0.0f;
}

void PlayerStateIdle::Update(Player& player)
{
	if (Input::GetInstance()->PushKey(DIK_W) || Input::GetInstance()->PushKey(DIK_A) || Input::GetInstance()->PushKey(DIK_S) || Input::GetInstance()->PushKey(DIK_D)) {
		player.ChangeState("Move");
	}

	// 攻撃のトリガー
	if (Input::GetInstance()->TriggerKey(DIK_J)) {
		player.ChangeState("Attack1");
	}

	// スペース入力でジャンプ
	if (Input::GetInstance()->PushKey(DIK_SPACE)) {
		player.ChangeState("Jump");
		return;
	}

	// 地面についていなければ空中状態へ
	if (!player.GetOnGround()) {
		player.ChangeState("Air");
		return;
	}
}

void PlayerStateIdle::Exit(Player& player)
{
}
