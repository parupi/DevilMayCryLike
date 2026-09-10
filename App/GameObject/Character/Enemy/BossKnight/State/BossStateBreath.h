#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyBoneAttackComponent;

/// <summary>
/// 必殺技の火炎ブレス。予備動作2.4秒→前方へ炎を吐き続ける1.6秒（攻撃全体4.2秒）。
///
/// 他の3種と違うところ:
///   - 判定はジョイントではなく **体の正面基準**（orientToBody）。
///     口から前方へまっすぐ伸びる幅3.5m・長さ11mの帯になる。
///   - 炎を吐いている間は向き直りが遅くなる（kBreathTurnSpeed）。
///     照準がプレイヤーを追いきれないので、横へ走れば炎から抜けられる。
///
/// HPが半分を切った瞬間に必ず1回、以降は低確率で BossStateCombatIdle が選ぶ。
/// </summary>
class BossStateBreath : public EnemyStateBase
{
public:
    explicit BossStateBreath(EnemyBoneAttackComponent* attack);
    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    // 炎VFXを口元から前方へ吹き出す
    void EmitFlame(Enemy& enemy);

    EnemyBoneAttackComponent* attack_;
    float emitTimer_ = 0.0f;
};
