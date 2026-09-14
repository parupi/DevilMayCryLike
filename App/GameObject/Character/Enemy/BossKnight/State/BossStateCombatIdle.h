#pragma once
#include <random>
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
    static constexpr int kNoAction = -1;

    bool breathUnlocked = false; // HPが半分を切った（＝ブレス解禁）
    bool breathUsed     = false; // 解禁後の「必ず1回」のブレスを撃った
    // 咆哮で知らせ終えたフェーズ。HPがこれより先のフェーズへ進んだら、次のフレームで咆哮する
    int  shownPhase     = 1;
    // 直前に選んだ行動（BossStateCombatIdle.cpp の BossAction）。同じ攻撃の連続を避けるのと、
    // 連携（突進 → 噛みつき）に使う。崩れや咆哮で流れが切れたら kNoAction に戻す
    int  lastAction     = kNoAction;
    // 行動の抽選に使う乱数。std::rand は雑魚のステートが種を蒔いたときしか散らばらないので、ボスが自前で持つ
    std::mt19937 rng{ std::random_device{}() };
};

/// <summary>
/// ボスの意思決定ステート。
/// HP割合からフェーズを、プレイヤーとの距離から間合い（近・中・遠）を決め、
/// その組み合わせの重みで次の行動を抽選する。フェーズが上がるほど攻撃的になり、
/// 次の行動までの待ち時間も短くなる。
///
/// 抽選の決まりごと:
///   - 攻撃は **予兆の奥の端までプレイヤーが入っているときだけ** 候補に残す（空振りしない）
///   - 直前と同じ攻撃は選ばれにくい
///   - フェーズ3では、突進で抜けた直後に振り返って噛みつきを繋げる
/// 間合いの境目と射程はどちらも配置スケールに合わせて伸び縮みするので、
/// ステージで2倍に置いたボスとトレーニングの等倍のボスで同じ判断になる。
///
/// 待ち時間の間は棒立ちせず、横へ回り込む・懐に入られたら下がる、で間合いを測る。
/// 接近を終えた直後は待たずに次を選ぶ（歩いては止まる、を繰り返さない）。
///
/// 次の2つは距離・フェーズと別枠:
///   - HPが次のフェーズへ進んだら、待ち時間を待たずに咆哮（BossStateRoar）
///   - 必殺技のブレスは「HPが半分を切ったら射程に入った最初の判断で必ず1回 → 以降は射程内で低確率」
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
    // 待ち時間の間の足さばき
    enum class Footwork {
        Hold,    // その場で構える
        Strafe,  // プレイヤーを中心に横へ回り込む
        Retreat, // 懐に入られたので下がって間合いを作る
    };

    // 現在のフェーズを返す (1=通常, 2=激化, 3=瀕死)
    int  GetPhase(float hp) const;
    // フェーズに応じた、次の行動を選ぶまでの待ち時間 (秒)
    float GetCooldown(int phase) const;
    // 必殺技のブレスを撃つべきか判定する（解禁の記録もここで行う）。
    // 射程の外なら、解禁していても撃たない
    bool ShouldUseBreath(const Enemy& enemy, bool inReach);
    // プレイヤーがその攻撃の予兆の奥の端まで入っているか（配置スケール込み）
    bool IsInReach(Enemy& enemy, const AttackTelegraphParams& telegraph, float distance) const;
    // 体の正面がおおむねプレイヤーを向いているか（連携の噛みつきを、振り返ってから出すため）
    bool IsFacingPlayer(Enemy& enemy) const;
    // 待ち時間の間の足さばきを決める / 毎フレーム動かす
    void PickFootwork(Enemy& enemy);
    void UpdateFootwork(Enemy& enemy);
    // フェーズ・間合いから次の行動を抽選して始める
    void ChooseAction(Enemy& enemy, int phase);
    // 行動を始めて、直前の行動として覚えておく
    void StartAction(Enemy& enemy, int action);
    // 0〜99 の乱数
    int  RollPercent();

    // ブレスが解禁されるHP割合。これを下回ったら、射程に入った最初の判断で必ず1回撃つ
    static constexpr float kBreathHpRatio = 0.5f;
    // 解禁後にブレスを選ぶ確率[%]。必殺技なので低めに抑える
    static constexpr int   kBreathChance = 12;
    // プレイヤーのコライダーの半分の幅[m]。判定は体の端が触れれば当たるので、射程にこのぶん足す。
    // プレイヤーの大きさはボスの配置スケールに関係なく一定
    static constexpr float kPlayerHalfWidth = 0.5f;
    // 直前と同じ攻撃の重みに掛ける倍率。0 にはしない（届く攻撃がそれしか無いこともある）
    static constexpr float kRepeatWeightScale = 0.35f;
    // 接近を終えてから次を選ぶまで[s]
    static constexpr float kAfterApproachWait = 0.15f;
    // 連携（突進 → 噛みつき）を狙う長さ[s]。この間に振り返って届けば噛みつく
    static constexpr float kChainWindow = 0.7f;
    // 連携の噛みつきを出してよい、正面とプレイヤーの方向のずれ[度]
    static constexpr float kChainFacingDegrees = 40.0f;
    // 足さばきの速さ（スケール1のときのワールド単位/秒。配置スケールを掛ける）
    static constexpr float kStrafeSpeed  = 1.25f;
    static constexpr float kRetreatSpeed = 1.5f;
    // 近距離の境目のこの割合より内側へ入られたら「懐に入られた」とみなす
    static constexpr float kCrowdedRatio = 0.75f;

    EnemySensorComponent*  sensor_;
    EnemyMovementComponent* movement_;
    float maxHp_;
    // ボス本体が持つ記憶への参照（3インスタンスで共有）
    BossBattleMemory* memory_;
    float cooldown_ = 0.0f;

    Footwork footwork_ = Footwork::Hold;
    float strafeDir_ = 1.0f;      // 回り込む向き（+1 / -1）
    bool chainPending_ = false;   // 突進の後、噛みつきを繋げようとしている

    // 射程を測るための各攻撃の予兆（スケール1基準）。攻撃ステートの定義から写してくるので、
    // 予兆の大きさを変えれば選ぶ距離も一緒に変わる
    AttackTelegraphParams biteTelegraph_;
    AttackTelegraphParams slamTelegraph_;
    AttackTelegraphParams breathTelegraph_;
};
