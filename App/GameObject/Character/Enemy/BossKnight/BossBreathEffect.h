#pragma once
#include <random>
#include <string>
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

class Enemy;
class DynamicPointLight;
class DynamicSpotLight;
class HitFlashEffect;
class ChromaticAberrationEffect;
class BloomEffect;
class HeatDistortionEffect;
class ColorGradingEffect;

/// <summary>
/// ボスの火炎ブレスの演出一式（DragonBress.md）。
/// 予兆（溜め）→ 発射（インパクト）→ 維持 → 余韻 の4段階を、ステートからの要求で進める。
///
/// ステート（BossStateBreath）は毎フレーム「今どの段階か」を伝えるだけで、
/// 粒・ライト・ポストエフェクト・カメラの揺れはすべてここが持つ。
/// 余韻はステートを抜けた後も続くので、更新は BossKnight::Update が毎フレーム回す。
///
/// 要求が途絶えたら（死亡演出でステートの更新が止まった等）自動で余韻へ移るので、
/// 炎が出しっぱなしになることはない。
/// </summary>
class BossBreathEffect
{
public:
	// ── VFX（Resource/VFX/<名前>.vfx.json）──
	/// 溜め: 口へ吸い込まれる煙・漏れる火の粉・口の中の光
	static constexpr const char* kChargeVfx = "BossBreathCharge";
	/// 発射: 閃光・炎の塊・衝撃波・火花・押し出される煙
	static constexpr const char* kIgniteVfx = "BossBreathIgnite";
	/// 維持の芯: 中心炎（白〜黄）＋火の粉
	static constexpr const char* kCoreVfx = "BossBreath";
	/// 維持の外側: 外炎（橙）＋煙。終わり際はこちらから先に止めて「炎が細くなる」を出す
	static constexpr const char* kOuterVfx = "BossBreathOuter";
	/// 地面を舐める炎・火花・煙・灰
	static constexpr const char* kGroundVfx = "BossBreathGround";
	/// 地面に残る焦げ跡と、地面の発光
	static constexpr const char* kScorchVfx = "BossBreathScorch";
	/// 余韻: 残る火の粉と漂う煙
	static constexpr const char* kTailVfx = "BossBreathTail";

	/// 炎を吐き出す口のジョイント（Dragon.gltf）。見つからなければ体の正面基準の位置へ落とす
	static constexpr const char* kMouthJoint = "Nose";

	/// <summary>上のVFXを読む（未登録のものだけ）。BossKnight::Initialize から呼ぶ</summary>
	static void LoadVfx();

	BossBreathEffect() = default;
	~BossBreathEffect();
	BossBreathEffect(const BossBreathEffect&) = delete;
	BossBreathEffect& operator=(const BossBreathEffect&) = delete;

	/// <param name="ownerName">スキンモデルのレンダラー名（＝ボスの名前）。ライト名にも使う</param>
	void Initialize(const std::string& ownerName);

	// ======================
	// ステートから毎フレーム呼ぶ
	// ======================

	/// <summary>溜め。progress は 0→1（吐く直前で1）</summary>
	void RequestCharge(float progress);
	/// <summary>
	/// 炎を吐いている最中。
	/// </summary>
	/// <param name="fireProgress">吐き始め0 → 吐き終わり1。終わり際に炎を細くするのに使う</param>
	/// <param name="reach">炎の帯の長さ[m]（体の位置から。配置スケール込み）</param>
	/// <param name="halfWidth">炎の帯の半幅[m]（配置スケール込み）</param>
	void RequestFire(float fireProgress, float reach, float halfWidth);
	/// <summary>吐き終わった／中断された。溜めの途中なら何も出さずに消え、吐いた後なら余韻へ移る</summary>
	void RequestStop();

	/// <summary>毎フレーム。ポーズ更新（Enemy::Update）の後に呼ぶこと（口のジョイント位置を使うため）</summary>
	void Update(Enemy& enemy, float deltaTime);

private:
	enum class Phase { None, Charge, Fire, Tail };
	enum class Request { None, Charge, Fire, Stop };

	// 口の位置・体の向き・足元を取り直す
	void UpdateMouth(Enemy& enemy);
	// 地面へ向けて少し下向きに吐く方向（帯の奥で地面に当たる）
	void UpdateAim();

	void EnterCharge();
	void EnterFire(Enemy& enemy);
	void EnterTail();

	void UpdateCharge(float deltaTime);
	void UpdateFire(float deltaTime);
	void UpdateTail(float deltaTime);
	void UpdateLight(float deltaTime);
	void UpdatePostEffects(float deltaTime);

	// 炎の帯の地面の上のランダムな点。ratio は帯の長さに対する位置、lateral は半幅に対する割合
	Vector3 RandomBandPoint(float nearRatio, float farRatio, float lateral, float height);

	std::string ownerName_;

	// ── 演出の部品（ライトの所有権は LightManager、ポストエフェクトは OffScreenManager）──
	DynamicPointLight* light_ = nullptr;
	DynamicSpotLight* spotLight_ = nullptr;           // 炎の進行方向を照らす
	HitFlashEffect* flash_ = nullptr;
	ChromaticAberrationEffect* chroma_ = nullptr;
	BloomEffect* bloom_ = nullptr;
	HeatDistortionEffect* heatDistortion_ = nullptr;  // 口元と炎の帯の陽炎
	ColorGradingEffect* colorGrading_ = nullptr;      // ブレス中だけ暖色に寄せる

	// ── 段階と要求 ──
	Phase phase_ = Phase::None;
	Request request_ = Request::None;
	float requestLostTimer_ = 0.0f; // 要求が途絶えてからの経過[s]
	float phaseTimer_ = 0.0f;       // 今の段階に入ってからの経過[s]
	bool tailFromFire_ = false;     // 余韻が「吐いた後」か「溜めの中断」か

	float chargeProgress_ = 0.0f;
	float fireProgress_ = 0.0f;
	float reach_ = 0.0f;
	float halfWidth_ = 0.0f;

	// ── 発生間隔 ──
	float chargeEmitTimer_ = 0.0f;
	float coreEmitTimer_ = 0.0f;
	float outerEmitTimer_ = 0.0f;
	float groundEmitTimer_ = 0.0f;
	float scorchEmitTimer_ = 0.0f;
	float tailEmitTimer_ = 0.0f;

	// ── ライト ──
	float lightIntensity_ = 0.0f;     // 今の明るさ（余韻の減衰の起点にする）
	float tailStartIntensity_ = 0.0f;
	float igniteBoostTimer_ = 0.0f;   // 発射の瞬間にひときわ明るくする残り時間
	float flickerPhase_ = 0.0f;       // 維持中の揺らぎ

	// ── ポストエフェクト ──
	float flashTimer_ = 0.0f;
	float postWeight_ = 0.0f;         // ブレス中のポストエフェクトの効き(0〜1)
	bool bloomBoosted_ = false;       // ブルームを上書き中か
	float bloomBaseIntensity_ = 0.0f; // 上書きする前のブルームの強さ（戻す先）

	// ── 毎フレーム取り直す位置 ──
	Vector3 mouth_{};
	Vector3 forward_{ 0.0f, 0.0f, 1.0f };
	Vector3 aimDirection_{ 0.0f, 0.0f, 1.0f };
	Vector3 foot_{};
	float ownerScale_ = 1.0f;

	std::mt19937 rng_;
};
