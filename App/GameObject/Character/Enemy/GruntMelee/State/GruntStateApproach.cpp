#include "GruntStateApproach.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"

namespace
{
    constexpr float kDuration  = 1.2f;
    constexpr float kSpeed     = 3.0f;
    constexpr float kStopDist  = 2.5f;
}

GruntStateApproach::GruntStateApproach(EnemyMovementComponent* movement)
    : movement_(movement)
{
}

void GruntStateApproach::Enter(Enemy& enemy)
{
    timer_ = 0.0f;
    enemy;
}

void GruntStateApproach::Update(Enemy& enemy, float deltaTime)
{
    if (!enemy.GetOnGround()) {
        enemy.ChangeState(EnemyStateName::Air);
        return;
    }

    timer_ += deltaTime;
    movement_->MoveToward(enemy, kSpeed, kStopDist);

    if (timer_ >= kDuration) {
        enemy.ChangeState(GruntMeleeStateName::CombatIdle);
    }
}

void GruntStateApproach::Exit(Enemy& enemy)
{
    movement_->Stop(enemy);
}
