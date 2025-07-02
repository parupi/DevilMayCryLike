#include "PlayerStateAttackComboB3.h"
#include <input/Input.h>
#include "GameObject/Player/Player.h"
PlayerStateAttackComboB3::PlayerStateAttackComboB3(std::string attackName) : PlayerStateAttackBase(attackName)
{
}

void PlayerStateAttackComboB3::Enter(Player& player)
{
	PlayerStateAttackBase::Enter(player);
}

void PlayerStateAttackComboB3::Update(Player& player)
{
	PlayerStateAttackBase::Update(player);
	if (attackPhase_ == AttackPhase::Cancel) {
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			player.ChangeState("Jump");
		}
	}
}

void PlayerStateAttackComboB3::Exit(Player& player)
{
	PlayerStateAttackBase::Exit(player);
}
