#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyBoneAttackComponent;
struct AttackTelegraphParams;

/// <summary>
/// 遅くて強力な叩きつけ攻撃。予備動作1.8秒→叩きつけ0.28秒（攻撃全体2.52秒）。
/// 予備動作が長い分、プレイヤーに回避の猶予がある大振り攻撃。
/// </summary>
class BossStateHeavySword : public EnemyStateBase
{
public:
    /// <summary>
    /// この攻撃の予兆（スケール1基準）。BossStateCombatIdle が「届くか」を測るのに使うので、
    /// 予兆の大きさを変えるとボスがこの攻撃を選ぶ距離も変わる
    /// </summary>
    static AttackTelegraphParams GetTelegraph();

    explicit BossStateHeavySword(EnemyBoneAttackComponent* attack);
    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    EnemyBoneAttackComponent* attack_;
};
