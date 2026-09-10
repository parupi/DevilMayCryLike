#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

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
/// HP割合からフェーズを判定し、距離に応じて次の行動をランダムに選択する。
/// フェーズが上がるほど攻撃的になり、判定インターバルも短くなる。
///
/// 必殺技のブレスだけは距離・フェーズと別枠で、
/// 「HPが半分を切った瞬間に必ず1回 → 以降は低確率」で選ばれる。
/// </summary>
class BossStateCombatIdle : public EnemyStateBase
{
public:
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
    // 必殺技のブレスを撃つべきか判定する（解禁の記録もここで行う）
    bool ShouldUseBreath(const Enemy& enemy, int roll);

    // ブレスが解禁されるHP割合。これを下回った瞬間に必ず1回撃つ
    static constexpr float kBreathHpRatio = 0.5f;
    // 解禁後にブレスを選ぶ確率[%]。必殺技なので低めに抑える
    static constexpr int   kBreathChance = 12;

    EnemySensorComponent*  sensor_;
    EnemyMovementComponent* movement_;
    float maxHp_;
    // ボス本体が持つ記憶への参照（3インスタンスで共有）
    BossBattleMemory* memory_;
    float cooldown_ = 0.0f;
};
