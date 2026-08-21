#include "EnemyBoneAttackComponent.h"
#include "EnemyHitbox.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Player/Player.h"
#include "World3D/Object/Model/Animation/AnimationPlayer.h"
#include <algorithm>

EnemyBoneAttackComponent::EnemyBoneAttackComponent(EnemyHitbox* hitbox)
	: hitbox_(hitbox) {}

void EnemyBoneAttackComponent::BeginAttack(Enemy& enemy, const BoneAttackParams& params) {
	params_ = params;

	// 判定の大きさ・位置は「オブジェクトのスケール1」を前提に書かれているので、
	// 実際に配置されたスケールを掛けて実寸に合わせる。
	// EnemyHitbox::Activate が打ち消すのは**レンダラー側の縮小だけ**なので、
	// これが無いとステージで敵を2倍に置いたときに見た目だけ大きくなり、
	// 判定が元の大きさのまま置いていかれる（ジョイントは体と一緒に離れていくので、
	// 頭に付けた噛みつきがプレイヤーの頭上を素通りする）
	const Vector3 ownerScale = enemy.GetWorldTransform()->GetWorldScale();
	params_.halfExtents = {
		params_.halfExtents.x * ownerScale.x,
		params_.halfExtents.y * ownerScale.y,
		params_.halfExtents.z * ownerScale.z
	};
	params_.offset = {
		params_.offset.x * ownerScale.x,
		params_.offset.y * ownerScale.y,
		params_.offset.z * ownerScale.z
	};

	timer_ = 0.0f;
	finished_ = false;
	hitActive_ = false;
	enemy.SetIsAttack(true);

	// クリップは Enemy::UpdateAnimation がステート名から選んで流す。
	// ここではそのクリップがイベントを持っているかだけ見て、判定の出し方を決める
	AnimationPlayer* anim = enemy.GetAnimationPlayer();
	useEvents_ = params_.preferEvents && anim && anim->HasEvents();
}

bool EnemyBoneAttackComponent::IsWindingUp() const {
	return !finished_ && !hitActive_ && timer_ < params_.duration * params_.hitStartRatio;
}

void EnemyBoneAttackComponent::Update(Enemy& enemy, float deltaTime) {
	if (finished_) return;

	timer_ += deltaTime;

	// ── 判定のON/OFF ──
	if (useEvents_) {
		// アニメーションイベント駆動。モーションのどこで当たるかはクリップ側が持つ
		if (AnimationPlayer* anim = enemy.GetAnimationPlayer()) {
			if (anim->WasEventFired(kHitStartTag)) SetHitActive(enemy, true);
			if (anim->WasEventFired(kHitEndTag))   SetHitActive(enemy, false);
		}
	} else {
		// イベント未設定のクリップ用。攻撃全体に対する比率で窓を作る
		const float ratio = (params_.duration > 0.01f) ? (timer_ / params_.duration) : 1.0f;
		const bool inWindow = (ratio >= params_.hitStartRatio && ratio < params_.hitEndRatio);
		if (inWindow != hitActive_) {
			SetHitActive(enemy, inWindow);
		}
	}

	// ── 突進 ──
	// 判定が出ている間だけ前へ出る。予備動作では動かない（見てから避けられるように）
	if (params_.rushSpeed > 0.0f && hitActive_) {
		if (Player* player = enemy.GetPlayer()) {
			Vector3 dir = player->GetWorldTransform()->GetTranslation() - enemy.GetWorldTransform()->GetTranslation();
			dir.y = 0.0f;
			if (Length(dir) > 0.001f) {
				enemy.GetWorldTransform()->GetTranslation() += Normalize(dir) * params_.rushSpeed * deltaTime;
			}
		}
	} else {
		// 突進していない間は水平に流れないよう速度を殺す
		Vector3 velocity = enemy.GetVelocity();
		velocity.x = 0.0f;
		velocity.z = 0.0f;
		enemy.SetVelocity(velocity);
	}

	if (timer_ >= params_.duration) {
		SetHitActive(enemy, false);
		finished_ = true;
		enemy.SetIsAttack(false);
		enemy.EndAttackAnimation();
	}
}

void EnemyBoneAttackComponent::Cancel(Enemy& enemy) {
	if (finished_) return;
	SetHitActive(enemy, false);
	finished_ = true;
	enemy.SetIsAttack(false);
	enemy.EndAttackAnimation();
}

void EnemyBoneAttackComponent::SetHitActive(Enemy& enemy, bool active) {
	hitActive_ = active;
	if (!hitbox_) return;

	if (active) {
		hitbox_->Activate(params_.jointName, params_.halfExtents, params_.offset, params_.damage);
	} else {
		hitbox_->Deactivate();
	}
	enemy;
}
