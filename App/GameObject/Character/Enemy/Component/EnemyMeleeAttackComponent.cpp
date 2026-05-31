#include "EnemyMeleeAttackComponent.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Player/Player.h"
#include "3d/Object/Object3d.h"
#include <algorithm>

EnemyMeleeAttackComponent::EnemyMeleeAttackComponent(Object3d* weapon)
	: weapon_(weapon) {}

void EnemyMeleeAttackComponent::BeginAttack(Enemy& enemy, const MeleeAttackParams& params) {
	params_ = params;
	timer_ = 0.0f;
	finished_ = false;
	enemy.SetIsAttack(true);
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
	}
}
