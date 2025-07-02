#include "PlayerStateAttackHighTime.h"

PlayerStateAttackHighTime::PlayerStateAttackHighTime(std::string attackName) : PlayerStateAttackBase(attackName)
{
}

void PlayerStateAttackHighTime::Enter(Player& player)
{
	PlayerStateAttackBase::Enter(player);
}

void PlayerStateAttackHighTime::Update(Player& player)
{
	PlayerStateAttackBase::Update(player);
}

void PlayerStateAttackHighTime::Exit(Player& player)
{
	PlayerStateAttackBase::Exit(player);
}
