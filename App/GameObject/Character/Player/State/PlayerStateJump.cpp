#include "PlayerStateJump.h"
#include "GameObject/Character/Player/Player.h"

void PlayerStateJump::Enter(Player& player)
{
	player.GetVelocity().y = 8.0f;
}

void PlayerStateJump::Update(Player& player, float deltaTime)
{
	player.ChangeState("Air");
}

void PlayerStateJump::Exit(Player& player)
{
	player;
}

void PlayerStateJump::ExecuteCommand(Player& player, const PlayerCommand& command)
{
}
