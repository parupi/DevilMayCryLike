#include "PlayerStateAttackComboA3.h"
#include <input/Input.h>
#include "GameObject/Player/Player.h"

PlayerStateAttackComboA3::PlayerStateAttackComboA3(std::string attackName) : PlayerStateAttackBase(attackName)
{
}

void PlayerStateAttackComboA3::Enter(Player& player)
{
	PlayerStateAttackBase::Enter(player);
}

void PlayerStateAttackComboA3::Update(Player& player)
{
	PlayerStateAttackBase::Update(player);
	if (attackPhase_ == AttackPhase::Cancel) {

		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			player.ChangeState("Jump");
		}
	}
}

void PlayerStateAttackComboA3::Exit(Player& player)
{
	PlayerStateAttackBase::Exit(player);
}
