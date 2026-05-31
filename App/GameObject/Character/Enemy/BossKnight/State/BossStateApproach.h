#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyMovementComponent;

class BossStateApproach : public EnemyStateBase
{
public:
    explicit BossStateApproach(EnemyMovementComponent* movement);
    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    EnemyMovementComponent* movement_;
    float timer_ = 0.0f;
    static constexpr float kDuration     = 1.5f;
    static constexpr float kSpeed        = 5.0f;
    static constexpr float kStopDistance = 3.5f;
};
