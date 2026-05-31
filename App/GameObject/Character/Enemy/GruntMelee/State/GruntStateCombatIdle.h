#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemySensorComponent;

/// <summary>
/// 戦闘中の行動決定ステート（意思決定）。
/// 距離と乱数で移動3種・攻撃2種のいずれかへ遷移する。
/// </summary>
class GruntStateCombatIdle : public EnemyStateBase
{
public:
    explicit GruntStateCombatIdle(EnemySensorComponent* sensor);

    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    EnemySensorComponent* sensor_;

    bool  inCooldown_    = false;
    float cooldownTimer_ = 0.0f;
};
