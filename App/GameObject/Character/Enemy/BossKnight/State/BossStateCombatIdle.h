#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemySensorComponent;
class EnemyMovementComponent;

/// <summary>
/// ボスの意思決定ステート。
/// HP割合からフェーズを判定し、距離に応じて次の行動をランダムに選択する。
/// フェーズが上がるほど攻撃的になり、判定インターバルも短くなる。
/// </summary>
class BossStateCombatIdle : public EnemyStateBase
{
public:
    BossStateCombatIdle(EnemySensorComponent* sensor,
                        EnemyMovementComponent* movement,
                        float maxHp);

    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    // 現在のフェーズを返す (1=通常, 2=激化, 3=瀕死)
    int  GetPhase(float hp) const;
    // フェーズに応じた判定インターバル (秒)
    float GetCooldown(int phase) const;

    EnemySensorComponent*  sensor_;
    EnemyMovementComponent* movement_;
    float maxHp_;
    float cooldown_ = 0.0f;
};
