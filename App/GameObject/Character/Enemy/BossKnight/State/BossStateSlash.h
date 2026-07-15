#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyMeleeAttackComponent;

/// <summary>
/// 速い縦斬り攻撃。予備動作0.45秒→振り0.22秒。
/// </summary>
class BossStateSlash : public EnemyStateBase
{
public:
    explicit BossStateSlash(EnemyMeleeAttackComponent* attack);
    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    EnemyMeleeAttackComponent* attack_;
};
