#include "GruntStateRetreat.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"

namespace
{
    constexpr float kDuration = 0.8f;
    constexpr float kSpeed    = 2.5f;
}

GruntStateRetreat::GruntStateRetreat(EnemyMovementComponent* movement)
    : movement_(movement)
{
}

void GruntStateRetreat::Enter(Enemy& enemy)
{
    timer_ = 0.0f;
    enemy;
}

void GruntStateRetreat::Update(Enemy& enemy, float deltaTime)
{
    if (!enemy.GetOnGround()) {
        enemy.ChangeState(EnemyStateName::Air);
        return;
    }

    timer_ += deltaTime;
    movement_->MoveAway(enemy, kSpeed);

    if (timer_ >= kDuration) {
        enemy.ChangeState(GruntMeleeStateName::CombatIdle);
    }
}

void GruntStateRetreat::Exit(Enemy& enemy)
{
    movement_->Stop(enemy);
}
