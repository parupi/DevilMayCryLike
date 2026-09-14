#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"
#include "GameObject/Effect/AttackTelegraph.h"

class EnemySensorComponent;
class EnemyMovementComponent;

/// <summary>
/// 1体のボスにつき1つだけ持つ戦闘中の記憶。
///
/// BossStateCombatIdle は Idle / Move / CombatIdle の **3インスタンス** に分かれて
/// 登録されているので、「必殺技を撃ったか」のような1体ぶんの記憶をステートの
/// メンバに置くと食い違う。ボス本体が1つ持って、全インスタンスへ渡す。
/// </summary>
struct BossBattleMemory
{
    bool breathUnlocked = false; // HPが半分を切った（＝ブレス解禁）
    bool breathUsed     = false; // 解禁後の「必ず1回」のブレスを撃った
};

/// <summary>
/// ボスの意思決定ステート。
/// HP割合からフェーズを、プレイヤーとの距離から間合い（近・中・遠）を決め、
/// その組み合わせの重みで次の行動を抽選する。フェーズが上がるほど攻撃的になり、
/// 判定インターバルも短くなる。
///
/// 攻撃は **予兆の奥の端までプレイヤーが入っているときだけ** 候補に残す
/// （届かない噛みつき・叩きつけを振って空振りしないように）。
/// 間合いの境目と射程はどちらも配置スケールに合わせて伸び縮みするので、
/// ステージで2倍に置いたボスとトレーニングの等倍のボスで同じ判断になる。
///
/// 必殺技のブレスだけは距離・フェーズと別枠で、
/// 「HPが半分を切ったら、射程に入った最初の判断で必ず1回 → 以降は射程内で低確率」で選ばれる。
/// </summary>
class BossStateCombatIdle : public EnemyStateBase
{
public:
    // 間合いの境目（スケール1のときのワールド単位。配置スケールを掛けて使う）。
    // ステージの配置2倍で 4m / 12m になる
    static constexpr float kCloseRange = 2.0f;
    static constexpr float kMidRange   = 6.0f;

    /// <summary>近距離の間合いの境目[m]（配置スケール込み）。接近はここまで詰めたら止まる</summary>
    static float GetCloseRange(Enemy& enemy);

    BossStateCombatIdle(EnemySensorComponent* sensor,
                        EnemyMovementComponent* movement,
                        float maxHp,
                        BossBattleMemory* memory);

    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    // 現在のフェーズを返す (1=通常, 2=激化, 3=瀕死)
    int  GetPhase(float hp) const;
    // フェーズに応じた判定インターバル (秒)
    float GetCooldown(int phase) const;
    // 必殺技のブレスを撃つべきか判定する（解禁の記録もここで行う）。
    // 射程の外なら、解禁していても撃たない
    bool ShouldUseBreath(const Enemy& enemy, bool inReach);
    // プレイヤーがその攻撃の予兆の奥の端まで入っているか（配置スケール込み）
    bool IsInReach(Enemy& enemy, const AttackTelegraphParams& telegraph, float distance) const;

    // ブレスが解禁されるHP割合。これを下回ったら、射程に入った最初の判断で必ず1回撃つ
    static constexpr float kBreathHpRatio = 0.5f;
    // 解禁後にブレスを選ぶ確率[%]。必殺技なので低めに抑える
    static constexpr int   kBreathChance = 12;
    // プレイヤーのコライダーの半分の幅[m]。判定は体の端が触れれば当たるので、射程にこのぶん足す。
    // プレイヤーの大きさはボスの配置スケールに関係なく一定
    static constexpr float kPlayerHalfWidth = 0.5f;

    EnemySensorComponent*  sensor_;
    EnemyMovementComponent* movement_;
    float maxHp_;
    // ボス本体が持つ記憶への参照（3インスタンスで共有）
    BossBattleMemory* memory_;
    float cooldown_ = 0.0f;

    // 射程を測るための各攻撃の予兆（スケール1基準）。攻撃ステートの定義から写してくるので、
    // 予兆の大きさを変えれば選ぶ距離も一緒に変わる
    AttackTelegraphParams biteTelegraph_;
    AttackTelegraphParams slamTelegraph_;
    AttackTelegraphParams breathTelegraph_;
};
