#pragma once
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/Component/EnemySensorComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyBoneAttackComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyHitbox.h"
#include "GameObject/Character/Enemy/BossKnight/State/BossStateCombatIdle.h"
#include "Graphics/Rendering/Particle/ParticleEmitter.h"

/// <summary>
/// ボスエネミー：BossKnight
///
/// 通常敵の約3倍のHPを持ち、HP割合に応じて3フェーズに変化する。
/// Slash（速い縦斬り）・HeavySword（遅い叩きつけ）・Rush（高速突進）の
/// 3種類の攻撃を状況と残りHPに応じて使い分ける。
///
/// HPが半分を切ると必殺技の火炎ブレス（Breath）を必ず1回撃ち、以降も低確率で使う。
///
/// 雑魚と違い、被弾しても **のけぞらない・吹き飛ばない**（常時スーパーアーマー）。
/// プレイヤーはボスの行動を止められないかわりに、どの攻撃も長い予備動作を持つので
/// 見てから回避できる。溜めの間は体が橙に光り、チャージリングが収束する。
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
    // 被弾クリップ。ボスはのけぞらないので今は再生していない（クリップ一覧として残す）
    static constexpr const char* kClipHit     = "Dragon_Hit";
    static constexpr const char* kClipDeath   = "Dragon_Death";
    /// 振り切る瞬間の位置。角速度ピークは Attack が BodyRoot 0.583/0.88秒=67%、
    /// Attack2 が翼の叩きつけ 1.083/1.67秒=65%
    static constexpr float kAttackImpactRatio  = 0.67f;
    static constexpr float kAttack2ImpactRatio = 0.65f;

    /// 必殺技ブレスの炎。Resource/VFX/BossBreath.vfx.json が中身を持つ
    static constexpr const char* kBreathVfxName = "BossBreath";

    BossKnight(std::string objectName);
    void Initialize() override;
    void Update(float deltaTime) override;

#ifdef _DEBUG
#endif

    void OnCollisionEnter(BaseCollider* other) override;
    void OnCollisionStay(BaseCollider* other) override;
    void OnCollisionExit(BaseCollider* other) override;

    /// <summary>
    /// 「攻撃を弾いている」表示を出すかどうか。
    ///
    /// ボスはダメージでのけぞりも吹き飛びもしないので、実際には常にノックバック無効。
    /// ただしこのフラグは弾かれ演出（紫オーラ・レティクルの色・控えめなヒット演出）に
    /// 直結していて、常時 true にすると通っているダメージまで弾かれて見えてしまう。
    /// そのため「本当に手が出せない」突進(Rush)中だけ true を返す。
    /// </summary>
    bool IsKnockbackImmune() const override;

    /// <summary>ボスは倒すのが難しいのでスタイルスコアを高めに補正する。</summary>
    float GetStyleMultiplier() const override { return 2.0f; }

protected:
    /// <summary>死亡演出終了時に武器を後始末する</summary>
    void OnDeathEffectFinished() override;

private:
    // 体の発光と追従ライトをまとめて更新する。
    // 弾かれ演出（紫）と攻撃の溜め（橙）が同じ場所を取り合うので、優先順をここで決める
    void UpdateBodyVisual(float deltaTime);

    // 剣は持たない。噛みつき・叩きつけ・突進のたびに、この判定を該当ジョイントへ付け替える
    EnemyHitbox*     hitbox_        = nullptr;
    ParticleEmitter* chargeEmitter_ = nullptr;
    ParticleEmitter* auraEmitter_   = nullptr;
    ParticleEmitter* armorHitEmitter_ = nullptr; // アーマー中被弾の弾かれ火花

    // 予備動作中のチャージリング。溜めが進むほど間隔を詰めて切迫感を出す
    float chargeEmitTimer_ = 0.0f;
    static constexpr float kChargeEmitIntervalStart = 0.14f;
    static constexpr float kChargeEmitIntervalEnd   = 0.04f;

    // 予備動作の発光の強さ(0〜1)。振り抜いた瞬間にパチッと消えないよう追従させる
    float windupGlow_ = 0.0f;
    static constexpr float kWindupGlowFollowRate = 12.0f;

    // スーパーアーマー中の紫オーラの発生間隔
    float auraEmitTimer_ = 0.0f;
    static constexpr float kAuraEmitInterval = 0.04f;

    // スーパーアーマー中の体の発光（脈動）用の経過時間
    float armorTintPhase_ = 0.0f;
    // アーマー中被弾時に体を一瞬強く光らせるタイマー
    float armorHitFlashTimer_ = 0.0f;
    static constexpr float kArmorHitFlashDuration = 0.15f;

    // 「HPが半分を切ってブレスを解禁したか」などの1体ぶんの記憶。
    // BossStateCombatIdle は3インスタンスに分かれているので、実体はここに1つ置いて共有する
    BossBattleMemory battleMemory_{};

    std::unique_ptr<EnemySensorComponent>       sensor_;
    std::unique_ptr<EnemyMovementComponent>     movement_;
    std::unique_ptr<EnemyBoneAttackComponent>   boneAttack_;
};
