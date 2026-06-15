#include "EnemyStateSideMove.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Player/Player.h"
#include <cstdlib>

void EnemyStateSideMove::Enter(Enemy&)
{
    timer_ = 0.0f;
    sideDir_ = (std::rand() % 2 == 0) ? -1.0f : 1.0f;
}

void EnemyStateSideMove::Update(Enemy& enemy, float deltaTime)
{
    timer_ += deltaTime;

    Player* player = enemy.GetPlayer();
    if (!player) return;

    Vector3 toPlayer = player->GetWorldTransform()->GetTranslation() -
                       enemy.GetWorldTransform()->GetTranslation();
    toPlayer.y = 0.0f;
    Normalize(toPlayer);

    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 side = Normalize(Cross(up, toPlayer));

    enemy.SetVelocity(side * sideDir_ * moveSpeed_);

    if (timer_ >= time_) {
        enemy.ChangeState(EnemyStateName::Idle);
    }
}

void EnemyStateSideMove::Exit(Enemy& enemy)
{
    enemy.SetVelocity({ 0.0f, 0.0f, 0.0f });
}
