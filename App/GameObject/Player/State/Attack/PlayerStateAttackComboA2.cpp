#include "PlayerStateAttackComboA2.h"
#include <input/Input.h>
#include "GameObject/Player/Player.h"

PlayerStateAttackComboA2::PlayerStateAttackComboA2(std::string attackName) : PlayerStateAttackBase(attackName)
{
}

void PlayerStateAttackComboA2::Enter(Player& player)
{
	PlayerStateAttackBase::Enter(player);
}

void PlayerStateAttackComboA2::Update(Player& player)
{
	PlayerStateAttackBase::Update(player);
	if (attackPhase_ == AttackPhase::Cancel) {
		if (Input::GetInstance()->TriggerKey(DIK_J)) {
			player.ChangeState("AttackComboA3");
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

void PlayerStateAttackComboA2::Exit(Player& player)
{
	PlayerStateAttackBase::Exit(player);
}
