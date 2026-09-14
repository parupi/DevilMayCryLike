#include "EnemyBoneAttackComponent.h"
#include "EnemyHitbox.h"
#include "GameObject/Character/Enemy/Enemy.h"
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
	// 予兆も判定と同じ縮尺で書かれているので、同じスケールを掛ける
	params_.telegraph.ApplyScale(ownerScale.x, ownerScale.z);
	aim_.Begin(enemy, params_.telegraph, params_.rushSpeed > 0.0f);

	// 溜めの間だけ向き直りを遅くする。判定が出た瞬間に向きは固定されるので、そこで戻す
	if (params_.windupTurnSpeed > 0.0f) {
		enemy.SetFaceTurnSpeedOverride(params_.windupTurnSpeed);
	} else {
		enemy.ClearFaceTurnSpeedOverride();
	}

	timer_ = 0.0f;
	finished_ = false;
	hitActive_ = false;
	hitRequested_ = false;
	enemy.SetIsAttack(true);

	// クリップは Enemy::UpdateAnimation がステート名から選んで流す。
	// ここではそのクリップがイベントを持っているかだけ見て、判定の出し方を決める
	AnimationPlayer* anim = enemy.GetAnimationPlayer();
	useEvents_ = params_.preferEvents && anim && anim->HasEvents();

	// ── 予備動作の引き伸ばし ──
	// クリップの溜め部分（先頭〜判定が出るまで）だけを extraWindupTime ぶん長く流す。
	// 振り抜きは等速のままなので「ゆっくり構えて鋭く振る」になり、
	// 判定が出る瞬間にクリップもちょうど振り抜きへ差しかかる
	windupClipSeconds_ = params_.duration * params_.hitStartRatio;
	windupDuration_ = windupClipSeconds_ + params_.extraWindupTime;
	windupTimer_ = 0.0f;
	inWindup_ = (params_.extraWindupTime > 0.0f) && (windupClipSeconds_ > 0.01f);
	if (inWindup_) {
		enemy.SetAttackAnimationSpeed(windupClipSeconds_ / windupDuration_);
	} else {
		enemy.ClearAttackAnimationSpeed();
	}
}

bool EnemyBoneAttackComponent::IsWindingUp() const {
	if (finished_) return false;
	if (inWindup_) return true;
	return !hitActive_ && timer_ < params_.duration * params_.hitStartRatio;
}

float EnemyBoneAttackComponent::GetWindupProgress() const {
	if (finished_ || windupDuration_ <= 0.01f) return 0.0f;
	const float progress = windupTimer_ / windupDuration_;
	return progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
}

float EnemyBoneAttackComponent::GetTelegraphProgress() const {
	if (finished_) return 1.0f;
	if (inWindup_) {
		return (windupDuration_ > 0.01f) ? std::clamp(windupTimer_ / windupDuration_, 0.0f, 1.0f) : 1.0f;
	}
	// 溜めを伸ばしていない攻撃は、クリップ側の溜め（判定が出るまで）がそのまま尺になる
	const float strikeAt = params_.duration * params_.hitStartRatio;
	if (strikeAt <= 0.01f) return 1.0f;
	return std::clamp(timer_ / strikeAt, 0.0f, 1.0f);
}

void EnemyBoneAttackComponent::UpdateTelegraph(Enemy& enemy) {
	// 狙いを固定する（＝判定が出る）までの間だけ出す。
	// 出されなくなったマーカーは、閃光を出しながら自動で畳まれる。
	// 固定した後は出し直さないので、判定の窓が閉じた後に予兆が出直すことはない
	if (finished_ || aim_.IsLocked()) return;
	aim_.Aim(enemy, GetTelegraphProgress());
}

void EnemyBoneAttackComponent::Update(Enemy& enemy, float deltaTime) {
	if (finished_) return;

	// ── 判定要求の更新 ──
	// AnimationPlayer は発火したイベントを毎フレーム捨てるので、溜め中でも必ず拾う。
	// （溜めの最終フレームで飛んだ hit_start を取りこぼすと、攻撃が丸ごと空振りになる）
	if (useEvents_) {
		// アニメーションイベント駆動。モーションのどこで当たるかはクリップ側が持つ
		if (AnimationPlayer* anim = enemy.GetAnimationPlayer()) {
			if (anim->WasEventFired(kHitStartTag)) hitRequested_ = true;
			if (anim->WasEventFired(kHitEndTag))   hitRequested_ = false;
		}
	}

	// ── 予備動作（溜め）──
	// 判定も突進も出さず、その場で構える。クリップは BeginAttack で遅くしてある
	if (inWindup_) {
		windupTimer_ += deltaTime;

		Vector3 velocity = enemy.GetVelocity();
		velocity.x = 0.0f;
		velocity.z = 0.0f;
		enemy.SetVelocity(velocity);

		if (windupTimer_ < windupDuration_) {
			// 溜めの間はここで抜ける。予兆だけは進めて「あと少しで来る」を見せ続ける
			UpdateTelegraph(enemy);
			return;
		}

		// 溜め終わり。クリップの溜め部分は消化済みとして本編へ入る
		inWindup_ = false;
		timer_ = windupClipSeconds_;
		deltaTime = windupTimer_ - windupDuration_; // はみ出した分だけ本編を進める
		ApplyStrikeAnimSpeed(enemy);
	}

	timer_ += deltaTime;

	// ── 判定のON/OFF ──
	if (!useEvents_) {
		// イベント未設定のクリップ用。攻撃全体に対する比率で窓を作る
		const float ratio = (params_.duration > 0.01f) ? (timer_ / params_.duration) : 1.0f;
		hitRequested_ = (ratio >= params_.hitStartRatio && ratio < params_.hitEndRatio);
	}
	if (hitRequested_ != hitActive_) {
		SetHitActive(enemy, hitRequested_);
	}

	// ── 突進 ──
	// 判定が出ている間だけ、振り始めに固定した正面へまっすぐ進む（プレイヤーは追わない）。
	// 予備動作では動かない（見てから避けられるように）
	if (params_.rushSpeed > 0.0f && hitActive_) {
		// 帯の奥の端まで、判定が出ているうちに届くよう、判定が出ている時間（比率から見積もる）を渡す
		const float hitWindow = params_.duration * (params_.hitEndRatio - params_.hitStartRatio);
		aim_.Rush(enemy, params_.rushSpeed, hitWindow, deltaTime);
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
		hitRequested_ = false;
		enemy.SetIsAttack(false);
		enemy.EndAttackAnimation();
		enemy.ClearFaceTurnSpeedOverride();
		aim_.Finish(enemy);
	}

	UpdateTelegraph(enemy);
}

void EnemyBoneAttackComponent::Cancel(Enemy& enemy) {
	if (finished_) return;
	SetHitActive(enemy, false);
	finished_ = true;
	// 溜めの途中で中断されても、遅くしたままの再生速度を残さない
	// （EndAttackAnimation が速度指定を解除する）
	inWindup_ = false;
	hitRequested_ = false;
	enemy.SetIsAttack(false);
	enemy.EndAttackAnimation();
	// 判定が出る前に中断された＝この攻撃はもう来ないので予兆を消す。
	// 出た後なら固定した体の向きを解く（マーカーは閃光を出して畳まれている最中なので触らない）
	aim_.Finish(enemy);
	enemy.ClearFaceTurnSpeedOverride();
}

void EnemyBoneAttackComponent::SetHitActive(Enemy& enemy, bool active) {
	hitActive_ = active;
	if (active) {
		// 判定が出た瞬間に、最後に出した予兆の場所と向きで狙いを固定する。
		// これで判定は予兆の上をなぞって出る（イベントで窓が2回開いても向きは変わらない）
		aim_.Lock(enemy);
		// 向きは固定されたので、溜めの間だけの遅い向き直りはここで終わり
		enemy.ClearFaceTurnSpeedOverride();
	}
	if (!hitbox_) return;

	if (!active) {
		hitbox_->Deactivate();
	} else if (params_.orientToBody) {
		hitbox_->ActivateOriented(params_.halfExtents, params_.offset, params_.damage);
	} else {
		hitbox_->Activate(params_.jointName, params_.halfExtents, params_.offset, params_.damage);
	}
}

// 判定が出ている本編の長さがクリップの残りより長い攻撃（ブレスのように撃ち続けるもの）では、
// クリップが先に終わって最後のポーズで固まってしまう。
// 残りのクリップを残り時間へ引き伸ばして、最後まで動いて見えるようにする。
// 攻撃の尺とクリップ長が同じ普通の攻撃では等速のまま（何も変わらない）
void EnemyBoneAttackComponent::ApplyStrikeAnimSpeed(Enemy& enemy) {
	const float attackRemain = params_.duration - windupClipSeconds_;
	if (AnimationPlayer* anim = enemy.GetAnimationPlayer(); anim && attackRemain > 0.01f) {
		const float clipRemain = anim->GetDuration() - anim->GetTime();
		if (clipRemain > 0.01f && clipRemain < attackRemain) {
			enemy.SetAttackAnimationSpeed(clipRemain / attackRemain);
			return;
		}
	}
	enemy.ClearAttackAnimationSpeed();
}
