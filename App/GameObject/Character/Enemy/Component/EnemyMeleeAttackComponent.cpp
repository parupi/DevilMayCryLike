#include "EnemyMeleeAttackComponent.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "World3D/Object/Object3d.h"
#include <algorithm>

EnemyMeleeAttackComponent::EnemyMeleeAttackComponent(Object3d* weapon)
	: weapon_(weapon) {}

void EnemyMeleeAttackComponent::BeginAttack(Enemy& enemy, const MeleeAttackParams& params) {
	params_ = params;
	// 予兆は「スケール1のときのワールド単位」で書かれているので、配置スケールを掛けて実寸に合わせる
	const Vector3 ownerScale = enemy.GetWorldTransform()->GetWorldScale();
	params_.telegraph.ApplyScale(ownerScale.x, ownerScale.z);
	aim_.Begin(enemy, params_.telegraph, params_.rushSpeed > 0.0f);

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

		// 予兆を足元へ出し直す。構えの間はまだプレイヤーへ向き直るので、予兆も一緒に回る
		const float progress = (params_.windupDuration > 0.01f)
			? std::clamp(timer_ / params_.windupDuration, 0.0f, 1.0f)
			: 1.0f;
		aim_.Aim(enemy, progress);
	}
	else {
		// ── Attack フェーズ ──────────────────────────────────────────
		// 振り始めた瞬間に、最後に出した予兆の場所と向きで狙いを固定する。
		// 以降は体も突進もプレイヤーを追わないので、剣は予兆の上をなぞって振られる
		// （予兆はここで出されなくなるので、マーカー側が閃光を出しながら畳む）
		aim_.Lock(enemy);

		float t = (timer_ - params_.windupDuration) / params_.attackDuration;
		t = std::min(t, 1.0f);

		ApplyWeaponPose(t);

		if (params_.rushSpeed > 0.0f) {
			aim_.Rush(enemy, params_.rushSpeed, params_.attackDuration, deltaTime);
		}
	}

	if (timer_ >= params_.windupDuration + params_.attackDuration) {
		finished_ = true;
		enemy.SetIsAttack(false);
		enemy.EndAttackAnimation();
		aim_.Finish(enemy);
	}
}

void EnemyMeleeAttackComponent::Cancel(Enemy& enemy) {
	// 予備動作の途中で中断された場合は予兆を消す（振り始めた後のマーカーは
	// 閃光を出して畳まれている最中なので触らない）。
	// 振り始めた後なら、固定した体の向きをここで解く
	aim_.Finish(enemy);

	// 攻撃そのものも終わった扱いにする（EnemyBoneAttackComponent::Cancel と同じ）。
	// ここを落とさないと、被弾で攻撃ステートを抜けても finished_ が false のまま残り、
	// ・IsWindingUp() が true のまま → のけぞり中や死亡演出中にもチャージリングが出続ける
	// ・振り始めた後なら武器の判定が有効のまま → 攻撃していないのに当たる
	// ・SetIsAttack(true) が残る
	// という「攻撃状態が続く」不具合になる
	if (finished_) return;
	finished_ = true;
	enemy.SetIsAttack(false);
	enemy.EndAttackAnimation();
}
