#include "EnemyStateMove.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Player/Player.h"

namespace
{
    constexpr float kStateDuration = 0.2f;
    constexpr float kMoveSpeed     = 0.5f;
    constexpr float kStopDistance  = 10.0f;
}

void EnemyStateMove::Enter(Enemy& enemy)
{
    stateTime_.max     = kStateDuration;
    stateTime_.current = 0.0f;
    enemy;
}

void EnemyStateMove::Update(Enemy& enemy, float deltaTime)
{
    if (!enemy.GetOnGround()) {
        enemy.ChangeState(EnemyStateName::Air);
        return;
    }

    stateTime_.current += (1.0f / stateTime_.max) * deltaTime;

    Vector3 currentVelocity = enemy.GetVelocity();
    Vector3 toPlayer = enemy.GetPlayer()->GetWorldTransform()->GetTranslation() -
                       enemy.GetWorldTransform()->GetTranslation();

    Vector3 newVelocity{};
    if (Length(toPlayer) > kStopDistance) {
        newVelocity = Normalize(toPlayer) * kMoveSpeed;
    }
    newVelocity.y = currentVelocity.y;
    enemy.SetVelocity(newVelocity);

    if (stateTime_.current >= 1.0f) {
        enemy.ChangeState(EnemyStateName::Idle);
    }
}

void EnemyStateMove::Exit(Enemy& enemy)
{
    enemy;
}
