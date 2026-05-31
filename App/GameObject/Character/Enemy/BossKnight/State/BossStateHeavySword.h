#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyMeleeAttackComponent;

/// <summary>
/// 遅くて強力な叩きつけ攻撃。予備動作0.9秒→叩きつけ0.35秒。
/// 予備動作が長い分、プレイヤーに回避の猶予がある大振り攻撃。
/// </summary>
class BossStateHeavySword : public EnemyStateBase
{
public:
    explicit BossStateHeavySword(EnemyMeleeAttackComponent* attack);
    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    EnemyMeleeAttackComponent* attack_;
};
