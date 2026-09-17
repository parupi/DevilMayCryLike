#include "BossStateDown.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"
#include "World3D/Object/Model/Animation/AnimationPlayer.h"

BossStateDown::BossStateDown(EnemyMovementComponent* movement)
	: movement_(movement) {}

void BossStateDown::Enter(Enemy& enemy) {
	timer_ = 0.0f;
	movement_->Stop(enemy);
	// 崩れている間は振り向かない。横や背中へ回り込んで殴れる時間にする
	enemy.LockFacing(enemy.GetForward());
}

void BossStateDown::Update(Enemy& enemy, float deltaTime) {
	timer_ += deltaTime;
	movement_->Stop(enemy);

	// 被弾クリップを崩れの長さいっぱいへ引き伸ばして1回流す（途中で待機へ戻って見えないように）。
	// クリップはこのあと UpdateAnimation が切り替えるので、長さは毎フレーム取り直す
	if (AnimationPlayer* anim = enemy.GetAnimationPlayer(); anim && anim->GetDuration() > 0.01f) {
		enemy.SetAttackAnimationSpeed(anim->GetDuration() / kDuration);
	}

	if (timer_ >= kDuration) {
		enemy.ChangeState(BossStateName::CombatIdle);
	}
}

void BossStateDown::Exit(Enemy& enemy) {
	enemy.UnlockFacing();
	enemy.ClearAttackAnimationSpeed();
}
