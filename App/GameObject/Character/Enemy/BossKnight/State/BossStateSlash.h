#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyBoneAttackComponent;

/// <summary>
/// 速い噛みつき攻撃。予備動作1.0秒→振り0.2秒（攻撃全体1.43秒）。
/// 3種のなかで一番速いが、それでも見てから避けられる長さの溜めを持つ。
/// </summary>
class BossStateSlash : public EnemyStateBase
{
public:
    explicit BossStateSlash(EnemyBoneAttackComponent* attack);
    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    EnemyBoneAttackComponent* attack_;
};
