#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyMeleeAttackComponent;

/// <summary>
/// 突進攻撃ステート。距離を詰めながら攻撃する。
/// MeleeAttackComponent の rushSpeed で突進を実行する。
/// </summary>
class GruntStateRushAttack : public EnemyStateBase
{
public:
    explicit GruntStateRushAttack(EnemyMeleeAttackComponent* attack);

    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    EnemyMeleeAttackComponent* attack_;
};
