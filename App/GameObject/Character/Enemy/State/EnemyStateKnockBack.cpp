#include "EnemyStateKnockBack.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"

void EnemyStateKnockBack::Enter(Enemy& enemy) {
	const DamageInfo& info = enemy.GetPendingDamageInfo();

	currentType_ = info.type;
	velocity_ = info.direction * info.impulseForce;
	stateTime_.current = 0.0f;
	currentTilt_ = 0.0f;
	targetTilt_ = 20.0f;

	switch (info.type) {
	case ReactionType::HitStun:
		stunTimer_ = info.stunTime;
		velocity_ *= 0.25f;
		targetTilt_ = 15.0f;
		break;

	case ReactionType::Knockback:
		velocity_.y += info.impulseForce * info.upwardRatio;
		angularVel_ = info.torqueForce;
		enemy.GetWorldTransform()->GetTranslation().y += 0.3f;
		enemy.SetOnGround(false);
		break;

	case ReactionType::Launch:
		velocity_.y += info.impulseForce * info.upwardRatio * 1.4f;
		angularVel_ = info.torqueForce;
		break;
	}
}

void EnemyStateKnockBack::Update(Enemy& enemy, float deltaTime) {
	stateTime_.current += deltaTime;

	velocity_.y += -9.8f * deltaTime;
	enemy.SetVelocity(velocity_);

	// レンダラーの回転を直接書かずに Enemy 経由で渡す。
	// モデルごとの向き補正（SetModelRotationOffset）と合成されるので、
	// 直接書くと吹き飛んだ敵が正面を向き直してしまう。
	if (currentType_ != ReactionType::HitStun) {
		float rotate = Lerp(0.0f, angularVel_, stateTime_.current);
		enemy.SetModelReactionRotation(EulerDegree({ rotate, rotate, rotate }));
	}
	else {
		currentTilt_ = Lerp(currentTilt_, targetTilt_, deltaTime * 5.0f);
		enemy.SetModelReactionRotation(EulerDegree({ currentTilt_, 0.0f, 0.0f }));

		if ((stunTimer_ -= deltaTime) <= 0.0f) {
			enemy.ChangeState(NextState());
			return;
		}
	}

	if (enemy.GetOnGround() && deltaTime != 0.0f) {
		OnLand(enemy);
	}
}

void EnemyStateKnockBack::OnLand(Enemy& enemy) {
	if (currentType_ == ReactionType::Launch || currentType_ == ReactionType::Knockback) {
		velocity_ *= 0.3f;
		enemy.ChangeState(NextState());
	}
}

const char* EnemyStateKnockBack::NextState() const {
	return EnemyStateName::Idle;
}

void EnemyStateKnockBack::Exit(Enemy& enemy) {
	// のけぞり・吹き飛びで付けた傾きをここで必ず戻す。
	// （以前は着地時にしか戻していなかったので、のけぞりで終わると傾いたままだった。
	//   立方体のときは気づけなかったが、人型のモデルでは傾きっぱなしが目に見える）
	enemy.ClearModelReactionRotation();
}
