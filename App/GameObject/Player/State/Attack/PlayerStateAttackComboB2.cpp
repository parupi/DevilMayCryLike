#include "PlayerStateAttackComboB2.h"
#include <input/Input.h>
#include "GameObject/Player/Player.h"
PlayerStateAttackComboB2::PlayerStateAttackComboB2(std::string attackName) : PlayerStateAttackBase(attackName)
{

}

void PlayerStateAttackComboB2::Enter(Player& player)
{
	PlayerStateAttackBase::Enter(player);
}

void PlayerStateAttackComboB2::Update(Player& player)
{
	PlayerStateAttackBase::Update(player);
	if (attackPhase_ == AttackPhase::Cancel) {
		if (Input::GetInstance()->TriggerKey(DIK_J) || Input::GetInstance()->TriggerButton(PadNumber::ButtonY)) {
			player.ChangeState("AttackComboB3");
		} 
		// 攻撃のトリガー
		if (Input::GetInstance()->TriggerKey(DIK_H)) {
			player.ChangeState("AttackHighTime");
			return;
		}

		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			player.ChangeState("Jump");
		}
	}
}

void PlayerStateAttackComboB2::Exit(Player& player)
{
	PlayerStateAttackBase::Exit(player);
}
