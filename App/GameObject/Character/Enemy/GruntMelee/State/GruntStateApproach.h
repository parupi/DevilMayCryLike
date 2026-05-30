#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyMovementComponent;

/// <summary>
/// 接近ステート。MovementComponent でプレイヤーへ近づく。
/// </summary>
class GruntStateApproach : public EnemyStateBase
{
public:
    explicit GruntStateApproach(EnemyMovementComponent* movement);

    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    EnemyMovementComponent* movement_;
    float timer_ = 0.0f;
};
