#include "PlayerStateJump.h"
#include "GameObject/Character/Player/Player.h"

void PlayerStateJump::Enter(Player& player)
{
	// ジャンプ開始時に上向きの速度を設定する
	player.GetVelocity().y = 8.0f;
}

void PlayerStateJump::Update(Player& player, float deltaTime)
{
	// 縦軸の移動を設定したらすぐに空中状態に遷移する
	player.ChangeState("Air");
}

void PlayerStateJump::Exit(Player& player)
{
	// ジャンプ状態から出るときは特に処理しない
	player;
}

void PlayerStateJump::ExecuteCommand(Player& player, const PlayerCommand& command)
{
	// ジャンプ中のコマンドは特に処理しない
}
