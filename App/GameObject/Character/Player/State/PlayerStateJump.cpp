#include "PlayerStateJump.h"
#include "GameObject/Character/Player/Player.h"

void PlayerStateJump::Enter(Player& player)
{
	// ジャンプ開始時に上向きの速度を設定する
	player.GetVelocity().y = 8.0f;
}

void PlayerStateJump::Update(Player& player, float)
{
	// ジャンプのチュートリアルを進める
	player.GetTutorialService()->StepTutorial(TutorialState::Jump);

	// 縦軸の移動を設定したらすぐに空中状態に遷移する
	player.ChangeState("Air");
}

void PlayerStateJump::Exit(Player&)
{
}

void PlayerStateJump::ExecuteCommand(Player&, const PlayerCommand&)
{
}
