#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyMeleeAttackComponent;

/// <summary>
/// 通常攻撃ステート。その場で武器を振る。
/// MeleeAttackComponent に実行を委譲する。
/// </summary>
class GruntStateAttackNormal : public EnemyStateBase
{
public:
    explicit GruntStateAttackNormal(EnemyMeleeAttackComponent* attack);

    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    EnemyMeleeAttackComponent* attack_;
};
