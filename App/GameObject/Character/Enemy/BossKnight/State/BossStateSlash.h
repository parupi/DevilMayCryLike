#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyBoneAttackComponent;
struct AttackTelegraphParams;

/// <summary>
/// 速い噛みつき攻撃。予備動作1.0秒→振り0.2秒（攻撃全体1.43秒）。
/// 3種のなかで一番速いが、それでも見てから避けられる長さの溜めを持つ。
/// </summary>
class BossStateSlash : public EnemyStateBase
{
public:
    /// <summary>
    /// この攻撃の予兆（スケール1基準）。BossStateCombatIdle が「届くか」を測るのに使うので、
    /// 予兆の大きさを変えるとボスがこの攻撃を選ぶ距離も変わる
    /// </summary>
    static AttackTelegraphParams GetTelegraph();

    explicit BossStateSlash(EnemyBoneAttackComponent* attack);
    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    EnemyBoneAttackComponent* attack_;
};
