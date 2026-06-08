#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyMovementComponent;

/// <summary>
/// 後退ステート。MovementComponent でプレイヤーから離れる。
/// </summary>
class GruntStateRetreat : public EnemyStateBase
{
public:
    explicit GruntStateRetreat(EnemyMovementComponent* movement);

    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    EnemyMovementComponent* movement_;
    float timer_ = 0.0f;
};
