#include "PlayerStateKnockBack.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/CharacterStructs.h"
#include "math/Vector3.h"

void PlayerStateKnockBack::Enter(Player& player) {
	timer_ = 0.0f;

	const DamageInfo& info = player.GetPendingDamageInfo();
	duration_ = (info.stunTime > 0.0f) ? info.stunTime : 0.6f;

	// 水平ノックバック
	Vector3& vel = player.GetVelocity();
	vel = info.direction * info.impulseForce;
	vel.y = info.impulseForce * info.upwardRatio;

	player.GetAcceleration() = { 0.0f, 0.0f, 0.0f };
}

void PlayerStateKnockBack::Update(Player& player, float deltaTime) {
	timer_ += deltaTime;

	// 重力
	player.GetAcceleration().y = -12.0f;

	// 水平方向の速度を減衰
	Vector3& vel = player.GetVelocity();
	vel.x *= 0.80f;
	vel.z *= 0.80f;

	// 着地 or スタン時間終了で復帰
	bool landedAndStable = player.GetOnGround() && timer_ > 0.15f;
	if (landedAndStable || timer_ > duration_) {
		vel.x = 0.0f;
		vel.z = 0.0f;
		if (vel.y < 0.0f) vel.y = 0.0f;
		player.GetAcceleration() = { 0.0f, 0.0f, 0.0f };
		player.ChangeState("Idle");
	}
}

void PlayerStateKnockBack::Exit(Player& player) {
	player.GetAcceleration() = { 0.0f, 0.0f, 0.0f };
}
