#pragma once
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/Component/EnemySensorComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyBoneAttackComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyHitbox.h"
#include "GameObject/Character/Enemy/BossKnight/State/BossStateCombatIdle.h"
#include "GameObject/Character/Enemy/BossKnight/BossBreathEffect.h"
#include "GameObject/Character/Enemy/BossKnight/BossAttackEffect.h"
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
/// プレイヤーは1発ごとにはボスの行動を止められないかわりに、どの攻撃も長い予備動作を持つので
/// 見てから回避できる。溜めの間は体が橙に光り、チャージリングが収束する。
///
/// ダメージを溜めると数秒崩れ（BossStateDown）、その間は被ダメージが増える。
/// フェーズが変わった瞬間は咆哮（BossStateRoar）で知らせ、フェーズ3の間は体が赤く脈打つ。
/// </summary>
class BossKnight : public Enemy
{
public:
    /// プレイヤーの1撃は 1〜4（コンボ1本で 5〜10 程度）。25 だとコンボ数本で終わってしまうので増やした
    static constexpr float kMaxHp = 60.0f;

    /// 見た目のモデル。Resource/models/Enemys/Dragon/Dragon.obj を指す
    static constexpr const char* kModelName = "Enemys/Dragon";
    /// 素の高さ約4.0m を約2.0m（雑魚の1.6倍・翼で横幅1.7m）にするスケール
    static constexpr float kModelScale = 0.5f;

    // Dragon.gltf が持つクリップ（5種）。待機に相当するのは Flying
    static constexpr const char* kClipIdle    = "Dragon_Flying";
    static constexpr const char* kClipAttack  = "Dragon_Attack";   // 速い斬り
    static constexpr const char* kClipAttack2 = "Dragon_Attack2";  // 遅い叩きつけ
    // 被弾クリップ。ボスはのけぞらないので、崩れ（BossStateDown）でだけ使う
    static constexpr const char* kClipHit     = "Dragon_Hit";
    static constexpr const char* kClipDeath   = "Dragon_Death";
    /// 振り切る瞬間の位置。角速度ピークは Attack が BodyRoot 0.583/0.88秒=67%、
    /// Attack2 が翼の叩きつけ 1.083/1.67秒=65%
    static constexpr float kAttackImpactRatio  = 0.67f;
    static constexpr float kAttack2ImpactRatio = 0.65f;

    // 必殺技ブレスの演出（Resource/VFX/BossBreath*.vfx.json）は BossBreathEffect が持つ
    /// フェーズが変わった瞬間の咆哮の衝撃波。Resource/VFX/BossRoar.vfx.json が中身を持つ
    static constexpr const char* kRoarVfxName = "BossRoar";
    /// 崩れた瞬間に弾ける火花。GameScene が読む Resource/VFX/HitImpact.vfx.json を使い回す
    static constexpr const char* kBreakVfxName = "HitImpact";

    /// 普段プレイヤーへ向き直る速さ[度/秒]。大きな体なので一瞬では振り向かない
    /// （攻撃の溜めの間はさらに遅い。各攻撃の windupTurnSpeed）
    static constexpr float kFaceTurnSpeed = 240.0f;

    /// 死亡モーションの最後に沈める量（オブジェクトのスケール1のときの単位）。
    /// Dragon_Death の最後のポーズは、一番低い関節（足・翼の先）でもモデル原点から 0.33 上に残る
    /// （飛んでいる姿勢のまま倒れるため）。関節は肉の内側にあるので少し控えめに 0.28 ぶん沈める。
    /// モデル単位 0.28 × kModelScale 0.5 = 0.14
    static constexpr float kDeathModelSink = 0.14f;

    // ── ブレイク（崩れ）──
    // 与えたダメージが溜まると数秒崩れ（BossStateDown）、その間は被ダメージが増える。
    // のけぞらないボスに「攻め続けた見返り」を作るためのもの
    // HP に対する比率は以前（HP25 に 8 / +4）と同じにしてある。1戦で崩れるのは2回ほど
    static constexpr float kBreakThresholdBase = 18.0f; // 最初に崩れるまでのダメージ（HPのおよそ1/3）
    static constexpr float kBreakThresholdStep = 10.0f; // 崩れるたびに次の必要量を増やす（崩し続けるハメを防ぐ）
    static constexpr float kBreakDecayDelay    = 2.5f; // 殴るのをやめてから減り始めるまで[s]
    static constexpr float kBreakDecayRate     = 2.0f; // 減り始めてから1秒に減る量
    static constexpr float kDownDamageScale    = 1.5f; // 崩れている間の被ダメージ倍率

    /// <summary>
    /// 通常時のノックバック耐性（仕様書 §9）。0.88 = 受けた強さの12%だけ効く。
    /// 「重くて動かないが、当たれば少しは押される」を出すための値。
    /// 突進ステートの間（溜めも含む）は GetKnockbackResistance() が完全無効へ上書きする
    /// </summary>
    static constexpr float kKnockbackResistance = 0.88f;

    BossKnight(std::string objectName);
    void Initialize() override;
    void Update(float deltaTime) override;

#ifdef _DEBUG
#endif

    void OnCollisionEnter(BaseCollider* other) override;
    void OnCollisionStay(BaseCollider* other) override;
    void OnCollisionExit(BaseCollider* other) override;

    /// <summary>攻撃の軌跡（噛みつきの風切り・突進の翼の軌跡）を描く。GameScene::Draw が敵ごとに呼ぶ</summary>
    void DrawEffect() override;

    /// <summary>
    /// 「攻撃を弾いている」表示を出すかどうか。
    ///
    /// ボスはダメージでのけぞりも吹き飛びもしないので、実際には常にノックバック無効。
    /// ただしこのフラグは弾かれ演出（紫オーラ・レティクルの色・控えめなヒット演出）に
    /// 直結していて、常時 true にすると通っているダメージまで弾かれて見えてしまう。
    /// そのため「本当に手が出せない」間だけ true を返す:
    ///   - 突進の踏み込み中（判定が出ている間）。溜めの間はその場で構えているだけなので false
    ///     （ここで紫を出すと溜めの橙＝攻撃が来る、が隠れ、殴っても弾かれたように見える）
    ///   - フェーズが変わった瞬間の咆哮中。この間はダメージも通らない
    /// </summary>
    bool IsKnockbackImmune() const override;

    /// <summary>
    /// 突進ステートの間（溜めも含む）と咆哮中は、ノックバックを完全に無効にする。
    /// 溜めで押されても踏み込みが鈍っても「予兆を見てから避ける」の読みが崩れるため。
    /// それ以外は通常の耐性（kKnockbackResistance）で少しだけ押される。
    /// </summary>
    const KnockbackResistance& GetKnockbackResistance() const override;

    /// <summary>ボスは倒すのが難しいのでスタイルスコアを高めに補正する。</summary>
    float GetStyleMultiplier() const override { return 2.0f; }

    /// <summary>画面上部のHPバー（BossHealthBar）に出す名前</summary>
    static constexpr const char* kDisplayName = "DRAGON";

    /// <summary>
    /// 咆哮で知らせ終えたフェーズ（1〜3）。HPが境目を越えたあと、咆哮に入る瞬間に1つ進む。
    /// HPバーのフェーズ移行演出をボスの咆哮に揃えるために使う
    /// </summary>
    int GetShownPhase() const { return battleMemory_.shownPhase; }

protected:
    /// <summary>死亡演出終了時に武器を後始末する</summary>
    void OnDeathEffectFinished() override;

private:
    // 今そのステートにいるか
    bool IsInState(const char* stateName) const;
    // 今の行動の種類（攻撃ごとの演出の出し分けに使う）
    BossActionKind GetCurrentAction() const;

    // ブレイク値の減衰と、溜まりきったときの崩れ
    void UpdateBreak(float deltaTime);
    void StartBreak();

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

    // 崩れ中・瀕死の体の脈動用の経過時間
    float bodyPulsePhase_ = 0.0f;

    // ブレイク（崩れ）の状態
    float breakGauge_ = 0.0f;                    // 溜まったダメージ
    float breakThreshold_ = kBreakThresholdBase; // 次に崩れるまでの必要量
    float breakDecayTimer_ = 0.0f;               // これが 0 になるまでは減らない
    // 崩れた瞬間の演出
    static constexpr float kBreakHitStopTime = 0.25f;
    static constexpr float kBreakHitStopIntensity = 0.0f; // 体は震わせず、揺れはカメラに任せる（プレイヤーの攻撃と同じ）
    static constexpr float kBreakShakeTrauma = 0.5f;
    static constexpr float kBreakVfxCountScale = 2.5f;

    // 「HPが半分を切ってブレスを解禁したか」などの1体ぶんの記憶。
    // BossStateCombatIdle は3インスタンスに分かれているので、実体はここに1つ置いて共有する
    BossBattleMemory battleMemory_{};

    std::unique_ptr<EnemySensorComponent>       sensor_;
    std::unique_ptr<EnemyMovementComponent>     movement_;
    std::unique_ptr<EnemyBoneAttackComponent>   boneAttack_;
    // ブレスの演出（溜め→発射→維持→余韻）。余韻はステートを抜けた後も続くので本体が持って毎フレーム回す
    std::unique_ptr<BossBreathEffect>           breathEffect_;
    // 噛みつき・叩きつけ・突進・咆哮の演出と、移動・着地の砂埃、被弾の閃光
    std::unique_ptr<BossAttackEffect>           attackEffect_;
};
