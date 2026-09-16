#pragma once
#include <Math/Vector2.h>
#include <Math/Vector4.h>
#include <array>
#include <cstdint>
#include <random>
#include <string>

class Sprite;
class TextLabel;
class BossKnight;

/// <summary>
/// ボスのHPバー（画面上部中央）。
///
/// ロックオンのレティクルに付く円形のHPとは別に、ボス戦の間ずっと出しておく横長のバー。
/// 見た目は4層（背景 / 遅れて減る白いゲージ / 赤いHPゲージ / 金属のフレーム）で、
/// 画像を使わず white.png を重ねて組んでいる（色と大きさはこのクラスの定数だけで決まる）。
///
/// 演出の流れ（Desktop の HealthBar.md 準拠）:
///   登場   … 名前がフェードイン → バーが中央から横へ伸びる → フレームが一瞬光る（約0.7秒）
///   被弾   … 赤ゲージは即座に減り、白ゲージが少し遅れて追いかける。赤いフラッシュ・横揺れ・削れた位置から火花
///   フェーズ移行 … 咆哮に合わせて一瞬止まり、バー全体が白く光って中央から波紋が広がる
///   撃破   … 赤ゲージが消え、白ゲージが最後まで追いつき、名前 → バー全体の順に縮みながら消える
///
/// ボスはステージから自分で探す（BossKnight を1体だけ想定）。
/// トレーニングで出し直されたときは、新しい相手に対してもう一度登場演出を流す。
///
/// スプライトは SpriteManager が生成順に描くので、ポーズの暗幕より先に作ること。
/// </summary>
class BossHealthBar
{
public:
	void Initialize();

	/// <summary>毎フレーム呼ぶ</summary>
	/// <param name="hudVisible">false の間は描かない（プレイヤーの死亡演出中など）。演出の時間は進める</param>
	void Update(bool hudVisible);

private:
	enum class State {
		Hidden,  ///< ボスがいない
		Intro,   ///< 登場演出
		Active,  ///< 戦闘中
		Defeat,  ///< 撃破演出
	};

	/// <summary>火花1粒。削れた位置から飛び散る</summary>
	struct Spark {
		Sprite* sprite = nullptr;
		Vector2 position{};
		Vector2 velocity{};
		float life = 0.0f;
		float maxLife = 0.0f;
	};

	BossKnight* FindBoss() const;
	void StartIntro(BossKnight& boss);
	void StartDefeat();
	void OnDamaged(float previousRatio, float newRatio);
	void OnPhaseChanged();
	void EmitSparks(float x, float y, int32_t count);

	void UpdateTimers(float dt);
	void UpdateDelayGauge(float dt);
	void UpdateSparks(float dt);
	/// <summary>今の状態から全スプライトの位置・大きさ・色を決める</summary>
	void ApplyLayout();
	void SetVisible(bool visible);

	Sprite* CreateRect(const char* name, const char* texture = "white.png");

private:
	// ==========================
	// レイアウト（画面 1280x720、上部中央）
	// ==========================
	static constexpr float kCenterX = 640.0f;
	static constexpr float kBarY = 66.0f;          // バーの中心の高さ
	static constexpr float kBarWidth = 560.0f;
	static constexpr float kBarHeight = 14.0f;
	static constexpr float kFramePad = 3.0f;       // フレームの太さ
	static constexpr float kNameFontSize = 24.0f;
	static constexpr float kNameOffsetY = -22.0f;  // バーの中心から名前の中心まで

	// ==========================
	// 演出の時間（HealthBar.md の推奨値）
	// ==========================
	static constexpr float kIntroNameTime = 0.2f;    // 名前のフェードイン
	static constexpr float kIntroExpandTime = 0.4f;  // バーが伸びる
	static constexpr float kIntroGlowTime = 0.1f;    // フレームの発光（この後に短い余韻を足す）
	static constexpr float kIntroGlowFade = 0.15f;
	static constexpr float kDelayHoldTime = 0.08f;   // 白ゲージが動き出すまで（連続で当てている間は待ち続ける）
	static constexpr float kDelayCatchUpTime = 0.25f;// 白ゲージが追いつくまで（合わせて約0.33秒）
	static constexpr float kHitFlashTime = 0.08f;
	static constexpr float kShakeTime = 0.12f;
	static constexpr float kShakeAmplitude = 6.0f;   // px
	static constexpr float kPhaseFreezeTime = 0.1f;
	static constexpr float kPhaseGlowTime = 0.35f;
	static constexpr float kRippleTime = 0.55f;
	static constexpr float kDefeatDrainTime = 0.35f; // 白ゲージが0まで追いつく
	static constexpr float kDefeatFadeTime = 0.5f;   // 縮みながら消える

	static constexpr int32_t kSparkCount = 14;

	// ==========================
	// スプライト（作った順＝描く順）
	// ==========================
	Sprite* shadow_ = nullptr;          // フレームの落ち影
	Sprite* frame_ = nullptr;           // 金属のフレーム（暗い鋼色）
	Sprite* frameLineTop_ = nullptr;    // フレーム上辺の金の縁
	Sprite* frameLineBottom_ = nullptr; // フレーム下辺の暗い金の縁
	Sprite* background_ = nullptr;      // 黒に近い暗い赤
	Sprite* backgroundGloss_ = nullptr; // 背景の上半分の光沢
	Sprite* delay_ = nullptr;           // 遅れて減る白いゲージ
	Sprite* hp_ = nullptr;              // HPゲージ（濃い赤）
	Sprite* hpGloss_ = nullptr;         // HPゲージの光沢
	Sprite* hpEdge_ = nullptr;          // HPゲージの先端の明るい線
	std::array<Sprite*, 2> ticks_{};    // フェーズの境目の目盛り
	Sprite* flash_ = nullptr;           // 被弾・フェーズ移行でバーを光らせる（加算）
	Sprite* glow_ = nullptr;            // フレームごと光らせる（加算）
	std::array<Sprite*, 2> sweeps_{};   // 登場時に中央から外へ流れる光（加算）
	std::array<Sprite*, 2> capLines_{}; // 両端から外へ伸びる金の飾り線
	std::array<Sprite*, 2> capOuter_{}; // 両端の菱形の飾り（金）
	std::array<Sprite*, 2> capInner_{}; // 菱形の中の暗い芯
	Sprite* ripple_ = nullptr;          // フェーズ移行の波紋（加算・楕円）
	Sprite* rippleLine_ = nullptr;      // 波紋と一緒に左右へ走る光の線（加算）
	std::array<Spark, kSparkCount> sparks_{};
	TextLabel* name_ = nullptr;

	// ==========================
	// 状態
	// ==========================
	State state_ = State::Hidden;
	std::string bossName_;       // 追いかけているボスのオブジェクト名（削除されても安全に引けるよう名前で持つ）
	float lastHp_ = 0.0f;
	int32_t lastPhase_ = 1;
	bool visible_ = false;

	float hpRatio_ = 1.0f;       // 赤ゲージ（即時）
	float delayRatio_ = 1.0f;    // 白ゲージ（遅れて追う）
	float delayFrom_ = 1.0f;     // 追いかけ始めた位置
	float delayHoldTimer_ = 0.0f;
	float delayCatchUpTimer_ = 0.0f;

	float stateTimer_ = 0.0f;    // Intro / Defeat の経過時間
	float hitFlashTimer_ = 0.0f;
	float shakeTimer_ = 0.0f;
	float phaseFreezeTimer_ = 0.0f;
	bool phaseEffectPending_ = false;
	float phaseGlowTimer_ = 0.0f;
	float rippleTimer_ = 0.0f;
	float pulseTime_ = 0.0f;     // 最終フェーズの脈動

	std::mt19937 rng_{ std::random_device{}() };
};
