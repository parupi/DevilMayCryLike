#include "PlayerStateMove.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Player/Controller/PlayerInput.h"
#include <numbers>

void PlayerStateMove::Enter(Player& player)
{
	player.GetAcceleration().y = 0.0f;
}

void PlayerStateMove::Update(Player& player, float deltaTime)
{
	Input* input = Input::GetInstance();

	player.Move();
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

void PlayerStateMove::Move(Player& player)
{
	auto* input = Input::GetInstance();
	BaseCamera* camera = CameraManager::GetInstance()->GetActiveCamera();
	if (!camera) return;

	// 各フレームでまず速度をゼロに初期化
	Vector3 velocity = { 0.0f, player.GetVelocity().y, 0.0f};
	// 入力方向をローカル（プレイヤーから見た）方向で作成
	Vector3 inputDir = { 0.0f, 0.0f, 0.0f };

	if (input->IsConnected()) {
		if (input->GetLeftStickY() != 0.0f) {
			inputDir.z = input->GetLeftStickY();
		}
		if (input->GetLeftStickX() != 0.0f) {
			inputDir.x = input->GetLeftStickX();
		}
	} else {
		if (Input::GetInstance()->PushKey(DIK_W)) inputDir.z += 1.0f;
		if (Input::GetInstance()->PushKey(DIK_S)) inputDir.z -= 1.0f;
		if (Input::GetInstance()->PushKey(DIK_D)) inputDir.x += 1.0f;
		if (Input::GetInstance()->PushKey(DIK_A)) inputDir.x -= 1.0f;
	}

	if (Length(inputDir) > 0.01f) {
		inputDir = Normalize(inputDir);

		// カメラのforward/right（Y方向カット）
		Vector3 camForward = camera->GetForward(); // カメラの「向き」
		camForward.y = 0.0f;
		camForward = Normalize(camForward);

		Vector3 camRight = camera->GetRight(); // カメラの右
		camRight.y = 0.0f;
		camRight = Normalize(camRight);

		// 入力方向をカメラの向きに投影
		Vector3 moveDir = camRight * inputDir.x + camForward * inputDir.z;
		moveDir = Normalize(moveDir);

		// 移動速度に反映
		float velocityY = player.GetVelocity().y;
		velocity = moveDir * 10.0f;
		velocity.y = velocityY;

		//// ロックオンしていなければプレイヤーの向きも更新
		//if (!isLockOn_) {
		//	if (Length(moveDir) > 0.001f) {
		//		moveDir.x *= -1.0f;
		//		Quaternion lookRot = LookRotation(moveDir);

		//		GetWorldTransform()->GetRotation() = lookRot;
		//	}
		//}

		//MoveIntent intent;
		//intent.moveDir = moveDir;
		//intent.moveScale = 10.0f;
		////intent.
		//player.SetIntent(intent);
	}
}
