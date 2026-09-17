#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyBoneAttackComponent;

/// <summary>
/// 高速突進攻撃。予備動作0.9秒→高速前進しながら体当たり0.44秒（攻撃全体1.48秒）。
/// 溜めの間はその場で構えるので、突っ込んでくる前に進路から外れられる。
/// </summary>
class BossStateRush : public EnemyStateBase
{
public:
    explicit BossStateRush(EnemyBoneAttackComponent* attack);
    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    // 突進中のループ音を止める。鳴っていなければ何もしない
    void StopRushLoop();

    EnemyBoneAttackComponent* attack_;
    // 突進中に鳴らしているループの再生番号。-1 なら鳴っていない
    int rushVoice_ = -1;
};
