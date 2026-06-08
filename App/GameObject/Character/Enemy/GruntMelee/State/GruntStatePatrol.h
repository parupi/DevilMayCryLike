#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemySensorComponent;

/// <summary>
/// 発見前の待機ステート。
/// SensorComponent でプレイヤーを感知したら CombatIdle へ遷移する。
/// </summary>
class GruntStatePatrol : public EnemyStateBase
{
public:
    explicit GruntStatePatrol(EnemySensorComponent* sensor);

    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    EnemySensorComponent* sensor_;
};
