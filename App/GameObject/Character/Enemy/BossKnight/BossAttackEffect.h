#pragma once
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include "Math/Vector2.h"
#include "Math/Vector3.h"

class Enemy;
class EnemyBoneAttackComponent;
class WeaponTrail;
class HitFlashEffect;
class RadialBlurEffect;
class SpeedLineEffect;
class HeatDistortionEffect;

/// <summary>今どの行動をしているか。BossKnight がステートから判定して渡す</summary>
enum class BossActionKind {
	None,   // 待機・移動など（移動の砂埃と着地だけ見る）
	Bite,   // 噛みつき（BossStateSlash）
	Slam,   // 叩きつけ（BossStateHeavySword）
	Rush,   // 突進（BossStateRush）
	Breath, // 火炎ブレス。見た目は BossBreathEffect が持つので、ここでは被弾の閃光だけ
	Roar,   // 咆哮
};

/// <summary>
/// ボスの近接攻撃（噛みつき・叩きつけ・突進）と、咆哮・移動・着地の演出（DragonAttackVFX.md）。
///
/// 攻撃の段階（溜め → 判定が出た瞬間 → 判定が出ている間 → 判定が消えた瞬間）は
/// EnemyBoneAttackComponent の状態をそのまま見て判断するので、各攻撃ステートは何も呼ばなくてよい。
///
/// 仕様書から「このゲームに合わせて」変えたところ:
///   - 予兆のアニメーション（頭を引く・前脚を上げる）はクリップ側なので作れない。
///     代わりに溜めの間に足元の砂埃・爪の火花・地響きを出して、攻撃ごとに見分けが付くようにする
///   - 当たり先ごとの演出（岩の破片・金属の火花）は、当たり先の材質を持っていないので
///     「プレイヤーに当たった＝白い閃光」「外れて地面を噛んだ＝土煙」の2つにした
///   - 地面のひびは本物のデカールではなく、地面へ寝かせた板のパーティクル（CrackDecal.png）
/// </summary>
class BossAttackEffect
{
public:
	// ── VFX（Resource/VFX/<名前>.vfx.json）──
	/// 足元の土煙＋小石。溜め・突進中・移動に使い回す
	static constexpr const char* kDustVfx = "BossDust";
	/// 爪が地面を擦る火花と土煙
	static constexpr const char* kClawScrapeVfx = "BossClawScrape";
	/// 地面への衝撃（衝撃波リング・大量の土煙・岩の破片）。量は発生数の倍率で変える
	static constexpr const char* kImpactVfx = "BossImpact";
	/// 地面のひび（地面に寝かせた板）と土煙
	static constexpr const char* kGroundCrackVfx = "BossGroundCrack";
	/// 咆哮で飛び散る唾液
	static constexpr const char* kRoarSalivaVfx = "BossRoarSaliva";

	/// <summary>上のVFXを読む（未登録のものだけ）。BossKnight::Initialize から呼ぶ</summary>
	static void LoadVfx();

	BossAttackEffect();
	~BossAttackEffect();
	BossAttackEffect(const BossAttackEffect&) = delete;
	BossAttackEffect& operator=(const BossAttackEffect&) = delete;

	/// <param name="ownerName">スキンモデルのレンダラー名（＝ボスの名前）</param>
	void Initialize(const std::string& ownerName);

	/// <summary>
	/// 毎フレーム。ポーズ更新（Enemy::Update）の後に呼ぶこと（爪・頭・翼のジョイント位置を使うため）
	/// </summary>
	/// <param name="action">今の行動。出現・死亡演出中は None を渡す</param>
	/// <param name="attack">攻撃の段階を読むコンポーネント（噛みつき・叩きつけ・突進が共有している）</param>
	void Update(Enemy& enemy, float deltaTime, BossActionKind action, const EnemyBoneAttackComponent& attack);

	/// <summary>軌跡（噛みつきの風切り・突進の翼の軌跡）を描く。BossKnight::DrawEffect から呼ぶ</summary>
	void Draw();

private:
	// 足元・向き・体の中心を取り直す
	void UpdateBody(Enemy& enemy);
	// ジョイントの位置。取れなければ fallback
	Vector3 JointOr(Enemy& enemy, const char* jointName, const Vector3& fallback) const;
	// 左右の足（交互に使う）の真下の地面
	Vector3 NextFoot(Enemy& enemy);
	// 足元のまわりのランダムな地面の点
	Vector3 RandomAroundFoot(float radius);

	void OnActionChanged(BossActionKind next);
	void UpdateBite(Enemy& enemy, float deltaTime, bool windingUp);
	void UpdateSlam(float deltaTime, bool windingUp, float windup);
	void UpdateRush(Enemy& enemy, float deltaTime, bool windingUp, float windup, bool hitActive);
	void UpdateRoar(Enemy& enemy, float deltaTime);
	void OnStrikeStart(Enemy& enemy, BossActionKind action);
	void OnStrikeEnd(Enemy& enemy, BossActionKind action);
	void OnPlayerHit(BossActionKind action);
	void UpdateLocomotion(float deltaTime, BossActionKind action, bool onGround, float velocityY, float horizontalSpeed);
	void UpdateTrails(Enemy& enemy, float deltaTime);
	void UpdatePostEffects(float deltaTime);
	void PlayRadialBlur(const Vector3& worldCenter, float strength, float duration);

	std::string ownerName_;

	// ── 軌跡 ──
	std::unique_ptr<WeaponTrail> biteTrail_;      // 噛みつきの風切り（鼻先〜首）
	std::unique_ptr<WeaponTrail> wingTrailLeft_;  // 突進の翼の軌跡（翼の先〜肩）
	std::unique_ptr<WeaponTrail> wingTrailRight_;
	bool biteTrailActive_ = false;
	bool wingTrailActive_ = false;

	// ── 画面効果（所有権は OffScreenManager。シーンをまたいで残る）──
	HitFlashEffect* hitFlash_ = nullptr;             // プレイヤーに当たった瞬間の白い閃光
	RadialBlurEffect* radialBlur_ = nullptr;         // 振り抜き・衝撃のモーションブラー
	SpeedLineEffect* speedLine_ = nullptr;           // 突進の集中線
	HeatDistortionEffect* roarDistortion_ = nullptr; // 咆哮の空気の歪み

	// ── 行動の段階 ──
	BossActionKind action_ = BossActionKind::None;
	bool prevHitActive_ = false;
	bool windupStarted_ = false;
	bool rushCrackPlayed_ = false;
	float windupEmitTimer_ = 0.0f;
	float strikeEmitTimer_ = 0.0f;
	int footSide_ = 0;
	float roarTimer_ = 0.0f;
	bool roarBurst_ = false;

	// ── 被弾・移動・着地 ──
	int32_t prevPlayerHp_ = -1;
	bool prevOnGround_ = true;
	float prevVelocityY_ = 0.0f;
	float moveDustTimer_ = 0.0f;
	// 羽ばたきの音の間隔用。待機中も飛んでいるので、行動していなくても刻む
	float wingbeatTimer_ = 0.0f;

	// ── 画面効果の残り時間 ──
	float hitFlashTimer_ = 0.0f;
	float hitFlashIntensity_ = 0.0f;
	float radialBlurTimer_ = 0.0f;
	float radialBlurDuration_ = 0.0f;
	float radialBlurStrength_ = 0.0f;
	Vector3 radialBlurCenter_{};
	float speedLineWeight_ = 0.0f;
	float speedLineTarget_ = 0.0f;
	float roarDistortionTimer_ = 0.0f;
	Vector3 roarCenter_{};

	// ── 毎フレーム取り直す位置 ──
	Vector3 foot_{};
	Vector3 forward_{ 0.0f, 0.0f, 1.0f };
	Vector3 right_{ 1.0f, 0.0f, 0.0f };
	Vector3 bodyCenter_{};
	float scale_ = 1.0f;

	std::mt19937 rng_;
};
