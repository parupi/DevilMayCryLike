#pragma once
#include "GameObject/Character/Enemy/Enemy.h"
#include "BossWeapon.h"
#include "GameObject/Character/Enemy/Component/EnemySensorComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyMeleeAttackComponent.h"
#include "Graphics/Rendering/Particle/ParticleEmitter.h"

/// <summary>
/// ボスエネミー：BossKnight
///
/// 通常敵の約3倍のHPを持ち、HP割合に応じて3フェーズに変化する。
/// Slash（速い縦斬り）・HeavySword（遅い叩きつけ）・Rush（高速突進）の
/// 3種類の攻撃を状況と残りHPに応じて使い分ける。
/// </summary>
class BossKnight : public Enemy
{
public:
    static constexpr float kMaxHp = 25.0f;

    BossKnight(std::string objectName);
    void Initialize() override;
    void Update(float deltaTime) override;

#ifdef _DEBUG
    void DebugGui() override;
#endif

    void OnCollisionEnter(BaseCollider* other) override;
    void OnCollisionStay(BaseCollider* other) override;
    void OnCollisionExit(BaseCollider* other) override;

    /// <summary>
    /// ノックバック無効（スーパーアーマー）中かどうか。
    /// 吹き飛ばされてから次の突進攻撃(Rush)を終えるまでが該当する。
    /// レティクルの色変化などプレイヤーへの状態表示にも使われる。
    /// </summary>
    bool IsKnockbackImmune() const override;

protected:
    /// <summary>死亡演出終了時に武器を後始末する</summary>
    void OnDeathEffectFinished() override;

private:
    // スーパーアーマーの視覚表示（紫オーラ・ライト・体の発光）をまとめて更新する
    void UpdateArmorVisual(float deltaTime);

    BossWeapon*      weapon_        = nullptr;
    ParticleEmitter* hitEmitter_    = nullptr;
    ParticleEmitter* chargeEmitter_ = nullptr;
    ParticleEmitter* auraEmitter_   = nullptr;
    ParticleEmitter* armorHitEmitter_ = nullptr; // アーマー中被弾の弾かれ火花

    float chargeEmitTimer_ = 0.0f;
    static constexpr float kChargeEmitInterval = 0.08f;

    // スーパーアーマー中の紫オーラの発生間隔
    float auraEmitTimer_ = 0.0f;
    static constexpr float kAuraEmitInterval = 0.04f;

    // スーパーアーマー中の体の発光（脈動）用の経過時間
    float armorTintPhase_ = 0.0f;
    // アーマー中被弾時に体を一瞬強く光らせるタイマー
    float armorHitFlashTimer_ = 0.0f;
    static constexpr float kArmorHitFlashDuration = 0.15f;

    // ── ヒット蓄積ノックバック ──────────────────────────────────────
    // kKnockbackThreshold 分のダメージが溜まると初めて吹き飛ぶ
    float hitAccumulation_           = 0.0f;
    static constexpr float kKnockbackThreshold = 3.0f; // 通常攻撃約3発分

    std::unique_ptr<EnemySensorComponent>      sensor_;
    std::unique_ptr<EnemyMovementComponent>    movement_;
    std::unique_ptr<EnemyMeleeAttackComponent> meleeAttack_;
};
