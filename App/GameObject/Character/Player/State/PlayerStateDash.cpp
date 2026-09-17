#include "PlayerStateDash.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Player/Controller/PlayerInput.h"
#include <algorithm>
#include <cmath>

namespace {
	// current を target へ、1フレームで maxRadians までの範囲で回す（水平のみ・単位ベクトル前提）
	Vector3 RotateTowards(const Vector3& current, const Vector3& target, float maxRadians) {
		const float dot = std::clamp(current.x * target.x + current.z * target.z, -1.0f, 1.0f);
		const float angle = std::acos(dot);
		if (angle <= maxRadians || angle < 1e-4f) return target;

		// 球面線形補間の水平版。angle が小さいときは sin で割ると荒れるので上で弾いている
		const float t = maxRadians / angle;
		const float sinAngle = std::sin(angle);
		const float a = std::sin((1.0f - t) * angle) / sinAngle;
		const float b = std::sin(t * angle) / sinAngle;
		Vector3 result = current * a + target * b;
		result.y = 0.0f;
		const float length = Length(result);
		return (length > 1e-4f) ? result * (1.0f / length) : current;
	}
}

void PlayerStateDash::Enter(Player& player) {
	elapsed_ = 0.0f;
	noInputTime_ = 0.0f;

	const PlayerDodgeParams& params = player.GetDodgeParams();
	const PlayerDodgeRuntime& runtime = player.GetDodgeRuntime();

	// 回避から速度を一段上げる。ここは滑らかに繋がずはっきり変える（仕様書 §6.2）
	Vector3& velocity = player.GetVelocity();
	const float fallSpeed = velocity.y;
	velocity = runtime.direction * params.dashSpeed;
	velocity.y = player.GetOnGround() ? 0.0f : fallSpeed;

	player.GetAcceleration() = { 0.0f, 0.0f, 0.0f };

	player.OnDashStart();
}

void PlayerStateDash::Update(Player& player, float deltaTime) {
	PlayerDodgeRuntime& runtime = player.GetDodgeRuntime();
	const PlayerDodgeParams& params = player.GetDodgeParams();

	elapsed_ += deltaTime;

	// 地面から離れたら空中状態へ（段差から飛び出した場合）。
	// 速度を組み立てる前に抜けておく。Air 側が自分で速度と重力を設定する
	if (!player.GetOnGround()) {
		player.ChangeState("Air");
		return;
	}

	// スティックが入っていれば進行方向を少しずつそちらへ寄せる。
	// 回避と違って曲がれるようにしておかないと、ダッシュが移動手段として使えない
	const Vector3 inputDir = player.GetMoveDirection();
	if (Length(inputDir) > 0.01f) {
		noInputTime_ = 0.0f;
		runtime.direction = RotateTowards(runtime.direction, inputDir, params.dashTurnRate * deltaTime);
	} else {
		noInputTime_ += deltaTime;
	}

	player.FaceDirection(runtime.direction);

	Vector3& velocity = player.GetVelocity();
	velocity = runtime.direction * params.dashSpeed;
	velocity.y = 0.0f;
	player.GetAcceleration().y = 0.0f;

	// 終了条件（仕様書 §6.3）：時間切れ、または移動入力が途切れた
	if (elapsed_ >= params.dashDuration || noInputTime_ >= params.dashInputGrace) {
		player.ChangeState(player.GetInput()->GetContext().isMove ? "Move" : "Idle");
		return;
	}
}

void PlayerStateDash::Exit(Player& player) {
	// 抜けた先が速度を設定しないステート（Idle等）でも滑り続けないように水平速度を落とす
	Vector3& velocity = player.GetVelocity();
	velocity.x = 0.0f;
	velocity.z = 0.0f;
	player.GetAcceleration() = { 0.0f, 0.0f, 0.0f };
}

void PlayerStateDash::ExecuteCommand(Player& player, const PlayerCommand& command) {
	// ジャンプでキャンセルできる。
	// 攻撃と回避は Player::ExecuteCommand が先に拾ってこのステートを畳むので、ここには来ない
	if (command.action == PlayerAction::Jump) {
		player.ChangeState("Jump");
		return;
	}
}
