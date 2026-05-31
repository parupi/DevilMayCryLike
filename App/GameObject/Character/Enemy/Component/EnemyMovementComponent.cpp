#include "EnemyMovementComponent.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Player/Player.h"

void EnemyMovementComponent::MoveToward(Enemy& enemy, float speed, float stopDistance)
{
    Player* player = enemy.GetPlayer();
    if (!player) return;

    Vector3 toPlayer = player->GetWorldTransform()->GetTranslation() -
                       enemy.GetWorldTransform()->GetTranslation();
    toPlayer.y = 0.0f;

    Vector3 vel = enemy.GetVelocity();
    if (Length(toPlayer) > stopDistance) {
        vel.x = Normalize(toPlayer).x * speed;
        vel.z = Normalize(toPlayer).z * speed;
    } else {
        vel.x = 0.0f;
        vel.z = 0.0f;
    }
    enemy.SetVelocity(vel);
}

void EnemyMovementComponent::MoveAway(Enemy& enemy, float speed)
{
    Player* player = enemy.GetPlayer();
    if (!player) return;

    Vector3 away = enemy.GetWorldTransform()->GetTranslation() -
                   player->GetWorldTransform()->GetTranslation();
    away.y = 0.0f;
    if (Length(away) < 0.0001f) return;

    Vector3 dir = Normalize(away);
    Vector3 vel = enemy.GetVelocity();
    vel.x = dir.x * speed;
    vel.z = dir.z * speed;
    enemy.SetVelocity(vel);
}

void EnemyMovementComponent::MoveSideways(Enemy& enemy, float speed, float direction)
{
    Player* player = enemy.GetPlayer();
    if (!player) return;

    Vector3 toPlayer = player->GetWorldTransform()->GetTranslation() -
                       enemy.GetWorldTransform()->GetTranslation();
    toPlayer.y = 0.0f;
    if (Length(toPlayer) < 0.0001f) return;
    Normalize(toPlayer);

    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 side = Normalize(Cross(up, toPlayer));

    Vector3 vel = enemy.GetVelocity();
    vel.x = side.x * speed * direction;
    vel.z = side.z * speed * direction;
    enemy.SetVelocity(vel);
}

void EnemyMovementComponent::Stop(Enemy& enemy)
{
    Vector3 vel = enemy.GetVelocity();
    vel.x = 0.0f;
    vel.z = 0.0f;
    enemy.SetVelocity(vel);
}
