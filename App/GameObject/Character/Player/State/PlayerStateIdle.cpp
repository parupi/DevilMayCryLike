#include "PlayerStateIdle.h"
#include "GameObject/Character/Player/Player.h"

void PlayerStateIdle::Enter(Player& player)
{
	player.GetAcceleration().y = 0.0f;
	player.GetVelocity().y = 0.0f;
}

void PlayerStateIdle::Update(Player& player)
{
	Input* input = Input::GetInstance();
	if (input->IsConnected()) {
		if (input->GetLeftStickX() != 0.0f || input->GetLeftStickY() != 0.0f) {
			player.ChangeState("Move");
		}

		if (player.IsLockOn()) {
			if (input->TriggerButton(PadNumber::ButtonY) && input->GetLeftStickY() < 0.0f) {
				player.ChangeState("AttackHighTime");
				return;
			}
		}

		// 攻撃のトリガー
		if (input->TriggerButton(PadNumber::ButtonY)) {
			player.ChangeState("AttackComboA1");
			return;
		}

		// スペース入力でジャンプ
		if (input->TriggerButton(PadNumber::ButtonA)) {
			player.ChangeState("Jump");
			return;
		}

	} else {
		if (Input::GetInstance()->PushKey(DIK_W) || Input::GetInstance()->PushKey(DIK_A) || Input::GetInstance()->PushKey(DIK_S) || Input::GetInstance()->PushKey(DIK_D)) {
			player.ChangeState("Move");
		}

		// 攻撃のトリガー
		if (Input::GetInstance()->TriggerKey(DIK_J)) {
			//player.ChangeState("AttackComboA1");
			player.RequestAttack(AttackType::Normal);
			return;
		}
		// 攻撃のトリガー
		if (Input::GetInstance()->TriggerKey(DIK_H)) {
			//player.ChangeState("AttackHighTime");
			player.RequestAttack(AttackType::RoundUp);
			return;
		}

		// スペース入力でジャンプ
		if (Input::GetInstance()->PushKey(DIK_SPACE)) {
			player.ChangeState("Jump");
			return;
		}
	}


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
