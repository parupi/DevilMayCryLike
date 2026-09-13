#include "PlayerStateKnockBack.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/CharacterStructs.h"
#include "Math/Vector3.h"

void PlayerStateKnockBack::Enter(Player& player) {
	timer_ = 0.0f;

	// 軽被弾／強被弾で拘束の長さを変える（仕様書 §12）。
	// ノックバックの速度そのものは Player::TakeDamage が KnockbackComponent へ渡し済み
	const float stunTime = player.GetKnockback().GetStunTime();
	maxDuration_ = player.IsHeavyHit() ? kHeavyMaxDuration : kLightMaxDuration;
	// 攻撃側が stunTime を指定していればそれを尊重するが、上限は超えさせない
	if (stunTime > 0.0f && stunTime < maxDuration_) {
		maxDuration_ = stunTime;
	}

	// 移動の速度はノックバックと混ざらないようここで捨てる。
	// 落下はノックバック側が重力で面倒を見るので、加速度も切っておく（二重に落ちる）
	player.GetVelocity() = {};
	player.GetAcceleration() = {};
}

void PlayerStateKnockBack::Update(Player& player, float deltaTime) {
	timer_ += deltaTime;

	// 着地したか、上限時間に達したら操作を返す。
	// ノックバックの速度はここで消さない（残った勢いは KnockbackComponent が減衰させる）。
	// そのため、動き出しても少し滑りながら立て直す形になる
	const bool landed = player.GetOnGround() && timer_ >= kMinDuration;
	if (landed || timer_ >= maxDuration_) {
		player.GetAcceleration() = {};
		player.ChangeState("Idle");
	}
}

void PlayerStateKnockBack::Exit(Player& player) {
	player.GetAcceleration() = { 0.0f, 0.0f, 0.0f };
}
