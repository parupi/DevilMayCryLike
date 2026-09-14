#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyBoneAttackComponent;
struct AttackTelegraphParams;

/// <summary>
/// 必殺技の火炎ブレス。予備動作2.4秒→前方へ炎を吐き続ける1.6秒（攻撃全体4.2秒）。
///
/// 他の3種と違うところ:
///   - 判定はジョイントではなく **体の正面基準**（orientToBody）。
///     口から前方へまっすぐ伸びる幅3.5m・長さ11mの帯になる。
///   - 判定が長く続く。吐き始めた瞬間に体の向きが予兆の向きで固定されるので、
///     炎は予兆の帯をなぞって伸びる。横へ走って帯から出れば当たらない。
///
/// HPが半分を切った瞬間に必ず1回、以降は低確率で BossStateCombatIdle が選ぶ。
/// </summary>
class BossStateBreath : public EnemyStateBase
{
public:
    /// <summary>
    /// この攻撃の予兆（スケール1基準）。BossStateCombatIdle が「届くか」を測るのに使うので、
    /// 予兆の大きさを変えるとボスがブレスを選ぶ距離も変わる
    /// </summary>
    static AttackTelegraphParams GetTelegraph();

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
