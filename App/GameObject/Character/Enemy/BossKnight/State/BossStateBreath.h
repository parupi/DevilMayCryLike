#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyBoneAttackComponent;
class BossBreathEffect;
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
/// 見た目（溜め→発射→維持→余韻）は BossBreathEffect が持つ。
/// ここは毎フレーム「今どの段階か」を伝えるだけ。
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

    BossStateBreath(EnemyBoneAttackComponent* attack, BossBreathEffect* effect);
    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    // 炎のループ音を止める。鳴っていなければ何もしない
    void StopBreathLoop();

    EnemyBoneAttackComponent* attack_;
    BossBreathEffect* effect_;
    // 吐き始めてからの経過[s]。炎が細くなっていく終わり際を演出側へ伝えるのに使う
    float fireTimer_ = 0.0f;
    // 炎のループ音の再生番号。-1 なら鳴っていない
    int breathVoice_ = -1;
    // 前のフレームに炎を吐いていたか。着火と消える瞬間を1回だけ拾うために持つ
    bool wasFiring_ = false;
};
