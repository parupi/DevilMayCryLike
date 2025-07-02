#include "PlayerStateAttackAerialRave1.h"
#include <input/Input.h>
#include "GameObject/Player/Player.h"

PlayerStateAttackAerialRave1::PlayerStateAttackAerialRave1(std::string attackName) : PlayerStateAttackBase(attackName)
{
}

void PlayerStateAttackAerialRave1::Enter(Player& player)
{
	PlayerStateAttackBase::Enter(player);
}

void PlayerStateAttackAerialRave1::Update(Player& player)
{
	PlayerStateAttackBase::Update(player);
	if (attackPhase_ == AttackPhase::Cancel) {
		if (Input::GetInstance()->TriggerKey(DIK_J)) {
			player.ChangeState("AttackAerialRave2");
		}
	}
}

void PlayerStateAttackAerialRave1::Exit(Player& player)
{
	PlayerStateAttackBase::Exit(player);
}
