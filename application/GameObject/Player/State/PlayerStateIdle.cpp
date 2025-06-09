#include "PlayerStateIdle.h"
#include "GameObject/Player/Player.h"

void PlayerStateIdle::Enter(Player& player)
{
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
}

void PlayerStateIdle::Exit(Player& player)
{
}
