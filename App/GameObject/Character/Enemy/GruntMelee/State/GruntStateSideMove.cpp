#include "GruntStateSideMove.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"
#include <cstdlib>

namespace
{
    constexpr float kDuration = 1.0f;
    constexpr float kSpeed    = 2.0f;
}

GruntStateSideMove::GruntStateSideMove(EnemyMovementComponent* movement)
    : movement_(movement)
{
}

void GruntStateSideMove::Enter(Enemy& enemy)
{
    timer_   = 0.0f;
    sideDir_ = (std::rand() % 2 == 0) ? 1.0f : -1.0f;
    enemy;
}

void GruntStateSideMove::Update(Enemy& enemy, float deltaTime)
{
    if (!enemy.GetOnGround()) {
        enemy.ChangeState(EnemyStateName::Air);
        return;
    }

    timer_ += deltaTime;
    movement_->MoveSideways(enemy, kSpeed, sideDir_);

    if (timer_ >= kDuration) {
        enemy.ChangeState(GruntMeleeStateName::CombatIdle);
    }
}

void GruntStateSideMove::Exit(Enemy& enemy)
{
    movement_->Stop(enemy);
}
