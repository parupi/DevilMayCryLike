#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyMeleeAttackComponent;

/// <summary>
/// 高速突進攻撃。予備動作0.6秒→高速前進しながら振り抜く0.3秒。
/// </summary>
class BossStateRush : public EnemyStateBase
{
public:
    explicit BossStateRush(EnemyMeleeAttackComponent* attack);
    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    EnemyMeleeAttackComponent* attack_;
};
