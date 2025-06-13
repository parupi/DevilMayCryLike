#include "PlayerStateMove.h"
#include <GameObject/Player/Player.h>
#include <numbers>

void PlayerStateMove::Enter(Player& player)
{
}

void PlayerStateMove::Update(Player& player)
{
	// 各フレームでまず速度をゼロに初期化
	player.GetVelocity() = {0.0f, player.GetVelocity().y, 0.0f};

	if (Input::GetInstance()->PushKey(DIK_W)) {
		player.GetVelocity().z += 1.0f;
	}
	if (Input::GetInstance()->PushKey(DIK_S)) {
		player.GetVelocity().z -= 1.0f;
	}
	if (Input::GetInstance()->PushKey(DIK_D)) {
		player.GetVelocity().x += 1.0f;
	}
	if (Input::GetInstance()->PushKey(DIK_A)) {
		player.GetVelocity().x -= 1.0f;
	}

	if (Length(player.GetVelocity()) > 0.01f) {
		player.GetVelocity() = Normalize(player.GetVelocity()) * 10.0f;;

		Vector3 moveDirection = player.GetVelocity();
		moveDirection.y = 0.0f;
		moveDirection.z *= -1.0f;

		// moveDirection がゼロベクトルかチェック
		if (Length(moveDirection) > 0.0001f) {
			// Z+が前を向くように回転を取得
			Quaternion lookRot = LookRotation(moveDirection);

			// Z+を向くように補正 → -Zを前にしたいのでY軸180度回転
			Quaternion correction = MakeRotateAxisAngleQuaternion({ 0.0f, 1.0f, 0.0f }, std::numbers::pi);

			// Apply correction BEFORE LookRotation
			player.GetWorldTransform()->GetRotation() = correction * lookRot; // ← 順番重要（補正を先に掛ける）
		}
	}

	if (!Input::GetInstance()->PushKey(DIK_W) && !Input::GetInstance()->PushKey(DIK_A) && !Input::GetInstance()->PushKey(DIK_S) && !Input::GetInstance()->PushKey(DIK_D)) {
		player.ChangeState("Idle");
	}
}

void PlayerStateMove::Exit(Player& player)
{
}
