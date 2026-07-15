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

	if (currentType_ != ReactionType::HitStun) {
		float rotate = Lerp(0.0f, angularVel_, stateTime_.current);
		enemy.GetRenderer(enemy.name_)->GetWorldTransform()->GetRotation() = EulerDegree({ rotate, rotate, rotate });
	}
	else {
		currentTilt_ = Lerp(currentTilt_, targetTilt_, deltaTime * 5.0f);
		enemy.GetRenderer(enemy.name_)->GetWorldTransform()->GetRotation() = EulerDegree({ currentTilt_, 0.0f, 0.0f });

		if ((stunTimer_ -= deltaTime) <= 0.0f) {
			enemy.ChangeState(EnemyStateName::Idle);
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
		enemy.GetRenderer(enemy.name_)->GetWorldTransform()->GetRotation() = { 0.0f, 0.0f, 0.0f };
		enemy.ChangeState(EnemyStateName::Idle);
	}
}

void EnemyStateKnockBack::Exit(Enemy& enemy) {
	enemy;
}
