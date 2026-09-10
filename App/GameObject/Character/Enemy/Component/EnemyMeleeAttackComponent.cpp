#include "EnemyMeleeAttackComponent.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Player/Player.h"
#include "World3D/Object/Object3d.h"
#include <algorithm>

EnemyMeleeAttackComponent::EnemyMeleeAttackComponent(Object3d* weapon)
	: weapon_(weapon) {}

void EnemyMeleeAttackComponent::BeginAttack(Enemy& enemy, const MeleeAttackParams& params) {
	params_ = params;
	// 予兆は「スケール1のときのワールド単位」で書かれているので、配置スケールを掛けて実寸に合わせる
	const Vector3 ownerScale = enemy.GetWorldTransform()->GetWorldScale();
	params_.telegraph.ApplyScale(ownerScale.x, ownerScale.z);

	timer_ = 0.0f;
	finished_ = false;
	enemy.SetIsAttack(true);
	// 武器の動き（構え→振り抜き）は据え置きで、体のクリップの方を合わせる。
	// 渡すのは「武器が斬り抜ける瞬間まで」＝構えの終わり＋振りの中間まで。
	// 攻撃ごとに長さが違うので、始めるたびに渡し直す
	enemy.BeginAttackAnimation(params_.windupDuration + params_.attackDuration * 0.5f);
}

void EnemyMeleeAttackComponent::ApplyWeaponPose(float t) {
	if (!params_.weaponTranslate.empty()) {
		weapon_->GetWorldTransform()->GetTranslation() = CatmullRomSpline(params_.weaponTranslate, t);
	}
	if (!params_.weaponRotate.empty()) {
		weapon_->GetRenderer(weapon_->name_)->GetWorldTransform()->GetRotation() = EulerDegree(CatmullRomSpline(params_.weaponRotate, t));
	}
}

void EnemyMeleeAttackComponent::Update(Enemy& enemy, float deltaTime) {
	if (finished_) return;

	timer_ += deltaTime;

	if (timer_ < params_.windupDuration) {
		// ── Windup フェーズ ──────────────────────────────────────────
		// t=0 のポーズ（構え）を維持する。敵は XZ 方向に動かない。
		ApplyWeaponPose(0.0f);

		Vector3 vel = enemy.GetVelocity();
		vel.x = 0.0f;
		vel.z = 0.0f;
		enemy.SetVelocity(vel);

	}
	else {
		// ── Attack フェーズ ──────────────────────────────────────────
		float t = (timer_ - params_.windupDuration) / params_.attackDuration;
		t = std::min(t, 1.0f);

		ApplyWeaponPose(t);

		if (params_.rushSpeed > 0.0f) {
			Player* player = enemy.GetPlayer();
			if (player) {
				Vector3 dir = Normalize(player->GetWorldTransform()->GetTranslation() - enemy.GetWorldTransform()->GetTranslation());
				dir.y = 0.0f;
				enemy.GetWorldTransform()->GetTranslation() += dir * params_.rushSpeed * deltaTime;
			}
		}
	}

	if (timer_ >= params_.windupDuration + params_.attackDuration) {
		finished_ = true;
		enemy.SetIsAttack(false);
		enemy.EndAttackAnimation();
	}

	UpdateTelegraph(enemy);
}

void EnemyMeleeAttackComponent::UpdateTelegraph(Enemy& enemy) {
	if (params_.telegraph.shape == TelegraphShape::None) return;
	// 振り始めたら出すのをやめる。
	// マーカー側が「出されなくなった＝発生した」と見て、閃光を出しながら畳んでくれる
	if (finished_ || timer_ >= params_.windupDuration) return;

	const float progress = (params_.windupDuration > 0.01f)
		? std::clamp(timer_ / params_.windupDuration, 0.0f, 1.0f)
		: 1.0f;
	AttackTelegraph::GetInstance().Submit(this, params_.telegraph,
		enemy.GetFootPosition(), enemy.GetForward(), progress);
}

void EnemyMeleeAttackComponent::CancelTelegraph() {
	// 予備動作の途中で中断された場合だけ消す。
	// 振り始めた後のマーカーは閃光を出して畳まれている最中なので触らない
	if (finished_ || timer_ >= params_.windupDuration) return;
	AttackTelegraph::GetInstance().Cancel(this);
}
