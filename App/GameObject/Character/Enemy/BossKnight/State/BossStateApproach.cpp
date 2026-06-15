#include "BossStateApproach.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"

BossStateApproach::BossStateApproach(EnemyMovementComponent* movement)
	: movement_(movement) {}

void BossStateApproach::Enter(Enemy&) { timer_ = 0.0f; }

void BossStateApproach::Update(Enemy& enemy, float deltaTime) {
	timer_ += deltaTime;
	movement_->MoveToward(enemy, kSpeed, kStopDistance);

	if (timer_ >= kDuration) {
		movement_->Stop(enemy);
		enemy.ChangeState(BossStateName::CombatIdle);
	}
}

void BossStateApproach::Exit(Enemy& enemy) {
	movement_->Stop(enemy);
}
