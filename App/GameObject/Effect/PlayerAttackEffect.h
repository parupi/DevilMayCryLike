#pragma once
#include <Math/Vector3.h>
#include <Math/Vector4.h>
#include <memory>
#include <random>
#include <string>
#include "GameObject/Character/CharacterStructs.h"

class Player;
class PlayerStateAttack;
class WeaponTrail;
class RadialBlurEffect;
class SpeedLineEffect;
class BaseRenderer;

/// <summary>
/// プレイヤーの剣攻撃の演出（Desktop の PlayerAttackVFX.md 準拠）。
///
/// 攻撃ステートは何も呼ばない。今の攻撃の段階（構え・溜め・振り・振り終わり）を毎フレーム読んで、
/// 段階が変わった瞬間と、その段階の間に出すものを決める（BossAttackEffect と同じ作り）。
///
///   構え     … 刀身が光り始め、柄から刃先へ光の粒が流れる
///   溜め     … 溜めるほど光と粒が強くなり、溜めきると足元に光の輪
///   振り始め … 刀身が一瞬強く光る・柄の近くから初速の火花
///   振り     … 二重の軌跡（太い色の帯＋刃先の白い帯）と、速く振っている間だけ刃のブレ
///   振り終わり … 軌跡が残光として消え、刀身の光が抜けていく（強攻撃ほどゆっくり）
///
/// 技ごとの追加演出は AttackVfxStyle で決まる（攻撃エディタの "VFX Style"、Auto なら攻撃の性能から）。
///   突進   … 前方の風の筋・衝撃リング・足元の砂埃・集中線・画角の蹴り
///   打ち上げ … 足元の衝撃・上へ昇る光・長い軌跡
///   強攻撃 … 前方へ風の筋・放射ブラー
///   叩きつけ … 刃先が地面に届いた所に衝撃波・ひび・土煙・火花、カメラの揺れと寄り
///
/// 当たった瞬間の演出は PlayerWeapon → HitEffectSystem が出す（ここは刃先の速度と種類を教えるだけ）。
/// </summary>
class PlayerAttackEffect
{
public:
	/// <summary>使うVFXを読み込む（未登録のものだけ）</summary>
	static void LoadVfx();

	/// <summary>Auto のときに攻撃の性能から見た目の種類を決める</summary>
	static AttackVfxStyle ResolveStyle(const std::string& attackName, const AttackData& data);

	PlayerAttackEffect();
	~PlayerAttackEffect();

	void Initialize(Player* player);

	/// <summary>
	/// 毎フレーム呼ぶ。プレイヤーと武器のワールド行列がこのフレームの値になった後に呼ぶこと
	/// （刃先の位置から軌跡を作るため）。武器の被弾フラッシュ（HitFlashComponent）より前に呼ぶ
	/// </summary>
	/// <param name="deltaTime">ヒットストップを反映した時間（止まっている間は軌跡も光も止まる）</param>
	void Update(float deltaTime);

	/// <summary>軌跡を描く（GameScene::Draw → Player::DrawEffect から）</summary>
	void Draw();

	/// <summary>軌跡・光・画面効果を即座に消す</summary>
	void Stop();

	/// <summary>刃先の速度[m/s]。火花を振った向きへ流すのに使う</summary>
	const Vector3& GetTipVelocity() const { return tipVelocity_; }
	/// <summary>今の攻撃の見た目の種類（攻撃中でなければ直前の攻撃のもの）</summary>
	AttackVfxStyle GetCurrentStyle() const { return style_; }

private:
	/// <summary>攻撃の段階</summary>
	enum class Stage { None, Startup, Charge, Active, After };

	/// <summary>見た目の種類ごとの色・強さ・長さ</summary>
	struct StyleProfile {
		Vector4 outerColor;   // 太い帯の色
		Vector4 coreColor;    // 刃先の細い帯の色（白寄り）
		Vector3 glowColor;    // 刀身の発光の色
		float glowPeak;       // 振り始めの刀身の発光の強さ
		float glowFadeRate;   // 振り終わってから光が抜ける速さ（大きいほど早い）
		float trailLifetime;  // 太い帯が残る時間
		float smearLifetime;  // 刃のブレが残る時間（強攻撃ほど長い）
		const char* moteVfx;  // 刀身に集まる光の粒
		float swingSparkCount;// 初速の火花の量
	};
	static const StyleProfile& GetProfile(AttackVfxStyle style);

	void UpdateBladePose(float deltaTime);
	void OnAttackBegin(const PlayerStateAttack& attack);
	void UpdateStartup(const PlayerStateAttack& attack, float deltaTime);
	void UpdateCharge(const PlayerStateAttack& attack, float deltaTime);
	void OnSwingBegin();
	void UpdateSwing(const PlayerStateAttack& attack, float deltaTime);
	void PlaySlamImpact();
	void UpdateGlow(float deltaTime);
	void UpdateScreenEffects(float realDeltaTime);

	void ApplyProfile();
	void EmitMote(float countScale);
	void PlayRadialBlur(const Vector3& worldCenter, float strength, float duration);
	void AddCameraShake(float trauma) const;
	void AddCameraFovPunch(float add) const;

	Vector3 RandomOnBlade();
	Vector3 GetFeet() const;
	Vector3 GetChest() const;
	Vector3 GetForward() const;

	Player* player_ = nullptr;
	BaseRenderer* bladeRenderer_ = nullptr;

	// 軌跡: 太い色の帯（刃全体）／刃先の細い白い帯／刃のブレ（モーションブラーの代わり）
	std::unique_ptr<WeaponTrail> mainTrail_;
	std::unique_ptr<WeaponTrail> coreTrail_;
	std::unique_ptr<WeaponTrail> smearTrail_;

	// 画面効果（シーンをまたいで残るので、使い終わったら必ず切る）
	RadialBlurEffect* radialBlur_ = nullptr;
	SpeedLineEffect* speedLine_ = nullptr;

	// ── 攻撃の状態 ──
	std::string attackName_;
	AttackVfxStyle style_ = AttackVfxStyle::Slash;
	const StyleProfile* profile_ = nullptr;
	Stage prevStage_ = Stage::None;
	float chargeRatio_ = 0.0f;
	bool chargeFull_ = false;
	bool slamImpactDone_ = false;
	float moteTimer_ = 0.0f;
	float thrustTimer_ = 0.0f;
	float pulseTime_ = 0.0f;

	// ── 刃 ──
	Vector3 bladeTip_{};
	Vector3 bladeBase_{};
	Vector3 prevTip_{};
	Vector3 tipVelocity_{};
	bool hasPrevTip_ = false;

	// ── 刀身の発光 ──
	float glow_ = 0.0f;
	float glowTarget_ = 0.0f;

	// ── 画面効果の再生状態（実時間で減衰させる）──
	float blurTimer_ = 0.0f;
	float blurDuration_ = 0.0f;
	float blurStrength_ = 0.0f;
	float speedLineTimer_ = 0.0f;
	float speedLineDuration_ = 0.0f;
	float speedLineStrength_ = 0.0f;

	std::mt19937 rng_;
};
