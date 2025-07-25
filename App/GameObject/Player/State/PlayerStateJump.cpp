#include "PlayerStateJump.h"
#include "GameObject/Player/Player.h"

void PlayerStateJump::Enter(Player& player)
{
	player.GetVelocity().y = 8.0f;
}

void PlayerStateJump::Update(Player& player)
{
	player.ChangeState("Air");
}

void PlayerStateJump::Exit(Player& player)
{
	player;
}
