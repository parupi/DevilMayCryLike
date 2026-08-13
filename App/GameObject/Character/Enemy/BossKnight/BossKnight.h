#pragma once
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/Component/EnemySensorComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyBoneAttackComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyHitbox.h"
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

    /// 見た目のモデル。Resource/models/Enemys/Dragon/Dragon.obj を指す
    static constexpr const char* kModelName = "Enemys/Dragon";
    /// 素の高さ約4.0m を約2.0m（雑魚の1.6倍・翼で横幅1.7m）にするスケール
    static constexpr float kModelScale = 0.5f;

    // Dragon.gltf が持つクリップ（5種）。待機に相当するのは Flying
    static constexpr const char* kClipIdle    = "Dragon_Flying";
    static constexpr const char* kClipAttack  = "Dragon_Attack";   // 速い斬り
    static constexpr const char* kClipAttack2 = "Dragon_Attack2";  // 遅い叩きつけ
    static constexpr const char* kClipHit     = "Dragon_Hit";
    static constexpr const char* kClipDeath   = "Dragon_Death";
    /// 振り切る瞬間の位置。角速度ピークは Attack が BodyRoot 0.583/0.88秒=67%、
    /// Attack2 が翼の叩きつけ 1.083/1.67秒=65%
    static constexpr float kAttackImpactRatio  = 0.67f;
    static constexpr float kAttack2ImpactRatio = 0.65f;

    BossKnight(std::string objectName);
    void Initialize() override;
    void Update(float deltaTime) override;

#ifdef _DEBUG
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

    /// <summary>ボスは倒すのが難しいのでスタイルスコアを高めに補正する。</summary>
    float GetStyleMultiplier() const override { return 2.0f; }

protected:
    /// <summary>死亡演出終了時に武器を後始末する</summary>
    void OnDeathEffectFinished() override;

private:
    // スーパーアーマーの視覚表示（紫オーラ・ライト・体の発光）をまとめて更新する
    void UpdateArmorVisual(float deltaTime);

    // 剣は持たない。噛みつき・叩きつけ・突進のたびに、この判定を該当ジョイントへ付け替える
    EnemyHitbox*     hitbox_        = nullptr;
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

    std::unique_ptr<EnemySensorComponent>       sensor_;
    std::unique_ptr<EnemyMovementComponent>     movement_;
    std::unique_ptr<EnemyBoneAttackComponent>   boneAttack_;
};
