#include "PlayerStateMove.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Player/Controller/PlayerInput.h"
#include <numbers>

void PlayerStateMove::Enter(Player& player)
{
	// 接地している状態で移動状態に入るので、y軸の速度は0にしておく
	player.GetAcceleration().y = 0.0f;
}

void PlayerStateMove::Update(Player& player, float deltaTime)
{
	// 移動方向を取得
	Vector3 moveDir = player.GetMoveDirection();
	// 移動と回転を実行
	player.Move(moveDir, deltaTime);
	player.Rotate(moveDir, deltaTime);
	// 接地中なのでy軸の速度は0にしておく
	player.GetVelocity().y = 0.0f;

	// 現在の入力状態が無ければ待機
	if (!player.GetInput()->GetContext().isMove) {
		player.ChangeState("Idle");
		return;
	}

	// 地面についていなければ空中状態へ
	if (!player.GetOnGround()) {
		player.ChangeState("Air");
		return;
	}
}

void PlayerStateMove::Exit(Player& player)
{
	// 移動状態から出るときは水平速度を0にしておく
	player.GetVelocity().x = 0.0f;
	player.GetVelocity().z = 0.0f;
}

void PlayerStateMove::ExecuteCommand(Player& player, const PlayerCommand& command)
{
	// ジャンプを要求されたら飛ぶ
	if (command.action == PlayerAction::Jump) {
		player.ChangeState("Jump");
		return;
	}
}