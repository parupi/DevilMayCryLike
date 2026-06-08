#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyMovementComponent;

/// <summary>
/// 左右移動ステート。MovementComponent でプレイヤーを軸に横へ移動。
/// </summary>
class GruntStateSideMove : public EnemyStateBase
{
public:
    explicit GruntStateSideMove(EnemyMovementComponent* movement);

    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    EnemyMovementComponent* movement_;
    float sideDir_ = 1.0f;
    float timer_   = 0.0f;
};
