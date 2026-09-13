#include "EnemyAttackAim.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include <algorithm>

void EnemyAttackAim::Begin(Enemy& enemy, const AttackTelegraphParams& params, bool rush) {
	Finish(enemy);
	params_ = params;
	rush_ = rush;
}

void EnemyAttackAim::SetAim(const Vector3& origin, const Vector3& forward) {
	origin_ = origin;
	Vector3 flat{ forward.x, 0.0f, forward.z };
	// 向きが取れないとき（真上を向いている等）は前の向きのまま
	if (Length(flat) < 0.001f) return;
	forward_ = Normalize(flat);
}

void EnemyAttackAim::Aim(Enemy& enemy, float progress) {
	if (phase_ == Phase::Locked) return;
	phase_ = Phase::Aiming;
	SetAim(enemy.GetFootPosition(), enemy.GetForward());
	AttackTelegraph::GetInstance().Submit(this, params_, origin_, forward_, progress);
}

void EnemyAttackAim::Lock(Enemy& enemy) {
	if (phase_ == Phase::Locked) return;
	// 予兆を出していない攻撃は、今の足元と向きをそのまま狙いにする
	if (phase_ == Phase::Idle) {
		SetAim(enemy.GetFootPosition(), enemy.GetForward());
	}
	phase_ = Phase::Locked;
	// 予兆は前フレームの姿勢で出しているので、体の方をその向きへ揃えてから止める。
	// こうしておくと武器・炎・判定が赤い範囲の上をなぞる
	enemy.LockFacing(forward_);
}

float EnemyAttackAim::GetTravel(Enemy& enemy) const {
	// forward_ は水平なので高さの差は入らない
	return Dot(enemy.GetWorldTransform()->GetTranslation() - origin_, forward_);
}

float EnemyAttackAim::GetRushHalfDepth() const {
	return (std::max)(0.0f, -params_.forwardOffset);
}

float EnemyAttackAim::GetRushMaxTravel() const {
	return (std::max)(0.0f, params_.forwardOffset + params_.length - GetRushHalfDepth());
}

void EnemyAttackAim::Rush(Enemy& enemy, float speed, float rushTime, float deltaTime) {
	if (phase_ != Phase::Locked) return;

	float step = speed * deltaTime;
	if (SweepsRect()) {
		const float maxTravel = GetRushMaxTravel();
		// 配置スケールで帯が伸びても、突進しているうちに奥の端まで届く速さは出す
		if (rushTime > 0.01f) {
			step = (std::max)(speed, maxTravel / rushTime) * deltaTime;
		}
		// 奥の端を越えない（体が予兆の帯からはみ出さない）
		const float remain = (std::max)(0.0f, maxTravel - GetTravel(enemy));
		step = (std::min)(step, remain);
	}
	enemy.GetWorldTransform()->GetTranslation() += forward_ * step;
}

void EnemyAttackAim::Finish(Enemy& enemy) {
	if (phase_ == Phase::Aiming) {
		// 振る前に終わった＝この攻撃はもう来ない。閃光を出さずに予兆を消す
		AttackTelegraph::GetInstance().Cancel(this);
	} else if (phase_ == Phase::Locked) {
		enemy.UnlockFacing();
	}
	phase_ = Phase::Idle;
}
